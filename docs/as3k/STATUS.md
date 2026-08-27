# AlphaSmart 3000 emulator — development status

Last updated: 2026-08-26

This document is the public handoff for AlphaSmart 3000 emulator development on branch `as3k-mame0289-dev` of `SamDelorean/mame-as3k`.

The branch is based on exact MAME 0.289 commit `f34f02505e32c1993c6a782b6814232cbfc74e36` and is intentionally separate from `master`.

## Current execution frontier

The valid-AlphaWord diagnostic path has now executed through **`KeyboardInitializeModule` Phase 1** and returned normally to its caller at `0x00420168` with `D0 = 0`.

`KeyboardInitializeModulePhase2 = 0x0042E2F0` has **not** been executed.

The next task is a static audit of MC68EZ328 Port-A/Port-D register coverage before any keyboard/core implementation is attempted.

## Validated milestones

### Startup Manager

Two local diagnostic systems exercise both startup branches:

- `asma3kdi`: invalid applet signature. Startup copies exactly 32,508 longwords from Flash to RAM and transfers control to `0x00000410`.
- `asma3kdv`: valid AlphaWord signature. Startup skips the copy loop and transfers control directly to AlphaWord at `0x00420030`.

The historical MAME TODO describing a failure at `0x0040016C` as `cmpa.w A0,A1` does not describe current MAME 0.289 behavior. The linked instruction is `CMPA.L`, and the tested loop terminates correctly.

### Interrupt initialization

`InterruptInitializeModule = 0x0043177E` has been correlated with historical AlphaSmart source/listings and executed successfully.

Validated state includes:

- ICR `0xFFFFF302 = 0x0000`
- IMR `0xFFFFF304 = 0x00FFFFFF`
- ISR `0xFFFFF30C = 0x00000000`
- IPR `0xFFFFF310 = 0x00000000`
- level-4 vector `0x00000110 = 0x00431A04`
- level-5 vector `0x00000114 = 0x00431AB0`
- level-6 vector `0x00000118 = 0x00431AE6`

### Timer initialization

`TimerInitializeModule = 0x004310BE` has been executed successfully.

Validated behavior includes:

- timer globals and all three `TimerInfo` entries initialized;
- `Timer_InterruptHandler = 0x004313D8` installed for `INTERRUPT_TIMER_6 = 0x2`;
- RTCCTL `0xFFFFFB0C = 0x0080`;
- RTCDAY `0xFFFFFB1A = 0x0000`;
- RTCHMSR `0xFFFFFB00 = 0x00000000`.

### LCD GPIO protocol and controller bridge

The AlphaSmart LCD uses MC68EZ328 Port C:

- PC0–PC3: DB4–DB7
- PC4: R/W
- PC5: RS
- PC6: E1 / top controller
- PC7: E2 / bottom controller

The driver bridges these signals to two `ks0066_device` instances.

An initial per-bit callback implementation exposed an ordering problem: one atomic PCDATA write was presented to the KS0066 as separate bit changes, allowing R/W to fall before E. The resulting false write produced command `0x8F` and an infinite busy loop.

The bridge was corrected locally in `alphasma3k_state` without modifying the MC68EZ328 or HD44780/KS0066 cores. It accumulates the Port-C byte and commits it atomically: falling E edges see the old DB/RW/RS state, then DB/RW/RS are updated, then rising E edges see the new state.

With the corrected bridge, AlphaWord observes a real finite busy flag (`0x80` observed and cleared on the next poll), with no false `0x8F` and no E1/E2 contention.

Key implementation commit: `0b414444` (`as3k: make LCD Port C bridge atomic`).

### 40×4 display composition

The display is modeled as two independent 2×40 character controllers:

- rows 1–2: E1 / `ks0066_0`, DDRAM lines `0x00` and `0x40`;
- rows 3–4: E2 / `ks0066_1`, DDRAM lines `0x00` and `0x40`.

