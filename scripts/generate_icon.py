#!/usr/bin/env python3
"""Resize the approved artwork to an APPLaunch-compatible 256px RGBA PNG.

Uses macOS sips and Python's standard library; no installed image dependencies.
The high-resolution source is preserved unchanged for future exports.
"""
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

root = Path(__file__).resolve().parents[1]
source = root/'assets/zerochord-source.png'
target = root/'assets/share/images/zerochord.png'
with tempfile.TemporaryDirectory(prefix='zerochord-icon-') as temporary:
    bitmap = Path(temporary)/'icon.bmp'
    subprocess.run(['/usr/bin/sips', '-z', '256', '256', '-s', 'format', 'bmp',
                    str(source), '--out', str(bitmap)], check=True,
                   stdout=subprocess.DEVNULL)
    data = bitmap.read_bytes()
assert data[:2] == b'BM'
offset = struct.unpack_from('<I', data, 10)[0]
width, signed_height = struct.unpack_from('<ii', data, 18)
bits = struct.unpack_from('<H', data, 28)[0]
assert width == 256 and abs(signed_height) == 256 and bits in (24, 32)
stride = ((width*bits+31)//32)*4
raw = bytearray()
for y in range(256):
    raw.append(0)
    row = y if signed_height < 0 else 255-y
    for x in range(256):
        at = offset+row*stride+x*(bits//8)
        blue, green, red = data[at:at+3]
        raw.extend((red, green, blue, 255))

def chunk(kind, payload):
    return (struct.pack('>I', len(payload))+kind+payload+
            struct.pack('>I', zlib.crc32(kind+payload) & 0xffffffff))

png = (b'\x89PNG\r\n\x1a\n'+
       chunk(b'IHDR', struct.pack('>IIBBBBB', 256, 256, 8, 6, 0, 0, 0))+
       chunk(b'IDAT', zlib.compress(raw, 9))+chunk(b'IEND', b''))
target.write_bytes(png)
print('Generated assets/share/images/zerochord.png from approved artwork')
