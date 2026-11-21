#!/usr/bin/env python3
"""Convert the app's native P6 screenshots without image-library dependencies."""
import struct
import sys
import zlib
from pathlib import Path

source, target = map(Path, sys.argv[1:])
header, size, maximum, pixels = source.read_bytes().split(b'\n', 3)
assert header == b'P6' and maximum == b'255'
width, height = map(int, size.split())
assert len(pixels) == width * height * 3
def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
raw = b''.join(b'\0' + pixels[y*width*3:(y+1)*width*3] for y in range(height))
target.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))
print(target)
