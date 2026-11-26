#!/usr/bin/env python3
"""Run the real app with Cardputer Tab, number and Z/C/F/X navigation and isolated settings."""
import argparse
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, default=ROOT/'build/desktop/M5CardputerZero-ZeroChord')
parser.add_argument('--out', type=Path, default=ROOT/'build/scenario-flows')
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)


def run(name, keys, expected, seed='', hold_escape=False):
    folder = args.out/name
    config = folder/'config/zerochord'
    config.mkdir(parents=True, exist_ok=True)
    # Fresh isolated state makes this repeatable and protects the player's setup.
    import shutil
    shutil.rmtree(config)
    config.mkdir()
    if seed:
        (config/'settings.conf').write_text('frontMappingVersion 2\n'+seed)
    events = []
    for n, key in enumerate(keys):
        at = 20+n*12
        events.extend([f'{at} {key} down', f'{at+5} {key} up'])
    duration = 100+len(keys)*12
    if hold_escape:
        events.append(f'{duration} ESC down')
        duration += 1000
    script = folder/'keys.events'
    script.write_text('\n'.join(events)+'\n')
    capture = folder/'screen.ppm'
    env = dict(os.environ, XDG_CONFIG_HOME=str(config.parent.resolve()))
    result = subprocess.run([str(args.binary.resolve()), '--headless', '--no-audio',
                             '--script', str(script.resolve()), '--capture', str(capture.resolve()),
                             '--quit-after-ms', str(duration)], cwd=ROOT, env=env,
                            capture_output=True, text=True, timeout=8)
    if result.returncode:
        raise RuntimeError(f'{name}: {result.stderr}')
    actual = dict(line.split(maxsplit=1) for line in (config/'settings.conf').read_text().splitlines())
    for field, value in expected.items():
        assert actual[field] == str(value), (name, field, value, actual[field])
    assert capture.read_bytes().startswith(b'P6\n320 170\n255\n'), name
    # Repeat startup to verify values survive both settings and preset session loading.
    result = subprocess.run([str(args.binary.resolve()), '--headless', '--no-audio',
                             '--quit-after-ms', '25'], cwd=ROOT, env=env,
                            capture_output=True, text=True, timeout=8)
    assert result.returncode == 0, (name, result.stderr)
    actual = dict(line.split(maxsplit=1) for line in (config/'settings.conf').read_text().splitlines())
    for field, value in expected.items():
        assert actual[field] == str(value), (name, 'restart', field, value, actual[field])
    if os.uname().sysname == 'Darwin':
        subprocess.run(['sips', '-s', 'format', 'png', str(capture), '--out', str(folder/'screen.png')],
                       check=True, stdout=subprocess.DEVNULL)
    print('PASS:', name)


# TAB opens settings and then moves to the next tab; number keys jump to a tab.
# Z/C (left/right) step numbers at once; Enter opens pages, lists and confirmations.
volume_effects = ['TAB','TAB','x','ENTER']
chord_config = ['TAB','TAB','x','x','ENTER']
front_envelope = ['TAB','x','x','x','ENTER','x','ENTER']

run('fresh-defaults', [], {'chordVolume':80, 'harpVolume':100, 'preset':1, 'mode':7, 'chordPattern':5, 'bpm':80})
run('volume-settings', volume_effects, {'masterVolume':100})
run('physical-volume-edit', volume_effects + ['x','x','c','x','z'],
    {'chordVolume':85, 'harpVolume':95})
run('physical-volume-restore', volume_effects + ['x','x','BACKSPACE'],
    {'chordVolume':80, 'harpVolume':62}, 'chordVolume 37\nharpVolume 62\n')
run('tempo-type-and-cancel', ['p','1','3','7','ENTER','p','2','4','0','ESC'], {'bpm':137})
run('menu-tempo-type', ['TAB','x','ENTER','4','0','ENTER'], {'bpm':40})
run('menu-tempo-arrows', ['TAB','x','c','c','c','z'], {'bpm':82})
run('tempo-invalid-stays-open', ['p','2','4','1','ENTER'], {'bpm':80})
run('absolute-roots-and-octaves',
    chord_config + ['x','ENTER','x','ENTER','2','ENTER'] + ['x']*7 + ['ENTER','3','f'] + ['c']*6,
    {'tonic':1, 'degreeChordRoot0':6, 'degreeChordOctave1':4})
