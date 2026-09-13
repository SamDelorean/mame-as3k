#!/usr/bin/env python3
# license:BSD-3-Clause
"""Small ROM-free positive/negative fixtures for the edit observation gate."""
import unittest

from as2k_check_newline_edit import CHECKPOINTS, check


def fixture(phase, replacements=None):
    lines = []

    def nibble(e, rs, n):
        lines.append(f'AS2KTRACE LCD pc=8000 e={e} rs={rs} rw=0 '
                     f'nibble={n:X} matrixh=00 pd=00')

    def byte(e, rs, value):
        nibble(e, rs, value >> 4)
        nibble(e, rs, value & 15)

    for e in (1, 2):
        nibble(e, 0, 2)  # Enter four-bit mode.
        byte(e, 0, 0x0c)
    for name, rows in CHECKPOINTS[phase]:
        rows = (replacements or {}).get(name, rows)
        lines.append('AS2KTRACE KEY fixture')
        for address, token in zip((0x80, 0xc0), rows):
            byte(1, 0, address)
            for c in token.ljust(40):
                byte(1, 1, ord(c))
        lines.append(f'AS2KNEWLINE observe {phase} {name}')
    lines.append(f'AS2KNEWLINE complete {phase}')
    return lines


class EditGateTest(unittest.TestCase):
    def test_valid_write_and_restart(self):
        for phase in CHECKPOINTS:
            self.assertEqual(check(fixture(phase), phase), len(CHECKPOINTS[phase]))

    def test_wrong_edit_content(self):
        for name, wrong in (('split', ('ab\xb5', 'd')), ('split', ('abcd', '')),
                            ('join', ('abcd', 'cd')), ('join', ('abd', '')),
                            ('resplit', ('abcd', '')), ('switch', ('ab', '')),
                            ('original', ('abcd!', '')),
                            ('split', ('ab', 'cd')),
                            ('split', ('ab\xb4', 'cd')),
                            ('split', ('ab\xb5!', 'cd'))):
            with self.subTest(name=name, wrong=wrong), self.assertRaises(AssertionError):
                check(fixture('write', {name: wrong}), 'write')
        with self.assertRaises(AssertionError):
            check(fixture('recall', {'restart': ('abcd', '')}), 'recall')

    def test_missing_evidence(self):
        good = fixture('write')
        for fragment in ('AS2KTRACE LCD', 'AS2KTRACE KEY', 'observe write split', 'complete'):
            with self.subTest(fragment=fragment), self.assertRaises(AssertionError):
                check([line for line in good if fragment not in line], 'write')

    def test_order_phase_duplicate(self):
        good = fixture('write')
        for bad in (good + ['AS2KNEWLINE complete write'],
                    [s.replace('observe write split', 'observe write join') for s in good],
                    [s.replace('observe write', 'observe recall') for s in good]):
            with self.assertRaises(AssertionError):
                check(bad, 'write')


if __name__ == '__main__':
    unittest.main()
