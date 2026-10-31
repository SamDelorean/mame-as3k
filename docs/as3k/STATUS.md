# AlphaSmart 3000 emulator — development status

Last updated: 2026-10-31

This document is the public handoff for AlphaSmart 3000 emulator development on branch `as3k-mame0289-dev` of `SamDelorean/mame-as3k`.

The branch is based on exact MAME 0.289 commit `f34f02505e32c1993c6a782b6814232cbfc74e36` and is intentionally separate from `master`.

## Mandatory AS3K-DIAG development rules — 2026-10-28

AS3K-DIAG is the **only** emulator product, based on the existing
mame-as3k-dev MAME 0.289 source tree. Never create isolated versions,
new source forks, alternate executables, external worktrees or stand-alone
emulators to support a particular ROM. Historical ROM sets and diagnostic
fixtures are *data inputs*, not emulator versions.

ROM selection must ultimately be dynamic (including a graphical selector),
supporting any hardware-compatible AS3000 ROM/firmware release without
recompiling or hardcoding firmware-version-specific jumps, offsets or patches.
BOOT and WRITER System3 ROMs are reference images, not permanent design
constraints. The product emulates real MC68EZ328/board hardware, not a
particular firmware.

Use the **same reduced/minimal MAME build and incremental STATIC_FIRST
methodology as AS2K-DIAG**, preserving existing working modules, source
provenance, one canonical working tree, directed tests and PASS/FAIL evidence.
Minimal refers to MAME dependencies, never to omission of real AS3000
hardware functionality. All testing/instrumentation remains inside the
same AS3K-DIAG. No blind fixes, new unrelated stages, routine Codex or
untested PASS assertions; update this status and Drive work log every
increment. No firmware binaries, ROMs, CGROM or generated executables in Git.

R2 compiled executable currently named alphasma3k remains a single
technical build of the canonical tree, not permission to produce another
emulator line. The multiple registered MAME names are ROM/diagnostic
variants only. The universal ROM selector is a later planned integration;
do not fold it into the narrowly defined R3 Port-A work.

## R3 — EZ328 Port A / WDTH8 (2026-10-28) — CLOSED / DYNAMIC_PASS

The original NXP MC68EZ328UM confirms PAPUEN at 0xFFFFF402 (8 bits,
reset 0xFF), reserved 0xFFFFF403, and SCR.WDTH8 selecting GPIO PA[7:0]
instead of external D[7:0]. Reference:
https://www.nxp.com/docs/en/reference-manual/MC68EZ328UM.pdf

The same existing AS3K-DIAG's mc68328.cpp/h were minimally corrected:
EZ-only PAPUEN map/read/write/reset/save; disconnected input fallback
follows PAPUEN while a connected callback takes priority. The
mc68328_device base fallback stays zero to preserve its old behavior.
EZ SCR.WDTH8 now sets AND clears internal m_pasel (0xff/0x00),
preserves other SCR bits, and saves m_pasel in the EZ state path.
No EZ PASEL was registered at 0xFFFFF403.

MAME 0.289 reduced rebuild + final link: BUILD_EXIT=0; MAME validity:
exit 0. Labels of the two private System3 ROMs were made lowercase
inside MAME only, with local filesystem aliases to the original data,
to satisfy MAME runtime label validation (without changing ROM bytes).
All four relevant romset audits passed; the already-known KS0066 F05
NEEDS REDUMP warning remains for BOOT/WRITER.

The bounded original AlphaWord diagnostic asma3kdv reached:
SCR write 0x1d at 0x0040001e; PAPUEN read 0xff at 0x0042e2bc;
PAPUEN write 0x80 at 0x0042e2c4; external latch write 0xff
at 0x0042e2c8; Phase-1 RTS at 0x0042e2ee with D0=0 and normal
caller return 0x00420168 with D0=0. No Phase 2, new IRQ or
unrelated fault handler executed in this bounded trace.
PAPUEN had no unmapped access warnings. Two unrelated startup
unmapped accesses at 0xFFFFFD0C and 0xFFFFFD0E persist; not R3.
A debugger-space SCR byte read printed 0x00 despite the traced
SCR write of 0x1d; that direct readback remains ambiguous, not a
proof that SCR stored 0x00. WDTH8 clear path was SOURCE-verified
only; not exercised dynamically. Full system boot not claimed.

Evidence: docs/as3k/CODEX_RESULT.md, and local PRIVATE scripts/logs
under ../private/roms/as3k/mame_sets/r3_*.
Next PLAN milestone R4: original System3 execution in the same
AS3K-DIAG, with no firmware-specific shortcuts.

