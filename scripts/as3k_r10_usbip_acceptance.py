# SPDX-License-Identifier: BSD-3-Clause
import socket,struct,time,json,sys
from pathlib import Path
import argparse
parser=argparse.ArgumentParser(description="AS3K original-ROM USB/IP R10 acceptance test")
parser.add_argument('--port',type=int,default=3240)
parser.add_argument('--output-directory',type=Path,default=Path('work/as3k_r10_usbip_evidence'))
args=parser.parse_args()
W=args.output_directory.resolve()
W.mkdir(parents=True,exist_ok=True)
records=[]
def rec(**k):
 records.append(k);print(json.dumps(k),flush=True);(W/'transactions.json').write_text(json.dumps(records,indent=2))
def rx(s,n):
 b=b''
 while len(b)<n:
  x=s.recv(n-len(b))
  if not x:raise RuntimeError('EOF %d/%d'%(len(b),n))
  b+=x
 return b
end=time.monotonic()+15
while True:
 try:s=socket.create_connection(('127.0.0.1',args.port),timeout=3);break
 except OSError:
  if time.monotonic()>end:raise
  time.sleep(.1)
s.settimeout(15)
s.sendall(struct.pack('!HHI',0x111,0x8003,0)+b'1-1'.ljust(32,b'\0'))
head=rx(s,8);info=rx(s,312);assert head==struct.pack('!HHI',0x111,3,0)
rec(event='IMPORT',header=head.hex())
seq=0
def urb(ep,direction,length,payload=b'',setup=bytes(8)):
 global seq
 seq+=1
 header=struct.pack('!IIIII',1,seq,0x10001,direction,ep)+struct.pack('!IIIII',0,length,0,0,0)+setup
 s.sendall(header+(payload if direction==0 else b''))
 r=rx(s,48);cmd,rseq=struct.unpack('!II',r[:8]);status=struct.unpack('!i',r[20:24])[0];actual=struct.unpack('!I',r[24:28])[0]
 data=rx(s,actual) if direction and actual else b''
 rec(seq=seq,ep=ep,direction=direction,setup=setup.hex(),sent=payload.hex(),status=status,actual=actual,received=data.hex())
 assert cmd==3 and rseq==seq and status==0,(cmd,rseq,status)
 if direction==0:assert actual==length,(actual,length)
 return data
def ctrl(bm,req,val=0,index=0,length=0,data=b''):
 return urb(0,1 if bm&128 else 0,length,data,struct.pack('<BBHHH',bm,req,val,index,length))
def enumerate_device(label):
 desc=ctrl(0x80,6,0x100,length=18);assert len(desc)==18 and desc[:2]==bytes([18,1])
 ctrl(0,5,5)
 cfg9=ctrl(0x80,6,0x200,length=9);assert len(cfg9)==9
 length=int.from_bytes(cfg9[2:4],'little');cfg=ctrl(0x80,6,0x200,length=length);assert len(cfg)==length
 langs=ctrl(0x80,6,0x300,length=255);lang=int.from_bytes(langs[2:4],'little')
 for i in sorted(set(desc[14:17])-{0}):ctrl(0x80,6,0x300|i,index=lang,length=255)
 ctrl(0,9,1);assert ctrl(0x80,8,length=1)==b'\x01'
 rec(event='ENUMERATION_COMPLETE',mode=label,descriptor=desc.hex(),configuration=cfg.hex())
 return desc,cfg
normal,cfg=enumerate_device('keyboard')
for led in [0xe0,0xe1,0xe2,0xe3,0xe4]:ctrl(0x21,9,0x200,length=1,data=bytes([led]))
time.sleep(3)
getdesc,getcfg=enumerate_device('get')
# Resolve endpoint addresses from authentic descriptor, not metadata.
eps=[];i=0
while i<len(getcfg):
 n=getcfg[i]
 if not n:raise RuntimeError('zero descriptor length')
 if getcfg[i+1]==5:eps.append(getcfg[i+2])
 i+=n
out=next(e for e in eps if not e&128)&15;inp=next(e for e in eps if e&128)&15
rec(event='GET_ENDPOINTS',out=out,inp=inp)
def put(p):urb(out,0,len(p),p)
def get(n=8):return urb(inp,1,n)
put(b'\x03');current=get();assert current[0]==0x43
put(b'\x01');size=get();assert size[0]==0x41
before=int.from_bytes(size[1:5],'big')
# Work on independent private SRAM, never the owner's original document state.
payload=b'R10 USB original: 2026-11-05 / AlphaSmart 3000\r'
put(b'\x04');assert get()==b'\x44'
put(b'\x06'+len(payload).to_bytes(4,'big'));assert get()==b'\x46'
for i in range(0,len(payload),7):put(b'\x08'+payload[i:i+7])
put(b'\x01');size=get();assert size==b'\x41'+len(payload).to_bytes(4,'big'),size
put(b'\x07');received=b''
while len(received)<len(payload):received+=get(min(8,len(payload)-len(received)))
assert received==payload,(received,payload)
rec(event='BIDIRECTIONAL_FILE_TRANSFER_PASS',sent=payload.hex(),received=received.hex(),original_size=before)

# Exercise simultaneous IN + EP0 + OUT on the actual ROM, not a mock server.
def submit_raw(ep,direction,length,payload=b'',setup=bytes(8)):
 global seq
 seq+=1
 s.sendall(struct.pack('!IIIII',1,seq,0x10001,direction,ep)+struct.pack('!IIIII',0,length,0,0xffffffff,0)+setup+(payload if direction==0 else b''))
 return seq
def response_raw():
 r=rx(s,48);cmd,num=struct.unpack('!II',r[:8]);status=struct.unpack('!i',r[20:24])[0];actual=struct.unpack('!I',r[24:28])[0]
 assert r[8:20]==bytes(12),'server response reserved fields must be zero'
 return r,cmd,num,status,actual
pending=submit_raw(inp,1,8)
control=submit_raw(0,1,1,setup=struct.pack('<BBHHH',0x80,8,0,0,1))
outseq=submit_raw(out,0,1,b'\x18')
answers={}
for _ in range(3):
 r,cmd,num,status,actual=response_raw();data=rx(s,actual) if num in (pending,control) and actual else b''
 rec(event='CONCURRENT_RESPONSE',seq=num,status=status,actual=actual,received=data.hex())
 assert cmd==3 and status==0
 answers[num]=data
assert len(answers[pending])==2 and answers[pending][0]==0x58 and answers[control]==b'\x01'
rec(event='ORIGINAL_GET_PROTOCOL_VERSION',version=answers[pending][1])
rec(event='CONCURRENT_IN_EP0_OUT_PASS')
# Successful UNLINK removes a waiting IN; a late UNLINK reports status zero.
pending=submit_raw(inp,1,8)
for expected in (-104,0):
 seq+=1;cancel=seq
 s.sendall(struct.pack('!IIIII',2,cancel,0x10001,0,0)+struct.pack('!I',pending)+bytes(24))
 r,cmd,num,status,actual=response_raw();assert (cmd,num,status)==(4,cancel,expected)
 rec(event='UNLINK_RESPONSE',seq=num,status=status)
assert ctrl(0x80,8,length=1)==b'\x01'
rec(event='USBIP_UNLINK_AND_NO_LATE_SUBMIT_PASS')

s.close()
