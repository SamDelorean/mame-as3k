#!/usr/bin/env python3
"""Conservative UART sender for MC68EZ328 B-record streams (standard library only).

Requires user to put device into bootstrap via EMUBRK/PG5 low during reset.
Transmission is opt-in, checks bootloader character-by-character echo, and
stops on any mismatch. Not executed on real AS3K by the author.
"""
import argparse
import os
import select
import termios
import time
from pathlib import Path

def setup_serial(fd):
    old=termios.tcgetattr(fd)
    attrs=termios.tcgetattr(fd)
    attrs[0]=0  # no input transformations or flow control
    attrs[1]=0  # raw output
    attrs[2] &= ~(termios.CSIZE|termios.PARENB|termios.CSTOPB)
    attrs[2] |= (termios.CS8|termios.CLOCAL|termios.CREAD)
    if hasattr(termios,'CRTSCTS'): attrs[2] &= ~termios.CRTSCTS
    attrs[3]=0
    attrs[4]=termios.B9600
    attrs[5]=termios.B9600
    attrs[6][termios.VMIN]=0
    attrs[6][termios.VTIME]=0
    termios.tcsetattr(fd,termios.TCSANOW,attrs)
    termios.tcflush(fd,termios.TCIFLUSH)
    return old

def write_all(fd,data):
    while data:
        sent=os.write(fd,data)
        if sent<=0: raise OSError('UART write failed')
        data=data[sent:]

def echo_exact(fd, expected, timeout=2.5):
    end=time.monotonic()+timeout
    got=bytearray()
    while len(got)<len(expected):
        remaining=end-time.monotonic()
        if remaining<=0: raise TimeoutError(f'bootstrap echo timeout {len(got)}/{len(expected)}')
        r,_,_=select.select([fd],[],[],remaining)
        if not r: continue
        chunk=os.read(fd, len(expected)-len(got))
        if not chunk: continue
        got.extend(chunk)
        if bytes(got) != expected[:len(got)]:
            raise ValueError(f'bootstrap echo mismatch got={bytes(got)!r} expected={expected!r}')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('stream',type=Path,help='CR-separated, uppercase B-record file')
    p.add_argument('--port',help='Serial device, e.g. /dev/ttyUSB0 or MAME PTY')
    p.add_argument('--execute',action='store_true',help='Explicitly authorize transmitting to the selected serial port')
    args=p.parse_args()
    data=args.stream.read_bytes()
    lines=[x for x in data.replace(b'\r\n',b'\r').split(b'\r') if x]
    assert len(lines)%2==0
    for k in range(0,len(lines),2):
        assert lines[k].startswith(b'FFFFFFAA08') and len(lines[k])==26
        assert lines[k+1]==b'FFFFFFAA00'
    print(f'{len(lines)//2} internal 8-byte instructions; {len(lines)} B-records; NO external RAM records')
    if not args.execute:
        print('DRY RUN: no UART opened or written. Use --port DEVICE --execute to transmit.')
        return
    if not args.port: p.error('--port is required with --execute')
    fd=os.open(args.port,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
    old=None
    try:
        old=setup_serial(fd)
        for k,line in enumerate(lines,1):
            out=line+b'\r'
            write_all(fd,out)
            echo_exact(fd,out)
            if k%20==0: print(f'B-record {k}/{len(lines)} echo OK')
        print(f'ALL {len(lines)} B-records echoed. Verify BOOT OK visually on hardware LCD.')
    finally:
        if old is not None: termios.tcsetattr(fd,termios.TCSANOW,old)
        os.close(fd)
if __name__=='__main__': main()