## R5 — Genuine System 3 LCD (2026-10-31) — CLOSED / DISPLAY_PASS

The existing Port C -> two KS0066 bridge and 40x4 (240x36)
renderer are now dynamically validated **against original
System 3 OS3K 2005 firmware in both BOOT and WRITER Flash images**.

Using the already linked reduced AS3K-DIAG executable, program-space
watchpoints recorded 8,308 PCDATA writes at 0xFFFFF411 for each
ROM (note 0xFFFFF409 is PB data, *not* the LCD port).
E1 had 1,492 rising strobes and E2 had 1,270; with RS=1,
402 were E1 data strobes and 370 E2 data strobes.

The real MAME LCD snapshots for BOOT/WRITER are visually and
pixel-for-pixel identical: 240x36, 387 active pixels on
the first row at the sampled moment. The genuine firmware displays
"An unexpected data change occurred." The identical string is
present in the original System3 Flash at offset 0x598D.
This is a genuine firmware warning and is NOT a fake MAME message.

Following display the CPU waits in original key-scan
0x004D0FD8; actual pressed keys are not yet wired.
The underlying reason for the data-change warning is unproven,
and no persistence or keyboard implementation is counted as PASS.
The LCD is nevertheless confirmed operational with original
OS output, including both controller enables. Existing KS0066
CGROM F05 retains its NEEDS REDUMP warning.

Both CPU runs exit correctly at a bounded debugger breakpoint,
MAME -validate passed, and 4/4 ROM audits passed. No
driver/core source changes or rebuild were needed in R5.
Full source evidence: docs/as3k/R5_SYSTEM3_LCD_DUAL_KS0066_20261031.md.
All scripts, logs and PNG captures are preserved in private.
Next previously planned milestone: R6 native keyboard matrix/input,
to replace fixed idle states (without any new emulator line).

## R4 — System 3 original CPU execution (2026-10-31) — CLOSED / CPU_EXEC_PASS

R4 is closed within its defined scope: true CPU entry into original System 3
and execution of an original System 3 initialization subroutine. Not a
claim of fully initialized GUI, AlphaWord launch, keyboard input or save.

Two minimal hardware fixes in the SAME AS3K-DIAG tree:
1. Board keyboard Port D rows Y1–Y8 are idle-high externally; before
   the fix Small ROM active-low row scan found all eight pressed
   (D1=0xff at 0x0040063A), blocking release polling. Eight
   in_port_d GPIO callbacks now represent idle high. No keyboard
   matrix, pressed events, IRQ or hardcoded firmware path was added.
2. EZ-owned Timer 1 registers were not reset (different from the
   original MC68328 registers). Small ROM wrote TCTL=0008, TCMP=0020,
   TCTL=0009 and waited at 0x00400A66 for compare status. Diagnostic
   logs identified uninitialized prescaler 0x7e99 and comparator
   0xd6d8. MC68EZ328::device_reset now resets its own timer register
   state: TCTL=0, TPRER=0, TCMP=ffff, TCR=0, TCN=0,
   TSTAT=0, TCLEAR=0. Temporary logging was removed.

Both OS3K_MIN_BOOT and OS3K_MIN_WRITER executed without ROM changes:
Small ROM -> native version validation -> handoff at 0x004FFF98
-> 0x004C007A -> System 3 entry 0x004C7390 -> initialization branch
0x004C73CA -> genuine routine 0x004CD010. All stages independently
observed under the same MAME debugger for both ROMs, with trace
files in the existing private r4_* path. The debugger's displayed PC
may show +2 relative to a breakpoint instruction address.
The genuine firmware execution was NOT bypassed.

Minimal MAME 0.289 build and link PASS (same alphasma3k executable).
MAME -validate EXIT 0. ROM audits for asma3kdi, asma3kdv,
asma3ksb, asma3ksw: 4/4 OK, EXIT 0; previously known KS0066
F05 NEEDS REDUMP warning unchanged. Previous unmapped accesses at
FFFFFD0C and FFFFFD0E remain unrelated and unaltered.

R4 engineering handoff: docs/as3k/R4_SYSTEM3_BOOT_EXEC_20261031.md.
Next planned increment R5: System 3 LCD user interface; reuse the
already accredited bridge, no redesign. AS3K-DIAG remains the only
minimal emulator; private ROMs are data inputs, not product variants.

## Current execution frontier
On genuine System 3 BOOT and WRITER, the LCD has displayed the
original data-change warning using both KS0066 controllers; the
CPU is now observed waiting for actual physical keyboard input at
0x004D0FD8. This is the current functional UI frontier after R5.


