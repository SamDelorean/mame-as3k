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
        for row, token in enumerate(rows):
            e, address = row // 2 + 1, (0x80, 0xc0)[row % 2]
            byte(e, 0, address)
            for c in token.ljust(40):
                byte(e, 1, ord(c))
        lines.append(f'AS2KNEWLINE observe {phase} {name}')
    lines.append(f'AS2KNEWLINE complete {phase}')
    return lines


class EditGateTest(unittest.TestCase):
    def test_valid_write_and_restart(self):
        for phase in CHECKPOINTS:
            self.assertEqual(check(fixture(phase), phase), len(CHECKPOINTS[phase]))

    def test_three_wrong_controller_content_and_recall(self):
        for phase, names in (('three', ('final', 'switch')),
                             ('three_recall', ('restart', 'switch'))):
            for name in names:
                for rows in (('ab\xb5', 'cd\xb5', '', ''),
                             ('ab\xb5', 'cd\xb5', 'eg', ''),
                             ('ab\xb5', 'cd\xb5', '', 'ef'),
                             ('ab\xb5', 'cd\xb5', 'ef', 'x'),
                             ('ab\xb5', 'cd', 'ef', '')):
                    with self.subTest(phase=phase, name=name, rows=rows), self.assertRaises(AssertionError):
                        check(fixture(phase, {name: rows}), phase)
        for phase in ('three', 'three_recall', 'boundary', 'boundary_recall',
                      'four', 'four_recall', 'five', 'five_recall'):
            good = fixture(phase)
            with self.assertRaises(AssertionError):
                check([s for s in good if ' e=2 ' not in s], phase)
            with self.assertRaises(AssertionError):
                check([s for s in good if ' e=2 rs=1 ' not in s], phase)
            with self.assertRaises(AssertionError):
                check([s.replace('nibble=C ', 'nibble=8 ') for s in good], phase)
            with self.assertRaises(AssertionError):
                check([s.replace('observe ' + phase + ' switch',
                                 'observe ' + phase + ' final') for s in good], phase)

    def test_five_scroll_and_recall(self):
        for phase, names in (('five', ('newline4', 'fifth', 'switch')),
                             ('five_recall', ('restart', 'switch'))):
            for name in names:
                wrong_rows = [
                    ('ab\xb5', 'cd\xb5', 'ef\xb5', 'gh'),
                    ('ab\xb5', 'cd\xb5', 'ef\xb5', 'gh\xb5'),
                    ('cd\xb5', 'ef\xb5', 'gh\xb5', 'ik'),
                    ('cd\xb5', 'ij', 'gh\xb5', 'ef\xb5'),
                    ('gh\xb5', 'ij', 'cd\xb5', 'ef\xb5'),
                ]
                if name != 'newline4':
                    wrong_rows.append(('cd\xb5', 'ef\xb5', 'gh\xb5', ''))
                else:
                    wrong_rows.append(('cd\xb5', 'ef\xb5', 'gh\xb5', 'ij'))
                for rows in wrong_rows:
                    with self.subTest(phase=phase, name=name, rows=rows), self.assertRaises(AssertionError):
                        check(fixture(phase, {name: rows}), phase)
        # A display-shift command leaves raw DDRAM unchanged, but invalidates
        # interpreting these raw rows as the viewport with the shared decoder.
        good = fixture('five')
        index = good.index('AS2KNEWLINE observe five newline4')
        shift = [f'AS2KTRACE LCD pc=8000 e=1 rs=0 rw=0 nibble={n} matrixh=00 pd=00'
                 for n in ('1', '8')]
        with self.assertRaises(AssertionError):
            check(good[:index] + shift + good[index:], 'five')

    def test_four_content_placement_and_stale_recall(self):
        for phase, names in (('four', ('final', 'switch')),
                             ('four_recall', ('restart', 'switch'))):
            for name in names:
                for rows in (('ab\xb5', 'cd\xb5', 'ef\xb5', ''),
                             ('ab\xb5', 'cd\xb5', 'ef\xb5', 'gi'),
                             ('ab\xb5', 'cd\xb5', 'ef', 'gh'),
                             ('ab\xb5', 'gh', 'ef\xb5', 'cd\xb5'),
                             ('ab\xb5', 'cd\xb5', 'gh', 'ef\xb5'),
                             ('ab\xb5', 'cd\xb5', 'ef', ''),
                             ('ab\xb5', 'cd\xb5', 'ef\xb5', 'gh!')):
                    with self.subTest(phase=phase, name=name, rows=rows), self.assertRaises(AssertionError):
                        check(fixture(phase, {name: rows}), phase)
        with self.assertRaises(AssertionError):
            check(fixture('four', {'newline3': ('ab\xb5', 'cd\xb5', 'ef', '')}), 'four')
        good = fixture('four')
        with self.assertRaises(AssertionError):
            check([s.replace('observe four newline2', 'observe four newline3')
                   for s in good], 'four')

    def test_boundary_join_resplit_and_recall(self):
        for name, rows in (
                ('join', ('ab\xb5', 'cd\xb5', 'ef', '')),  # ignored
                ('join', ('ab\xb5', 'cdf', '', '')),  # wrong deletion
                ('join', ('ab\xb5', 'cdef', 'ef', '')),  # stale third row
                ('join', ('ab\xb5', '', 'cdef', '')),  # wrong controller
                ('join', ('ab\xb5', 'cdef', '', 'ef')),  # wrong row
                ('resplit', ('ab\xb5', 'cdef', '', '')),
                ('resplit', ('ab\xb5', 'cd\xb5', '', 'ef')),
                ('switch', ('ab\xb5', 'cdef', '', ''))):
            with self.subTest(name=name, rows=rows), self.assertRaises(AssertionError):
                check(fixture('boundary', {name: rows}), 'boundary')
        for name in ('restart', 'switch'):
            with self.subTest(name=name), self.assertRaises(AssertionError):
                check(fixture('boundary_recall', {name: ('ab\xb5', 'cdef', '', '')}),
                      'boundary_recall')
        for phase in ('boundary', 'boundary_recall'):
            good = fixture(phase)
            for fragment in (' e=2 ', ' e=2 rs=1 '):
                with self.subTest(phase=phase, fragment=fragment), self.assertRaises(AssertionError):
                    check([s for s in good if fragment not in s], phase)
            with self.assertRaises(AssertionError):
                check([s.replace('nibble=C ', 'nibble=8 ') for s in good], phase)
        good = fixture('boundary')
        with self.assertRaises(AssertionError):
            check([s.replace('observe boundary join', 'observe boundary resplit')
                   for s in good], 'boundary')

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

    def test_traversal_wrong_boundary_or_overwrite(self):
        for name, wrong in (
                ('left_insert', ('ab\xb5', 'xcd')),
                ('left_insert', ('axb\xb5', 'cd')),
                ('left_insert', ('abx', 'cd')),
                ('right_insert', ('abxy\xb5', 'cd')),
                ('right_insert', ('abx\xb5', 'cyd')),
                ('right_insert', ('abx\xb5', 'yd')),
                ('switch', ('abx\xb5', 'cd'))):
            with self.subTest(name=name, wrong=wrong), self.assertRaises(AssertionError):
                check(fixture('traverse', {name: wrong}), 'traverse')
        for name in ('restart', 'switch'):
            with self.subTest(name=name), self.assertRaises(AssertionError):
                check(fixture('traverse_recall', {name: ('ab\xb5', 'cd')}),
                      'traverse_recall')

    def test_vertical_ignored_horizontal_or_overwrite(self):
        for name, wrong in (
                ('up_insert', ('ab\xb5', 'xcd')),  # ignored Up
                ('up_insert', ('abx\xb5', 'cd')),  # horizontal Left
                ('up_insert', ('ab\xb5', 'cxd')),  # horizontal Right
                ('up_insert', ('xb\xb5', 'cd')),   # overwrite
                ('down_insert', ('xyab\xb5', 'cd')),  # ignored Down
                ('down_insert', ('yxab\xb5', 'cd')),  # horizontal Left
                ('down_insert', ('xayb\xb5', 'cd')),  # horizontal Right
                ('down_insert', ('xab\xb5', 'ycd')),  # wrong column
                ('down_insert', ('xab\xb5', 'cy')),   # overwrite
                ('switch', ('ab\xb5', 'cd'))):
            with self.subTest(name=name, wrong=wrong), self.assertRaises(AssertionError):
                check(fixture('vertical', {name: wrong}), 'vertical')
        for name in ('restart', 'switch'):
            with self.subTest(name=name), self.assertRaises(AssertionError):
                check(fixture('vertical_recall', {name: ('xab\xb5', 'cd')}),
                      'vertical_recall')
        with self.assertRaises(AssertionError):
            check([s.replace('observe vertical up_insert',
                             'observe vertical down_insert')
                   for s in fixture('vertical')], 'vertical')

    def test_traversal_missing_or_unordered_evidence(self):
        for phase in ('traverse', 'traverse_recall', 'vertical', 'vertical_recall',
                      'three', 'three_recall', 'boundary', 'boundary_recall',
                      'four', 'four_recall', 'five', 'five_recall'):
            good = fixture(phase)
            for fragment in ('AS2KTRACE LCD', 'AS2KTRACE KEY', 'complete',
                             'observe ' + phase + ' switch'):
                with self.subTest(phase=phase, fragment=fragment), self.assertRaises(AssertionError):
                    check([line for line in good if fragment not in line], phase)
            with self.assertRaises(AssertionError):
                check(good + [f'AS2KNEWLINE complete {phase}'], phase)
        with self.assertRaises(AssertionError):
            check([s.replace('observe traverse left_insert',
                             'observe traverse right_insert')
                   for s in fixture('traverse')], 'traverse')

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
