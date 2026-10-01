#!/usr/bin/env python3
"""Static guard for the Payload-1b FAIL display MAME fixture."""

from pathlib import Path

SRC = Path("src/mame/skeleton/alphasma.cpp").read_text()
GEN = Path("scripts/as2k_prepare_payload1b_fail.py").read_text()

required_src = [
    "void asma2k1bf(machine_config &config);",
    "bool m_payload1b_fail_diag = false;",
    "bool m_payload1b_fail_seen = false;",
    "AS2K_P1B_FAIL requires exact 127-byte stage0 region",
    "AS2K_P1B_FAIL INJECT payload size=127 entry=0040 message=FAIL PB",
    "AS2K_P1B_FAIL DISPLAY_READY pc=008D message=FAIL PB",
    "void asma2k_state::asma2k1bf(machine_config &config)",
    "m_payload1b_fail_diag = true;",
    "ROM_START( asma2k1bf )",
    'ROM_REGION( 0x007f, "stage0", 0 )',
    'ROM_LOAD( "as2k_payload1b_fail.bin", 0x0000, 0x007f, CRC(e0e98168) SHA1(ff3136f95aff8f74c77eb968ecb684af2905ce40) )',
    'COMP( 2026, asma2k1bf',
    '"AlphaSmart 2000 (Payload-1b FAIL Display Test)"',
]
for needle in required_src:
    assert needle in SRC, "missing driver invariant: " + needle

required_gen = [
    'PAYLOAD0[-8:] == b"BOOT OK\\x00"',
    'FAIL = PAYLOAD0[:-8] + b"FAIL PB\\x00"',
    'EXPECTED_DONOR_SHA256 = "54582354a972848c642b2c87893ae2cc693483eae3d161693edf8f10d26f634a"',
    'EXPECTED_FAIL_SHA256 = "7469f23dbe03b3a35cfac66fc4be5e107d15498542a892f947b70c900cc288a"',
]
for needle in required_gen:
    assert needle in GEN, "missing generator invariant: " + needle

# Do not weaken the existing BT8 exact-stage0 diagnostic.
assert "AS2K_BOOT requires exact 27-byte stage0 region" in SRC
assert "ROM_START( asma2kbt )" in SRC

print("PASS: Payload-1b FAIL fixture is isolated in asma2k1bf")
print("PASS: FAIL image differs from frozen G0 only in final message field")
print("PASS: expected FAIL display loop is $008D")
print("PASS: existing asma2kbt diagnostic remains present")
