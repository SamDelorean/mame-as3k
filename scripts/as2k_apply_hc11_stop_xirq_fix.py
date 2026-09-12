#!/usr/bin/env python3
# license:BSD-3-Clause
"""Apply a diagnostic MC68HC11 STOP/XIRQ wake fix.

The MC68HC11 datasheet specifies that XIRQ wakes the MCU from STOP even when
the CCR X mask is set.  In that masked case execution resumes after STOP and
no XIRQ service is requested.  The current core masks XIRQ before it can wake
the STOP state, so this helper applies the minimum diagnostic correction.

Kept as a helper while AS2000 validation is in progress; it does not modify the
production source on the branch.
"""
from pathlib import Path

path = Path("src/devices/cpu/mc68hc11/mc68hc11.cpp")
text = path.read_text()
old = '''\tcase MC68HC11_XIRQ_LINE:\n\t\tset_irq_state(0x05, state != CLEAR_LINE);\n\t\tbreak;'''
new = '''\tcase MC68HC11_XIRQ_LINE:\n\t\t// XIRQ wakes the HC11 from STOP even when X is masked.  In the\n\t\t// masked case no interrupt service is requested or left pending;\n\t\t// execution resumes with the instruction following STOP.\n\t\tif (state != CLEAR_LINE && m_stop_state == 1 && (m_ccr & CC_X))\n\t\t{\n\t\t\tm_stop_state = 2;\n\t\t\tset_irq_state(0x05, false);\n\t\t}\n\t\telse\n\t\t\tset_irq_state(0x05, state != CLEAR_LINE);\n\t\tbreak;'''
count = text.count(old)
assert count == 1, f"expected one XIRQ input handler, found {count}"
path.write_text(text.replace(old, new, 1))
