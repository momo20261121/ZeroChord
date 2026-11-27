#!/usr/bin/env python3
"""Measure ZeroChord's CPU and memory on the Cardputer in a scripted worst case.

Runs the deployed app with isolated settings and a key script: the Dance style (all layers),
live playing while a loop records, the count-in, the loop alone, the loop with live chords,
strums and front notes on top, and the loop under All front. Samples the whole process, its
busiest thread (the audio thread must keep real-time headroom) and memory, then restores the
preview. Load the project .env first; --local runs the desktop build instead, for a dry run.
"""
import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BPM = 124  # Dance.
SIXTEENTH = 15000 / BPM
BAR = 16 * SIXTEENTH

# Samples /proc for the app's process and threads every 0.25 s until it exits.
SAMPLER = r'''
import os, sys, time
pid = int(sys.argv[1]); start = float(sys.argv[2])
tick = os.sysconf('SC_CLK_TCK')
def cpu(path):
    with open(path) as f: fields = f.read().rsplit(')', 1)[1].split()
    return int(fields[11]) + int(fields[12])
while True:
    try:
        total = cpu(f'/proc/{pid}/stat')
        threads = {}
        for task in os.listdir(f'/proc/{pid}/task'):
            try: threads[task] = cpu(f'/proc/{pid}/task/{task}/stat')
            except OSError: pass
        with open(f'/proc/{pid}/status') as f:
            rss = next(int(l.split()[1]) for l in f if l.startswith('VmRSS'))
    except (OSError, StopIteration):
        break
    print(f'{time.time() - start:.3f} {total} {rss} ' +
          ' '.join(f'{k}:{v}' for k, v in threads.items()), flush=True)
    time.sleep(.25)
print('tick', tick)
'''


def script():
    """Key events and the phases they make, in milliseconds from launch."""
    events, phases = [], []

    def tap(at, key, hold=40):
        events.append((at, key, 'down'))
        events.append((at + hold, key, 'up'))

    def play(start, bars, chords=True, front=True, strums=True):
        # Chords held a bar each (Hold is off), front notes on 8ths, strums every second bar.
        progression = '2675'
        for bar in range(bars):
            t = start + bar * BAR
            if chords:
                tap(t, progression[bar % 4], int(BAR) - 60)
            if front:
                for step in range(8):
                    tap(t + step * 2 * SIXTEENTH + 20, 'zxcvbnm'[(bar + step) % 7], 90)
            if strums and bar % 2:
                tap(t + 8 * SIXTEENTH + 50, 'l', 30)

    tap(500, '9')    # Dream -> Dance, the last style.
    tap(800, 'i')    # Hold off: released chords stop, so the loop can play alone.
    phases.append(('idle', 1500, 4000))
    tap(4000, 'ENTER')  # Arm.
    record = 4500
    play(record, 8)
    phases.append(('live + recording', record + 500, record + 8 * BAR - 100))
    tap(int(record + 8 * BAR + 30), 'ENTER')  # Finish: 8 bars.
    count = int(record + 9 * BAR)
    tap(count, 'ENTER')  # Count-in, then the loop.
    phases.append(('count-in', count + 200, count + BAR - 100))
    loop = count + BAR
    phases.append(('loop alone', loop + 300, loop + 6 * BAR))
    both = loop + 6 * BAR
    play(both, 10)
    phases.append(('loop + live', both + 300, both + 10 * BAR - 100))
    front = both + 10 * BAR
    events.append((front, 'SPACE', 'down'))
    events.append((front + 700, 'SPACE', 'up'))
    for n in range(int(6 * BAR / 70)):  # Fast runs across all four rows.
        tap(front + 900 + n * 70, '4567890ertyuiodfghjklzxcvbnm'[(n * 5) % 28], 60)
    phases.append(('loop + All front', front + 1200, front + 6 * BAR))
    end = int(front + 6 * BAR + 800)
    events.sort(key=lambda e: e[0])
    return [f'{int(at)} {key} {action}' for at, key, action in events], phases, end


