# Codex result — AS3K external keyboard latch

Date: 2026-08-26

Status: **complete; implementation, validation, commit, and push gates passed**.

## Repository gate and evidence

- Pulled `as3k-project/as3k-mame0289-dev` with `--ff-only`; it was already current.
- Confirmed branch `as3k-mame0289-dev`, initially clean tracked status, audit commit `08816104002f27365ace1798eddd06b78550c764` in HEAD, and initial `git diff --check` success.
- Inspected the 16-bit MC68EZ328 program map and local historical `KeyboardModule.c`/listing evidence. The source defines and emits byte writes only at `0x00600000`; it does not prove that the physical latch is mirrored through every address in the established 32 KiB chip-select window.

## Driver change

Only `src/mame/skeleton/alphasma3k.cpp` was changed for implementation:

- added `u8 m_keyboard_column_latch`;
- registered it with `save_item()`;
- reset it deterministically to `0xFF`;
- added a write handler that retains the byte;
- mapped aligned range `0x00600000–0x00600001` with `umask16(0xff00)`, exposing only byte address `0x00600000` on the observed high lane of the 16-bit CPU bus.

The physical power-on latch value is unknown. `0xFF` is explicitly an emulator initialization choice providing safe no-active-low-column output, not a measured hardware reset fact. The mapping is deliberately not mirrored across `0x00600000–0x00607fff` because only exact address `0x00600000` is demonstrably decoded by available source; the 32 KiB board chip-select window remains documented without silently guessing lower address-line decode.

No keyboard matrix, PA/PD callback, input key, PAPUEN, PDSEL, PDKBEN/PKBDINT, Phase 2, LCD, diagnostic ROM definition, or MAME core change was made.

## Build and static validation

- Reduced no-clean build command: `make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2` — passed against final source.
- Driver-list generation reported four drivers; `-listfull` confirmed `asma3k`, `asma3kdi`, `asma3kdv`, and `asma3kdvl`.
- `./alphasma3k -verifyroms asma3kdi` — good.
- `./alphasma3k -verifyroms asma3kdv` — good.
- Original `asma3k` was not executed.

## Bounded dynamic validation

Local-only script: `../diagnostic/as3kdv_keyboard_latch.cmd`, run only with `asma3kdv` under an 8-second alarm.

Observed:

- Phase-1 entry reached at the expected breakpoint (`PC` presentation `0x0042e2aa`).
- Exact latch watchpoint: `ADDR=00600000 DATA=FF PC=0042E2D0`.
- A temporary handler observation immediately reported retained latch value `ff`; that diagnostic log statement was removed before the final build and commit.
- The former `unmapped program memory write to 00600000` message was absent both with the temporary observation and against final source.
- Phase-1 RTS breakpoint was reached (`PC` presentation `0x0042e2f0`) with `D0=00000000`.
- Caller-return breakpoint was reached (`PC` presentation `0x0042016a`) with `D0=00000000`, then the script quit.
- No Phase 2, `InterruptInstallHandler`, keyboard interrupt handler, bus error, address error, illegal-instruction, or divide-by-zero safety break fired.

The final-source rerun repeated the `0xFF` watchpoint, normal RTS/caller return, absence of the old unmapped message, and absence of every failure marker.

## Regression and publication boundary

- `asma3kdi` and `asma3kdv` fixture audits remain good; the valid-AlphaWord path still returns from Phase 1.
- No file under `src/devices/` changed; LCD bridge source is untouched.
- The debugger script and logs remain outside Git. No ROM, proprietary material, fixture, generated binary, or build artifact is included.
- Final `git diff --check`: passed.
- Pre-commit tracked status contains only `src/mame/skeleton/alphasma3k.cpp`, `docs/as3k/STATUS.md`, and `docs/as3k/CODEX_RESULT.md`; post-publication status is clean.


## R3 — MC68EZ328 Port-A PAPUEN / WDTH8 (2026-10-28)

**CLOSED / BUILD_PASS / PHASE1_DYNAMIC_PASS (bounded), no R4 boot test.**
This section supersedes the previous next-stage status, but retains the
historical external-latch proof above for traceability.

Files changed:
- src/devices/machine/mc68328.cpp and .h: EZ-only PAPUEN mapped
  0xFFFFF402, state resets 0xFF, read/write/save; PADATA missing-
  callback GPIO input defaults to PAPUEN bit, original MC68328
  fallback remains zero; external callback/output latch priority preserved;
  SCR.WDTH8 now both sets/clears m_pasel 0xFF/0x00, stores only WDTH8
  bit to m_scr preserving unrelated bits, and EZ m_pasel is saved.
  0xFFFFF403 remains reserved, no fictitious PASEL.