System 3 original CPU entry was reached at 0x004C7390 and a real
initialization subroutine began at 0x004CD010 on BOOT and WRITER.
This is separate from the following historical diagnostic result.

The valid-AlphaWord diagnostic path has executed through **`KeyboardInitializeModule` Phase 1** and returned normally to its caller at `0x00420168` with `D0 = 0`.

`KeyboardInitializeModulePhase2 = 0x0042E2F0` has **not** been executed.

The MC68EZ328 keyboard-related GPIO audit and the first driver-only implementation stage are complete. The AS3K board's external 8-bit keyboard column latch at `0x00600000` is mapped, saved, deterministically reset, and validated through the existing Phase-1 path. The matrix, key mappings and Phase 2 remain deferred; the EZ core reset correction was completed in R4.

## Validated milestones

### Startup Manager

Two local diagnostic systems exercise both startup branches:

- `asma3kdi`: invalid applet signature. Startup copies exactly 32,508 longwords from Flash to RAM and transfers control to `0x00000410`.
- `asma3kdv`: valid AlphaWord signature. Startup skips the copy loop and transfers control directly to AlphaWord at `0x00420030`.

The historical MAME TODO describing a failure at `0x0040016C` as `cmpa.w A0,A1` does not describe current MAME 0.289 behavior. The linked instruction is `CMPA.L`, and the tested loop terminates correctly.

### Interrupt initialization

`InterruptInitializeModule = 0x0043177E` has been correlated with historical AlphaSmart source/listings and executed successfully.

Validated state includes ICR `0xFFFFF302 = 0x0000`, IMR `0xFFFFF304 = 0x00FFFFFF`, ISR `0xFFFFF30C = 0`, IPR `0xFFFFF310 = 0`, and the level-4/5/6 vectors at `0x00000110/114/118`.

### Timer initialization

`TimerInitializeModule = 0x004310BE` has been executed successfully. The three timer records and globals initialize correctly, `Timer_InterruptHandler = 0x004313D8` is installed for `INTERRUPT_TIMER_6 = 0x2`, and RTCCTL/RTCDAY/RTCHMSR receive the expected values.

### LCD GPIO protocol and controller bridge

The AlphaSmart LCD uses MC68EZ328 Port C: PC0–PC3 DB4–DB7, PC4 R/W, PC5 RS, PC6 E1, PC7 E2.

The driver bridges these signals to two `ks0066_device` instances. An initial per-bit bridge exposed an ordering error in which an atomic PCDATA write could lower R/W before E, producing a false `0x8F` LCD command and infinite busy loop. The bridge was corrected locally in `alphasma3k_state` without changing the MC68EZ328 or HD44780/KS0066 cores.

With the corrected bridge, a real finite busy flag is observed and clears normally. Key implementation commit: `0b414444`.

### 40×4 display composition

The display is modeled as two independent 2×40 controllers: rows 1–2 from E1/`ks0066_0`, rows 3–4 from E2/`ks0066_1`. The driver composes one 240×36 LCD screen using 6×9 cells with 5×8 active glyph pixels. Key implementation commit: `38528ee8ffca9ff718d87e88145cae92d9e0134a`.

### Exception vectors

`SystemInstallExceptionVectors = 0x00430504` has been executed successfully. Bus error, address error, illegal instruction and divide-by-zero vectors are written as the expected eight big-endian 16-bit transactions; previously installed vectors remain intact and no exception handler fires.

### Keyboard hardware contract and Phase 1

Historical AlphaSmart source/listings establish a **15-column × 8-row** keyboard matrix:

- X1–X8: external 8-bit latch at `0x00600000`;
- X9–X15: PA0–PA6;
- Y1–Y8: PD0–PD7;
- scan is active-low;
- idle rows are externally pulled high;
- row low means pressed;
- logical column `0x8` is the separate power switch, not part of the scanned 15×8 matrix.

`KeyboardInitializeModule` Phase 1 is a 72-byte leaf at `0x0042E2A8–0x0042E2EF`, RTS `0x0042E2EE`. It has no calls, globals, loops, waits or interrupt installation. The dynamic test observed the exact 12 expected transactions and returned to `0x00420168` with `D0 = 0`. Phase 2, the keyboard interrupt handler and CPU exception handlers were not entered.

The external latch write `0x00600000 = 0xFF` is now mapped on the high byte lane and retained by driver state. Phase 1 still returns normally to `0x00420168` with `D0 = 0`. PAPUEN and the AlphaSmart write to `0xFFFFF403` remain nonfatal in the current core.

