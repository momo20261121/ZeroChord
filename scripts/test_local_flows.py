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

run('fresh-defaults', [], {'chordVolume':80, 'harpVolume':100, 'preset':0, 'mode':7, 'chordPattern':5, 'bpm':70, 'drums':1, 'drumMix':100, 'bass':1, 'bassMix':71, 'swingMode':0, 'swingAmount':50})
run('volume-settings', volume_effects, {'masterVolume':100})
run('physical-volume-edit', volume_effects + ['x','x','c','x','z'],
    {'chordVolume':85, 'harpVolume':95})
run('physical-volume-restore', volume_effects + ['x','x','BACKSPACE'],
    {'chordVolume':80, 'harpVolume':62}, 'chordVolume 37\nharpVolume 62\n')
run('tempo-type-and-cancel', ['p','1','3','7','ENTER','p','2','4','0','ESC'], {'bpm':137})
run('menu-tempo-type', ['TAB','x','ENTER','4','0','ENTER'], {'bpm':40})
run('menu-tempo-arrows', ['TAB','x','c','c','c','z'], {'bpm':72})
run('tempo-invalid-stays-open', ['p','2','4','1','ENTER'], {'bpm':70})
run('tempo-swing', ['p','DOWN','ENTER','DOWN','RIGHT','RIGHT','c','z','ESC'],
    {'bpm':70, 'swingMode':1, 'swingAmount':60})
run('output-speaker', volume_effects + ['x']*8 + ['ENTER','x','ENTER'], {'output':1})
run('absolute-roots-and-octaves',
    chord_config + ['x','ENTER','x','ENTER','2','ENTER'] + ['x']*7 + ['ENTER','3','f'] + ['c']*6,
    {'tonic':1, 'degreeChordRoot0':6, 'degreeChordOctave1':4})
run('chord-scale-and-hold', chord_config + ['x','x','ENTER','x','ENTER','x','ENTER','f','ENTER'],
    {'chordScale':1, 'hold':0})
run('chord-timing', chord_config + ['x']*8 + ['c']*18 + ['c'], {'combinationMs':120})
run('front-scale-physical', ['TAB','TAB','x','x','x','ENTER','ENTER','x','ENTER','x','x','ENTER','x','x','ENTER'],
    {'frontMode':1, 'scale':1, 'frontFollow':0})
run('all-front-free-play', ['TAB','TAB','x','x','x','x','ENTER','ENTER','x','ENTER','x','c','c'],
    {'allFrontMode':1, 'allFrontLowest':57})
