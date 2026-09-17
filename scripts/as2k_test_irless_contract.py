#!/usr/bin/env python3
"""Fail-closed classifier for deterministic AS2000 IRLESS Send/Print evidence."""
import unittest

KNOWN_FAIL = (0xD099, 0xD2DC, 0xD437)

def positions(log, marker):
    out, pos = [], 0
    while True:
        pos = log.find(marker, pos)
        if pos < 0:
            return out
        out.append(pos)
        pos += len(marker)

def first_after(log, marker, pos):
    found = log.find(marker, pos + 1)
    return None if found < 0 else found

def classify(log, rc=0, profile='full'):
    if rc:
        return 20, [f'EMULATOR_RUNTIME rc={rc}']
    failures, notes = [], []
    if positions(log, 'AS2K_GATE1A FORBIDDEN_PC='):
        failures.append('forbidden IrDA/reclaim execution observed')
    fail_markers = {
        0xD099: 'AS2K_GATE1A FAIL_IR_SEND_D099 PC=D099',
        0xD2DC: 'AS2K_GATE1A FAIL_IR_SEND_D2DC PC=D2DC',
        0xD437: 'AS2K_GATE1A FAIL_IR_PRINT_D437 PC=D437',
    }
    for pc, marker in fail_markers.items():
        if marker in log:
            failures.append(f'known IrDA handler executed: ${pc:04X}')
    send = False
    for start in positions(log, 'AS2K_GATE1A SEND_REDIRECT PC=9716'):
        if first_after(log, 'AS2K_GATE1A SEND_CABLE PC=8606', start) is not None:
            send = True
            break
    print_fallback = False
    for call in ('AS2K_GATE1A PRINT_DETACH_962D PC=962D',
                 'AS2K_GATE1A PRINT_DETACH_9804 PC=9804'):
        for start in positions(log, call):
            if first_after(log, 'AS2K_GATE1A PRINT_FALLBACK PC=ABC9', start) is not None:
                print_fallback = True
                break
    notes += [f'SEND_SEQUENCE_9716_TO_8606={send}',
              f'PRINT_SEQUENCE_CALLSITE_TO_ABC9={print_fallback}']
    if profile in ('send', 'full') and not send:
        failures.append('required ordered Send evidence missing')
    if profile in ('print', 'full') and not print_fallback:
        failures.append('required ordered Print evidence missing')
    if profile not in ('safety', 'send', 'print', 'full'):
        failures.append('invalid profile')
    return (0 if not failures else 10), failures + notes

class Tests(unittest.TestCase):
    good = ('AS2K_GATE1A SEND_REDIRECT PC=9716\n'
            'AS2K_GATE1A SEND_CABLE PC=8606\n'
            'AS2K_GATE1A PRINT_DETACH_9804 PC=9804\n'
            'AS2K_GATE1A PRINT_FALLBACK PC=ABC9')
    def test_full_good(self):
        self.assertEqual(classify(self.good)[0], 0)
    def test_missing_send(self):
        log = ('AS2K_GATE1A PRINT_DETACH_9804 PC=9804\n'
               'AS2K_GATE1A PRINT_FALLBACK PC=ABC9')
        self.assertEqual(classify(log)[0], 10)
    def test_wrong_send_order(self):
        log = ('AS2K_GATE1A SEND_CABLE PC=8606\n'
               'AS2K_GATE1A SEND_REDIRECT PC=9716')
        self.assertEqual(classify(log, 0, 'send')[0], 10)
    def test_forbidden(self):
        self.assertEqual(classify(self.good + '\nAS2K_GATE1A FORBIDDEN_PC=E104')[0], 10)
    def test_known_fail(self):
        self.assertEqual(classify(self.good + '\nAS2K_GATE1A FAIL_IR_PRINT_D437 PC=D437')[0], 10)
    def test_safety(self):
        self.assertEqual(classify('AS2K_GATE1A PROBE_ACTIVE', 0, 'safety')[0], 0)
    def test_runtime(self):
        self.assertEqual(classify(self.good, 1)[0], 20)

if __name__ == '__main__':
    unittest.main()