run('chord-scale-and-hold', chord_config + ['x','x','ENTER','x','ENTER','x','ENTER','f','ENTER'],
    {'chordScale':1, 'hold':0})
run('chord-timing', chord_config + ['x']*8 + ['c']*18 + ['c'], {'combinationMs':120})
run('front-scale-physical', ['TAB','TAB','f','ENTER','ENTER','x','ENTER','x','x','ENTER','x','x','ENTER'],
    {'frontMode':1, 'scale':1, 'frontFollow':0})
run('front-key-follows-again',
    ['TAB','TAB','f','ENTER','ENTER','x','ENTER','x','ENTER','x','ENTER','BACKSPACE'],
    {'frontMode':1, 'frontFollow':1})
run('envelope-physical', front_envelope + ['c','ESC','ESC','f','ENTER','2','x','x','ENTER','x','x','c'],
    {'envelope00':5000, 'envelope12':55}, 'envelope00 3000\nenvelope12 50\n')
run('picker-cancel-physical', front_envelope + ['ENTER','x','ESC'], {'envelope00':-1})
run('light-volume-editor', volume_effects + ['x','x'], {'theme':1,'chordVolume':80}, 'theme 1\n')
run('long-escape-saves', volume_effects + ['x','x','c'], {'chordVolume':85}, hold_escape=True)
run('eq-physical-edit', volume_effects + ['x']*5 + ['c']*6 + ['x'] + ['z']*3 +
    ['x','RIGHT','RIGHT','RIGHT','RIGHT','ESC'], {'eqLowDb':6,'eqMidDb':-3,'eqHighDb':12},
    'eqHighDb 10\n')
run('eq-physical-restore', volume_effects + ['x']*5+['BACKSPACE'],
    {'eqLowDb':0,'eqMidDb':-3,'eqHighDb':2}, 'eqLowDb 6\neqMidDb -3\neqHighDb 2\ntheme 1\n')
run('sustained-chord-physical', ['TAB','x','x','ENTER','2','ENTER','x','ENTER'],
    {'chordPattern':5}, 'chordPattern 0\n')
run('backing-live-mute', ['o','ENTER','ESC'], {'mode':6})
run('backing-mute-and-unmute', ['o','ENTER','ENTER','ESC'], {'mode':7})
run('backing-mix', ['o','z','z','x','c'], {'chordMix':90, 'arpMix':100, 'mode':7})
run('backing-front-only', ['o','ENTER','x','ENTER','x','ENTER'], {'mode':3})
run('backing-arp-row', ['o','x','ENTER'], {'mode':4})
run('backing-style-page', ['TAB','x','x','ENTER','x','x','ENTER'], {'mode':5})
run('backing-two-layers', ['o','ENTER','x','x','ENTER'], {'mode':2})
run('restore-preset-confirm', ['TAB','f','ENTER','ENTER'], {'bpm':80}, 'bpm 120\n')
run('restore-preset-keep', ['TAB','f','ENTER','ESC'], {'bpm':120}, 'bpm 120\n')
run('front-source-rate', ['TAB','x','x','x','ENTER','ENTER'], {'harpProgram':10})
run('mixed-source-rate', ['TAB','x','x','ENTER','2','x','ENTER'], {'chordProgram':88})
run('strings-source-rate', ['TAB','x','x','x','ENTER','ENTER','TAB'], {'harpProgram':10})
run('about-dark', ['TAB','TAB','TAB','x','ENTER','BACKSPACE'], {'theme':0, 'preset':1})
run('about-light', ['TAB','3'], {'theme':1}, 'theme 1\n')
run('about-wrap', ['TAB','3','TAB','x','ENTER','1','3','7','ENTER'], {'bpm':137})
print('PASS: 37 real-app flows with physical navigation, persistence, 320x170 capture and clean/long-Esc exit')