def analyse(samples, tick, phases):
    rows = []
    for name, start, end in phases:
        inside = [s for s in samples if start / 1000 <= s[0] <= end / 1000]
        if len(inside) < 2:
            rows.append((name, None, None, None))
            continue
        first, last = inside[0], inside[-1]
        span = last[0] - first[0]
        total = (last[1] - first[1]) / tick / span * 100
        busiest = max(((last[3].get(t, 0) - v) / tick / span * 100 for t, v in first[3].items()),
                      default=0)
        rows.append((name, total, busiest, max(s[2] for s in inside) / 1024))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--local', action='store_true', help='Run the desktop build instead')
    parser.add_argument('--no-sounds', action='store_true',
                        help="Leave out the device's installed sound packs")
    parser.add_argument('--max-thread', type=float, default=70,
                        help='Fail when the busiest thread uses more of a core (percent)')
    args = parser.parse_args()
    events, phases, end = script()
    with tempfile.TemporaryDirectory() as folder:
        folder = Path(folder)
        (folder/'load.events').write_text('\n'.join(events) + '\n')
        (folder/'sampler.py').write_text(SAMPLER)
        if args.local:
            output = run_local(folder, end)
        else:
            output = run_device(folder, end, not args.no_sounds)
    samples, tick, overflow = [], 100, None
    for line in output.splitlines():
        parts = line.split()
        if parts[:1] == ['tick']:
            tick = int(parts[1])
        elif 'queue_overflows=' in line:
            overflow = int(line.rsplit('=', 1)[1])
        elif len(parts) >= 3 and parts[0].replace('.', '', 1).isdigit():
            threads = dict((k, int(v)) for k, v in (p.split(':') for p in parts[3:]))
            samples.append((float(parts[0]), int(parts[1]), int(parts[2]), threads))
    rows = analyse(samples, tick, phases)
    print(f'{"phase":<18} {"process":>8} {"busiest":>8} {"RSS":>7}')
    ok = True
    for name, total, busiest, rss in rows:
        if total is None:
            print(f'{name:<18} {"no samples":>8}')
            ok = False
            continue
        print(f'{name:<18} {total:7.0f}% {busiest:7.0f}% {rss:5.0f} MB')
        ok = ok and busiest <= args.max_thread
    print('Audio queue overflows:', 'unknown' if overflow is None else overflow)
    ok = ok and overflow == 0
    print('PASS' if ok else 'FAIL', f'(busiest thread limit {args.max_thread:.0f}% of one core)')
    return 0 if ok else 1


def run_local(folder, end):
    config = folder/'config'
    (config/'zerochord').mkdir(parents=True)
    env = dict(os.environ, XDG_CONFIG_HOME=str(config))
    binary = ROOT/'build/desktop/M5CardputerZero-ZeroChord'
    import time
    start = time.time()
    app = subprocess.Popen([str(binary), '--headless', '--script', str(folder/'load.events'),
                            '--quit-after-ms', str(end)], env=env, stderr=subprocess.PIPE,
                           text=True)
    sampler = subprocess.run([sys.executable, str(folder/'sampler.py'), str(app.pid), str(start)],
                             capture_output=True, text=True)
    return sampler.stdout + app.communicate()[1]


def run_device(folder, end, sounds):
    host = os.environ.get('ZEROCHORD_SSH_HOST')
    if not host:
        sys.exit('Load the project .env first (ZEROCHORD_SSH_HOST).')
    ssh = ['-o', 'LogLevel=ERROR']
    if os.environ.get('ZEROCHORD_SSH_CONFIG'):
        ssh += ['-F', os.environ['ZEROCHORD_SSH_CONFIG']]
    remote = os.environ.get('ZEROCHORD_REMOTE_DIR', 'ZeroChord')
    target = 'build/load-test'
    subprocess.run(['ssh', *ssh, host, 'mkdir', '-p', f'{remote}/{target}'], check=True)
    subprocess.run(['scp', '-q', *ssh, str(folder/'load.events'), str(folder/'sampler.py'),
                    f'{host}:{remote}/{target}/'], check=True)
    # The preview owns the screen and audio: stop it, run the test, then bring it back.
    command = f'''set -eu
cd "{remote}"
root=$(pwd)
test=$root/{target}
rm -rf "$test/config"; mkdir -p "$test/config/zerochord"
# Installed sound packs load as they do for the player (each bank renders on every part).
if {'true' if sounds else 'false'} && test -d "$HOME/.config/zerochord/sounds"; then
    ln -s "$HOME/.config/zerochord/sounds" "$test/config/zerochord/sounds"
fi
systemctl --user stop zerochord-preview.service 2>/dev/null || true
XDG_CONFIG_HOME="$test/config" sh scripts/run_device_preview.sh --script "$test/load.events" \\
    --quit-after-ms {end} 2> "$test/app.log" &
start=$(date +%s.%N)
for n in $(seq 50); do
    pid=$(pgrep -f '^build/device/M5CardputerZero-ZeroChord --script' || true)
    test -n "$pid" && break
    sleep .1
done
python3 "$test/sampler.py" "$pid" "$start"
wait
cat "$test/app.log"
systemd-run --user --unit=zerochord-preview --collect --service-type=exec \\
    /bin/sh "$root/scripts/run_device_preview.sh" >/dev/null
'''
    result = subprocess.run(['ssh', *ssh, host, 'sh', '-s'], input=command, capture_output=True,
                            text=True, timeout=end / 1000 + 120)
    if result.returncode:
        sys.exit(result.stderr)
    return result.stdout


if __name__ == '__main__':
    sys.exit(main())
