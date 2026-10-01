#!/usr/bin/env python3
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SRC=(ROOT/'src/mame/skeleton/alphasma.cpp').read_text()

required=[
 'm_zpsd_cpbf_raw = 0xff',
 'AS2K_ZPSD SPECIAL_04_80 PASS',
 'AS2K_ZPSD SPECIAL_00_08 PASS PORT_SELECTED',
 'AS2K_ZPSD PSEN_PULSE count=1',
 'm_zpsd_cpbf_raw = 0xef',
 'AS2K_P1 %s special=2 addr=0003 data=EF pulses=1 cpbf=%02X',
 'AS2K_P1B_FUNCTIONAL %s cpbf=%02X display=%s pc=%04X',
 'ROM_START( asma2kp1 )',
 'ROM_START( asma2kp1f )',
 'zpsd_pb_pin_r()',
 'ROM_START( asma2k1bp )',
 'ROM_START( asma2k1bu )',
]
for x in required:
    assert x in SRC, x
assert SRC.count('m_zpsd_cpbf_raw = 0xef;') == 2  # P1 mutation + P1b PASS fixture seed
assert 'fuse_grid' not in SRC.lower()
print('PASS: bounded shared ZPSD CPBF model present for Payload-1 and Payload-1b')
