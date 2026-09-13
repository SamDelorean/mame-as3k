#!/usr/bin/env python3
"""Compare private AS2000 screen pixels with bus text and selected F05 ROM.

No glyph tables or captures are emitted. This verifies emulator consistency,
not correspondence with an independently verified physical AS2000 display.
"""
import hashlib
from pathlib import Path
import sys
import zipfile

from as2k_decode_trace import Controller, parse_events, screen_tuple


def main():
    log, capture, archive = map(Path, sys.argv[1:])
    with zipfile.ZipFile(archive) as z:
        rom = z.read('ks0066_f05.bin')
    assert len(rom) == 4096
    assert hashlib.sha1(rom).hexdigest() == '0196e871584ee5d370856e7307c0f9d1466e3e51'
    controllers = {1: Controller(1), 2: Controller(2)}
    for event in parse_events(log.read_text().splitlines()):
        if event.e in controllers:
            for byte in controllers[event.e].feed(event):
                controllers[event.e].apply(byte)
    lines = screen_tuple(controllers)
    words = capture.read_text().split()
    assert tuple(map(int, words[:2])) == (240, 36)
    pixels = [int(v, 16) for v in words[2:]]
    assert len(pixels) == 240 * 36
    assert set(pixels) == {0x8a9294, 0x5c5358}, 'blank or unexpected screen palette'
    matches = [(row, line.index('az09=+')) for row, line in enumerate(lines) if 'az09=+' in line]
    assert len(matches) == 1, f'ambiguous/missing bus text: {lines!r}'
    row, col = matches[0]
    assert controllers[1 + row // 2].display_on
    for offset, code in enumerate(b'az09=+'):
        for y in range(8):
            for x in range(5):
                expected = bool(rom[code * 16 + y] & (1 << (4 - x)))
                actual = pixels[(row * 9 + y) * 240 + (col + offset) * 6 + x] == 0x5c5358
                assert actual == expected, f'pixel mismatch code={code:02X} row={row} col={col+offset} x={x} y={y}'
    print('PASS LCD: bus codes 61 7A 30 39 3D 2B match six rendered 5x8 F05 glyphs')
    print('OPEN/DEFERRED: physical AS2000 CGROM/reference display equivalence unverified')


if __name__ == '__main__':
    main()