- src/mame/skeleton/alphasma3k.cpp: MAME R2 ROM filenames use
  lowercase labels os3k_min_boot.bin / os3k_min_writer.bin; existing
  proprietary ROM bytes untouched, local-only symlink aliases inside
  the same private ROM directories. This closes a runtime MAME validity
  issue discovered during R3 test; there is still only ONE AS3K-DIAG.

Source: NXP MC68EZ328UM pages for Ports A/SCR, especially PAPUEN,
reset 0xFF, and data-pin mux controlled by WDTH8; the physical
manufacturer guide: https://www.nxp.com/docs/en/reference-manual/MC68EZ328UM.pdf.

Command: TMPDIR=/dev/shm make SUBTARGET=alphasma3k
SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl -j3
=> BUILD_EXIT=0, linked the same ./alphasma3k executable.
./alphasma3k -validate => exit 0.
ROM audit (rompath ../private/roms/as3k/mame_sets;../private/roms):
asma3kdi and asma3kdv GOOD; asma3ksb and asma3ksw BEST_AVAILABLE;
4 romsets found, 4 were OK, exit 0. KS0066 F05 remains
NEEDS REDUMP, a previously known CGROM issue.

Bounded real CPU test against historical AlphaWord fixture asma3kdv,
using built-in Qt debugger in offscreen mode and private script
r3_keyboard_phase1_20261028.cmd:
- SCRIPT_LOADED;
- SCR wrote 0x1D from PC=0x0040001E;
- PAPUEN initial read at Phase 1: D0=0xFF;
- PAPUEN write via Phase 1: 0x80;
- following D0 still 0x80;
- external 0x00600000 latch write: 0xFF;
- Phase 1 RTS at 0x0042E2EE D0=0;
- caller-return at 0x00420168 D0=0 and debugger exited;
- no entered Phase 2 (breakpoint at 0x0042E2F0 did not fire),
  and no unrelated new IRQ/exception or PAPUEN unmapped warning.
- Two already-present unmapped startup accesses to FFFFFD0C
  and FFFFFD0E are unrelated to R3 and not repaired here.

Not independently established:
- debugger memory SCR read showed 0x00 although the CPU watchpoint
  proved the SCR write of 0x1D; readback is not reliable here.
  This ambiguity is explicitly NOT marked PASS.
- WDTH8 clear transition is source-verified, not CPU-exercised.
- No complete System3/Writer BOOT_PASS, no matrix, PDSEL, PDKBEN,
  IRQ, UART, or dynamic ROM selection work.

Source-scoped git diff --check PASS. No temporary logging added to
C++ source; debugger script and logs remain in private. No firmware
or generated executables staged or published. Git commit hash and
push disposition recorded separately in the final status.


## R4 — Real System 3 CPU entry (2026-10-31)
CLOSED / CPU_EXEC_PASS, not GUI/Writer full boot. Same reduced AS3K-DIAG:
fixed idle-high Port D board row callbacks and reset of EZ-owned timer
register state. Both original BOOT and WRITER ROMs reached System3
0x004C7390, continued 0x004C73CA, and invoked 0x004CD010.
Build PASS, -validate PASS, four ROM audits PASS. Temporary timer
tracing removed. See R4_SYSTEM3_BOOT_EXEC_20261031.md. R5 next.


## R5 — Genuine System3 LCD output (2026-10-31)
CLOSED / DISPLAY_PASS; no code changes, no Codex. Both BOOT and
WRITER original firmware images, in the ONE reduced AS3K-DIAG,
issued 8308 writes to real PCDATA 0xFFFFF411, with
1492 E1 strobes, 1270 E2 strobes (RS=1: 402 and 370).
Actual 240x36 screenshots match pixel-for-pixel and display the
original OS warning 'An unexpected data change occurred.'
The CPU awaits real key input at 0x004D0FD8. Key mapping, Writer
launch and memory integrity are NOT claimed PASS. -validate PASS,
4/4 ROM audit PASS, known CGROM F05 warning. No source edits.
See R5_SYSTEM3_LCD_DUAL_KS0066_20261031.md and private R5 logs.


