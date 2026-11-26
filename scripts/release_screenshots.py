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
store_scenes = {
    'performance': ['8','v'],
    'style': ['TAB','x','x'],
    'volume-effects': ['TAB','TAB','x','ENTER','x'],
    'sounds': ['TAB','x','x','x','ENTER','ENTER'],
    'chord-config': ['TAB','TAB','x','x','ENTER'],
}
docs_scenes = {'app-settings': ['TAB','TAB','x'], 'backing-layers': ['o']}
with tempfile.TemporaryDirectory(prefix='zerochord-release-') as temporary:
    work = Path(temporary)
    for name, keys in {**store_scenes, **docs_scenes}.items():
        script = work/(name+'.events')
        events = []
        for n, key in enumerate(keys):
            events.extend([f'{20+n*25} {key} down', f'{30+n*25} {key} up'])
        script.write_text('\n'.join(events)+'\n')
        ppm = work/(name+'.ppm')
        environment = dict(os.environ, XDG_CONFIG_HOME=str(work/name))
        subprocess.run([str(root/'build/desktop/M5CardputerZero-ZeroChord'),
                        '--headless', '--no-audio', '--no-save', '--script', str(script),
                        '--capture', str(ppm), '--quit-after-ms', str(100+len(keys)*25)],
                       cwd=root, env=environment, check=True)
        if not ppm.read_bytes().startswith(b'P6\n320 170\n255\n'):
            raise SystemExit('Screenshot is not 320x170')
        prefix = 'release-' if name in store_scenes else ''
        destination = docs/(prefix+name+'.png')
        subprocess.run([sys.executable, str(root/'scripts/ppm_to_png.py'), str(ppm), str(destination)],
                       check=True, stdout=subprocess.DEVNULL)
        if name in store_scenes:
            shutil.copy2(destination, out/(name+'.png'))
        print(destination)
