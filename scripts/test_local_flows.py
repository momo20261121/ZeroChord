#!/usr/bin/env python3
"""Run the real app with Cardputer Z/C/F/X navigation and isolated settings."""
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


run('fresh-defaults', [], {'chordVolume':80, 'harpVolume':100, 'preset':1, 'mode':7, 'chordPattern':5, 'bpm':80})
run('physical-volume-edit', ['TAB','c','x','x','ENTER','x','ENTER','x','ENTER','f','ENTER'],
    {'chordVolume':85, 'harpVolume':95})
run('physical-volume-restore', ['TAB','c','x','x','BACKSPACE'],
    {'chordVolume':80, 'harpVolume':62}, 'chordVolume 37\nharpVolume 62\n')
run('tempo-type-and-cancel', ['p','1','3','7','ENTER','p','2','4','0','ESC'], {'bpm':137})
run('menu-tempo-type', ['TAB','x','ENTER','4','0','ENTER'], {'bpm':40})
run('tempo-invalid-stays-open', ['p','2','4','1','ENTER'], {'bpm':80})
run('absolute-roots-and-octaves',
    ['TAB','c','f','ENTER','x','ENTER','x','ENTER','c','ENTER']+['x']*7+
    ['ENTER','c','f','ENTER']+['x']*6+['ENTER'],
    {'tonic':1, 'degreeChordRoot0':6, 'degreeChordOctave1':4})
run('front-scale-physical',
    ['TAB','c','f','f','ENTER','ENTER','x','ENTER','x','ENTER','x','ENTER'],
    {'frontMode':1,'scale':1})
run('envelope-physical', ['TAB']+['x']*7+['ENTER','ENTER']+['x']*6+['ENTER','c','x','ENTER']+
    ['f']*4+['ENTER'], {'envelope00':100,'envelope11':50})
run('picker-cancel-physical', ['TAB']+['x']*7+['ENTER','ENTER']+['x']*6+['ESC'], {'envelope00':-1})
run('light-volume-editor', ['TAB','c','x','x','ENTER'], {'theme':1,'chordVolume':80}, 'theme 1\n')
run('long-escape-saves', ['TAB','c','x','x','ENTER','x','ENTER'], {'chordVolume':85}, hold_escape=True)
run('eq-physical-edit-cancel', ['TAB','c']+['x']*5+['ENTER']+['x']*6+
    ['ENTER','x','ENTER']+['f']*3+['ENTER','x','ENTER','x','x','ESC'],
    {'eqLowDb':6,'eqMidDb':-3,'eqHighDb':0})
run('eq-physical-restore', ['TAB','c']+['x']*5+['BACKSPACE'],
    {'eqLowDb':0,'eqMidDb':-3,'eqHighDb':2}, 'eqLowDb 6\neqMidDb -3\neqHighDb 2\ntheme 1\n')
run('sustained-chord-physical', ['TAB']+['x']*4+['ENTER','ENTER'], {'chordPattern':5})
run('backing-popup-cancel', ['o','ENTER','ESC'], {'mode':7})
run('backing-all-layers', ['o','x','x','x','ENTER'], {'mode':7})
run('backing-front-only', ['o','ENTER','x','ENTER','x','ENTER','x','ENTER'], {'mode':3})
run('backing-style-popup', ['TAB','x','x','ENTER','x','x','x','ENTER'], {'mode':7})
run('backing-popup-preview', ['o','ENTER','x','x','ENTER'], {'mode':7})
run('front-source-rate', ['TAB']+['x']*6+['ENTER'], {'harpProgram':10})
run('mixed-source-rate', ['TAB']+['x']*8+['ENTER'], {'chordProgram':88})
run('strings-source-rate', ['TAB']+['x']*6+['ENTER','c'], {'harpProgram':10})
run('about-dark', ['TAB','c','c','x','ENTER','BACKSPACE'], {'theme':0, 'preset':1})
run('about-light', ['TAB','c','c'], {'theme':1}, 'theme 1\n')
run('about-wrap', ['TAB','z','c','x','ENTER','1','3','7','ENTER'], {'bpm':137})
print('PASS: 26 real-app flows with physical navigation, persistence, 320x170 capture and clean/long-Esc exit')