## R6 — Original host input, physical matrix and key recognition (2026-10-31)
CLOSED / BUILD_PASS / CPU_KEY_PASS / MATRIX_PASS. Reused original
KeyboardModule.c column arrays and PC key positions. The single
AS3K-DIAG now has 16 logical 8-row ports (power col8 excluded),
77 identified MAME keys and supported natural-keyboard typing.
CPU BOOT Y 0F accepted and released; WRITER N 7F accepted and
released; natural MAME y accepted. Direct CPU-visible hardware
inputs both ROMs: FF idle, FE latch, EF PA, EE both.
R3 Phase1 regression, R5 LCD image equivalence, -validate and
4/4 ROM audits PASS. Integrity warning, full Writer launch,
PDSEL/PDKBEN IRQ behavior NOT credited. Complete detail in
R6_AS3K_KEYBOARD_MATRIX_20261031.md, logs private.


## R7 — Native AlphaWord Plus original launch and typing (2026-10-31)

CLOSED / ORIGINAL_APPLET_DISCOVERY_PASS /
ALPHAWORD_LAUNCH_PASS / EDITOR_TYPING_PASS.
Native Flash header watchpoint at 00408000 produced FFFFFFFF
on BOOT and C0FFEEAD on WRITER, actual CPU PC 004C3254.
Original System3 integrity dialog Y -> native restart with
Return -> original AlphaWord Plus 3.0 / System3.01 splash.
Real PG4 battery input from original PowerModule.c resolves
low-power warning without firmware modification. Exact CPU
PDKBEN=FF write at 004CE98C was missing from EZ core;
added masked row OR level4 interrupt, state/reset/save and
MAME input-change notifications to awaken CPU from native STOP.
Original editor now displays host-typed 'hello' on its KS0066 LCD.
Minimal link/validate 0; four ROM audits OK; R3 Phase1 and R5
no-key LCD regression PASS. No applet code patch or fork.
Document persistence, PDSEL and peripherals NOT credited.
See R7_ORIGINAL_ALPHAWORD_LAUNCH_TYPING_20261031.md.


## R8 — Battery-backed SRAM and original AlphaWord persistence (2026-10-31)

CLOSED / CLEAN_EXIT_PERSISTENCE_PASS / EDITOR_RESTORE_PASS.
One driver, standard MAME NVRAM bound to existing external 256 KiB
RAM pointer. Original WRITER first process: Y, Return, editor,
type r8proof, actual LCD display and normal exit -> 262144-byte
private NVRAM image. Second NEW process, same ROM/NVRAM, NO input,
recovered r8proof + cursor pixel-identically (240x36) and no
initial data-change warning. BOOT also generated its own 256 KiB
NVRAM via the same emulator. Minimal link PASS, MAME validate 0,
4/4 ROM audits OK, R3 keyboard Phase1 D0=0 PASS.
No firmware modification or user binaries in Git.
Flash writing, crash recovery and exhaustive file operations
NOT credited. R8_AS3K_BATTERY_SRAM_PERSISTENCE_20261031.md contains
full private test evidence. R9 next; no push.

## R9 — Intel B3 Flash / original SmartApplet install and boot (2026-10-31)

CLOSED / FLASH_PROGRAM_PASS / ERASE_PASS / MANUFACTURER_APPLET_PASS.
The same minimal AS3K-DIAG now uses a byte-wide Intel 28F008B3B
flash device (89/D3) derived minimally from the native MAME
intelfsh core, with 8x8-KiB parameter and 15x64-KiB main
blocks, NOR programming, and native persistent Flash NVRAM.
All original ROM images are unchanged input data.
The 68k 16-bit bus bridge preserves flash byte lanes and Flash
command semantics. No new emulators or alternate executable.

Native CPU-space test: Intel ID 89/D3, blank FF, programmed
A5, NOR reprogram FF kept A5, erase 20/D0 busy 00 and
returned FF. Independent parameter test erased exactly lower
8 KiB, all remaining original BOOT bytes unaffected.
Genuine manufacturer AlphaWordPlus.OS3KApp (101596 bytes)
was byte-programmed into the empty applet window of the
original BOOT image and saved as 1-MiB private Flash NVRAM.
Installed region matched original manufacturer file byte-for-byte;
all outside bytes matched original BOOT.
Reopen original BOOT, System3 discovered applet and launched
real AlphaWord: native keyboard typed 'flash', genuine LCD
displayed it. Another reopen without input recovered 'flash'
with pixel-identical 240x36 screenshot. Original R8 SRAM
'r8proof' recovery was regression tested and pixel-identical.

