#!/usr/bin/env python3
"""Device-only integration test: real evdev input, framebuffer and audio.

Creates a temporary uinput keyboard, pauses the user's launcher, runs the app,
tests press/release combinations and Esc, then always restores the launcher.
Does not alter the physical keyboard by-path symlink. Requires input/video groups.
"""
import fcntl
from array import array
import os
from pathlib import Path
import struct
import subprocess
import time

root = Path(__file__).resolve().parents[1]
binary = root / 'build/device/M5CardputerZero-ZeroChord'
output = root / 'build/device-tests'
output.mkdir(exist_ok=True)
fd = os.open('/dev/uinput', os.O_WRONLY | os.O_NONBLOCK)
name = 'ZeroChord integration keyboard'
process = None
monitor = None
recording = None
launcher_stopped = False
created = False

def emit(code, down):
    os.write(fd, struct.pack('llHHi', 0, 0, 1, code, int(down)))
    os.write(fd, struct.pack('llHHi', 0, 0, 0, 0, 0))

def tap(code):
    emit(code, True)
    time.sleep(.045)
    emit(code, False)
    time.sleep(.08)

def recorded_energy(duration):
    """Measure the live output monitor, after allowing its buffer to settle."""
    start = (output/'output.s16le').stat().st_size
    time.sleep(duration)
    end = (output/'output.s16le').stat().st_size
    with (output/'output.s16le').open('rb') as audio:
        audio.seek(start)
        data = audio.read(end-start)
    values = array('h')
    values.frombytes(data)
    assert len(values) > 4800, 'live monitor did not deliver audio frames'
    # Ten-millisecond energy windows expose gaps between arpeggio notes.
    return [sum(v*v for v in values[n:n+960])/960 for n in range(0, len(values)-960, 960)]