The driver composes one 240×36 LCD screen using 6×9 cells with 5×8 active glyph pixels. Both KS0066 devices are configured as 2×40.

Palette and refresh/vblank values remain provisional rather than measured from original AS3K hardware.

Key implementation commit: `38528ee8ffca9ff718d87e88145cae92d9e0134a`.

### Exception vectors

`SystemInstallExceptionVectors = 0x00430504` has been correlated and executed successfully.

It installs:

| Vector | RAM entry | Handler |
| --- | --- | --- |
| 2 — bus error | `0x00000008` | `0x00430526` |
| 3 — address error | `0x0000000C` | `0x004305B0` |
| 4 — illegal instruction | `0x00000010` | `0x0043063A` |
| 5 — divide by zero | `0x00000014` | `0x004306C4` |

The four longword stores appeared as the expected eight big-endian 16-bit transactions. TRAP 0 and level-4/5/6 vectors remained intact and no exception handler executed.

Validation commit: `72b9935edfafe270e91becdd30fb7459c6f07a03`.

### Keyboard hardware contract

Historical `KeyboardModule.c`, object listings, symbols and `AWordApplet02.bin` establish the AS3K keyboard as **15 scanned columns × 8 rows**.

- X1–X8 are driven by an external 8-bit latch at `0x00600000`.
- X9–X15 are driven by PA0–PA6.
- Y1–Y8 are read on PD0–PD7.
- logical column code `0x8` is the separate power switch and is not part of the normal scan.
- normal scan is active-low, one column at a time.
- idle rows are expected externally pulled high; row low means pressed.
- `Keyboard_GetNewKeyStates` complements PDDATA so the internal key bitmap is active-high.

`KeyboardInitializeModule` Phase 1 is a 72-byte leaf at `0x0042E2A8–0x0042E2EF`, RTS `0x0042E2EE`. It has no direct calls, globals, loops, waits or interrupt setup.

Its exact hardware sequence is:

- PASEL `0xFFFFF403`: read `0x00`, write `0x7F`;
- PADIR `0xFFFFF400`: read `0x00`, write `0x7F`;
- PAPUEN `0xFFFFF402`: read `0x00`, write `0x00`;
- external latch `0x00600000`: write `0xFF`;
- PADATA `0xFFFFF401`: read `0x00`, write `0x7F`;
- PDSEL `0xFFFFF41B`: write `0xFF`;
- PDDIR `0xFFFFF418`: write `0x00`;
- PDPUEN `0xFFFFF41A`: write `0x00`.

The dynamic Phase-1 test completed and reached caller return `0x00420168` with `D0 = 0`. No Phase-2 entry, exception handler, `InterruptInstallHandler` or keyboard interrupt handler fired.

The external latch write is currently unmapped but nonfatal. PAPUEN accesses are also unmapped. PASEL transactions were observed at the correct byte address but produced aligned unmapped diagnostics around `0xFFFFF402`. PDSEL accepted a bus transaction but retained readback was not demonstrated. Direct debugger readback at the Phase-1 RTS returned zero for the tested GPIO register bytes even where watchpoints had observed writes.

This makes the next core audit necessary before implementing the keyboard.

Static MAME source inspection already shows one important correction to the earlier support classification: `base_internal_map()` maps PADIR/PADATA and Port-D registers, while PASEL is explicitly added by `mc68328_device::internal_map()` and is not obviously added by `mc68ez328_device::internal_map()`. PASEL support for the EZ core must therefore be considered **unconfirmed** until the audit is complete.

`KeyboardInitializeModulePhase2 = 0x0042E2F0–0x0042E3C3` initializes software state and installs `Keyboard_InterruptHandler = 0x0042EA74` through `InterruptInstallHandler(0x40, ...)`; it does not enable the keyboard interrupt. Phase 2 has not yet been executed.

Static-study commit: `eb42b589a18644eeb02e5e1f131e6962f12768c4`.

