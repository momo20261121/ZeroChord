#!/usr/bin/env python3
"""Fetch the optional, pinned MIT-licensed FluidR3 piano into this project."""
import hashlib
from pathlib import Path
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
REV = '703eb1e0926c9d4ab3b34997e854c06b180e2c7b'
URL = f'https://raw.githubusercontent.com/CardputerZero/Piano/{REV}/assets/soundfonts/piano.sf2'
SHA = 'addb732569676727b02777a8df1f5d49eca0a19d4f306cca31624fdbfa0e1909'

if __name__ == '__main__':
    dest = ROOT/'sounds/fluid-grand/piano.sf2'
    if not dest.exists() or hashlib.sha256(dest.read_bytes()).hexdigest() != SHA:
        with urllib.request.urlopen(URL, timeout=60) as response:
            data = response.read(16 * 1024 * 1024 + 1)
        if hashlib.sha256(data).hexdigest() != SHA:
            raise SystemExit('Piano checksum mismatch; no sound was installed.')
        dest.write_bytes(data)
    print('Verified optional piano pack: sounds/fluid-grand (7.5 MiB)')