### External keyboard column latch

The driver models the write-only 8-bit latch at the only firmware-proven byte address, `0x00600000`, using the high byte lane of the 16-bit program bus. The board's 32 KiB chip-select window is established, but the available source does not prove that the latch ignores all address lines throughout that window, so broader mirroring is deliberately not asserted.

The latch is save-state registered and resets to `0xFF`, an emulator initialization choice that leaves all active-low columns inactive; it is not a measured hardware power-on value. Bounded `asma3kdv` validation observed and retained the Phase-1 `0xFF` write, eliminated the old unmapped-access message, and reached the Phase-1 RTS and caller return with `D0 = 0` without entering Phase 2, interrupt code, or exception handlers.

## MC68EZ328 keyboard GPIO audit

Static audit commit: `08816104002f27365ace1798eddd06b78550c764` (`as3k: audit EZ328 keyboard GPIO gaps`).

The audit compared MAME 0.289 `mc68328.cpp/.h`, AlphaSmart `M68328EZ.h`/`KeyboardModule.c`/listing material, and the Motorola/Freescale MC68EZ328 manual.

Important corrections and boundaries:

- **PADIR/PADATA** are mapped for EZ through `base_internal_map()` and are reusable for AS3K column outputs once the proper internal Port-A select state and callbacks are in place.
- **PAPUEN `0xFFFFF402`** is genuinely missing from the MAME EZ core. Correct support requires new state, reset/save handling, handlers, and Port-A input pull-up fallback.
- **`0xFFFFF403` is not an EZ PASEL register.** The EZ manual marks it reserved. MAME's original-MC68328 `pasel_r/w` must not simply be mapped into EZ. EZ Port-A GPIO selection is controlled by `SCR.WDTH8`; the core already uses that bit to set its internal `m_pasel`. AlphaSmart's write to `0x403` is therefore best treated as a harmless reserved-register write rather than evidence for an EZ PASEL register.
- The EZ save-state path does not currently save the internal Port-A selection state changed by `SCR.WDTH8`; this is a separate fidelity defect to address with the Port-A core work.
- **PDDIR/PDDATA/PDPUEN** are mapped and provide a reusable row-input path.
- **PDSEL `0xFFFFF41B`** is genuinely missing. Correct fidelity requires new state/handlers plus PD7–PD4 mux gating; PD3–PD0 are always GPIO on the EZ. Current MAME effectively treats all eight Port-D bits as GPIO.
- **PKBDINT in AlphaSmart sources corresponds to EZ PDKBEN `0xFFFFF41E`**, not a pending/acknowledge register. It is an 8-bit enable mask feeding an active-low level-sensitive OR keyboard interrupt. Correct support requires new core state and interrupt semantics, but it is not needed for Phase 1 or Phase 2 and can be deferred until `KeyboardEnableKeyboardInterrupt` is reached.
- Existing PDPOL/PDIRQEN/PDIRQEDGE state is incomplete/over-broad for exact EZ semantics. In particular, current individual interrupt logic does not fully gate on PDIRQEN. This should be handled together with the later PDKBEN interrupt stage rather than mixed into the initial keyboard-latch work.
- The external `0x00600000` component is board logic, not DragonBall state. It belongs entirely in `alphasma3k_state` as a saved 8-bit write latch whose outputs eventually become keyboard columns X1–X8.

A previous debugger readback of zero after writes to mapped GPIO registers remains partially unresolved. Static handler inspection shows PADIR should retain `0x7F`, so a later narrow debugger-space/readback test may be useful. This does not block the driver-only latch stage.

## Minimum patch sequence

Development should remain staged:

1. **Driver-only external latch at `0x00600000`** — complete and dynamically validated.
2. **EZ Port-A core fidelity** — PAPUEN plus correct/saveable `SCR.WDTH8` selection behavior; do not add a fake PASEL register.
3. **Driver matrix plumbing** — connect PA0–PA6 outputs and latch X1–X8 to PD0–PD7 row evaluators, with externally idle-high rows and support for multiple active-low columns.
4. **EZ PDSEL fidelity** — add correct upper-nibble mux state/behavior.
5. **Keyboard interrupt stage** — implement PDKBEN/PKBDINT active-low OR and audit/fix related Port-D interrupt semantics immediately before the firmware actually enables keyboard interrupts.

This sequence intentionally avoids combining latch, matrix, key definitions, core register fixes and interrupt logic into one change.

