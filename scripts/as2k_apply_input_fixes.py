#!/usr/bin/env python3
# license:BSD-3-Clause
"""Apply AS2000 input fixes validated against firmware v3.1.4 traces.

This helper is intentionally separate from the production driver while the
AS2000 work is still on the diagnostic branch. Every replacement is scoped to
INPUT_PORTS_START(asma2k) and asserted so CI fails if upstream context changes.
"""
from pathlib import Path

path = Path("src/mame/skeleton/alphasma.cpp")
text = path.read_text()

asma2k_start = text.index('static INPUT_PORTS_START( asma2k )')
asma2k_end = text.index('INPUT_PORTS_END', asma2k_start)
block = text[asma2k_start:asma2k_end]

replacements = [
    (
        "PORT_CODE(KEYCODE_K)     PORT_CHAR('k') PORT_CHAR('k')",
        "PORT_CODE(KEYCODE_K)     PORT_CHAR('k') PORT_CHAR('K')",
    ),
    (
        "PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('_')",
        "PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')",
    ),
    (
        "PORT_CODE(KEYCODE_Z)    PORT_CHAR('z')  PORT_CHAR('z')",
        "PORT_CODE(KEYCODE_Z)    PORT_CHAR('z')  PORT_CHAR('Z')",
    ),
]

for old, new in replacements:
    count = block.count(old)
    assert count == 1, f"expected exactly one AS2000 occurrence of {old!r}, found {count}"
    block = block.replace(old, new, 1)

# Firmware v3.1.4 scans keycode 0x47 as Send. Matrix encoding is
# keycode=(bit_index<<4)|column, so 0x47 is COL.7 bit 0x10.
col7_start = block.index('PORT_START("COL.7")')
col7_end = block.index('PORT_START("COL.8")', col7_start)
col7 = block[col7_start:col7_end]
old_send = "\tPORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)"
new_send = "\tPORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F12) PORT_NAME(\"Send\") PORT_CHAR(UCHAR_MAMEKEY(F12)) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart_state::kb_irq), 0)"
assert col7.count(old_send) == 1, "expected one unused AS2000 COL.7 bit 0x10 field"
col7 = col7.replace(old_send, new_send, 1)
block = block[:col7_start] + col7 + block[col7_end:]

text = text[:asma2k_start] + block + text[asma2k_end:]
path.write_text(text)
