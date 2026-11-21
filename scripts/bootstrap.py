#!/usr/bin/env python3
"""Fetch project-local dependencies. Never installs system packages."""
import pathlib
import subprocess
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEPS = ROOT / '.tools' / 'deps'


def clone(name, url, revision):
    dest = DEPS / name
    if (dest / '.git').exists():
        return
    subprocess.run(['git', 'clone', '--depth', '1', '--branch', revision, url, str(dest)], check=True)


def download(url, dest):
    if dest.exists():
        return
    dest.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url, timeout=60) as response:
        dest.write_bytes(response.read())


def main():
    DEPS.mkdir(parents=True, exist_ok=True)
    clone('lvgl', 'https://github.com/lvgl/lvgl.git', 'v9.5.0')
    clone('SDL', 'https://github.com/libsdl-org/SDL.git', 'release-2.32.10')
    revision = '853a0a171759f1ddba0de1442133a75912bbeffa'
    for filename in ('tsf.h', 'LICENSE'):
        download('https://raw.githubusercontent.com/schellingb/TinySoundFont/' + revision + '/' + filename,
                 DEPS / 'TinySoundFont' / filename)
    for filename in ('miniaudio.h', 'LICENSE'):
        download('https://raw.githubusercontent.com/mackron/miniaudio/0.11.25/' + filename,
                 DEPS / 'miniaudio' / filename)
    for filename in ('toml.hpp', 'LICENSE'):
        download('https://raw.githubusercontent.com/marzer/tomlplusplus/v3.4.0/' + filename,
                 DEPS / 'tomlplusplus' / filename)
    print('Dependencies ready under', DEPS)


if __name__ == '__main__':
    main()
