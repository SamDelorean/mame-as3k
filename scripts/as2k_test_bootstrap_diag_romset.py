#!/usr/bin/env python3
"""Static guard for the runnable asma2kbt diagnostic ROM set."""

from pathlib import Path

SRC = Path("src/mame/skeleton/alphasma.cpp").read_text()

required = [
    "ROM_START( asma2kbt )",
    'ROM_REGION( 0x10000, "maincpu", 0 )',
    'ROM_LOAD( "alphasmart__2000__v3.1.4__h4.zpsd211r.plcc44.bin", 0x0000, 0x81e5, CRC(49487f6d) SHA1(e0b777dc68c671c31ba808e214fb9d2573b9a853) )',
    'ROM_LOAD( "as2k_bt8_dictrom.bin", 0x00000, 0x20000, CRC(c5bd89df) SHA1(6e84689da9e0705920bca24137c88a387dedfac1) )',
    'ROM_REGION( 0x001b, "stage0", 0 )',
    'ROM_LOAD( "as2k_stage0.bin", 0x0000, 0x001b, CRC(ff5dedf9) SHA1(ab76eafa386311b2ab70ea644345fb15767e908f) )',
    'COMP( 2026, asma2kbt, asma2k, 0,      asma2kbt,   asma2k,     asma2k_state, empty_init, "SamDelorean", "AlphaSmart 2000 (Bootstrap Takeover Diagnostic)", MACHINE_NOT_WORKING | MACHINE_NO_SOUND )',
]

for needle in required:
    assert needle in SRC, "missing ROM-set invariant: " + needle

assert SRC.count("ROM_START( asma2kbt )") == 1
assert SRC.count("COMP( 2026, asma2kbt") == 1

print("PASS: runnable asma2kbt ROM set declared")
print("PASS: stage0 hash = ff5dedf9 / ab76eafa...")
print("PASS: BT8 DictROM hash = c5bd89df / 6e84689d...")
print("PASS: asma2kbt remains a clone of asma2k")
