#!/usr/bin/env python3
"""Capture four clean release screenshots using only isolated local settings."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
out = root/'store/screenshots'
out.mkdir(parents=True, exist_ok=True)
scenes = {
    'performance': ['8'],
    'style': ['TAB'],
    'sounds': ['TAB']+['x']*6+['ENTER'],
    'chord-config': ['TAB','c','f','ENTER'],
}
with tempfile.TemporaryDirectory(prefix='zerochord-release-') as temporary:
    work = Path(temporary)
    for name, keys in scenes.items():
        script = work/(name+'.events')
        events = []
        for n, key in enumerate(keys):
            events.extend([f'{20+n*25} {key} down', f'{30+n*25} {key} up'])
        script.write_text('\n'.join(events)+'\n')
        ppm = work/(name+'.ppm')
        environment = dict(os.environ, XDG_CONFIG_HOME=str(work/name))
        subprocess.run([str(root/'build/desktop/M5CardputerZero-ZeroChord'),
                        '--headless', '--no-audio', '--no-save', '--script', str(script),
                        '--capture', str(ppm), '--quit-after-ms', str(150+len(keys)*25)],
                       cwd=root, env=environment, check=True)
        if not ppm.read_bytes().startswith(b'P6\n320 170\n255\n'):
            raise SystemExit('Screenshot is not 320x170')
        destination = out/(name+'.png')
        subprocess.run(['sips', '-s', 'format', 'png', str(ppm), '--out', str(destination)],
                       check=True, stdout=subprocess.DEVNULL)
        shutil.copy2(destination, root/'docs/images'/('release-'+name+'.png'))
        print(destination)
