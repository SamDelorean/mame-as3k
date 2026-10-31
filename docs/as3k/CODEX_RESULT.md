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