Reduced make REGENIE=1 PRECOMPILE=0 built and linked exit0,
MAME -validate exit0, all 4 ROM audits OK, R4 System3 boot
and R3 KeyboardInitialize Phase1 D0=0 PASS. Existing F05
NEEDS REDUMP notice unrelated. Chip top/bottom suffix on
actual PCB, VPP/lock behavior and official AlphaSmart Manager
USB/serial transfer protocol remain NOT credited.
Report: R9_INTEL_FLASH_SMARTAPPLET_20261031.md.
Private test logs, manufacturer applet and programmed RAM/Flash
are outside Git. No Codex or push. Next R10 external I/O.

## R10 — MC68EZ328 UART bitstream / RS-232 loopback (2026-10-31)

STATE: R10 OPEN; basic UART 8N1 TX/RX + 8-byte FIFO **DYNAMIC PASS**.
Original UartModule.c / DebugModule.c and MC68EZ328UM chapter 11
used as reference. Previous core UART data handlers were nonfunctional.
Now the SAME minimal AS3K-DIAG has baud-timed TXD, sampled RXD,
RX/TX FIFOs, register statuses, and EZ UART interrupt source
logic, wired to real MAME RS232 loopback/null_modem/pty slots
instead of an artificial text-copy shortcut.

Dynamic original WRITER CPU-space register test:
UBAUD 0226, USTCNT E000, UTX FFFFF907=A5 -> URX=20A5;
UTX=5A -> URX=205A. Timer/bit pipeline traversed real
RS232 loopback; both bytes recovered. Separate 8-byte burst
10:32:54:76:98:BA:DC:FE recovered in order, RX FIFO
empty=0000 after eight reads. Both tests exit code 0.
Minimal MAME build and link exit 0 (one system driver source,
six registered ROM entries); -validate exit 0; ROM audits
4/4 OK; historic R3 Phase1 D0=0; original R8 and R9
persisted AlphaWord document screenshots pixel-identical.
Existing F05 NEEDS REDUMP untouched.

Parities, 7-bit/2-stop modes, RTS/CTS, verified serial IRQ
semantics, unverified 16-MHz oscillator vs actual baud,
USB, IrDA, BBUG execution, native firmware RS232Framer,
full file transfer and official updater remain NOT PASSED.
External host PTY bidirectional communication is independently
PASS: a separate Python process opened /dev/pts/1, read byte
A5 from MAME TXD, sent byte 5A, and MAME URX returned 205A.
Host script logged R10_PTY_BIDIRECTIONAL_PASS=True,
MAME exit 0. Initial time alignment error corrected in
TEST SCRIPT without modifying UART hardware model.
R10 is not CLOSED. See
docs/as3k/R10_EZ_UART_RS232_LOOPBACK_20261031.md.
Native original source audit: R10_ORIGINAL_DEV_TOOLS_AUDIT_20261031.md.
All scripts/logs and images are private in T160; no Codex.

## R10 — PDIUSBD11D I²C port hardware, no USB-host protocol (2026-11-01)

R10 GENERAL OPEN. D11 I2C GPIO_BUS_PASS / ACK_PASS /
COMMAND_DATA_PASS / 4_ROM_REGRESSION_PASS.

Reused original AlphaSmart sources from Drive:
UsbAsmModule.s, UsbModule.c, UsbChip.c/.h and Philips D11
datasheet. The actual AS3000 drives PE1=SDA and PE2=SCL
by toggling PEDIR while latching PEDATA low. Previously
EZ callbacks were driven only by PEDATA writes. Corrected
EZ-only effective open-drain line output on PEDIR,
PEDATA and PESEL changes, with external low-level sample
on released input. Older MC68328 behavior unaffected.

The SAME minimal alphasma3k.cpp driver now contains a
board-level PDIUSBD11D I2C slave model: START/STOP,
8-bit MSB-first transfer, SDA drive for ACK, master
ACK/NACK and basic D11 command/data addresses 34/35/36.
Limited D1 address / D8 endpoint enable / F3 mode
configuration and F4 empty interrupt read. No USB
host traffic or invented endpoint packet.

Real emulated CPU GPIO bit-bang (not direct slave
field access), BOOT and WRITER: released PE1/PE2=06,
36 ACK, F4 ACK, data-read 35 ACK, ISR 0000,
invalid address 30 NACK. WRITER full D1/80 and
F3/11/0B command/data exchange PASS. Build/link
same reduced binary exit0, -validate exit0, four ROM
audits 4/4 OK. UART 8-byte FIFO unchanged PASS,
R3 KeyboardInitialize Phase1 D0=0 PASS, original
R8 `r8proof` and R9 `flash` persisted LCD images
pixel-identical after running this build.

