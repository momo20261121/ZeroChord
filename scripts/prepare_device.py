#!/usr/bin/env python3
"""Create a source bundle with project-local Linux ARM64 CMake (no system install)."""
from pathlib import Path
import tarfile
import zipfile

root = Path(__file__).resolve().parents[1]
wheel = next((root / '.tools/downloads').glob('cmake-*-*aarch64*.whl'))
destination = root / '.tools/cmake-linux'
with zipfile.ZipFile(wheel) as archive:
    for item in archive.infolist():
        if item.filename.startswith('cmake/data/'):
            target = destination / item.filename.removeprefix('cmake/data/')
            if item.is_dir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(archive.read(item))
                if target.parent.name == 'bin':
                    target.chmod(0o755)

output = root / '.tools/zerochord-device-source.tar.gz'
with tarfile.open(output, 'w:gz') as archive:
    for name in ['CMakeLists.txt', 'lv_conf.h', 'src', 'tests', 'tools', 'scripts', 'assets', 'LICENSE', 'THIRD_PARTY_NOTICES.md', 'third_party']:
        path = root / name
        if path.exists():
            archive.add(path, arcname=name)
    for name in ['TinySoundFont', 'miniaudio']:
        archive.add(root / '.tools/deps' / name, arcname='.tools/deps/' + name)
    lvgl = root / '.tools/deps/lvgl'
    for name in ['CMakeLists.txt', 'lvgl.h', 'lvgl_private.h', 'lv_version.h', 'lv_version.h.in', 'lvgl.pc.in', 'LICENCE.txt', 'COPYRIGHTS.md', 'src', 'env_support/cmake']:
        archive.add(lvgl / name, arcname='.tools/deps/lvgl/' + name)
    archive.add(destination, arcname='.tools/cmake')
print(output)
