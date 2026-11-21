#!/usr/bin/env python3
"""Finish the APPLaunch helper staging tree and build the native ARM64 package."""
import argparse
from pathlib import Path
import shutil
import struct
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, default=root/'build/device/M5CardputerZero-ZeroChord')
parser.add_argument('--stage', type=Path, default=root/'build/package/debian-zerochord')
parser.add_argument('--helper', type=Path, required=True, help='Path to make_applauncher_package.py from the CardputerZero application skill')
args = parser.parse_args()
binary = args.binary.resolve()
header = binary.read_bytes()[:20]
assert header[:4] == b'\x7fELF' and header[4:6] == b'\x02\x01', 'Expected Linux ELF64 little-endian binary'
assert struct.unpack_from('<H', header, 18)[0] == 183, 'Expected AArch64 binary'
assert (root/'assets/share/images/zerochord.png').read_bytes()[:8] == b'\x89PNG\r\n\x1a\n'
subprocess.run([
    'python3', str(args.helper), '--app-name', 'ZeroChord', '--package', 'zerochord',
    '--version', '0.1.0', '--exec', 'bin/run-zerochord',
    '--icon', str(root/'assets/share/images/zerochord.png'),
    '--maintainer', 'ZeroChord contributors <zerochord@example.invalid>',
    '--description', 'Keyboard chord and harp instrument for CardputerZero',
    '--out', str(args.stage.parent),
], check=True)
stage = args.stage
app = stage/'usr/share/APPLaunch'
shutil.copy2(binary, app/'bin/M5CardputerZero-ZeroChord')
shutil.copy2(root/'assets/bin/run-zerochord', app/'bin/run-zerochord')
for name in ['M5CardputerZero-ZeroChord', 'run-zerochord']:
    (app/'bin'/name).chmod(0o755)
subprocess.run(['strip', '--strip-unneeded', str(app/'bin/M5CardputerZero-ZeroChord')], check=True)
shutil.copy2(root/'assets/applications/zerochord.desktop', app/'applications/zerochord.desktop')
docs = stage/'usr/share/doc/zerochord'
docs.mkdir(parents=True, exist_ok=True)
for name in ['LICENSE', 'THIRD_PARTY_NOTICES.md']:
    shutil.copy2(root/name, docs/name)
shutil.copytree(root/'third_party/licenses', docs/'licenses', dirs_exist_ok=True)
(docs/'copyright').write_text((root/'THIRD_PARTY_NOTICES.md').read_text()+'\n'+(root/'LICENSE').read_text())
(stage/'DEBIAN/control').write_text('''Package: zerochord
Version: 0.1.0
Architecture: arm64
Maintainer: ZeroChord contributors <zerochord@example.invalid>
Section: sound
Priority: optional
Depends: libc6 (>= 2.38), libstdc++6 (>= 14), libgcc-s1, libpulse0, timgm6mb-soundfont
Description: Keyboard chord and harp instrument for CardputerZero
 Scale-relative chord grid, combination chords, seven harp keys, strums,
 arpeggiator, complete editable presets and a native 320x170 LVGL interface.
''')
dist = root/'dist'
dist.mkdir(exist_ok=True)
destination = dist/'zerochord_0.1.0_arm64.deb'
for directory in [stage, *[p for p in stage.rglob('*') if p.is_dir()]]:
    directory.chmod(0o755)
for path in docs.rglob('*'):
    if path.is_file():
        path.chmod(0o644)
subprocess.run(['dpkg-deb', '--root-owner-group', '-Zxz', '-z3', '-b', str(stage), str(destination)], check=True)
print(destination)
