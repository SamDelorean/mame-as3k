#!/usr/bin/env python3
# license:BSD-3-Clause
"""Apply AS2000 input fixes validated against firmware v3.1.4 traces.

Production includes these corrections. Keep accepting legacy and corrected
fields for the diagnostic workflow, rejecting missing, duplicate or unexpected
character mappings. All replacements are scoped to INPUT_PORTS_START(asma2k).
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

for (old, new), mask, gap in zip(replacements, ("0x04", "0x10", "0x40"), ("  ", " ", "  ")):
    prefix = f"\tPORT_BIT({mask}, IP_ACTIVE_LOW, IPT_KEYBOARD) "
    suffix = gap + "PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart_state::kb_irq), 0)"
    key = old.split(" PORT_CHAR", 1)[0].rstrip()
    old, new = prefix + old + suffix, prefix + new + suffix
    fields = [line for line in block.splitlines() if key in line]
    count = block.count(old) + block.count(new)
    if len(fields) != 1 or count != 1 or fields[0] not in (old, new):
        raise ValueError(f"unexpected AS2000 mapping for {key}: {fields!r}")
    block = block.replace(old, new, 1)

# Firmware v3.1.4 scans keycode 0x47 as Send. Matrix encoding is
# keycode=(bit_index<<4)|column, so 0x47 is COL.7 bit 0x10.
col7_start = block.index('PORT_START("COL.7")')
col7_end = block.index('PORT_START("COL.8")', col7_start)
col7 = block[col7_start:col7_end]
old_send = "\tPORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)"
new_send = "\tPORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F12) PORT_NAME(\"Send\") PORT_CHAR(UCHAR_MAMEKEY(F12)) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart_state::kb_irq), 0)"
fields = [line for line in col7.splitlines() if "PORT_BIT(0x10," in line]
if fields not in ([old_send], [new_send]):
    raise ValueError(f"unexpected AS2000 COL.7 bit 0x10 mapping: {fields!r}")
col7 = col7.replace(old_send, new_send, 1)
block = block[:col7_start] + col7 + block[col7_end:]

text = text[:asma2k_start] + block + text[asma2k_end:]
path.write_text(text)
