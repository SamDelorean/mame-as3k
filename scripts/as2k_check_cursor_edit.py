#!/usr/bin/env python3
# license:BSD-3-Clause
"""Check ordered diagnostic bus content, not completion or NVRAM substrings."""
import sys

from as2k_decode_trace import Controller, parse_events, screen_tuple

CHECKPOINTS = {
    'write': (('original', 'abcd'), ('insert', 'abxcd'),
              ('backspace', 'abcd'), ('final', 'abycd'), ('switch', 'abycd')),
    'recall': (('restart', 'abycd'), ('switch', 'abycd')),
}


def check(lines, phase):
    expected = CHECKPOINTS[phase]
    controllers = {1: Controller(1), 2: Controller(2)}
    seen, keys, writes = 0, 0, 0
    complete = False
    for line in lines:
        keys += 'AS2KTRACE KEY' in line
        for event in parse_events([line]):
            if event.e in controllers:
                for byte in controllers[event.e].feed(event):
                    writes += controllers[event.e].apply(byte)
        if 'AS2KEDIT observe ' in line:
            assert not complete and seen < len(expected), 'extra checkpoint'
            name, token = expected[seen]
            assert line.strip().endswith(f'AS2KEDIT observe {phase} {name}'), 'checkpoint order/phase'
            assert keys and writes, 'missing keyboard transitions or LCD writes since checkpoint'
            screen = screen_tuple(controllers)
            assert all(c.display_on for c in controllers.values()), 'LCD disabled'
            assert screen[0] == token.ljust(40), (phase, name, screen)
            seen += 1
            keys = writes = 0
        if 'AS2KEDIT complete ' in line:
            assert not complete and seen == len(expected), 'incomplete/duplicate completion'
            assert line.strip().endswith('AS2KEDIT complete ' + phase), 'completion phase'
            complete = True
    assert complete, 'missing completion'
    return seen


if __name__ == '__main__':
    with open(sys.argv[1]) as log:
        count = check(log, sys.argv[2])
    print(f'PASS cursor edit {sys.argv[2]}: {count} ordered exact 40-column row checkpoints')
