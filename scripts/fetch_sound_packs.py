#!/usr/bin/env python3
"""Build the optional, pinned MIT-licensed FluidR3 synth and drum packs in this project.

Downloads Debian's fluid-soundfont-gm package into .tools/downloads/, extracts
FluidR3_GM.sf2 into .tools/fluidr3/ without installing anything, and cuts
sounds/fluid-synths/bank.sf2 and sounds/fluid-drums/bank.sf2 from it with
scripts/sf2_subset.py. Every input and output is checked against a pinned SHA-256.
"""
import hashlib
import io
import re
import sys
import tarfile
import time
import tomllib
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sf2_subset  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
DEB = 'fluid-soundfont-gm_3.1-5.3_all.deb'
URL = f'https://deb.debian.org/debian/pool/main/f/fluid-soundfont/{DEB}'
DEB_SIZE = 119_610_208
DEB_SHA = '6f531493ac4e4d9772fd96b2488ea1790af81c196135fbdd25997da0781fc60e'
SF2_MEMBER = './usr/share/sounds/sf2/FluidR3_GM.sf2'
SF2_SHA = '74594e8f4250680adf590507a306655a299935343583256f3b722c48a1bc1cb0'
COPYRIGHT_MEMBER = './usr/share/doc/fluid-soundfont-gm/copyright'
DOWNLOADS = ROOT / '.tools/downloads'
SOURCE = ROOT / '.tools/fluidr3'

MIB = 1024 * 1024
PACK_LIMIT, TOTAL_LIMIT = 16 * MIB, 32 * MIB
PIANO_BYTES = 7_833_208  # sounds/fluid-grand/piano.sf2, pinned by fetch_piano_pack.py.

# (program, id, name, category). All eight requested kits came to 16,837,216 bytes,
# over the 16 MiB pack limit, so Orchestra (program 48) is left out, as the drop
# order Orchestra, Power, Room, Jazz requires.
PACKS = {
    'fluid-synths': {
        'bank': 0,
        'sounds': [
            (80, 'square-lead', 'Square Lead (FluidR3)', 'Synths'),
            (81, 'saw-lead', 'Saw Lead (FluidR3)', 'Synths'),
            (38, 'synth-bass', 'Synth Bass (FluidR3)', 'Synths'),
            (62, 'synth-brass', 'Synth Brass (FluidR3)', 'Synths'),
            (90, 'polysynth', 'Polysynth (FluidR3)', 'Synths'),
            (50, 'synth-strings', 'Synth Strings (FluidR3)', 'Strings'),
        ],
        'size': 1_065_558,
        'sha': '255f3f92fbc2533df8db9bdf18e6750b86b119f066b54cf5eb0b37af4b92ec0c',
    },
    'fluid-drums': {
        'bank': 128,
        'sounds': [
            (0, 'standard', 'Standard Kit (FluidR3)', 'Drums'),
            (8, 'room', 'Room Kit (FluidR3)', 'Drums'),
            (16, 'power', 'Power Kit (FluidR3)', 'Drums'),
            (24, 'electronic', 'Electronic Kit (FluidR3)', 'Drums'),
            (25, 'tr-808', 'TR-808 Kit (FluidR3)', 'Drums'),
            (32, 'jazz', 'Jazz Kit (FluidR3)', 'Drums'),
            (40, 'brush', 'Brush Kit (FluidR3)', 'Drums'),
        ],
        'size': 14_691_522,
        'sha': '1b206d2f97e1eb58c50c4f48df2b49be9ab9d35b0e2f4a0cf01071e4b151c6df',
    },
}


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as f:
        while chunk := f.read(1 << 20):
            digest.update(chunk)
    return digest.hexdigest()


def fetch_deb():
    dest = DOWNLOADS / DEB
    if dest.exists() and sha256_file(dest) == DEB_SHA:
        return dest
    DOWNLOADS.mkdir(parents=True, exist_ok=True)
    part = dest.with_suffix('.part')
    digest, size = hashlib.sha256(), 0
    with urllib.request.urlopen(URL, timeout=60) as response, open(part, 'wb') as out:
        while chunk := response.read(1 << 20):
            size += len(chunk)
            if size > DEB_SIZE:
                break
            digest.update(chunk)
            out.write(chunk)
    if size != DEB_SIZE or digest.hexdigest() != DEB_SHA:
        part.unlink()
        raise SystemExit(f'{DEB} checksum mismatch; nothing was extracted.')
    part.replace(dest)
    return dest


class _Slice(io.RawIOBase):
    """Read-only view of `size` bytes of an open file, starting at its current position."""

    def __init__(self, f, size):
        self.f, self.left = f, size

    def readable(self):
        return True

    def readinto(self, buffer):
        data = self.f.read(min(len(buffer), self.left))
        self.left -= len(data)
        buffer[:len(data)] = data
        return len(data)


def _data_tar(f):
    """Position `f` at the data.tar.* member of a Debian ar archive; return its name and size."""
    if f.read(8) != b'!<arch>\n':
        raise SystemExit(f'{DEB} is not an ar archive')
    while header := f.read(60):
        if len(header) != 60 or header[58:60] != b'`\n':
            raise SystemExit(f'{DEB} has a malformed ar header')
        name = header[:16].decode('ascii').strip().rstrip('/')
        size = int(header[48:58])
        if name.startswith('data.tar'):
            return name, size
        f.seek(size + (size & 1), io.SEEK_CUR)
    raise SystemExit(f'{DEB} has no data.tar member')