def framebuffer_capture(destination):
    """Read the kernel framebuffer, independently of the app's screenshot buffer."""
    entries = Path('/proc/fb').read_text().splitlines()
    match = next(line.split()[0] for line in entries if 'panel-mipi' in line or 'st7789' in line)
    with open('/dev/fb'+match, 'rb', buffering=0) as framebuffer:
        info = bytearray(160)
        fcntl.ioctl(framebuffer.fileno(), 0x4600, info)  # FBIOGET_VSCREENINFO
        values = struct.unpack_from('=40I', info)
        width, height, _, _, xoffset, yoffset, bpp = values[:7]
        assert (width, height) == (320, 170) and bpp in (16, 32)
        stride = int(Path('/sys/class/graphics/fb'+match+'/stride').read_text())
        raw = framebuffer.read(stride*(height+yoffset))
    rgb = bytearray()
    for y in range(height):
        for x in range(width):
            offset = (y+yoffset)*stride+(x+xoffset)*(bpp//8)
            pixel = int.from_bytes(raw[offset:offset+bpp//8], 'little')
            for shift, bits in [(values[8], values[9]), (values[11], values[12]), (values[14], values[15])]:
                rgb.append(((pixel >> shift) & ((1 << bits)-1))*255//((1 << bits)-1))
    destination.write_bytes(b'P6\n320 170\n255\n'+rgb)
    assert len(set(rgb)) > 16, 'kernel framebuffer is blank or missing UI'
    return rgb

try:
    fcntl.ioctl(fd, 0x40045564, 1)  # UI_SET_EVBIT / EV_KEY
    for code in range(1, 256):
        fcntl.ioctl(fd, 0x40045565, code)  # UI_SET_KEYBIT
    os.write(fd, struct.pack('80sHHHHI', name.encode(), 3, 0x1209, 0x0001, 1, 0) + bytes(1024))
    fcntl.ioctl(fd, 0x5501)  # UI_DEV_CREATE
    created = True
    device = None
    for attempt in range(40):
        for path in Path('/sys/class/input').glob('event*'):
            if (path / 'device/name').read_text().strip() == name:
                device = '/dev/input/' + path.name
        if device and os.access(device, os.R_OK):
            break
        time.sleep(.1)
    assert device and os.access(device, os.R_OK), 'test keyboard unavailable'
    environment = dict(os.environ, ZEROCHORD_INPUT_DEVICE=device, XDG_CONFIG_HOME=str(output/('config-'+str(os.getpid()))))
    test_config = Path(environment['XDG_CONFIG_HOME'])/'zerochord/settings.conf'
    test_config.parent.mkdir(parents=True, exist_ok=True)
    test_config.write_text('preset 1\necho 0\nbpm 100\n')
    subprocess.run(['systemctl', '--user', 'stop', 'APPLaunch.service'], check=True)
    launcher_stopped = True
    # Reproduce an SSH preview launched while the launcher display was asleep.
    backlight = Path('/sys/class/backlight/backlight/brightness')
    backlight.write_text('0\n')
    with (output/'wake.log').open('w') as log:
        process = subprocess.Popen(['/bin/sh', str(root/'scripts/run_device_preview.sh'), '--no-save', '--no-audio', '--quit-after-ms', '1800'], cwd=root, env=environment, stdout=log, stderr=log)
        time.sleep(.8)
        assert process.poll() is None, 'preview failed to start'
        assert int(backlight.read_text()) > 0, 'preview failed to wake backlight'
        framebuffer_capture(output/'wake-fb.ppm')
        assert process.wait(timeout=5) == 0, 'preview failed to restore launcher'
    assert subprocess.check_output(['systemctl', '--user', 'is-active', 'APPLaunch.service'], text=True).strip() == 'active'
    subprocess.run(['systemctl', '--user', 'stop', 'APPLaunch.service'], check=True)
    sink = subprocess.check_output(['pactl', 'get-default-sink'], text=True).strip()
    recording = (output/'output.s16le').open('wb')
    monitor = subprocess.Popen(['parec', '--device='+sink+'.monitor', '--format=s16le', '--rate=48000', '--channels=2', '--latency-msec=50'], stdout=recording, stderr=subprocess.DEVNULL)
    with (output/'performance.log').open('w') as log:
        process = subprocess.Popen([str(binary), '--no-save', '--capture', str(output/'performance.ppm')], cwd=root, env=environment, stdout=log, stderr=log)
        time.sleep(1.5)
        assert process.poll() is None, 'app failed to start'
        tap(9)  # 8: C, sustained Dream pad
        time.sleep(1.2)
        pad = recorded_energy(.6)
        framebuffer_capture(output/'pad-fb.ppm')
        tap(24)  # O: Chord, four simultaneous notes immediately
        time.sleep(1.5)
        chord_tail = recorded_energy(.4)
        framebuffer_capture(output/'chord-fb.ppm')
        tap(24)  # O: Arp, repeated distinct notes
        time.sleep(.3)
        arp = recorded_energy(1.5)
        framebuffer_capture(output/'arp-fb.ppm')
        print('Live Dream modes: Pad peak energy='+str(round(max(pad), 2))+
              ', Chord tail='+str(round(max(chord_tail), 2))+
              ', Arp min/max='+str(round(min(arp), 2))+'/'+str(round(max(arp), 2)))
        assert max(pad) > 1, 'Pad output was silent'
        assert max(chord_tail) < max(pad)*.0001, 'Manual chord kept sustaining or repeating'
        assert max(arp) > 1 and min(arp) < max(arp)*.001, 'Arp output has no quiet gaps'
        tap(24)  # O: Front only
        tap(24)  # O: back to Pad
        emit(9, True)  # 8: C
        emit(36, True)  # J: Cmaj7
        time.sleep(.1)
        emit(9, False)
        emit(36, False)
        tap(38)  # L ascending
        time.sleep(.4)
        tap(37)  # K descending
        tap(24)  # O: Chord
        tap(24)  # O: Arp
        time.sleep(.5)
        tap(57)  # Space panic
        tap(15)  # Tab settings
        framebuffer_capture(output/'style-settings-fb.ppm')
        tap(106) # Right: App tab, no setting changes
        tap(103); tap(28); tap(108) # Theme -> Chord config -> Key
        tap(28); tap(108); tap(28) # C -> C#
        tap(1); tap(1) # short Esc returns through config/settings; app remains alive
        assert process.poll() is None, 'short Esc incorrectly exited'
        tap(9)   # C# major
        tap(38)
        time.sleep(.3)
        # Edit shared no3 and a V-only sus4 override using actual evdev arrows.
        tap(15)  # Tab
        tap(28) # Reopen Chord config
        tap(108) # Key -> first combination
        tap(2)   # 1: All
        tap(28)
        tap(108)  # dim -> aug
        tap(108)  # aug -> no3
        tap(28)   # Commit shared no3
        tap(6)    # 5: V
        tap(28)
        tap(103)  # Up: inherited -> sus4 (wrap)
        tap(28)
        framebuffer_capture(output/'combinations-fb.ppm')
        tap(28)   # Open current sus4
        tap(108)  # Highlight inherited, then cancel
        framebuffer_capture(output/'chord-choices-fb.ppm')
        tap(1)    # Cancel picker
        tap(1)    # Back to Setup
        tap(1)    # Back to performance
        emit(6, True)    # V root (Ab with C# key)
        emit(19, True)   # R: combine -> Absus4
        time.sleep(.08)
        emit(19, False)
        emit(6, False)
        time.sleep(.2)
        emit(1, True)  # Long Esc exits
        time.sleep(1.1)
        emit(1, False)
        assert process.wait(timeout=3) == 0, 'long Esc did not exit cleanly'
        assert (output/'performance.ppm').stat().st_size > 160000
        assert int(backlight.read_text()) > 0, 'display backlight turned off'
        framebuffer = framebuffer_capture(output/'performance-fb.ppm')
        rendered = (output/'performance.ppm').read_bytes().split(b'\n', 3)[3]
        assert max(abs(a-b) for a, b in zip(framebuffer, rendered)) <= 7, 'kernel framebuffer differs from rendered UI'
    with (output/'signal.log').open('w') as log:
        process = subprocess.Popen([str(binary), '--no-save'], cwd=root, env=environment, stdout=log, stderr=log)
        time.sleep(1.5)
        tap(9)
        process.send_signal(2)
        assert process.wait(timeout=3) == 0, 'SIGINT did not exit cleanly'
    with (output/'settings.log').open('w') as log:
        process = subprocess.Popen([str(binary), '--no-save', '--script', 'tests/settings.events', '--capture', str(output/'settings.ppm'), '--quit-after-ms', '800'], cwd=root, env=environment, stdout=log, stderr=log)
        assert process.wait(timeout=8) == 0, 'settings capture failed'
    with (output/'persistence.log').open('w') as log:
        process = subprocess.Popen([str(binary), '--script', 'tests/customization.events', '--quit-after-ms', '1800'], cwd=root, env=environment, stdout=log, stderr=log)
        assert process.wait(timeout=8) == 0, 'customization persistence run failed'
    saved = Path(environment['XDG_CONFIG_HOME'])/'zerochord/settings.conf'
    settings = dict(line.split() for line in saved.read_text().splitlines())
    assert settings['combo3'] == '7' and settings['degree3combo3'] == '9', 'custom mappings not saved'
    with (output/'reload.log').open('w') as log:
        process = subprocess.Popen([str(binary), '--no-save', '--script', 'tests/reloaded-combinations.events', '--capture', str(output/'reloaded.ppm'), '--quit-after-ms', '1000'], cwd=root, env=environment, stdout=log, stderr=log)
        assert process.wait(timeout=8) == 0, 'customization reload failed'
    with (output/'presets.log').open('w') as log:
        process = subprocess.Popen([str(binary)], cwd=root, env=environment, stdout=log, stderr=log)
        time.sleep(1.5)
        tap(15)  # Tab: Style, Preset
        tap(106) # App: Theme
        tap(103); tap(28); tap(108) # Theme -> Chord config -> Key
        tap(28); tap(108); tap(28) # Customize key C# -> D
        tap(14)  # Backspace: restore just key to C
        tap(28); tap(108); tap(108); tap(28) # D is app-wide
        tap(108); tap(6) # Key -> first combination, V scope
        framebuffer_capture(output/'global-combinations-fb.ppm')
        tap(1) # Back to App
        tap(106) # Style, Preset
        tap(108) # Tempo
        tap(28); tap(2); tap(11); tap(6); tap(28) # Type BPM 105
        tap(28); tap(19); tap(1) # R selects default; cancel retains 105
        tap(108) # Mode
        tap(28); tap(108); tap(28) # Pad -> Chord
        for _ in range(2): tap(108) # Mode -> Arp pattern -> Chord pattern
        tap(28); tap(108); tap(28) # Every beat
        tap(108) # Pad motion
        tap(28); tap(108); tap(108); tap(28) # Breathe
        tap(106) # App, combinations (remembered row)
        tap(108) # Theme (wrap)
        tap(108) # Master
        tap(28); tap(103); tap(28) # 100 -> 95
        tap(103); tap(28); tap(108); tap(28) # Theme -> Light
        framebuffer_capture(output/'light-settings-fb.ppm')
        tap(106) # Style, Pad motion (remembered row)
        for _ in range(7): tap(108) # Pad motion -> Save as
        tap(28); tap(108); tap(28) # User001
        tap(1)
        tap(11); tap(10) # Switch presets, preserving global key/mappings
        tap(9)
        time.sleep(.8)
        framebuffer_capture(output/'preset-performance-fb.ppm')
        tap(24) # Customize Mode before restore
        tap(15); tap(108) # Style, Restore preset
        tap(28); tap(108); tap(28)
        framebuffer_capture(output/'preset-restore-fb.ppm')
        process.send_signal(2)
        assert process.wait(timeout=5) == 0, 'preset customization exit failed'
    settings = dict(line.split() for line in saved.read_text().splitlines())
    assert settings['tonic'] == '2' and settings['bpm'] == '105', 'individual reset/cancel or preset recall failed'
    assert settings['mode'] == '1' and settings['chordPattern'] == '1' and settings['padMotion'] == '2', 'preset patterns not retained'
    assert settings['combo3'] == '7' and settings['degree3combo3'] == '9', 'preset operation changed app mappings'
    assert settings['masterVolume'] == '95' and settings['theme'] == '1', 'preset reset changed app controls'
    config_root = saved.parent
    assert 'User001' in (config_root/'presets/user/user-001.toml').read_text(), 'save-as TOML missing'
    assert 'user-001' in (config_root/'session.toml').read_text(), 'stable preset selection missing'
    with (output/'preset-reload.log').open('w') as log:
        process = subprocess.Popen([str(binary), '--no-save'], cwd=root, env=environment, stdout=log, stderr=log)
        time.sleep(1.5)
        tap(9); time.sleep(.8)
        framebuffer_capture(output/'preset-reload-fb.ppm')
        tap(15)
        tap(28) # Style opens with Preset at top
        framebuffer_capture(output/'presets-fb.ppm')
        tap(1); tap(1)
        process.send_signal(2)
        assert process.wait(timeout=5) == 0, 'preset reload exit failed'
finally:
    if process is not None and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
    if monitor is not None:
        monitor.terminate()
        monitor.wait(timeout=3)
    if recording is not None:
        recording.close()
    try:
        if created:
            fcntl.ioctl(fd, 0x5502)  # UI_DEV_DESTROY
    finally:
        os.close(fd)
        if launcher_stopped:
            subprocess.run(['systemctl', '--user', 'start', 'APPLaunch.service'], check=True)
samples = array('h')
samples.frombytes((output/'output.s16le').read_bytes())
peak = max(map(abs, samples), default=0)
assert peak > 0, 'device audio monitor captured silence'
for name in ['performance.log', 'signal.log', 'settings.log', 'persistence.log', 'reload.log', 'presets.log', 'preset-reload.log']:
    text = (output/name).read_text()
    assert 'queue_overflows=0' in text, 'audio queue overflow or missing audio callbacks'
print('Audio output monitor: frames='+str(len(samples)//2)+' peak='+str(peak))
print('PASS: asleep-display wake, kernel framebuffer, evdev combinations, global/degree edits, picker cancel, persistence/reload, presets, field defaults, save-as, patterns, master/theme, sweeps, arp, panic, Esc, SIGINT; launcher restored')
