#!/usr/bin/env python3
# license:BSD-3-Clause
"""Add a synthetic periodic XIRQ source to the AS2000 diagnostic driver.

This is an experiment only.  1.25 s is a candidate period inferred from the
firmware's 148-count idle timeout and the manual's roughly 3–4 minute auto-off
interval.  It must not be treated as a hardware value until measured.
"""
from pathlib import Path

path = Path("src/mame/skeleton/alphasma.cpp")
text = path.read_text()

old = '''private:\n\tvoid zpsd_pmr_w(uint8_t data);\n\tvoid lcd_ctrl_w(uint8_t data);\n\tvirtual void port_a_w(uint8_t data) override;'''
new = '''private:\n\tTIMER_CALLBACK_MEMBER(xirq_test_tick);\n\tvoid zpsd_pmr_w(uint8_t data);\n\tvoid lcd_ctrl_w(uint8_t data);\n\tvirtual void port_a_w(uint8_t data) override;'''
assert text.count(old) == 1, "diagnostic declaration block changed"
text = text.replace(old, new, 1)

old = '''\tmemory_view m_io_view;\n\trequired_memory_bank m_dictbank;\n\n\tuint8_t m_lcd_ctrl;'''
new = '''\tmemory_view m_io_view;\n\trequired_memory_bank m_dictbank;\n\temu_timer *m_xirq_test_timer = nullptr;\n\n\tuint8_t m_lcd_ctrl;'''
assert text.count(old) == 1, "AS2000 member block changed"
text = text.replace(old, new, 1)

marker = '''void asma2k_state::machine_start()\n{\n\talphasmart_state::machine_start();\n\n\tm_dictbank->configure_entries(0, 8, memregion("spellcheck")->base(), 0x4000);\n\tm_dictbank->set_entry(0);\n\tlogerror("AS2KTRACE START dict=0 trace=lcd-key-v2\\n");\n}'''
replacement = '''TIMER_CALLBACK_MEMBER(asma2k_state::xirq_test_tick)\n{\n\tlogerror("AS2KTRACE XIRQTEST pc=%04X period_ms=1250\\n", unsigned(m_maincpu->pc()));\n\tm_maincpu->pulse_input_line(MC68HC11_XIRQ_LINE, attotime::from_usec(10));\n}\n\nvoid asma2k_state::machine_start()\n{\n\talphasmart_state::machine_start();\n\n\tm_dictbank->configure_entries(0, 8, memregion("spellcheck")->base(), 0x4000);\n\tm_dictbank->set_entry(0);\n\tlogerror("AS2KTRACE START dict=0 trace=lcd-key-v2\\n");\n\tm_xirq_test_timer = timer_alloc(FUNC(asma2k_state::xirq_test_tick), this);\n\tm_xirq_test_timer->adjust(attotime::from_msec(1250), 0, attotime::from_msec(1250));\n\tlogerror("AS2KTRACE XIRQTEST enabled candidate_period_ms=1250\\n");\n}'''
assert text.count(marker) == 1, "instrumented AS2000 machine_start changed"
text = text.replace(marker, replacement, 1)

path.write_text(text)