def extract(deb):
    sf2, copyright_file = SOURCE / 'FluidR3_GM.sf2', SOURCE / 'copyright'
    if sf2.exists() and copyright_file.exists() and sha256_file(sf2) == SF2_SHA:
        return sf2, copyright_file
    SOURCE.mkdir(parents=True, exist_ok=True)
    wanted = {SF2_MEMBER: sf2, COPYRIGHT_MEMBER: copyright_file}
    with open(deb, 'rb') as f:
        name, size = _data_tar(f)
        mode = {'data.tar.xz': 'r|xz', 'data.tar.gz': 'r|gz', 'data.tar': 'r|'}[name]
        with tarfile.open(fileobj=io.BufferedReader(_Slice(f, size)), mode=mode) as tar:
            for member in tar:
                if member.name in wanted and member.isreg():
                    part = wanted[member.name].with_suffix('.part')
                    with tar.extractfile(member) as src, open(part, 'wb') as out:
                        while chunk := src.read(1 << 20):
                            out.write(chunk)
                    part.replace(wanted.pop(member.name))
    if wanted:
        raise SystemExit(f'{DEB} lacks {", ".join(wanted)}')
    if sha256_file(sf2) != SF2_SHA:
        sf2.unlink()
        raise SystemExit('FluidR3_GM.sf2 checksum mismatch')
    return sf2, copyright_file


def _permission(text):
    match = re.search(r'Permission is hereby granted.*?OTHER DEALINGS IN THE\s+SOFTWARE\.', text,
                      re.S)
    return ' '.join(match.group(0).split()) if match else None


def check_license(copyright_file, pack_dir):
    """Require the pack's LICENSE.txt to carry the package's MIT notice and copyright holders."""
    upstream = copyright_file.read_text(encoding='utf-8')
    if 'I hereby release Fluid under the MIT license' not in upstream:
        raise SystemExit('Package copyright no longer states the MIT license')
    ours = (pack_dir / 'LICENSE.txt').read_text(encoding='utf-8')
    if _permission(upstream) is None or _permission(ours) != _permission(upstream):
        raise SystemExit(f'{pack_dir.name}/LICENSE.txt does not match the package MIT text')
    for holder in ('2000-2002, 2008 Frank Wen', '2008 Toby Smithe'):
        if holder not in upstream or holder not in ours:
            raise SystemExit(f'{pack_dir.name}/LICENSE.txt lacks copyright {holder}')


def check_manifest(pack_dir, spec):
    manifest = tomllib.loads((pack_dir / 'pack.toml').read_text(encoding='utf-8'))
    listed = [(s['program'], s['id'], s['name'], s.get('category', 'Custom'))
              for s in manifest['sounds'] if s.get('bank', 0) == spec['bank']]
    if manifest.get('file') != 'bank.sf2' or listed != spec['sounds'] or \
            len(listed) != len(manifest['sounds']):
        raise SystemExit(f'{pack_dir.name}/pack.toml does not match this script')


def main():
    started = time.monotonic()
    deb = fetch_deb()
    print(f'Verified {DEB} ({DEB_SIZE:,} bytes, SHA-256 {DEB_SHA[:12]}...)')
    sf2, copyright_file = extract(deb)
    print(f'Verified .tools/fluidr3/FluidR3_GM.sf2 (SHA-256 {SF2_SHA[:12]}...)')
    source = sf2_subset.SoundFont(sf2.read_bytes())
    total = PIANO_BYTES
    for pack, spec in PACKS.items():
        pack_dir = ROOT / 'sounds' / pack
        check_manifest(pack_dir, spec)
        check_license(copyright_file, pack_dir)
        wanted = [(spec['bank'], program) for program, *_ in spec['sounds']]
        data = sf2_subset.subset(source, wanted, {
            b'INAM': f'Fluid R3 GM: ZeroChord {pack}',
            b'ICMT': 'Licensed under the MIT License. Subset of FluidR3_GM.sf2 from '
                     'fluid-soundfont-gm 3.1-5.3; samples unchanged.',
            b'ISFT': 'SFEDT v1.28:ZeroChord sf2_subset.py',
        })
        sf2_subset.check(data, wanted)
        sha = hashlib.sha256(data).hexdigest()
        if len(data) != spec['size'] or sha != spec['sha']:
            raise SystemExit(f'{pack} checksum mismatch ({len(data):,} bytes, {sha}); '
                             'nothing was installed.')
        if len(data) > PACK_LIMIT:
            raise SystemExit(f'{pack} exceeds the 16 MiB pack limit')
        total += len(data)
        dest = pack_dir / 'bank.sf2'
        if not dest.exists() or sha256_file(dest) != sha:
            dest.write_bytes(data)
        names = ', '.join(name.removesuffix(' (FluidR3)') for _, _, name, _ in spec['sounds'])
        print(f'Verified sounds/{pack}/bank.sf2 ({len(data):,} bytes, {len(data) / MIB:.1f} MiB, '
              f'SHA-256 {sha[:12]}...), pack.toml and MIT LICENSE.txt: {names}')
    if total > TOTAL_LIMIT:
        raise SystemExit('Packs exceed the 32 MiB total custom sound limit')
    print(f'With fluid-grand, the custom packs total {total:,} bytes '
          f'({total / MIB:.1f} of 32 MiB); done in {time.monotonic() - started:.1f} s')
    return 0


if __name__ == '__main__':
    sys.exit(main())
