#!/usr/bin/env python3
# license:BSD-3-Clause
"""Assert per-file diagnostic LCD content at ordered idle-gated checkpoints."""
import re
import sys
from pathlib import Path

from as2k_decode_trace import Controller, parse_events, screen_tuple

TOKENS = tuple(f'file{i}{chr(96+i)*2}' for i in range(1, 9))
# Canonical fn_9486 banks and ROM D96A bounds; context, not exhaustive decode proof.
FILES = ((0, 0x069d, 0x7fff), (1, 0x0100, 0x3fff),
         (1, 0x4000, 0x7fff), (2, 0x0100, 0x3fff),
         (2, 0x4000, 0x7fff), (3, 0x0100, 0x2fff),
         (3, 0x3000, 0x5fff), (3, 0x6000, 0x7fde))
MARKER = re.compile(r'AS2KFILES observe (write|recall) F([1-8])$')


def check(lines, phase):
    controllers = {1: Controller(1), 2: Controller(2)}
    seen = []
    complete = False
    keys = False
    for line in lines:
        keys |= 'AS2KTRACE KEY' in line
        for event in parse_events([line]):
            if event.e in controllers:
                for byte in controllers[event.e].feed(event):
                    controllers[event.e].apply(byte)
        match = MARKER.search(line.strip())
        if match:
            assert not complete and match[1] == phase, 'unexpected checkpoint phase/order'
            number = int(match[2])
            assert number == len(seen) + 1, 'missing/duplicate/out-of-order checkpoint'
            screen = screen_tuple(controllers)
            assert all(c.display_on for c in controllers.values()), 'LCD display disabled'
            assert sum(row.count(TOKENS[number-1]) for row in screen) == 1, (phase, number, screen)
            for other in TOKENS:
                if other != TOKENS[number-1]:
                    assert not any(other in row for row in screen), (phase, number, other, screen)
            seen.append(number)
        if 'AS2KFILES complete ' in line:
            assert not complete and line.strip().endswith('AS2KFILES complete ' + phase)
            assert seen == list(range(1, 9)), 'incomplete file coverage'
            complete = True
    assert complete and keys, 'missing completion or keyboard transitions'
    return seen


def main():
    root = Path(sys.argv[1])
    for phase in ('write', 'recall'):
        with (root / phase / 'error.log').open() as log:
            check(log, phase)
        print(f'PASS {phase}: F1-F8 each contains its own token exactly once and no other file token')
    payloads = [p for p in (root / 'nvram').rglob('*') if p.is_file() and p.stat().st_size == 131072]
    assert len(payloads) == 1, 'expected one 128 KiB external RAM image'
    ram = payloads[0].read_bytes()
    for token, (bank, start, end) in zip(TOKENS, FILES):
        data = ram[bank * 0x8000 + start:bank * 0x8000 + end + 1]
        assert token.encode() in data, (token, bank, hex(start), hex(end))
        assert all(other.encode() not in data for other in TOKENS if other != token), token
    print('PASS persisted tokens in canonical bank/file bounds; no exhaustive address/decode claim')


if __name__ == '__main__':
    main()
