#!/usr/bin/env python3
"""ROM-free shell-harness classification tests; fake MAME is not runtime evidence."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


HARNESS = Path(__file__).resolve().parents[1] / 'tools/automation/test_as2k_boot_editor.sh'


class BootEditorHarnessTest(unittest.TestCase):
    def run_harness(self, *, probe=True, marker=True, status=0):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scripts = root / 'scripts'
            scripts.mkdir(parents=True)
            if probe:
                (scripts / 'as2k_editor_ready.lua').write_text('-- fixture\n')
            rom = root / 'fixture.bin'
            rom.touch()
            (root / 'ks0066_f05.bin').touch()
            commands = root / 'bin'
            commands.mkdir()
            sha1 = commands / 'sha1sum'
            sha1.write_text('#!/bin/sh\necho e0b777dc68c671c31ba808e214fb9d2573b9a853\n')
            sha1.chmod(0o755)
            mame = commands / 'mame'
            mame.write_text(
                '#!/bin/sh\n'
                'touch error.log\n'
                + ('echo "AS2K_GATE1A EDITOR_READY PC=87D7 stable_frames=60" >error.log\n'
                   if marker else '')
                + f'exit {status}\n')
            mame.chmod(0o755)
            env = dict(os.environ, AS2K_EMU_ROOT=str(root),
                       AS2K_DIAG_BIN=str(mame), AS2K_ROM=str(rom),
                       AS2K_CGROM_ZIP_DIR=str(root), AS2K_CGROM_RAW_DIR=str(root),
                       XDG_STATE_HOME=str(root / 'state'),
                       PATH=str(commands) + os.pathsep + os.environ['PATH'])
            return subprocess.run(['bash', str(HARNESS)], env=env,
                                  capture_output=True, text=True, timeout=10)

    def test_ready_success(self):
        result = self.run_harness()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('VALIDATED BOOT_EDITOR', result.stdout)

    def test_ready_does_not_hide_runtime_failure(self):
        result = self.run_harness(status=1)
        self.assertEqual(result.returncode, 20)
        self.assertIn('mame_rc=1', result.stdout)

    def test_missing_repository_probe(self):
        result = self.run_harness(probe=False)
        self.assertEqual(result.returncode, 30)
        self.assertIn('editor_probe_unavailable', result.stdout)

    def test_missing_marker(self):
        result = self.run_harness(marker=False)
        self.assertEqual(result.returncode, 30)
        self.assertIn('no_editor_readiness_marker', result.stdout)


if __name__ == '__main__':
    unittest.main()
