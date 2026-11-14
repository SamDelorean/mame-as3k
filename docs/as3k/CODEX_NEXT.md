# CODEX NEXT — SINGLE INTEGRAL AS3K-DIAG R10 USB TASK (2026-11-02)
CURRENT HANDOFF: R10 OPEN/FAIL, NOT AN ACCREDITED USB IMPLEMENTATION.
Supersedes old R3 handoff, which was archived privately without loss.

## Binding instructions
Work in ONE canonical tree only: /home/spc/Projects/alphasmart/mame-as3k-dev;
branch as3k-mame0289-dev; MAME 0.289 minimal build; binary ./alphasma3k.
Never create clone, branch, worktree, standalone peripheral, fake emulator,
ROM-version-specific emulator, replacement firmware or side project.
Do not run Codex workers/timers or fragment this job into minor milestones.
Do not commit/push without NEW owner approval. No git reset, stash, clean,
pull, merge, checkout or rebase. Preserve the dirty worktree exactly outside
explicitly scoped USB edits. Protect private ROM/dumps/original source/Ghidra
against publication or uploads. No firmware patch or direct CPU-RAM injection.

BEFORE editing read:
- AGENTS.md and /home/spc/Projects/alphasmart/AGENTS.md
- docs/as3k/STATUS.md, docs/as3k/R10_PDIUSBD11_I2C_HW_20261101.md
- docs/as3k/CODEX_RESULT.md and this CURRENT task
- git status --short --branch, git diff, git diff --check
HEAD at handoff 3ba23b4713f9a1eceb7135bdf9622bdadb43b599,
ahead 12 / behind 2. Dirty before this handoff:
AGENTS.md, STATUS.md, R10_PDIUSBD11_I2C_HW_20261101.md,
src/mame/skeleton/alphasma3k.cpp; untracked R1 static report and
src/mame/skeleton/as3k_usbip.inc. Both have prior valuable work.

## Original reference material ALREADY on T160 — do not re-invent
Manufacturer source (read in place; not for Git or public ZIP):
/home/spc/GoogleDrive/ALPHASMART/Soruce code/Software/ModuleSources/
Important: the historical spelling is Soruce code.
Read SystemModule.c/.h, InterruptModule.c/.h, UsbModule.c,
UsbChip.c/.h, UsbAsmModule.s, UsbIsr.c, UsbMain.c,
UsbRequestResponse.c/.h, UsbToAS.c, UsbGet.h, Usb100.h,
UpdaterCommands.h, KeyboardModule.c, AS3000InitCode.s,
SystemAssembly.s and supporting headers as necessary.
Static source may not match active ROM build: verify its relevant
functions with Ghidra/actual ROM, do not blindly assume a source label.

Ghidra: /home/spc/GhidraProjects/AS3KReference/AS3K_Firmware.gpr;
headless /home/spc/.local/bin/ghidra-headless. Reuse project; do not
repeat complete bulk disassembly/import. gdb-multiarch permitted to
disassemble selected ROM addresses using local PRIVATE temporary ELF.

Private original ROM and evidence paths:
/home/spc/Projects/alphasmart/private/roms/as3k/os3k_minimal/
/home/spc/Projects/alphasmart/private/roms/as3k/mame_sets/
BOOT/WRITER ROM sets: asma3ksb, asma3ksw, no ROM modification.
MAME diagnostics asma3kdi, asma3kdv are ROM variants in SAME emulator.
Do not run original asma3k machine outside AGENTS.md authorizations.

Existing software:
src/mame/skeleton/alphasma3k.cpp
src/mame/skeleton/as3k_usbip.inc (uncommitted experimental USB/IP)
src/devices/machine/mc68328.cpp/.h
USB controller: Philips PDIUSBD11D is a USB DEVICE, not a USB host.
Use datasheet and exact original EP0 and interrupt semantics.

## Precisely confirmed before Codex
- OS3K original ROM boots, AlphaWord runs, LCD dual KS0066, matrix,
  RAM/NVRAM, Flash B3 and original applet persist; UART RS232 PTY test PASS.
- D11 PE1/PE2 I2C via original MC68EZ328 bitbang, endpoint indices 2–9,
  F0 buffer, split pointer, FULL=20h, STALL=08h, PF2 active-low reset,
  PF1 INT_N no-host handling. BOOT/WRITER each R10_D11_SPEC_PASS=1
  when physical PF2 RESET_N is **released before I2C script**.
  Do not run older pre-reset-hold script unchanged: it returns FF
  due to missing power/reset setup. Use private derivative
  r10_d11_spec_reset_released_20261101.lua for valid check.
- New experimental opt-in USB/IP listener binds loopback
  AS3K_USBIP_PORT=3240. An independent client runs USB/IP
  DEVLIST + IMPORT PASS using original source VID 081E, PID BD04,
  but this is ONLY USB/IP metadata, NOT USB enumeration.
