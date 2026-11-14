#!/usr/bin/env python3
"""Assemble the two own 68000 sources; no proprietary firmware input."""
import argparse,hashlib,json,shutil,struct,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--output-directory',required=True);p.add_argument('--regenerate-model-header',action='store_true');a=p.parse_args()
root=Path(__file__).resolve().parents[1];out=Path(a.output_directory).resolve();out.mkdir(parents=True,exist_ok=True)
prefix=str(Path.home()/'.local/bin/m68k-elf-')
meta={}
for name,src,entry,address in [('model',root/'src/devices/machine/mc68ez328_bootstrap.s','model_start','0xffffff00'),('payload0',root/'scripts/as3k_payload0.s','_start','0x30000')]:
 for cmd in [[prefix+'as','-m68000','-o',str(out/(name+'.o')),str(src)],[prefix+'ld','-e',entry,'-Ttext',address,'-o',str(out/(name+'.elf')),str(out/(name+'.o'))],[prefix+'objcopy','-O','binary',str(out/(name+'.elf')),str(out/(name+'.bin'))]]:subprocess.run(cmd,check=True)
 raw=(out/(name+'.bin')).read_bytes();meta[name]={'source':str(src.relative_to(root)),'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'address':address,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
 (out/(name+'.lst')).write_text(subprocess.check_output([prefix+'objdump','-d',str(out/(name+'.elf'))],text=True))
 if name=='model':
  assert len(raw)<=256 and raw[0xaa:0xb2]==bytes(8)
  words=struct.unpack('>128H',raw.ljust(256,b'\xff'))
  text='// license:BSD-3-Clause\n// Independently authored MC68EZ328 bootstrap service model, NOT Motorola mask ROM.\n// Generated from mc68ez328_bootstrap.s; regenerate with scripts/as3k_bootstrap_build.py.\nstatic constexpr u16 ez328_bootstrap_model[128] = {\n'
  text+=''.join('\t'+', '.join(f'0x{x:04x}' for x in words[i:i+8])+',\n' for i in range(0,128,8))+'};\n'
  header=root/'src/devices/machine/mc68ez328_bootstrap.h'
  if a.regenerate_model_header:header.write_text(text)
  else:assert header.read_text()==text,'Model header differs from assembled source'
 else:
  assert len(raw)<0x1000,'Payload must remain below 0x31000'
  records=''.join(f'{0x30000+i:08X}{len(raw[i:i+16]):02X}{raw[i:i+16].hex().upper()}\r\n' for i in range(0,len(raw),16))
  records+='0003000000';(out/'payload0.b').write_bytes(records.encode('ascii'))
(out/'assembly.json').write_text(json.dumps(meta,indent=2));print(json.dumps(meta))