run('front-key-follows-again',
    ['TAB','TAB','x','x','x','ENTER','ENTER','x','ENTER','x','ENTER','x','ENTER','BACKSPACE'],
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
run('backing-mix', ['o','z','z','x','c'], {'chordMix':27, 'arpMix':77, 'mode':7})
run('backing-front-only', ['o','ENTER','x','ENTER','x','ENTER'], {'mode':3})
run('backing-arp-row', ['o','x','ENTER'], {'mode':4})
run('backing-style-page', ['TAB','x','x','ENTER','x','x','ENTER'], {'mode':5})
run('backing-two-layers', ['o','ENTER','x','x','ENTER'], {'mode':2})
run('backing-bass-mute', ['o','x','x','x','ENTER','ESC'], {'mode':7, 'bass':0, 'drums':1})
run('backing-drums-mute', ['o','x','x','x','x','ENTER','ESC'], {'mode':7, 'drums':0, 'bass':1})
run('backing-drum-kit', ['o','6','ENTER','x','ENTER','ESC'], {'drumKit':8, 'drums':1})
run('backing-bass-pattern', ['o','5','ENTER','x','ENTER','ESC'], {'bassPattern':2, 'bass':1})
run('restore-preset-confirm', ['TAB','f','ENTER','ENTER'], {'bpm':70}, 'bpm 120\n')
run('restore-preset-keep', ['TAB','f','ENTER','ESC'], {'bpm':120}, 'bpm 120\n')
run('front-source-rate', ['TAB','x','x','x','ENTER','ENTER'], {'harpProgram':11})
run('mixed-source-rate', ['TAB','x','x','ENTER','2','x','ENTER'], {'chordProgram':49})
run('strings-source-rate', ['TAB','x','x','x','ENTER','ENTER','TAB'], {'harpProgram':11})
run('shortcuts-read-only', ['TAB','3','x','x','c','ENTER','BACKSPACE'], {'theme':0, 'preset':0, 'bpm':70})
run('about-dark', ['TAB','TAB','TAB','TAB','x','ENTER','BACKSPACE'], {'theme':0, 'preset':0})
run('about-light', ['TAB','4'], {'theme':1}, 'theme 1\n')
run('about-wrap', ['TAB','4','TAB','x','ENTER','1','3','7','ENTER'], {'bpm':137})
# Keys 12 ms apart tap faster than the range allows: the tempo stops at 240 BPM.
run('tap-tempo', ['p','p','p','p','ENTER'], {'bpm':240})


def playing_screen(name, events, end, seed=''):
    """Capture the playing screen after timed key events, from fresh isolated settings."""
    import shutil
    folder = args.out/'playing'/name
    shutil.rmtree(folder, ignore_errors=True)
    config = folder/'config/zerochord'
    config.mkdir(parents=True)
    if seed:
        (config/'settings.conf').write_text('frontMappingVersion 2\n'+seed)
    script = folder/'keys.events'
    script.write_text(''.join(f'{at} {key} {action}\n' for at, key, action in events))
    capture = folder/'screen.ppm'
    result = subprocess.run([str(args.binary.resolve()), '--headless', '--no-audio', '--no-save',
                             '--script', str(script.resolve()), '--capture', str(capture.resolve()),
                             '--quit-after-ms', str(end)], cwd=ROOT,
                            env=dict(os.environ, XDG_CONFIG_HOME=str(config.parent.resolve())),
                            capture_output=True, text=True, timeout=8)
    assert result.returncode == 0, (name, result.stderr)
    return capture.read_bytes()


# The playing screen updates in place; after any playing it must match a fresh drawing.
busy = [(100,'2','down'), (150,'x','down'), (200,'x','up'), (250,'1','down'), (300,'7','down'),
        (320,'m','down'), (400,'m','up'), (420,'7','up'), (430,'1','up'), (450,'2','up'),
        (470,'l','down'), (490,'l','up')]
for theme in ('', 'theme 1\n'):
    for target in ([(1200,'5','down')], [(1200,'j','down'), (1250,'b','down')], [(1200,'i','down')]):
        fresh = playing_screen('fresh', target, 1500, theme)
        played = playing_screen('played', busy + target, 1500, theme)
        assert fresh == played, ('in-place playing screen differs', theme, target)
print('PASS: playing screen after busy playing matches a fresh drawing (dark and light)')
# All front (hold Space) also updates in place; a Space tap stops without leaving it.
enter = [(20,'SPACE','down'), (700,'SPACE','up')]
for mode in ('', 'allFrontMode 1\n'):
    keys = [(760,'e','down'), (780,'1','down'), (800,'d','down'), (820,'e','up'), (840,'w','down'),
            (850,'w','up'), (860,'q','down'), (870,'q','up'), (880,'1','up'), (900,'d','up'),
            (920,'SPACE','down'), (940,'SPACE','up')]
    target = [(1200,'m','down'), (1220,'4','down')]
    fresh = playing_screen('all-front-fresh', enter + target, 1500, mode)
    played = playing_screen('all-front-played', enter + keys + target, 1500, mode)
    assert fresh == played, ('in-place All front screen differs', mode)
    chords = playing_screen('all-front-back', enter + [(800,'SPACE','down'), (1500,'SPACE','up')], 1600, mode)
    assert chords == playing_screen('chord-screen', [], 1600, mode), ('Space hold did not return', mode)
print('PASS: All front matches a fresh drawing in both modes; holding Space returns to chords')
print('PASS: 46 real-app flows with physical navigation, persistence, 320x170 capture and clean/long-Esc exit')
