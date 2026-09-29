#!/usr/bin/env python3
"""Static guard for the bounded HC11D0 bootstrap diagnostic core patch.

This does not replace a C++ build. It prevents accidental loss of the exact
source-level invariants required before wiring the AS2000 diagnostic driver.
"""

from pathlib import Path

H = Path("src/devices/cpu/mc68hc11/mc68hc11.h").read_text()
C = Path("src/devices/cpu/mc68hc11/mc68hc11.cpp").read_text()
D = Path("src/mame/skeleton/alphasma.cpp").read_text()

required_h = [
    "void diagnostic_bootstrap_entry(const uint8_t *data, uint16_t size, uint16_t address);",
    "void set_diag_bootstrap(bool enable) { m_diag_bootstrap = enable; }",
    "uint8_t hprio() const { return m_hprio; }",
    "void diag_bootstrap_load(const uint8_t *data, uint16_t size, uint16_t address = 0x0040);",
    "uint8_t hprio_r();",
    "void hprio_w(uint8_t data);",
]

required_c = [
    "block(base + 0x3c, base + 0x3c).rw(FUNC(mc68hc11d0_device::hprio_r), FUNC(mc68hc11d0_device::hprio_w));",
    "m_hprio = m_diag_bootstrap ? 0xc0 : 0x00;",
    "if (!m_diag_bootstrap || !(m_hprio & 0x40))",
    "m_hprio = (m_hprio & ~0x20) | (data & 0x20);",
    "m_hprio = 0xc0; // RBOOT=1, SMOD=1, MDA=0",
    "m_config |= 0x04; // NOCOP=1 in special mode",
    "m_irq_state = 0;",
    "m_wait_state = 0;",
    "m_pc = address;",
]

for needle in required_h:
    assert needle in H, "missing header invariant: " + needle

for needle in required_c:
    assert needle in C, "missing core invariant: " + needle

# Increment 1 must not silently activate diagnostic bootstrap in the AS2000
# driver. Stock emulation remains on the existing path until Increment 2.
assert "set_diag_bootstrap(" not in D
assert "diag_bootstrap_load(" not in D

print("PASS: HC11D0 bounded diagnostic bootstrap core invariants present")
print("PASS: AS2000 driver still does not activate diagnostic bootstrap")