`KeyboardInitializeModulePhase2 = 0x0042E2F0–0x0042E3C3` initializes software state and installs `Keyboard_InterruptHandler = 0x0042EA74` through `InterruptInstallHandler(0x40, ...)`; it does not enable the keyboard interrupt. Phase 2 remains unexecuted.

## Current machine model

The branch currently models or assumes:

- Motorola MC68EZ328 DragonBall EZ at configured 16 MHz;
- 256 KiB RAM at `0x00000000`;
- 1 MiB Flash window at `0x00400000`;
- static final memory map plus reset-time vector-copy workaround;
- two KS0066-compatible LCD controllers through Port C;
- saved write-only external keyboard column latch at `0x00600000`;
- one composite 40×4 LCD screen.

Historical AlphaSmart material supports the production map of 256 KiB SRAM, 1 MiB Flash and an external write-latch window at `0x00600000`. Dynamic DragonBall chip-select remapping is not yet reproduced.

## Diagnostic systems

The branch contains `asma3k`, `asma3kdi`, `asma3kdv`, and `asma3kdvl`. Diagnostic ROM/fixture files are **not distributed in this repository**. No AlphaSmart firmware, physical ROM dump, proprietary AlphaWord binary, real KS0066 CGROM or generated diagnostic ROM is committed.

## Reduced build

```sh
make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2
```

Executable: `./alphasma3k`

## R2 — Original System 3 Flash ROM selection (2026-10-27)

The original driver and three historical diagnostics are retained unchanged.
Two independent optional private Flash ROM systems are now registered in
src/mame/skeleton/alphasma3k.cpp:

- asma3ksb: OS3K_MIN_BOOT.bin, 1 MiB, CRC32 ac3b2a2c,
  SHA-1 c2936532f37514d0eaa74e2cf97bf96bb17e883d;
- asma3ksw: OS3K_MIN_WRITER.bin, 1 MiB, CRC32 c79df93f,
  SHA-1 02ae5d564b48476abf7067b71a7e0166e471bdfa.

Their source files are private under ../private/roms/as3k/os3k_minimal/
The MAME ROM search directory is ../private/roms/as3k/mame_sets/
with private asma3ksb and asma3ksw directory symlinks.
No firmware bytes are stored in Git.

MAME executable alphasma3k (128,810,144 bytes, x86-64 ELF) linked
successfully on 2026-10-28: BUILD_EXIT=0. The source registry file
src/mame/mame.lst additionally lists both new systems, so -listfull
reports six AS3K machines. Both -verifyroms audits pass with exit code 0
using the private ROM path and existing private ks0066 font ROMs.
MAME reports ks0066_f05.bin as NEEDS REDUMP / best available, which is
a known CGROM fidelity warning, not a failure to locate either OS3K ROM.
The original firmware was not executed.

These are AS3000 2005 System 3 construction variants, distinct from the
physical ROM asma3k and older AlphaWord diagnostic fixtures. Integration
does not certify actual System 3 / Writer boot: the R1 transfer contract
was verified statically only. R4 is the runtime boot milestone.
Keyboard Port-A core corrections closed in R3; future matrix and Port-D stages remain separate.

## Known gaps

The emulator is not yet a complete usable AlphaSmart 3000. Remaining work includes PA/latch-to-PD matrix plumbing, PDSEL, later PDKBEN keyboard interrupt behavior, actual MAME input-port/key mapping, Phase 2 and later AlphaWord modules, dynamic chip-select/remapping fidelity, power/battery behavior, UART/RS-232, ADB, PS/2, IrDA, USB/PDIUSBD11D, Flash update/write behavior, measured LCD timing/colors, exact controller confirmation, and an explicit synthetic four-row visual fixture.

## Next development step

R3 Port-A PAPUEN/WDTH8 is closed by bounded CPU dynamic evidence.
Continue with the existing R4 milestone: boot the original AS3000
System3 on the SAME AS3K-DIAG executable, then reuse the LCD bridge.
Only fix a demonstrated hardware-emulation blocker; no ROM-version-
specific patches, isolated builds, keyboard Phase 2 or unrelated work.
The validated historical latch, RAM, display, and startup paths remain
unchanged. Maintain STATIC_FIRST -> minimal patch -> bounded validation.

## Reproducibility and contribution notes

Development coordination files live in `docs/as3k/`: `STATUS.md`, `CODEX_NEXT.md`, and `CODEX_RESULT.md`. `AGENTS.md` contains repository-specific scope and safety rules.

Keep proprietary ROMs, firmware, historical binaries and generated diagnostic ROMs out of Git. Public source changes, documentation, reproducible scripts and factual test results are appropriate for this branch.
