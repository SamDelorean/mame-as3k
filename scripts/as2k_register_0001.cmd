// license:BSD-3-Clause
// Opt-in debugger capture. Run in a PRIVATE output directory: trace contains firmware.
// No input injection: reads during this run belong to boot/idle, not user actions.
trace context-private.trace,maincpu,,{tracelog "pc=%04X a=%02X b=%02X x=%04X y=%04X sp=%04X ccr=%02X init=%02X ",pc,a,b,ix,iy,sp,ccr,init}
wp 1,1,r,1,{logerror "AS2KREG01 read pc=%04X value=%02X init=%02X sp=%04X a=%02X b=%02X\n",pc,wpdata,init,sp,a,b;g}
wp 23,1,w,1,{logerror "AS2KREG01 tflg1 pc=%04X value=%02X init=%02X\n",pc,wpdata,init;g}
g
