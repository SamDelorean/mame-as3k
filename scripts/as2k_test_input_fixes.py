#!/usr/bin/env python3
# license:BSD-3-Clause
"""Cheap subprocess fixtures for the diagnostic input compatibility helper."""
from itertools import product
from pathlib import Path
import subprocess
import sys
import tempfile

helper = Path(__file__).with_name('as2k_apply_input_fixes.py').resolve()
source = Path('src/mame/skeleton/alphasma.cpp').read_text()
start = source.index('static INPUT_PORTS_START( asma2k )')
end = source.index('INPUT_PORTS_END', start)
block = source[start:end]
pairs = [
    ("PORT_CHAR('k') PORT_CHAR('k')", "PORT_CHAR('k') PORT_CHAR('K')"),
    ("PORT_CHAR('z')  PORT_CHAR('z')", "PORT_CHAR('z')  PORT_CHAR('Z')"),
    ("PORT_CHAR('=') PORT_CHAR('_')", "PORT_CHAR('=') PORT_CHAR('+')"),
    ('\tPORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)', next(
        line for line in block.splitlines() if 'PORT_NAME("Send")' in line)),
]
for _, new in pairs:
    assert block.count(new) == 1, f'production correction missing: {new}'

with tempfile.TemporaryDirectory(prefix='as2k-input-fixtures-') as tmp:
    path = Path(tmp) / 'src/mame/skeleton/alphasma.cpp'
    path.parent.mkdir(parents=True)

    def run(text, success=True):
        path.write_text(text)
        result = subprocess.run([sys.executable, str(helper)], cwd=tmp,
                                capture_output=True, text=True)
        assert (result.returncode == 0) == success, result.stderr
        assert path.read_text() == (source if success else text)

    for legacy in product((False, True), repeat=4):
        fixture = block
        for use_old, (old, new) in zip(legacy, pairs):
            if use_old:
                fixture = fixture.replace(new, old, 1)
        run(source[:start] + fixture + source[end:])
        run(path.read_text())  # idempotence, including unchanged Pro/outside text
    for _, new in pairs:
        for replacement in ('', new + '\n' + new, new.replace('PORT_', 'BAD_', 1)):
            run(source[:start] + block.replace(new, replacement, 1) + source[end:], False)
    for old, new in pairs[:3]:
        run(source[:start] + block.replace(new, old[:-2] + "?')", 1) + source[end:], False)
print('PASS: production mappings, 16 legacy/corrected combinations, idempotence, outside-block preservation, 15 rejected fixtures')
