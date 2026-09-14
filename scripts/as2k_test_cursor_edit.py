#!/usr/bin/env python3
# license:BSD-3-Clause
"""Small ROM-free positive/negative fixtures for the edit observation gate."""
import unittest

from as2k_check_cursor_edit import CHECKPOINTS, check


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
    for name, token in CHECKPOINTS[phase]:
        token = (replacements or {}).get(name, token)
        lines.append('AS2KTRACE KEY fixture')
        byte(1, 0, 0x80)
        for c in token.ljust(40):
            byte(1, 1, ord(c))
        lines.append(f'AS2KEDIT observe {phase} {name}')
    lines.append(f'AS2KEDIT complete {phase}')
    return lines


class EditGateTest(unittest.TestCase):
    def test_valid_write_and_restart(self):
        for phase in CHECKPOINTS:
            self.assertEqual(check(fixture(phase), phase), len(CHECKPOINTS[phase]))

    def test_wrong_edit_content(self):
        for name, wrong in (('insert', 'abxd'), ('backspace', 'abxd'),
                            ('backspace', 'abxcd'), ('final', 'abcd'),
                            ('switch', 'abcd'), ('original', 'abcd!')):
            with self.subTest(name=name, wrong=wrong), self.assertRaises(AssertionError):
                check(fixture('write', {name: wrong}), 'write')
        with self.assertRaises(AssertionError):
            check(fixture('recall', {'restart': 'abcd'}), 'recall')

    def test_missing_evidence(self):
        good = fixture('write')
        for fragment in ('AS2KTRACE LCD', 'AS2KTRACE KEY', 'observe write insert', 'complete'):
            with self.subTest(fragment=fragment), self.assertRaises(AssertionError):
                check([line for line in good if fragment not in line], 'write')

    def test_order_phase_duplicate(self):
        good = fixture('write')
        for bad in (good + ['AS2KEDIT complete write'],
                    [s.replace('observe write insert', 'observe write final') for s in good],
                    [s.replace('observe write', 'observe recall') for s in good]):
            with self.assertRaises(AssertionError):
                check(bad, 'write')


if __name__ == '__main__':
    unittest.main()
