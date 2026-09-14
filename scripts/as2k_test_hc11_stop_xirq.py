#!/usr/bin/env python3
# license:BSD-3-Clause
"""Exercise production HC11 interrupt/STOP functions in an isolated C++ harness.

Run from the repository root with Python 3 and a C++20 g++ compiler installed.
Timer cycles, memory/vector reads and stack writes are stubbed. This checks CPU
control flow, not MAME scheduling, AS2000 firmware or physical wake timing.
"""
from pathlib import Path
import subprocess
import tempfile
s=Path('src/devices/cpu/mc68hc11/mc68hc11.cpp').read_text()
ops=Path('src/devices/cpu/mc68hc11/hc11ops.hxx').read_text()
def function(source, signature):
 start=source.index(signature)
 brace=source.index('{',start)
 depth=1; end=brace+1
 while depth:
  depth += (source[end]=='{')-(source[end]=='}'); end+=1
 return source[start:end]
code=r'''
#include <bit>
#include <cstdint>
#include <cassert>
#include <cstdio>
#define BIT(v,b) (((v)>>(b))&1)
#define LOGMASKED(...) ((void)0)
#define CC_X 0x40
#define CC_I 0x10
#define CC_S 0x80
#define CLEAR_LINE 0
#define MC68HC11_IRQ_LINE 0
#define MC68HC11_XIRQ_LINE 1
#define REG_A 0
#define REG_B 0
#define PUSH16(x) (stack_bytes+=2)
#define PUSH8(x) (++stack_bytes)
#define SET_PC(x) (m_pc=(x))
#define READ16(x) (vector_address=(x),0x9000)
#define CYCLES(x) ((void)0)
#define HC11OP(x) mc68hc11_cpu_device::x
struct mc68hc11_cpu_device {
 uint8_t m_ccr=CC_X|CC_I, m_tmsk1=0,m_tmsk2=0,m_tflg2=0,m_tflg1=0,m_pactl=0,m_option=0;
 uint16_t m_toc[5]={},m_tcnt=0,m_pc=0x87d7,m_ppc=0x87d7,m_ix=0,m_iy=0;
 uint64_t m_frc_base=0,m_reset_time=0;
 uint32_t m_irq_state=0;
 int m_stop_state=0,m_wait_state=0,stack_bytes=0,vector_address=0,level_seen=-1;
 bool m_irq_asserted=false;
 int div_tab[4]={1,4,8,16};
 uint64_t total_cycles() {return 0;}
 void standard_irq_callback(int level, int) {level_seen=level;}
 void logerror(const char*,int,int) {}
 void check_irq_lines(); void set_irq_state(uint8_t,bool); void execute_set_input(int,int); void stop();
 void step_stop() {m_ppc=m_pc; ++m_pc; stop();}
};
'''
for f in ['check_irq_lines()','set_irq_state(uint8_t irqn, bool state)','execute_set_input(int inputnum, int state)']:
 code+=function(s,'void mc68hc11_cpu_device::'+f)+'\n'
code+=function(ops,'void HC11OP(stop)()')+'\n'
code+=r'''
int main() {
 {mc68hc11_cpu_device c; c.step_stop(); c.check_irq_lines();
 assert(c.m_stop_state==1 && c.m_pc==0x87d7 && c.level_seen==-1);
 c.execute_set_input(1,1); c.execute_set_input(1,0); c.check_irq_lines();
 assert(c.m_stop_state==1 && c.m_irq_state==0);
 puts("PASS STOP stays asleep without an asserted source");}
 {mc68hc11_cpu_device c; c.m_ccr|=CC_S; c.step_stop();
 assert(c.m_stop_state==0 && c.m_pc==0x87d8);
 puts("PASS S mask still disables STOP");}
 {mc68hc11_cpu_device c; c.m_wait_state=1; c.execute_set_input(1,1); c.check_irq_lines();
 assert(c.m_wait_state==1 && c.m_stop_state==0 && c.m_irq_state==0x04000000 && c.level_seen==-1);
 puts("PASS masked XIRQ does not wake WAI");}
 {mc68hc11_cpu_device c; c.step_stop(); assert(c.m_stop_state==1 && c.m_pc==0x87d7);
 c.execute_set_input(1,1); c.check_irq_lines();
 assert(c.m_stop_state==2 && c.m_irq_state==0 && c.level_seen==-1 && c.stack_bytes==0);
 c.step_stop(); assert(c.m_pc==0x87d8 && c.m_stop_state==0);
 c.m_ccr=0; c.check_irq_lines(); assert(c.level_seen==-1);
 puts("PASS masked STOP wake, next PC=87D8, no stack/vector/pending after unmask");}
 {mc68hc11_cpu_device c; c.execute_set_input(1,1); c.check_irq_lines();
 assert(c.m_irq_state==0x04000000 && c.level_seen==-1 && c.m_stop_state==0);
 c.step_stop(); c.check_irq_lines(); c.step_stop(); assert(c.m_pc==0x87d8 && c.m_irq_state==0);
 puts("PASS masked outside STOP unchanged; asserted-before-STOP wake");}
 {mc68hc11_cpu_device c; c.m_ccr=CC_I; c.step_stop(); c.execute_set_input(1,1); c.check_irq_lines();
 assert(c.level_seen==5 && c.vector_address==0xfff4 && c.m_pc==0x9000 && c.stack_bytes==9 && c.m_stop_state==2);
 assert(c.m_irq_state==0x04000000); c.execute_set_input(1,0); assert(c.m_irq_state==0);
 puts("PASS unmasked STOP XIRQ vectors FFF4 and stacks 9 bytes despite I mask");}
 {mc68hc11_cpu_device c; c.m_ccr=0; c.execute_set_input(1,1); c.check_irq_lines();
 assert(c.level_seen==5 && c.vector_address==0xfff4 && c.m_stop_state==0);
 puts("PASS unmasked XIRQ outside STOP");}
 {mc68hc11_cpu_device c; c.step_stop(); c.execute_set_input(0,1); c.check_irq_lines();
 assert(c.m_stop_state==1 && c.level_seen==-1 && c.m_irq_state==0x02000000);
 c.m_ccr=CC_X; c.check_irq_lines(); assert(c.level_seen==6 && c.vector_address==0xfff2);
 puts("PASS IRQ masked STOP stays asleep; unmasked IRQ vectors FFF2");}
 {mc68hc11_cpu_device c; for(int i=0;i<148;i++) {
 c.m_pc=0x87d7; c.step_stop(); c.execute_set_input(1,1); c.check_irq_lines(); c.step_stop(); c.execute_set_input(1,0);
 assert(c.m_pc==0x87d8 && c.m_stop_state==0 && c.m_irq_state==0 && c.stack_bytes==0);}
 puts("PASS 148 core STOP/wake cycles (not firmware auto-off evidence)");}
}
'''
with tempfile.TemporaryDirectory(prefix="as2k-hc11-test-") as temp:
    source = Path(temp) / "test.cpp"
    binary = Path(temp) / "test"
    source.write_text(code)
    subprocess.run(['g++', '-std=c++20', '-o', str(binary), str(source)], check=True)
    subprocess.run([str(binary)], check=True)