This is a physical PDIUSBD11D CPU-control-path PASS,
NOT USB enumeration, host GET packets, endpoint FIFO
transfer, AlphaSmart Manager install or printer USB.
All those remain OPEN under existing R10.
See R10_PDIUSBD11_I2C_HW_20261101.md and
T160 private r10_d11_* logs / scripted register tests.

## R10 — D11 original endpoint FIFOs + PF2 reset / PF1 IRQ wiring (2026-11-01)

R10 still OPEN. Continuation of the SAME AS3K-DIAG hardware
source: no new emulator, application, host utility or ROM patch.
Extended the existing Philips PDIUSBD11D I2C controller in
alphasma3k.cpp with ten endpoint-index buffers (reserved +
length + eight bytes each), Select Endpoint 00–09, F0
Write/Read Buffer, FA Validate Buffer for IN, F2 Clear
Buffer for OUT, 40–49 Set Endpoint Status and 80–89
Read Endpoint Status; STALL handling and manufacturer
D0 hub-compatible initialization. Reused original
UsbModule.c, UsbChip.c and Philips D11 specification.
Hardware RESET_N is on EZ PF2; fixed direction callback
in EZ core mc68328.cpp to propagate the actual PF2 output
level. D11 INT_N idle level on PF1 feeds IRQ5;
SUSPEND output is on PE0, no bus suspend in no-host state.
No fake USB packet, endpoint interrupt or host device.

Original ROM BOOT and original ROM WRITER CPU-space I2C
bitbang tests each returned R10_D11_FIFO_PASS=1:
empty select 00 -> validated full 01, content
00:03:A1:B2:C3 exact, stalled status 80,
unstalled 00, INT_N idle PF1 bit 02, physical
RESET_N PF2 pulse cleared buffer; exit0.
Second capacity test: 00:08 header and eight original
byte values 11:22:33:44:55:66:77:88,
R10_D11_FIFO8_PASS=1, exit0.

Reduced MAME same ./alphasma3k build/link exit0,
-validate exit0, four ROMs OK; UART eight-byte
loopback PASS, R3 KeyboardInitialize D0=0 PASS,
original R8/R9 persistent LCD images pixel-identical.
Chip-end CPU-control PASS is NOT bus-level USB D+/D-,
SIE SETUP/OUT/IN token exchange, real interrupt events,
host USB connectivity or transfer-protocol PASS.
Evidence appended to R10_PDIUSBD11_I2C_HW_20261101.md,
logs/scripts only in private/roms/as3k/mame_sets.

## R10 — Philips PDIUSBD11 hardware fidelity correction (2026-11-01)

R10 OPEN. Manufacturer original UsbChip.c and Philips D11
datasheet reconciled against SAME AS3K-DIAG hardware model.
Replaced incorrect full/status masks (Read Endpoint Status
full=0x20 and STALL=0x08; Select Endpoint still full=0x01).
Made F0 buffer read/write preserve its D11 SRAM pointer
across separate I²C data phases, reset pointer only on
Select Endpoint. Read Last Transaction Status now
independent of endpoint buffer and clears endpoint IRQ
pending bit, if any; F4 reads two IRQ bytes and clears
bus-reset flag. Removed nonexistent PDIUSBD11 hub D0
registration. Eight actual endpoints are indexed 2–9;
array storage includes 0–1 compatibility/reserved indices,
not ten independent physical endpoints.

Two ORIGINAL ROMs BOOT/WRITER ran GPIO real bit-bang
R10_D11_SPEC_PASS=1 and emulator exit0 without Lua error:
fragmented 5-byte buffer writing/reading exact, endpoint
select FULL=01, status FULL=20, STALL=08, CLEAR=00,
idle INT_N PF1=02, physical RESET_N PF2 clears buffer.
Same reduced MAME link PASS; validate exit0, four ROM
audits 4/4 OK, UART eight-byte FIFO PASS,
R3 keyboard D0=0 PASS, R8/R9 original document LCD
pixels identical to baseline. Older logs with FULL status
01 and STALL 80 are superseded as NON-conformant.

No USB D+/D- packet SIE, host enumeration, synthetic printer,
other emulators or programs were added. R10 remains OPEN.
See R10_PDIUSBD11_I2C_HW_20261101.md and private
r10_d11_spec_* / r10_spec_* T160 evidence.