## Current machine model

The branch currently models or assumes:

- Motorola MC68EZ328 DragonBall EZ at configured 16 MHz;
- 256 KiB RAM at `0x00000000`;
- 1 MiB Flash window at `0x00400000`;
- static final memory map plus reset-time vector-copy workaround;
- two KS0066-compatible LCD controllers through Port C;
- one composite 40×4 LCD screen.

Historical AlphaSmart material supports the production map of 256 KiB SRAM, 1 MiB Flash and an external write-latch window at `0x00600000`.

Dynamic DragonBall chip-select remapping is not yet reproduced by the current MAME core/driver.

## Diagnostic systems

The branch contains ROM definitions for:

- `asma3k` — original MAME AlphaSmart 3000 set;
- `asma3kdi` — invalid-applet startup diagnostic;
- `asma3kdv` — valid-AlphaWord diagnostic without LCD devices;
- `asma3kdvl` — valid-AlphaWord diagnostic with LCD bridge and both controllers.

Diagnostic ROM/fixture files are **not distributed in this repository**. No AlphaSmart firmware, physical ROM dump, proprietary AlphaWord binary, real KS0066 CGROM or generated diagnostic ROM is committed.

Controller-protocol testing used a local 4096-byte all-zero synthetic `ks0066_f05.bin`; MAME intentionally reports a checksum warning for it. It is not a fidelity substitute for the real character ROM.

## Reduced build

```sh
make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2
```

Executable: `./alphasma3k`

Current subtarget drivers: `asma3k`, `asma3kdi`, `asma3kdv`, `asma3kdvl`.

## Known gaps

The emulator is not yet a complete usable AlphaSmart 3000. Remaining work includes:

- complete MC68EZ328 Port-A/Port-D register audit for PASEL/PAPUEN/PDSEL/PKBDINT;
- external 8-bit keyboard latch at `0x00600000`;
- PA0–PA6 output-column callbacks and PD0–PD7 row-input model;
- actual 15×8 keyboard matrix/input-port definitions and AlphaSmart key mapping;
- dynamic validation of Keyboard Phase 2 and keyboard interrupt behavior;
- later AlphaWord initialization modules and main loop;
- dynamic chip-select/remapping fidelity;
- other DragonBall register omissions where execution proves they matter;
- power/battery behavior;
- UART/RS-232, ADB, PS/2, IrDA and USB/PDIUSBD11D;
- Flash update/write behavior;
- measured LCD timing/colors and exact original controller confirmation;
- explicit synthetic four-row visual-rendering fixture.

## Next development step

Perform a **static MC68EZ328 keyboard GPIO audit** against `mc68328.cpp/.h`, historical `M68328EZ.h`, and the available MC68EZ328 documentation.

The audit must classify each keyboard-related register as:

- already correct for MC68EZ328;
- existing handler/state missing only from the EZ map;
- genuinely missing state/semantics;
- driver-only functionality;
- safe to defer until interrupt use.

In particular it must resolve PASEL, PAPUEN, PDSEL and PKBDINT, explain the observed dynamic readbacks/unmapped messages, and define the smallest correct patch sequence. No source modification should occur until that audit is complete.

The evidence-first development method remains:

1. original AlphaSmart source/listings;
2. exact linked binary behavior;
3. narrow dynamic test;
4. static MAME core audit where needed;
5. minimum demonstrated implementation;
6. regression test and public documentation.

## Reproducibility and contribution notes

Development coordination files live in `docs/as3k/`:

- `STATUS.md` — this public handoff;
- `CODEX_NEXT.md` — next narrowly scoped Codex task;
- `CODEX_RESULT.md` — factual result of the most recently completed task.

`AGENTS.md` contains repository-specific scope and safety rules.

Keep proprietary ROMs, firmware, historical binaries and generated diagnostic ROMs out of Git. Public source changes, documentation, reproducible scripts and factual test results are appropriate for this branch.
