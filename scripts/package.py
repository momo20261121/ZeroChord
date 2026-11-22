#!/usr/bin/env python3
"""Package the locally built ARM64 release using Python's standard library."""
import argparse
import hashlib
import io
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tarfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, default=root/'build/cross-device/M5CardputerZero-ZeroChord')
parser.add_argument('--stage', type=Path, default=root/'build/package/debian-zerochord')
parser.add_argument('--helper', type=Path, help='Optional CardputerZero APPLaunch staging helper')
args = parser.parse_args()
version = (root/'VERSION.txt').read_text().strip()
if not re.fullmatch(r'\d+\.\d+\.\d+', version):
    raise SystemExit('VERSION.txt must contain a semantic version, e.g. 1.0.0')
binary = args.binary.resolve()
header = binary.read_bytes()[:20]
if header[:6] != b'\x7fELF\x02\x01' or struct.unpack_from('<H', header, 18)[0] != 183:
    raise SystemExit('Expected Linux AArch64 ELF64 binary')
icon = root/'assets/share/images/zerochord.png'
if icon.read_bytes()[:8] != b'\x89PNG\r\n\x1a\n':
    raise SystemExit('Missing PNG app icon')
maintainer = 'ZeroChord contributors <maintainers@example.invalid>'
stage = args.stage.resolve()
# Only replace the generated package staging directory, never project sources.
if stage == root or root not in stage.parents or 'build' not in stage.relative_to(root).parts:
    raise SystemExit('The staging directory must be below this project build directory')
if args.helper:
    subprocess.run([sys.executable, str(args.helper), '--app-name', 'ZeroChord',
                    '--package', 'zerochord', '--version', version, '--exec', 'bin/run-zerochord',
                    '--icon', str(icon), '--maintainer', maintainer,
                    '--description', 'Pocket chord and harp instrument for CardputerZero',
                    '--out', str(stage.parent)], check=True)
    helper_stage = stage.parent/'debian-zerochord'
    if helper_stage != stage:
        raise SystemExit('Helper requires a staging directory named debian-zerochord')
else:
    if stage.exists():
        shutil.rmtree(stage)
app = stage/'usr/share/APPLaunch'
for folder in [stage/'DEBIAN', app/'bin', app/'applications', app/'share/images']:
    folder.mkdir(parents=True, exist_ok=True)
shutil.copy2(binary, app/'bin/M5CardputerZero-ZeroChord')
shutil.copy2(root/'assets/bin/run-zerochord', app/'bin/run-zerochord')
shutil.copy2(root/'assets/applications/zerochord.desktop', app/'applications/zerochord.desktop')
shutil.copy2(icon, app/'share/images/zerochord.png')
for name in ['M5CardputerZero-ZeroChord', 'run-zerochord']:
    (app/'bin'/name).chmod(0o755)
docs = stage/'usr/share/doc/zerochord'
docs.mkdir(parents=True, exist_ok=True)
for name in ['README.md', 'CHANGELOG.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md']:
    shutil.copy2(root/name, docs/name)
shutil.copytree(root/'docs', docs/'docs', dirs_exist_ok=True)
(docs/'sounds').mkdir(exist_ok=True)
shutil.copy2(root/'sounds/README.md', docs/'sounds/README.md')
(docs/'assets/share/images').mkdir(parents=True)
shutil.copy2(icon, docs/'assets/share/images/zerochord.png')
shutil.copytree(root/'third_party/licenses', docs/'third_party/licenses', dirs_exist_ok=True)
(docs/'copyright').write_text((root/'THIRD_PARTY_NOTICES.md').read_text()+'\n'+(root/'LICENSE').read_text())
installed_size = sum(p.stat().st_size for p in (stage/'usr').rglob('*') if p.is_file())
(stage/'DEBIAN/control').write_text(f'''Package: zerochord
Version: {version}
Architecture: arm64
Maintainer: {maintainer}
Section: sound
Priority: optional
Homepage: https://example.invalid/ZeroChord
Installed-Size: {(installed_size+1023)//1024}
Depends: libc6 (>= 2.36), libpulse0, timgm6mb-soundfont
Description: Pocket chord and harp instrument for CardputerZero
 Play chords, front notes, strums and arpeggios on the CardputerZero keyboard.
 Includes nine editable styles, independent sounds and envelopes, global EQ,
 custom key mappings and a native 320x170 interface.
''')
for directory in [stage, *[p for p in stage.rglob('*') if p.is_dir()]]:
    directory.chmod(0o755)
for path in stage.rglob('*'):
    if path.is_file() and path.parent != app/'bin':
        path.chmod(0o644)
# Debian is an ar archive with two standard tar members. Build locally without
# GNU strip/dpkg on macOS; the Release link already strips the AArch64 executable.
def tar_bytes(folder):
    output = io.BytesIO()
    with tarfile.open(fileobj=output, mode='w:xz', format=tarfile.GNU_FORMAT) as tar:
        for path in sorted(folder.rglob('*')):
            info = tar.gettarinfo(str(path), arcname='./'+str(path.relative_to(folder)))
            info.uid = info.gid = 0
            info.uname = info.gname = 'root'
            if path.is_file():
                with path.open('rb') as source:
                    tar.addfile(info, source)
            else:
                tar.addfile(info)
    return output.getvalue()
dist = root/'dist'
dist.mkdir(exist_ok=True)
destination = dist/f'zerochord_{version}_arm64.deb'
# Data must exclude DEBIAN metadata.
data_folder = root/'build/package/data'
if data_folder.exists():
    shutil.rmtree(data_folder)
data_folder.mkdir(parents=True)
shutil.copytree(stage/'usr', data_folder/'usr')
with destination.open('wb') as out:
    out.write(b'!<arch>\n')
    for name, payload in [('debian-binary', b'2.0\n'),
                          ('control.tar.xz', tar_bytes(stage/'DEBIAN')),
                          ('data.tar.xz', tar_bytes(data_folder))]:
        entry = f'{name+"/":<16}{0:<12}{0:<6}{0:<6}{"100644":<8}{len(payload):<10}`\n'
        out.write(entry.encode('ascii'))
        out.write(payload)
        if len(payload) % 2:
            out.write(b'\n')
digest = hashlib.sha256(destination.read_bytes()).hexdigest()
(destination.with_suffix('.deb.sha256')).write_text(f'{digest}  {destination.name}\n')
print(f'{destination}\nSHA256 {digest}')
