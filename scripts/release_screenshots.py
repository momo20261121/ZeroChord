#!/usr/bin/env python3
"""Capture native-size release screenshots using isolated local settings."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
out = root/'store/screenshots'
out.mkdir(parents=True, exist_ok=True)
docs = root/'docs/images'
# Key scenes: keys 25 ms apart. Loop scenes: timed (ms, key, down) events and an end time.
key_scenes = {
    'performance': ['8','v'],
    'style': ['TAB','x','x'],
    'volume-effects': ['TAB','TAB','x','ENTER','x'],
    'sounds': ['TAB','x','x','x','ENTER','ENTER'],
    'chord-config': ['TAB','TAB','x','x','ENTER'],
    'app-settings': ['TAB','TAB','x'],
    'backing-layers': ['o'],
}
def tap(at, key, length=10):
    return [(at, key, True), (at+length, key, False)]
# Pop at its own 100 BPM (no edits): record C, G, Am, F a bar each, play the loop after a
# one-bar count-in, and catch it in its second bar (G).
bar = 4*60000/100
loop = tap(20, '0') + tap(300, 'ENTER') + tap(400, '8', 200)
for n, key in enumerate(['5','t','4']):
    loop += tap(int(400+(n+1)*bar), key, 200)
finish = int(400+4*bar)
loop += tap(finish, 'ENTER') + tap(finish+200, 'ENTER')
second = int(finish+200+bar+bar*1.5) # Halfway through the loop's second bar.
loop_scenes = {
    'loop': (loop + [(second-300, 'b', True)], second),
    'all-front': (loop + tap(second-2000, 'SPACE', 700) + [(second-400, 'r', True),
                                                          (second-350, 'j', True)], second),
    'loop-settings': (loop + [e for n, k in enumerate(['TAB','TAB','x','x','x','x','x','ENTER'])
                              for e in tap(second-1200+n*40, k)], second),
}
# The store leads with the looper; the other scenes illustrate the guides.
store = ['loop', 'all-front', 'backing-layers', 'sounds', 'loop-settings']
docs_prefix = {'app-settings': '', 'backing-layers': ''}
with tempfile.TemporaryDirectory(prefix='zerochord-release-') as temporary:
    work = Path(temporary)
    scenes = {name: ([e for n, k in enumerate(keys) for e in tap(20+n*25, k)], 100+len(keys)*25)
              for name, keys in key_scenes.items()}
    scenes.update(loop_scenes)
    for name, (events, end) in scenes.items():
        script = work/(name+'.events')
        script.write_text(''.join(f'{at} {key} {"down" if down else "up"}\n'
                                  for at, key, down in sorted(events, key=lambda e: e[0])))
        ppm = work/(name+'.ppm')
        environment = dict(os.environ, ZEROCHORD_HOME=str(work/name/'zerochord'))
        subprocess.run([str(root/'build/desktop/M5CardputerZero-ZeroChord'),
                        '--headless', '--no-audio', '--no-save', '--script', str(script),
                        '--capture', str(ppm), '--quit-after-ms', str(end)],
                       cwd=root, env=environment, check=True, timeout=30)
        if not ppm.read_bytes().startswith(b'P6\n320 170\n255\n'):
            raise SystemExit('Screenshot is not 320x170')
        destination = docs/(docs_prefix.get(name, 'release-')+name+'.png')
        subprocess.run([sys.executable, str(root/'scripts/ppm_to_png.py'), str(ppm), str(destination)],
                       check=True, stdout=subprocess.DEVNULL)
        if name in store:
            shutil.copy2(destination, out/(str(store.index(name)+1)+'-'+name+'.png'))
        print(destination)
