#!/usr/bin/env python3
"""Build the Linux ARM64 app locally; keep compiler/downloads/caches in .tools."""
import argparse
import hashlib
import os
from pathlib import Path
import platform
import subprocess
import tarfile
import time
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
VERSION = '0.15.2'
HOST_ARCHIVES = {
    ('Darwin', 'arm64'): (
        'aarch64-macos', '3cc2bab367e185cdfb27501c4b30b1b0653c28d9f73df8dc91488e66ece5fa6b',
    ),
    ('Darwin', 'x86_64'): (
        'x86_64-macos', '375b6909fc1495d16fc2c7db9538f707456bfc3373b14ee83fdd3e22b3d43f7f',
    ),
    ('Linux', 'aarch64'): (
        'aarch64-linux', '958ed7d1e00d0ea76590d27666efbf7a932281b3d7ba0c6b01b0ff26498f667f',
    ),
    ('Linux', 'x86_64'): (
        'x86_64-linux', '02aa270f183da276e5b5920b1dac44a63f1a49e55050ebde3aecc9eb82f93239',
    ),
}


def compiler():
    host = (platform.system(), platform.machine())
    if host not in HOST_ARCHIVES:
        raise SystemExit('This build supports ARM64 and x86-64 on macOS and Linux.')
    archive_platform, expected_sha256 = HOST_ARCHIVES[host]
    tools = ROOT / '.tools'
    zig = tools / 'zig'
    if (zig / 'zig').exists():
        actual = subprocess.check_output([str(zig / 'zig'), 'version'], text=True).strip()
        if actual != VERSION:
            raise SystemExit(f'Expected Zig {VERSION}, found {actual} in {zig}')
        return
    name = f'zig-{archive_platform}-{VERSION}'
    archive = tools / 'downloads' / f'{name}.tar.xz'
    archive.parent.mkdir(parents=True, exist_ok=True)
    if not archive.exists():
        print(f'Downloading project-local Zig {VERSION}…', flush=True)
        temporary = archive.with_suffix('.partial')
        with urllib.request.urlopen(f'https://ziglang.org/download/{VERSION}/{name}.tar.xz',
                                    timeout=60) as response, temporary.open('wb') as out:
            while chunk := response.read(1024 * 1024):
                out.write(chunk)
        temporary.replace(archive)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != expected_sha256:
        raise SystemExit(f'Compiler checksum mismatch: remove {archive} and retry.')
    with tarfile.open(archive) as package:
        # The official archive's SHA256 was verified before extraction.
        package.extractall(tools)
    (tools / name).rename(zig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-j', '--jobs', type=int, default=min(os.cpu_count() or 2, 8))
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('jobs must be positive')
    started = time.monotonic()
    compiler()
    cmake = ROOT / '.venv/bin/cmake'
    if not cmake.exists():
        raise SystemExit('Install CMake in the project .venv first; see README.md.')
    if not (ROOT / '.tools/deps/lvgl/CMakeLists.txt').exists():
        raise SystemExit('Fetch the project dependencies with scripts/bootstrap.py first.')
    build = ROOT / 'build/cross-device'
    subprocess.run([str(cmake), '-S', str(ROOT), '-B', str(build), '-G', 'Unix Makefiles',
                    f'-DCMAKE_TOOLCHAIN_FILE={ROOT / "cmake/aarch64-linux-zig.cmake"}',
                    '-DCMAKE_BUILD_TYPE=Release', '-DZEROCHORD_DESKTOP=OFF'], check=True)
    subprocess.run([str(cmake), '--build', str(build), '--target', 'ZeroChord',
                    '--parallel', str(args.jobs)], check=True)
    binary = build / 'M5CardputerZero-ZeroChord'
    header = binary.read_bytes()[:20]
    if header[:6] != b'\x7fELF\x02\x01' or int.from_bytes(header[18:20], 'little') != 183:
        raise SystemExit(f'Build did not produce a Linux ARM64 ELF: {binary}')
    print(f'Linux ARM64 binary ready in {time.monotonic() - started:.1f}s: {binary}', flush=True)


if __name__ == '__main__':
    main()
