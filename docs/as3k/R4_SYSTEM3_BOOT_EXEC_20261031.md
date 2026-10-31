# R4 — Original AS3000 System 3 boot boundary — 2026-10-31

STATUS: CLOSED / CPU_EXEC_PASS / BUILD_PASS / ROM_AUDIT_PASS.
SCOPE: real System 3 CPU entry and internal initialization.
Not full GUI BOOT_PASS or AlphaWord runtime function.

Canonical tree: /home/spc/Projects/alphasmart/mame-as3k-dev
Single MAME 0.289 reduced target: ./alphasma3k. No new branches,
alternate emulator binaries, firmware changes or Codex.

## Hardware fault A — idle keyboard
The Small ROM reads active-low keyboard rows and blocked in its
release/poll loop. At 0x0040063A debugger recorded D1=0xff,
all eight rows asserted. The driver was missing external idle-high
Port D keyboard row levels. Connected in_port_d<0..7> callbacks
to constant high for unpressed physical rows. No real matrix or
pressed-key scanning implemented (reserved for planned later stages).

## Hardware fault B — EZ Timer 1 register reset
After fault A, Small ROM at 0x00400A50 programmed TCTL1=0008
at FFFFF600, TCMP1=0020 at FFFFF604, TCTL1=0009,
then polled TSTAT bit 0 at FFFFF60B from 0x00400A66.
Temporary instrumented MC68EZ328 MAME runs found uninitialized
TPRER=7e99 and TCMP=d6d8. The EZ member m_timer_regs
had no per-device reset unlike the different MC68328 array.
Added EZ-only reset defaults:
tctl=0000, tprer=0000, tcmp=ffff,
tcr=0000, tcn=0000, tstat=0000, tclear=0.
No ROM patch, forced compare or timer-loop bypass. All temporary
logerror diagnostics removed before final rebuild.

## Verified CPU path, twice
Both asma3ksb (BOOT) and asma3ksw (WRITER) independently reached
the real addresses (debugger may display post-fetch +2):
Small ROM 0x0040054C -> dispatch 0x00400A90 ->
version check 0x00400AD2 -> normal handoff 0x00400BD8 ->
trampoline 0x004FFF98 -> bridge 0x004C007A ->
System 3 0x004C7390 -> init block 0x004C73CA ->
System3 own subroutine 0x004CD010.
The last breakpoint exits the debugger; no further UI execution
or Writer A000 applet launch was claimed.

Evidence logs and script in existing private path:
../private/roms/as3k/mame_sets/r4_boot_posttimer.log
../private/roms/as3k/mame_sets/r4_writer_posttimer.log
../private/roms/as3k/mame_sets/r4_system_internal_boot.log
../private/roms/as3k/mame_sets/r4_system_internal_writer.log
Plus intermediate timer and keyboard diagnostic logs there.

## Build and regression
Existing reduced MAME make returned BUILD_EXIT=0 and linked the
SAME executable alphasma3k.
-validate return 0.
Four ROM audits: 4 romsets found, 4 OK, return 0.
The existing KS0066 F05 NEEDS REDUMP warning remains.
Two unrelated startup unmapped writes at FFFFFD0C and FFFFFD0E
were observed but did not prevent System3 entry; untouched.
No proprietary data, build objects or binary included in Git.

## Next boundary
R5: verify System 3 LCD path using previously working GPIO bridge.
No new ROM selection architecture, keyboard Phase 2, storage
persistence or extra device work was started in R4.
