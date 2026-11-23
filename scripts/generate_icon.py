#!/usr/bin/env python3
"""Flatten ZeroChord's approved icon background without redrawing the artwork."""
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

root = Path(__file__).resolve().parents[1]
source = root/'assets/zerochord-source.png'
target = root/'assets/share/images/zerochord.png'

SIZE = 100
BACKGROUND = (14, 23, 32)
STAFF = (119, 136, 149)
NOTE = (255, 240, 200)
BACKGROUND_BRIGHTNESS = 74
FOREGROUND_CUTOFF = 110

with tempfile.TemporaryDirectory(prefix='zerochord-icon-') as temporary:
    bitmap = Path(temporary)/'icon.bmp'
    subprocess.run(['/usr/bin/sips', '-z', str(SIZE), str(SIZE), '-s', 'format', 'bmp',
                    str(source), '--out', str(bitmap)], check=True,
                   stdout=subprocess.DEVNULL)
    data = bitmap.read_bytes()

assert data[:2] == b'BM'
offset = struct.unpack_from('<I', data, 10)[0]
width, signed_height = struct.unpack_from('<ii', data, 18)
bits = struct.unpack_from('<H', data, 28)[0]
assert width == SIZE and abs(signed_height) == SIZE and bits in (24, 32)
stride = ((width * bits + 31) // 32) * 4
pixels = bytearray()

for y in range(SIZE):
    row = y if signed_height < 0 else SIZE - 1 - y
    for x in range(SIZE):
        at = offset + row * stride + x * (bits // 8)
        blue, green, red = data[at:at + 3]
        brightness = red + green + blue
        if brightness <= FOREGROUND_CUTOFF:
            pixels.extend((*BACKGROUND, 255))
            continue
        foreground = NOTE if red > blue else STAFF
        alpha = (brightness - BACKGROUND_BRIGHTNESS) / (sum(foreground) - BACKGROUND_BRIGHTNESS)
        alpha = max(0.0, min(1.0, alpha))
        pixels.extend((round(BACKGROUND[channel] * (1 - alpha) + foreground[channel] * alpha)
                       for channel in range(3)))
        pixels.append(255)

def chunk(kind, payload):
    return (struct.pack('>I', len(payload)) + kind + payload +
            struct.pack('>I', zlib.crc32(kind + payload) & 0xffffffff))

raw = b''.join(b'\0' + pixels[row * SIZE * 4:(row + 1) * SIZE * 4]
               for row in range(SIZE))
png = (b'\x89PNG\r\n\x1a\n' +
       chunk(b'IHDR', struct.pack('>IIBBBBB', SIZE, SIZE, 8, 6, 0, 0, 0)) +
       chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))
target.write_bytes(png)
print('Generated 100px ZeroChord icon with a flat navy background')
