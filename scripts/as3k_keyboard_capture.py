#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
"""Decode AS3K physical-host trace, never Writer RAM.

Set-2 mapping and make/break/modifier behavior adapted from the proven
AS2K STABLE_HOST_SOURCE.cpp pc_set2_byte/pc_key_event receiver. AS3K GPIO
and electrical frame validation live in its own driver, not AS2K's HC11.
ADB uses the distinct standard Macintosh keyboard code set.
"""
import argparse,json,re
from pathlib import Path
SET2={
 0x1c:'a',0x32:'b',0x21:'c',0x23:'d',0x24:'e',0x2b:'f',0x34:'g',
 0x33:'h',0x43:'i',0x3b:'j',0x42:'k',0x4b:'l',0x3a:'m',0x31:'n',
 0x44:'o',0x4d:'p',0x15:'q',0x2d:'r',0x1b:'s',0x2c:'t',0x3c:'u',
 0x2a:'v',0x1d:'w',0x22:'x',0x35:'y',0x1a:'z',
 0x45:'0)',0x16:'1!',0x1e:'2@',0x26:'3#',0x25:'4$',0x2e:'5%',
 0x36:'6^',0x3d:'7&',0x3e:'8*',0x46:'9(',0x0e:'`~',0x4e:'-_',
 0x55:'=+',0x54:'[{',0x5b:']}',0x5d:'\\|',0x4c:';:',0x52:'\'"',
 0x41:',<',0x49:'.>',0x4a:'/?',0x29:' ',0x5a:'\r',0x0d:'\t',0x66:'\b',
}
ADB={
 0x00:'a',0x01:'s',0x02:'d',0x03:'f',0x04:'h',0x05:'g',0x06:'z',
 0x07:'x',0x08:'c',0x09:'v',0x0b:'b',0x0c:'q',0x0d:'w',0x0e:'e',
 0x0f:'r',0x10:'y',0x11:'t',0x12:'1!',0x13:'2@',0x14:'3#',0x15:'4$',
 0x16:'6^',0x17:'5%',0x18:'=+',0x19:'9(',0x1a:'7&',0x1b:'-_',
 0x1c:'8*',0x1d:'0)',0x1e:']}',0x1f:'o',0x20:'u',0x21:'[{',0x22:'i',
 0x23:'p',0x24:'\r',0x25:'l',0x26:'j',0x27:'\'"',0x28:'k',0x29:';:',
 0x2a:'\\|',0x2b:',<',0x2c:'/?',0x2d:'n',0x2e:'m',0x2f:'.>',
 0x30:'\t',0x31:' ',0x32:'`~',0x33:'\b',
}

def decode(values,protocol):
 text=[];shift=set();caps=False;brk=False;ext=False;unknown=[];events=[]
 for value in values:
  if protocol=='set2':
   if value in (0xaa,0xfa):continue
   if value==0xf0:brk=True;continue
   if value==0xe0:ext=True;continue
   code=value;released=brk;brk=False
   if code in (0x12,0x59):
    (shift.discard if released else shift.add)(code);ext=False;continue
   if code==0x58 and not released:caps=not caps;continue
   mapping=SET2;ignore=(0x11,0x14)
  else:
   if value==0xff:continue
   code=value&0x7f;released=bool(value&0x80);mapping=ADB;ignore=(0x37,0x3a,0x3b,0x3d,0x3e)
   if code in (0x38,0x3c):
    (shift.discard if released else shift.add)(code);continue
   if code==0x39:
    caps=not released;continue
  if code in ignore:ext=False;continue
  if released:ext=False;continue
  if ext or code not in mapping:unknown.append(value);ext=False;continue
  pair=mapping[code];use_shift=bool(shift)
  if len(pair)==1 and pair.isalpha():char=pair.upper() if use_shift^caps else pair
  else:char=pair[int(use_shift)] if len(pair)>1 else pair
  text.append(char);events.append({'code':value,'text_hex':char.encode('ascii').hex()});ext=False
 return ''.join(text).encode('ascii'),unknown,events

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path);p.add_argument('--protocol',choices=['set2','adb'],required=True);p.add_argument('--output-directory',type=Path,required=True);p.add_argument('--expected-hex',required=True);a=p.parse_args()
 source=a.log.read_text(errors='replace');errors=[]
 if a.protocol=='set2':
  frames=re.findall(r'AS3K PS2: byte=([0-9A-F]{2}) valid=([01]) frame=([0-9A-F]+)',source)
  values=[]
  for value,valid,frame in frames:
   raw=int(frame,16);v=int(value,16)
   if valid!='1' or raw&1 or not raw&0x400 or ((v.bit_count()+((raw>>9)&1))&1)!=1:errors.append(raw)
   else:values.append(v)
 else:
  frames=re.findall(r'AS3K ADB: byte=([0-9A-F]{2}) cells=(\d+)',source)
  values=[int(v,16) for v,_ in frames]
  errors=[int(n) for _,n in frames if int(n)!=17]
 data,unknown,events=decode(values,a.protocol);expected=bytes.fromhex(a.expected_hex)
 result={'status':'PASS' if data==expected and not errors and not unknown else 'FAIL','protocol':a.protocol,'expected_hex':expected.hex(),'received_hex':data.hex(),'wire_bytes_hex':bytes(values).hex(),'frame_errors':errors,'unknown_make_codes':unknown,'events':events}
 a.output_directory.mkdir(exist_ok=True,parents=True);(a.output_directory/'keyboard.raw').write_bytes(bytes(values));(a.output_directory/'capture.txt').write_bytes(data);(a.output_directory/'capture.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));return 0 if result['status']=='PASS' else 1
if __name__=='__main__':raise SystemExit(main())
