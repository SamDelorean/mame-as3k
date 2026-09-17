#!/usr/bin/env python3
import re, sys
from pathlib import Path

SET2 = {0x1c:'a',0x32:'b',0x21:'c',0x23:'d',0x24:'e',0x2b:'f',0x34:'g',0x33:'h',
        0x43:'i',0x3b:'j',0x42:'k',0x4b:'l',0x3a:'m',0x31:'n',0x44:'o',0x4d:'p',
        0x15:'q',0x2d:'r',0x1b:'s',0x2c:'t',0x3c:'u',0x2a:'v',0x1d:'w',0x22:'x',
        0x35:'y',0x1a:'z',0x45:'0',0x16:'1',0x1e:'2',0x26:'3',0x25:'4',0x2e:'5',
        0x36:'6',0x3d:'7',0x3e:'8',0x46:'9',0x29:' ',0x5a:'\n',0x0d:'\t',0x66:'\b'}
if len(sys.argv) != 3: raise SystemExit('usage: decoder ERROR.LOG salida.txt')
raw=[int(x,16) for x in re.findall(r'BYTE_AA54 value=([0-9A-Fa-f]{2})',Path(sys.argv[1]).read_text(errors='ignore'))]
out=[]; released=False; pressed=set()
for b in raw:
    if b==0xf0: released=True; continue
    if released: pressed.discard(b); released=False; continue
    if b in SET2: out.append(SET2[b]); pressed.add(b)
    else: raise SystemExit(f'unknown Set-2 make code {b:02X}')
Path(sys.argv[2]).write_text(''.join(out))
print(f'AS2K_DECODE bytes={len(raw)} text={"".join(out)!r}')