- Independent host then sends genuine USB control request
  GET_DESCRIPTOR setup bytes 80 06 00 01 00 00 12 00,
  needing 18 bytes from the genuine firmware. It TIMEOUTs / EOFs.
  NO original D11 SetMode(F3), firmware EP0 reply, USB soft-connect,
  bus reset, original USB_Isr execution or genuine enumeration seen.
  Firmware remains in keyboard-scan PC 004CE922/004CE9E2/
  004D0FDE. GPIO when host imported:
  PB=00, PBDIR=78, PBSEL=FA, PE=00, PEDIR=67, PF=01, PFSEL=F5.
  IRQ5 pending and service upper word 0010; IMR upper word 00FF.
  ICR is 0080 when read **16-bit** (prior byte-only read showed
  misleading 00). A **diagnostic-only** forcing IMR 00FF->00EF
  still did NOT start firmware USB. DO NOT force-mask as a fix.
- Original C states that SystemCheckHostConnection inspects SDA,
  System_CallConnectionHandler uses PB3/PB6 cable signals,
  System_HostInterruptHandler sets connection, and
  USBHandleConnection initiates original D11 setup.
  Determine ACTUAL ROM call graph/control path via Ghidra.
- USB/IP transport code has NOT been kernel-VHCI-attached nor
  proven correct for real control transfers. It must be audited,
  not presumed done. Fake USB/IP VID/PID metadata cannot replace
  firmware-generated descriptor, no invented USB descriptor/echo.
- Last executable compiled/link PASS, -validate exit0,
  four ROM audits 4/4 OK, but R10 END TO END FAIL.

## ONE continuous engineering assignment (not a plan-only exercise)
1. STATIC FIRST: map original USB entry and its real ROM call paths
   and IRQ5 enable/dispatch, original boot and USB service routines.
   Explain/verify precisely why ROM stays in keyboard scanning,
   PE outputs low and IRQ5 IMR upper 00FF. Check firmware revision,
   System3 ISR vectors, hardware pin polarities, MAME irq logic.
   No blind patches and no redoing closed features.
2. Complete *physical hardware emulation* of cable VBUS/PB3/PB6,
   SDA/SCL external pullups PE1/PE2, PF1 cable IRQ versus D11 INT_N
   multiplexing, PF2 RESET_N, EZ328 interrupt and direction/mux,
   PDIUSBD11 command/data I2C, SETUP, IRQ F4/status, EP0 handshake,
   FIFO, address/configure, NAK/STALL, bus reset, power changes.
   Reuse MAME features and fix the uncommitted in-driver USB/IP
   adapter (packet lengths, endianness, command parsing,
   control stages, ZLP, short packet, disconnect, host timing).
   Keep one driver and one executable; no synthetic firmware response.
3. Independent host must receive 18-byte original Device Descriptor
   from ROM EP0 in response to 80 06 00 01 00 00 12 00; verify
   original ROM's actual VID/PID; complete standard GET_DESCRIPTOR
   configuration/string, SET_ADDRESS, SET_CONFIGURATION and control
   status stages. The original firmware must generate the bytes.
   DEVLIST/IMPORT alone explicitly DO NOT COUNT.
4. Complete at least one nontrivial original Get Utility /
   AlphaSmart Manager compatible transfer in BOTH directions, with
   precise byte-by-byte evidence and source-backed protocol
   (UsbToAS.c, UsbMain.c, UsbRequestResponse.c etc).
   Do not claim support for printer/unsupported hardware pathways.
5. Build same reduced MAME ./alphasma3k, -validate, audit 4 ROMs;
   recheck D11 BOOT/WRITER I2C, original UART, keyboard, SRAM and
   Intel Flash relevant regressions. Real CPU + original firmware
   and independent-host evidence REQUIRED, including return codes,
   MMIO/IRQ trace, original response bytes and actual transfer.

Build: TMPDIR=/dev/shm make SUBTARGET=alphasma3k
 SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1
 PRECOMPILE=0 -j3
(As one shell command). Do not delete /tmp files or user data to
resolve quota. Validate with ./alphasma3k -validate.
Audit four ROMs with -verifyroms and correct private -rompath.

## Acceptance is BINARY
R10 PASS only when the original unmodified firmware enters USB handler,
responds with its 18-byte genuine device descriptor to real host
control transfer, completes enumeration and a meaningful genuine
bidirectional original file/GET transaction, and targeted regressions
all PASS. No partial fake milestones or cosmetically renamed tests.
If it cannot finish, label R10 OPEN/FAIL and provide exact
non-speculative root cause, proof and remaining blocked step.
Do not say PASS because of linking, FIFO, DEVLIST, IMPORT or injected
data. No repeated planning or asking user for already known details.

Append factual R10 results to docs/as3k/CODEX_RESULT.md
(preserve existing entries); update STATUS.md and R10 technical doc
and keep private logs. End with exact observed bytes, pass/fail matrix,
files changed, git diff --check, and git status --short --branch.
NO commit/push or release without additional specific permission.
