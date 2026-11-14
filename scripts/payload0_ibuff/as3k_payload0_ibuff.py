#!/usr/bin/env python3
"""AlphaSmart 3000 payload-0: BOOT OK without using external SRAM or Flash.

MC68EZ328UM §16.4: load and execute an 8-byte instruction at 0xFFFFFFAA.
Each instruction is MOVE.B #imm,($FFFFF4xx).L; it changes GPIO registers
without using CPU data registers, stack or any off-chip memory.

No raw ROM, RAM snapshot, firmware or host-side LCD drawing is required.
This generator is independent of any MAME or hardware adapter; the recorded
board pin mapping must be confirmed on real AS3000 hardware.
"""
from __future__ import annotations
import argparse
from dataclasses import dataclass
from pathlib import Path
import json
import hashlib

IBUFF = 0xFFFFFFAA
PCDIR = 0xFFFFF410
PCDATA = 0xFFFFF411
PCSEL = 0xFFFFF413
E0 = 0x40                # PC6 -> KS0066 #0 Enable, verified in AS3K driver
RS = 0x20                # PC5 -> RS
RW = 0x10                # PC4 -> R/W; kept low
MESSAGE = b"BOOT OK"

@dataclass(frozen=True)
class Step:
    addr: int
    value: int
    operation: str

def build_steps() -> list[Step]:
    steps = []
    def write(addr: int, value: int, op: str):
        assert 0 <= value <= 0xff
        assert addr in (PCDIR, PCDATA, PCSEL)
        steps.append(Step(addr,value,op))

    # The chip's built-in LCD controller must be disengaged: PCSEL=1 means GPIO.
    # PC[7:0]: E2, E1, RS, R/W, DB7, DB6, DB5, DB4.
    write(PCSEL, 0xff, 'GPIO select: disable dedicated LCD peripheral pins')
    write(PCDIR, 0xff, 'configure all port C bits as outputs')
    write(PCDATA, 0x00, 'both enables deasserted; RS=RW=0')

    def nibble(n: int, data: bool, label: str):
        assert 0 <= n <= 15
        base = n | (RS if data else 0)
        write(PCDATA,base,f'{label}: data stable, E0=0')
        write(PCDATA,base|E0,f'{label}: E0=1')
        write(PCDATA,base,f'{label}: E0 falling edge (latch)')

    def lcd_byte(v: int, data: bool, label: str):
        nibble(v>>4,data,f'{label} high')
        nibble(v&15,data,f'{label} low')

    # Exactly the AS2K/ASPro algorithm: power-on 3,3,3,2 and 4-bit command/data.
    for i, n in enumerate((3,3,3,2),1):
        nibble(n,False,f'4-bit wake {i}/4')
    for value, name in ((0x28,'function set 4-bit 2-line'),
                        (0x08,'display off'),
                        (0x01,'clear display'),
                        (0x06,'entry mode'),
                        (0x0c,'display on'),
                        (0x80,'DDRAM home')):
        lcd_byte(value,False,name)
    for i,c in enumerate(MESSAGE):
        lcd_byte(c,True,f'char {i} {chr(c)!r}')
    return steps

def assemble_moveb_imm_abs(addr: int,value: int) -> bytes:
    """68k instruction 13FC 00NN AAAAAAAA; exactly 8 bytes, no stack."""
    return bytes.fromhex('13FC')+value.to_bytes(2,'big')+addr.to_bytes(4,'big')

def brecord(addr: int, payload: bytes = b'') -> str:
    assert len(payload) <= 31
    return f'{addr:08X}{len(payload):02X}{payload.hex().upper()}'

def build_records(steps: list[Step]):
    lines=[]
    for s in steps:
        ins=assemble_moveb_imm_abs(s.addr,s.value)
        assert len(ins)==8
        lines.append(brecord(IBUFF,ins))
        lines.append(brecord(IBUFF))
    return lines

def audit(steps,lines):
    assert len(lines)==len(steps)*2
    allowed={PCDIR,PCDATA,PCSEL}
    decoded=[]
    for s,load,run in zip(steps,lines[::2],lines[1::2]):
        assert load.startswith('FFFFFFAA08') and len(load)==26
        assert run=='FFFFFFAA00'
        ins=bytes.fromhex(load[10:]);assert len(ins)==8
        assert ins[:2]==b'\x13\xfc' and ins[2]==0
        value=ins[3];addr=int.from_bytes(ins[4:8],'big')
        assert addr in allowed and addr==s.addr and value==s.value
        assert not (0x00000000<=addr<=0x0003ffff)
        decoded.append((addr,value))
    # On each falling E0 edge the KS0066 latches the nibble from PCDATA.
    previous=0
    captured=[]
    for a,v in decoded:
        if a!=PCDATA: continue
        if previous & E0 and not v & E0:
            captured.append((bool(v & RS),v & 0x0f))
        previous=v
    expected=[(False,n) for n in (3,3,3,2)]
    for b in (0x28,0x08,0x01,0x06,0x0c,0x80):
        expected.extend(((False,b>>4),(False,b&0xf)))
    for b in MESSAGE:
        expected.extend(((True,b>>4),(True,b&0xf)))
    assert captured==expected, (captured,expected)
    return {'result':'STATIC_PROTOCOL_PASS','source':'AS2K original payload0.asm',
            'boot_text':MESSAGE.decode(), 'load_address':f'0x{IBUFF:08X}',
            'instruction_bytes':8,'instruction_count':len(steps),
            'b_records':len(lines), 'lcd_nibble_strobes':len(captured),
            'allowed_destinations':[f'0x{x:08X}' for x in sorted(allowed)],
            'external_sram_writes':0,'external_sram_reads':0,'external_flash_writes':0,
            'external_stack_accesses':0,'cpu_register_changes':'No D0-D7/A0-A7 write instructions',
            'b_stream_sha256':hashlib.sha256(('\r'.join(lines)+'\r').encode()).hexdigest(),
            'qualification':'STATIC ONLY: not executed on MAME or physical AlphaSmart 3000'}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out',default='payload0_ibuff_boot_ok.b',help='B-record output')
    parser.add_argument('--audit',default='audit.json',help='Static audit JSON')
    args=parser.parse_args()
    steps=build_steps();lines=build_records(steps);report=audit(steps,lines)
    Path(args.out).write_bytes(('\r'.join(lines)+'\r').encode('ascii'))
    Path(args.audit).write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
    print(json.dumps(report,indent=2,ensure_ascii=False))

if __name__=='__main__': main()
