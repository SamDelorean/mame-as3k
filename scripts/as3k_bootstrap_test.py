#!/usr/bin/env python3
"""Independent host -> real MAME PTY -> EZ328 UART -> 68000 bootstrap.

No RAM writes or PC/register assignments from the test host/Lua/debugger.
Lua only observes memory/state and switches the physical reset strap.
All NVRAM/Flash fixtures are disposable copies in a fresh output directory.
"""
import argparse,hashlib,json,os,re,select,shutil,struct,subprocess,time,tty
from pathlib import Path

def main():
 p=argparse.ArgumentParser();p.add_argument('--output-directory',required=True);p.add_argument('--rompath',required=True);p.add_argument('--system',default='asma3ksw');p.add_argument('--payload',required=True);p.add_argument('--cycle-fixture');a=p.parse_args()
 root=Path(__file__).resolve().parents[1];out=Path(a.output_directory).resolve();out.mkdir(parents=True,exist_ok=False)
 cfg=out/'cfg';cfg.mkdir();(cfg/(a.system+'.cfg')).write_text(f'''<?xml version="1.0"?><mameconfig version="10"><system name="{a.system}"><input><port tag=":BOOT" type="CONFIG" mask="1" defvalue="1" value="{1 if a.cycle_fixture else 0}"/></input></system></mameconfig>''')
 nv=out/'nvram'/a.system
 if a.cycle_fixture:
  assert a.system=='asma3ksw','Cycle fixture is the accredited WRITER SRAM regression'
  shutil.copytree(Path(a.cycle_fixture),out/'nvram')
 else:
  nv.mkdir(parents=True);(nv/'nvram').write_bytes(bytes(0x40000))
 payload=Path(a.payload).read_bytes();assert 0<len(payload)<0x1000
 lua=r'''local m=manager.machine
local c=m.devices[":maincpu"]
local s=c.spaces["program"]
local done=false
-- Autoboot runs again after soft reset; retire old observers explicitly.
if bootstrap_tx_tap then bootstrap_tx_tap:remove() end
if bootstrap_rx_tap then bootstrap_rx_tap:remove() end
if payload_lcd_tap then payload_lcd_tap:remove() end
local cycle=__CYCLE__
local function exists(n) local f=io.open("__OUT__/"..n,"r");if f then f:close();return true end return false end
local function mark(n,v) local f=assert(io.open("__OUT__/"..n,"w"));f:write(v or "ok");f:close() end
payload_lcd_tap=s:install_write_tap(0xfffff410,0xfffff413,"payload_lcd_gpio",function(offset,data,mask)
 local pc=c.state["PC"].value
 if pc>=0x30000 and pc<0x31000 then
  local addr=offset;local value=(data>>8)&0xff
  if (mask&0xff)~=0 then addr=offset+1;value=data&0xff end
  print(string.format("PAYLOAD_LCD_MMIO us=%.3f PC=%08X addr=%08X value=%02X",m.time.seconds*1e6+m.time.attoseconds/1e12,pc,addr,value))
 end
end)
bootstrap_tx_tap=s:install_write_tap(0xfffff906,0xfffff907,"bootstrap_uart_tx",function(offset,data,mask)
 if (mask & 0xff)~=0 then print(string.format("UART_NATIVE_TX us=%.0f byte=%02X mask=%04X",m.time.seconds*1e6+m.time.attoseconds/1e12,data & 0xff,mask)) end
end)
bootstrap_rx_tap=s:install_read_tap(0xfffff904,0xfffff905,"bootstrap_uart_rx",function(offset,data,mask)
 if (mask & 0xff)~=0 and (mask==0xff or (data & 0x2000)~=0) then print(string.format("UART_NATIVE_RX us=%.0f byte=%02X mask=%04X",m.time.seconds*1e6+m.time.attoseconds/1e12,data & 0xff,mask)) end
end)
emu.register_periodic(function()
 if m.time.seconds>1 and not done then
  done=true
  m.screens[":screen"]:snapshot("__OUT__/before_payload.png")
  print(string.format("BOOTSTRAP_OBSERVE PC=%08X SP=%08X UST=%04X UBAUD=%04X",c.state["PC"].value,(c.state["A7"] or c.state["SP"]).value,s:read_u16(0xfffff900),s:read_u16(0xfffff902)))
 end
end)
local function capture_payload()
 local f=assert(io.open("__OUT__/payload_observed.bin","wb"))
 for x=0,__SIZE__-1 do f:write(string.char(s:read_u8(0x30000+x))) end
 f:close()
 local f=assert(io.open("__OUT__/scratch_observed.bin","wb"))
 for x=0,254 do f:write(string.char(s:read_u8(0x32000+x))) end
 f:close()
 m.screens[":screen"]:snapshot("__OUT__/payload0_lcd.png")
 print(string.format("BOOTSTRAP_END PC=%08X SP=%08X D7=%08X",c.state["PC"].value,(c.state["A7"] or c.state["SP"]).value,c.state["D7"].value))
 mark("payload_captured")
end
emu.register_periodic(function()
 local t=m.time.seconds
 if not cycle then
  if t>=32 and not exists("payload_captured") then capture_payload() end
  return
 end
 if t>=4 and not exists("normal_before") then
  m.screens[":screen"]:snapshot("__OUT__/normal_before.png");mark("normal_before")
  print(string.format("RESET_CYCLE_NORMAL_BEFORE PC=%08X",c.state["PC"].value))
 end
 if t>=5 and not exists("strap_low") then
  for _,f in pairs(m.ioport.ports[":BOOT"].fields) do f.user_value=0 end;mark("strap_low")
 end
 if t>=6 and not exists("bootstrap_reset") then mark("bootstrap_reset");m:soft_reset() end
 if t>=7 and not exists("bootstrap_ready") then
  print(string.format("RESET_CYCLE_BOOTSTRAP PC=%08X SP=%08X",c.state["PC"].value,(c.state["A7"] or c.state["SP"]).value));mark("bootstrap_ready")
 end
 if exists("host_exchange_done") and not exists("payload_captured") then capture_payload() end
 if exists("host_restoration_done") and not exists("restoration_observed") then
  local f=assert(io.open("__OUT__/restoration_observed.bin","wb"))
  for _,r in ipairs({{0x30000,__SIZE__},{0x32000,255},{0x32100,10},{0x3eff4,12}}) do
   for i=0,r[2]-1 do f:write(string.char(s:read_u8(r[1]+i))) end
  end
  f:close();mark("restoration_observed")
  for _,f in pairs(m.ioport.ports[":BOOT"].fields) do f.user_value=1 end
  mark("normal_reset_due",tostring(t+1))
 end
 if exists("normal_reset_due") then
  local f=assert(io.open("__OUT__/normal_reset_due","r"));local due=tonumber(f:read("*a"));f:close()
  if t>=due and not exists("normal_reset") then mark("normal_reset");m:soft_reset() end
  if t>=due+5 and not exists("normal_after") then
   m.screens[":screen"]:snapshot("__OUT__/normal_after.png")
   print(string.format("RESET_CYCLE_NORMAL_AFTER PC=%08X VECTOR_SP=%08X VECTOR_PC=%08X",c.state["PC"].value,s:read_u32(0),s:read_u32(4)))
   mark("normal_after")
  end
 end
end)
'''.replace('__OUT__',str(out)).replace('__SIZE__',str(len(payload))).replace('__CYCLE__','true' if a.cycle_fixture else 'false')
 (out/'observe.lua').write_text(lua)
 debug='''bpset 0xffffff00,1,{logerror "BOOTSTRAP_ENTRY PC=%08X SP=%08X\\n",pc,sp;g}
bpset 0x00030000,1,{logerror "PAYLOAD0_ENTRY PC=%08X SP=%08X\\n",pc,sp;g}
bpset 0xffffffaa,1,{logerror "IBUFF_ENTRY PC=%08X\\n",pc;g}
g
'''
 (out/'observe.cmd').write_text(debug)
 cmd=[str(root/'alphasma3k'),a.system,'-rompath',a.rompath,'-rs232','pty','-cfg_directory',str(cfg),'-nvram_directory',str(out/'nvram'),'-snapshot_directory',str(out/'snap'),'-video','none','-sound','none','-skip_gameinfo','-throttle','-seconds_to_run','35','-autoboot_delay','0','-autoboot_script',str(out/'observe.lua'),'-debug','-debugger','qt','-debugscript',str(out/'observe.cmd'),'-log','-oslog']
 (out/'command.json').write_text(json.dumps(cmd,indent=2))
 env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',QT_QPA_PLATFORM='offscreen')
 for k in ['AS3K_USBIP_PORT','AS3K_IR_PORT','AS3K_KEYBOARD_HOST','AS3K_ADB_TRACE']:env.pop(k,None)
 meta={'binary_sha256':hashlib.sha256((root/'alphasma3k').read_bytes()).hexdigest(),'payload_sha256':hashlib.sha256(payload).hexdigest(),'payload_bytes':len(payload),'tx_hex':'','rx_hex':'','status':'FAIL'}
 console=(out/'console.log').open('w');proc=subprocess.Popen(cmd,cwd=out,stdout=console,stderr=subprocess.STDOUT,env=env);fd=None;tx=bytearray();rx=bytearray();events=[];beg=time.monotonic()
 def readn(n,timeout=4):
  got=bytearray();deadline=time.monotonic()+timeout
  while len(got)<n:
   if proc.poll() is not None:raise RuntimeError('Emulator exited before UART reply')
   left=deadline-time.monotonic()
   if left<=0:raise TimeoutError(f'UART received {got.hex()}, expected {n} bytes')
   if select.select([fd],[],[],min(left,.1))[0]:
    b=os.read(fd,n-len(got));got.extend(b);rx.extend(b)
  return bytes(got)
 def send(b,expected=None):
  os.write(fd,b);tx.extend(b);got=readn(len(b) if expected is None else len(expected));want=b if expected is None else expected
  events.append({'elapsed':time.monotonic()-beg,'tx':b.hex(),'rx':got.hex()});assert got==want,(b,got,want)
 def record(addr,data=b''):
  assert len(data)<=255
  b=f'{addr:08X}{len(data):02X}{data.hex().upper()}'.encode('ascii')
  # BBUG sends an execution record's ten characters without a trailing CR.
  for ch in b:send(bytes([ch]))
  if data:send(b'\r')
 try:
  deadline=time.monotonic()+8
  while fd is None and time.monotonic()<deadline and proc.poll() is None:
   for desc in Path(f'/proc/{proc.pid}/fdinfo').glob('*'):
    try:s=desc.read_text()
    except OSError:continue
    if 'tty-index:' in s:
     name='/dev/pts/'+s.split('tty-index:')[1].strip().split()[0]
     fd=os.open(name,os.O_RDWR|os.O_NONBLOCK|os.O_NOCTTY);tty.setraw(fd);meta['pty']=name;break
   if fd is None:select.select([],[],[],.03)
  assert fd is not None,'No native MAME PTY'
  # Wait for configured reset/bootstrap UART initialization, not host injection.
  if a.cycle_fixture:
   deadline=time.monotonic()+12
   while not (out/'bootstrap_ready').exists():
    assert proc.poll() is None,'Emulator stopped before bootstrap reset'
    assert time.monotonic()<deadline,'Bootstrap reset did not complete'
    select.select([],[],[],.03)
  else:select.select([],[],[],1.3)
  send(b'B',b'@')
  send(b'\r')
  # Maximum eight-bit COUNT, streamed data; verify via observational Lua.
  scratch=bytes((i*37+11)&255 for i in range(255));record(0x32000,scratch)
  # Invalid/lowercase chars and CR cancel a partial record, never execute.
  send(b'0003zz\r');send(b'0003\r')
  # Executed IBUFF instruction writes D7 (unused by documented loader).
  record(0xffffffaa,bytes.fromhex('7E554E714E714E71'));record(0xffffffaa)
  send(b'\r')
  for i in range(0,len(payload),16):record(0x30000+i,payload[i:i+16])
  record(0x30000)
  ident=readn(len(b'AS3K:P0:v1:BOOT OK\r\n'));assert ident==b'AS3K:P0:v1:BOOT OK\r\n',ident
  checksum=readn(4);assert checksum==struct.pack('>I',sum(payload)),checksum.hex()
  challenge=bytes.fromhex('00a5ff5a')
  for ch in challenge:send(bytes([ch]))
  status=readn(4);assert status==b'OK\r\n',status
  # Successful reentry is a real JMP FFFFFF44 made by downloaded 68000 code.
  send(b'\r');record(0x32100,b'REENTER_OK');meta.update({'identity_hex':ident.hex(),'checksum_hex':checksum.hex(),'challenge_hex':challenge.hex(),'status_reply_hex':status.hex(),'native_exchange':'PASS'})
  if a.cycle_fixture:
   (out/'host_exchange_done').write_text('UART exchange passed')
   deadline=time.monotonic()+4
   while not (out/'payload_captured').exists():
    assert time.monotonic()<deadline,'Payload observation not completed'
    select.select([],[],[],.02)
   original=(Path(a.cycle_fixture)/a.system/'nvram').read_bytes()
   original_cpu=bytearray(original);original_cpu[0::2]=original[1::2];original_cpu[1::2]=original[0::2]
   restored=bytearray()
   for addr,size in [(0x30000,len(payload)),(0x32000,255),(0x32100,10),(0x3eff4,12)]:
    data=bytes(original_cpu[addr:addr+size]);restored.extend(data)
    for i in range(0,len(data),16):record(addr+i,data[i:i+16])
   (out/'host_restoration_done').write_text('B-record restoration complete')
  proc.wait(timeout=42)
  assert proc.returncode==0,proc.returncode
  console.flush();log=(out/'error.log').read_text(errors='replace')
  assert all(s in log for s in ['BOOTSTRAP_ENTRY fetch address=FFFFFF00','SP=FFFCFFFC','PAYLOAD0_ENTRY','IBUFF_ENTRY']),log[-2000:]
  assert (out/'payload_observed.bin').read_bytes()==payload,'RAM payload differs from UART source'
  assert (out/'scratch_observed.bin').read_bytes()==scratch,'COUNT FF data differs'
  raw=(nv/'nvram').read_bytes()
  if not a.cycle_fixture:assert raw[:256]==bytes(256),'Bootstrap overwrote first SRAM page'
  cpu=bytearray(raw);cpu[0::2]=raw[1::2];cpu[1::2]=raw[0::2]
  if not a.cycle_fixture:assert cpu[0x32100:0x3210a]==b'REENTER_OK','Reentry did not write the next B-record'
  allowed=set(range(0x30000,0x30000+len(payload)))|set(range(0x32000,0x320ff))|set(range(0x32100,0x3210a))|set(range(0x3eff4,0x3f000))
  if not a.cycle_fixture:assert all(v==0 for i,v in enumerate(cpu) if i not in allowed),'Unexpected SRAM write'
  else:
   assert (out/'restoration_observed.bin').read_bytes()==restored,'Native B-record SRAM restoration differs'
   assert b'r8proof' in cpu,'Original document lost'
   assert (out/'normal_after').exists(),'Normal reset did not finish'
   text=(out/'console.log').read_text()
   assert 'RESET_CYCLE_NORMAL_AFTER PC=004' in text,'Original firmware did not resume'
   modes=re.findall(r'EZ328 RESET EMUBRK=(\d) bootstrap=(\d)',log)
   assert modes==[('1','0'),('0','1'),('1','0')],modes
   meta.update({'reset_modes':modes,'normal_reset':'PASS','original_document_preserved':True,'native_sram_restoration':'PASS'})
  roms=list((Path(a.rompath.split(';')[0])/a.system).glob('os3k_min_*.bin'))
  assert len(roms)==1,'Cannot identify the selected original ROM for Flash comparison'
  assert (nv/'flash').read_bytes()==roms[0].read_bytes(),'Flash differs from selected original ROM'
  meta.update({'unexpected_sram_changes':0 if not a.cycle_fixture else None,'flash_matches_original':True,'reentry_data':'REENTER_OK'})
  assert 'D7=00000055' in (out/'console.log').read_text(),'IBUFF did not execute or D7 lost'
  # Decode only native CPU writes to Port C, independently of KS0066 DDRAM.
  text=(out/'console.log').read_text();rows=re.findall(r'PAYLOAD_LCD_MMIO us=([0-9.]+) PC=([0-9A-F]+) addr=([0-9A-F]+) value=([0-9A-F]+)',text)
  assert any(int(addr,16)==0xfffff413 and int(v,16)==0xff for _,_,addr,v in rows),'No native PCSEL configuration'
  assert any(int(addr,16)==0xfffff410 and int(v,16)==0xff for _,_,addr,v in rows),'No native PCDIR configuration'
  previous=0;strobes={6:[],7:[]}
  for us,pc,addr,v in rows:
   if int(addr,16)!=0xfffff411:continue
   value=int(v,16)
   for bit in [6,7]:
    if previous & (1<<bit) and not value & (1<<bit):strobes[bit].append({'us':float(us),'rs':bool(previous&0x20),'nibble':previous&15,'pc':pc})
   previous=value
  decoded={}
  for bit in [6,7]:
   cells=strobes[bit];assert [x['nibble'] for x in cells[:4]]==[3,3,3,2],(bit,cells[:4])
   tail=cells[4:];assert len(tail)%2==0;bytes_lcd=[]
   for i in range(0,len(tail),2):
    assert tail[i]['rs']==tail[i+1]['rs'];bytes_lcd.append({'rs':tail[i]['rs'],'value':tail[i]['nibble']*16+tail[i+1]['nibble']})
   decoded[str(bit)]={'falling_strobes':len(cells),'commands':[x['value'] for x in bytes_lcd if not x['rs']],'text':bytes(x['value'] for x in bytes_lcd if x['rs']).decode('ascii')}
  assert decoded['6']['text']=='BOOT OK' and decoded['7']['text']=='',decoded
  assert decoded['6']['commands']==[0x28,0x08,0x01,0x06,0x0c,0x80],decoded
  assert decoded['7']['commands']==[0x28,0x08,0x01,0x06,0x0c],decoded
  assert (out/'payload0_lcd.png').is_file(),'No native framebuffer capture'
  lcd={'status':'NATIVE_MMIO_PASS_VISUAL_REVIEW_REQUIRED','controllers':decoded,'mmio_writes':len(rows),'snapshot_sha256':hashlib.sha256((out/'payload0_lcd.png').read_bytes()).hexdigest()}
  (out/'lcd_trace.json').write_text(json.dumps({'summary':lcd,'mmio':rows,'strobes':strobes},indent=2));meta['lcd']=lcd
  meta.update({'status':'PASS','mame_exit':proc.returncode,'sram_first_256_unchanged':True if not a.cycle_fixture else None,'count_ff_bytes':255,'payload_observed_sha256':hashlib.sha256((out/'payload_observed.bin').read_bytes()).hexdigest(),'ibuff_d7':'00000055'})
 except Exception as e:meta['error']=type(e).__name__+': '+str(e)
 finally:
  if fd is not None:os.close(fd)
  if proc.poll() is None:proc.terminate();proc.wait(timeout=8)
  console.close();meta['mame_exit']=proc.returncode;meta['tx_hex']=tx.hex();meta['rx_hex']=rx.hex();meta['elapsed']=time.monotonic()-beg
  (out/'uart_events.json').write_text(json.dumps(events,indent=2));(out/'summary.json').write_text(json.dumps(meta,indent=2));(out/'host_tx.bin').write_bytes(tx);(out/'host_rx.bin').write_bytes(rx)
 print(json.dumps(meta));return 0 if meta['status']=='PASS' else 1
if __name__=='__main__':raise SystemExit(main())
