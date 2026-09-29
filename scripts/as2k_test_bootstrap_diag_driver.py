#!/usr/bin/env python3
"""Static guard for AS2000 bootstrap diagnostic driver activation."""

from pathlib import Path
import re

SRC = Path("src/mame/skeleton/alphasma.cpp").read_text()

expected_stage0 = [
    0x0f, 0x8e, 0x00, 0xc3, 0x14, 0x3f, 0x04, 0x14, 0x3c,
    0x20, 0x15, 0x00, 0x70, 0x86, 0x04, 0xb7, 0x40, 0x00,
    0x15, 0x3c, 0x80, 0x15, 0x3c, 0x40, 0x7e, 0x40, 0x00,
]

required = [
    "void asma2k(machine_config &config);",
    "void asma2kbt(machine_config &config);",
    "virtual void machine_reset() override ATTR_COLD;",
    "bool m_bootstrap_diag = false;",
    "bool m_takeover_seen = false;",
    'fatalerror("AS2K_BOOT forbidden Z fetch at %04X", pc);',
    'logerror("AS2K_BOOT FIRST_DICT_FETCH pc=%04X bank=%u PA=%02X CTRL=%02X\\n",',
    'memory_region *const stage0 = memregion("stage0");',
    'cpu.set_diag_bootstrap(true);',
    'cpu.diag_bootstrap_load(stage0->base(), sizeof(expected_stage0), 0x0040);',
    'void asma2k_state::asma2kbt(machine_config &config)',
    'm_bootstrap_diag = true;',
]

for needle in required:
    assert needle in SRC, "missing driver invariant: " + needle

m = re.search(
    r"static constexpr uint8_t expected_stage0\[\]\s*=\s*\{([^}]*)\};",
    SRC,
    re.S,
)
assert m, "missing embedded exact-stage0 verification array"
actual = [int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]{2})", m.group(1))]
assert actual == expected_stage0, "diagnostic stage0 guard differs from BT7 image"
assert len(actual) == 27

# The driver configuration exists but the diagnostic system is not exposed as
# a runnable COMP entry until the dedicated ROM set is added next.
assert "COMP( 1997, asma2kbt" not in SRC
assert "ROM_START( asma2kbt )" not in SRC

print("PASS: exact BT7 stage0 guard = 27 bytes")
print("PASS: diagnostic machine config and injection path present")
print("PASS: zero-Z guard and first-DictROM-fetch logging present")
print("PASS: asma2kbt not yet exposed without its dedicated ROM set")
