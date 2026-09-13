#!/usr/bin/env python3
# license:BSD-3-Clause
"""Check ordered diagnostic bus content, not completion or NVRAM substrings."""
import sys

from as2k_decode_trace import Controller, parse_events

# v314 LOCAL IUI9Jp1C: split/resplit/switch emit DDRAM B5 after ab;
# saved F1 begins 61 62 0D 63 64. B5 is a bus marker, not ASCII or
# a claim about its physical glyph. Compare cells without decoder substitution.
CHECKPOINTS = {
    'write': (('original', ('abcd', '')), ('split', ('ab\xb5', 'cd')),
              ('join', ('abcd', '')), ('resplit', ('ab\xb5', 'cd')),
              ('switch', ('ab\xb5', 'cd'))),
    # Left/Right traversal accepted in LOCAL Ifdikbk6.
    'traverse': (('original', ('abcd', '')), ('split', ('ab\xb5', 'cd')),
                 ('left_insert', ('abx\xb5', 'cd')),
                 ('right_insert', ('abx\xb5', 'ycd')),
                 ('switch', ('abx\xb5', 'ycd'))),
    'traverse_recall': (('restart', ('abx\xb5', 'ycd')),
                        ('switch', ('abx\xb5', 'ycd'))),
    # Up/Down expectations await LOCAL; not verified physical behavior.
    'vertical': (('original', ('abcd', '')), ('split', ('ab\xb5', 'cd')),
                 ('up_insert', ('xab\xb5', 'cd')),
                 ('down_insert', ('xab\xb5', 'cyd')),
                 ('switch', ('xab\xb5', 'cyd'))),
    'vertical_recall': (('restart', ('xab\xb5', 'cyd')),
                        ('switch', ('xab\xb5', 'cyd'))),
    'recall': (('restart', ('ab\xb5', 'cd')), ('switch', ('ab\xb5', 'cd'))),
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
        if 'AS2KNEWLINE observe ' in line:
            assert not complete and seen < len(expected), 'extra checkpoint'
            name, rows = expected[seen]
            assert line.strip().endswith(f'AS2KNEWLINE observe {phase} {name}'), 'checkpoint order/phase'
            assert keys and writes, 'missing keyboard transitions or LCD writes since checkpoint'
            screen = tuple(bytes(controllers[1].ddram[a:a + 40]) for a in (0, 0x40))
            assert all(c.display_on for c in controllers.values()), 'LCD disabled'
            assert screen == tuple(row.encode('latin-1').ljust(40, b' ') for row in rows), (phase, name, screen)
            print(f'PASS {phase}/{name}: rows={screen[:2]!r}')
            seen += 1
            keys = writes = 0
        if 'AS2KNEWLINE complete ' in line:
            assert not complete and seen == len(expected), 'incomplete/duplicate completion'
            assert line.strip().endswith('AS2KNEWLINE complete ' + phase), 'completion phase'
            complete = True
    assert complete, 'missing completion'
    return seen


if __name__ == '__main__':
    with open(sys.argv[1]) as log:
        count = check(log, sys.argv[2])
    print(f'PASS newline edit {sys.argv[2]}: {count} ordered exact two-row 40-column checkpoints')
