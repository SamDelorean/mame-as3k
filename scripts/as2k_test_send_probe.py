#!/usr/bin/env python3
"""Stock v3.1.4 normal Send probe; no wired transport or IR-detach claim.

Run with --runtime for private firmware validation; default runs ROM-free tests.
Exit 0 validated, 10 route/host-sense mismatch, 20 infrastructure, 30 evidence gap.
"""
import hashlib
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

MARKERS = (
    'AS2K_SEND_PROBE FIELD port=:COL.7 mask=10 name=Send',
    'AS2K_SEND_PROBE READY',
    'AS2K_SEND_PROBE PRESS after_typing_idle',
    'AS2K_GATE1A SEND_REDIRECT PC=9716',
    'AS2K_GATE1A FAIL_IR_SEND_D2DC PC=D2DC',
    'AS2K_SEND_PROBE RELEASE',
    'AS2K_SEND_PROBE HOST_SENSE reads=',
    'AS2K_SEND_PROBE COMPLETE',
    'AS2K_GATE1A PROBE_STOP instructions=',
)


def classify(log, rc):
    if rc:
        return 20, f'EMULATOR_RUNTIME rc={rc}'
    position = 0
    for marker in MARKERS:
        found = log.find(marker, position)
        if found < 0:
            return 30, f'EVIDENCE missing_or_unordered={marker}'
        position = found + len(marker)
    observations = re.findall(
        r'AS2K_SEND_PROBE HOST_SENSE reads=([0-9]+) high_mask=([0-9A-F]{2})$',
        log, re.MULTILINE)
    if (log.count('AS2K_SEND_PROBE HOST_SENSE') != 1
            or len(observations) != 1 or int(observations[0][0]) == 0):
        return 30, 'EVIDENCE missing_or_invalid_host_sense_reads'
    if int(observations[0][1], 16) != 0:
        return 10, 'stock_host_sense_mismatch PA0_or_PA2_high'
    if 'AS2K_GATE1A SEND_CABLE PC=8606' in log:
        return 10, 'stock_route_mismatch unexpected_cable_entry'
    return 0, ('VALIDATED stock_Send_matrix_to_9716_D2DC; '
        f'host_sense_reads={observations[0][0]} PA0_PA2_low; wired_Send_unproven')


def runtime():
    root = Path(__file__).resolve().parents[1]
    private = Path.home() / 'Projects/alphasmart/private'
    rom = Path(os.environ.get('AS2K_ROM', private / 'roms/as2k/AS2000_v3.1.4.bin'))
    binary = Path(os.environ.get('AS2K_DIAG_BIN', root / 'as2kdiag')).resolve()
    try:
        if hashlib.sha1(rom.read_bytes()).hexdigest() != 'e0b777dc68c671c31ba808e214fb9d2573b9a853':
            print('PRIVATE_INPUT wrong_stock_rom_sha1')
            return 20
        state = Path(os.environ.get('XDG_STATE_HOME', Path.home() / '.local/state')) / 'as2k-emulator-automation/send-probe'
        state.mkdir(parents=True, exist_ok=True)
        run = Path(tempfile.mkdtemp(prefix='run.', dir=state))
        rompath = ';'.join(map(str, (rom.resolve().parent,
            os.environ.get('AS2K_CGROM_ZIP_DIR', private / 'cgrom'),
            os.environ.get('AS2K_CGROM_RAW_DIR', private / 'roms/ks0066'))))
        command = [str(binary), 'asma2k', '-rompath', rompath,
            '-autoboot_script', str(root / 'scripts/as2k_send_probe.lua'),
            '-video', 'none', '-sound', 'none', '-skip_gameinfo', '-nothrottle',
            '-log', '-seconds_to_run', '20']
        with (run / 'console.log').open('w') as console:
            result = subprocess.run(command, cwd=run, stdout=console,
                stderr=subprocess.STDOUT, timeout=30, check=False)
        log = (run / 'error.log').read_text(errors='replace')
        rc, detail = classify(log, result.returncode)
        print(f'{detail} log={run / "error.log"}')
        return rc
    except (OSError, subprocess.TimeoutExpired) as exc:
        print(f'BLOCKED infrastructure_or_private_input {exc}')
        return 20


def fixture(markers=MARKERS):
    return '\n'.join(markers).replace(
        'HOST_SENSE reads=', 'HOST_SENSE reads=77 high_mask=00')


class EvidenceTests(unittest.TestCase):
    def test_ordered_stock_route(self):
        self.assertEqual(classify(fixture(), 0)[0], 0)

    def test_every_marker_required(self):
        for marker in MARKERS:
            with self.subTest(marker=marker):
                self.assertEqual(classify(fixture(m for m in MARKERS if m != marker), 0)[0], 30)

    def test_boot_landmarks_not_send_evidence(self):
        order = MARKERS[3:5] + MARKERS[:3] + MARKERS[5:]
        self.assertEqual(classify(fixture(order), 0)[0], 30)

    def test_runtime_failure_overrides_markers(self):
        self.assertEqual(classify(fixture(), 1)[0], 20)

    def test_host_sense_evidence_invalid(self):
        for value in ('reads=0 high_mask=00', 'reads=x high_mask=00',
                      'reads=77 high_mask=GG', 'reads=77 high_mask=00 extra'):
            with self.subTest(value=value):
                self.assertEqual(classify(fixture().replace(
                    'reads=77 high_mask=00', value), 0)[0], 30)

    def test_host_sense_unexpected_high(self):
        for mask in ('01', '04', '05'):
            self.assertEqual(classify(fixture().replace(
                'high_mask=00', 'high_mask=' + mask), 0)[0], 10)

    def test_duplicate_host_observation(self):
        self.assertEqual(classify(fixture() +
            '\nAS2K_SEND_PROBE HOST_SENSE reads=77 high_mask=00', 0)[0], 30)

    def test_outside_observation_cannot_replace_malformed(self):
        log = fixture().replace('reads=77', 'reads=x')
        log += '\nAS2K_SEND_PROBE HOST_SENSE reads=77 high_mask=00'
        self.assertEqual(classify(log, 0)[0], 30)

    def test_boot_host_observation_not_send_evidence(self):
        order = MARKERS[6:7] + MARKERS[:6] + MARKERS[7:]
        self.assertEqual(classify(fixture(order), 0)[0], 30)

    def test_unexpected_wired_route(self):
        self.assertEqual(classify(fixture() + '\nAS2K_GATE1A SEND_CABLE PC=8606', 0)[0], 10)


if __name__ == '__main__':
    if sys.argv[1:] == ['--runtime']:
        sys.exit(runtime())
    unittest.main()
