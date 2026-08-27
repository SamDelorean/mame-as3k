# AlphaSmart 3000 emulator — development status

Last updated: 2026-08-26

This document is the public handoff for the AlphaSmart 3000 work on branch `as3k-mame0289-dev` of `SamDelorean/mame-as3k`.

The branch is based on the exact MAME 0.289 commit `f34f02505e32c1993c6a782b6814232cbfc74e36`. It is intentionally separate from `master`.

## Current execution frontier

The valid-AlphaWord diagnostic path now reaches the entry of:

`KeyboardInitializeModule = 0x0042E2A8`

The debugger stops there before executing any keyboard instruction. The keyboard Phase 1 routine has now been correlated statically against the historical AlphaSmart source/listing, but it has not yet been executed.

## Validated milestones

### Startup Manager

Two local diagnostic systems exercise both startup branches:

- `asma3kdi`: invalid applet signature. The Startup Manager copies exactly 32,508 longwords from Flash to RAM and transfers control to `0x00000410`.
- `asma3kdv`: valid AlphaWord signature. The Startup Manager skips the copy loop and transfers control directly to AlphaWord at `0x00420030`.

The historical MAME TODO that describes a failure at `0x0040016C` as `cmpa.w A0,A1` does not describe the current MAME 0.289 behavior. The linked instruction is `CMPA.L`, and the tested loop terminates correctly.

### Interrupt initialization

`InterruptInitializeModule` at `0x0043177E` has been executed and correlated with the historical AlphaSmart source/listing.

Validated results include:

- ICR `0xFFFFF302 = 0x0000`
- IMR `0xFFFFF304 = 0x00FFFFFF`
- ISR `0xFFFFF30C = 0x00000000`
- IPR `0xFFFFF310 = 0x00000000`
- level-4 vector `0x00000110 = 0x00431A04`
- level-5 vector `0x00000114 = 0x00431AB0`
- level-6 vector `0x00000118 = 0x00431AE6`

### Timer initialization

`TimerInitializeModule` at `0x004310BE` has been executed successfully.

Validated behavior includes:

- timer globals and three `TimerInfo` entries initialized as expected;
- `Timer_InterruptHandler = 0x004313D8` installed for `INTERRUPT_TIMER_6 = 0x2`;
- RTCCTL `0xFFFFFB0C = 0x0080`;
- RTCDAY `0xFFFFFB1A = 0x0000`;
- RTCHMSR `0xFFFFFB00 = 0x00000000`.

### LCD protocol bridge

The AlphaSmart LCD is driven through MC68EZ328 Port C:

- PC0–PC3: DB4–DB7
- PC4: R/W
- PC5: RS
- PC6: E1 / top controller
- PC7: E2 / bottom controller

The driver now bridges those GPIO signals to two `ks0066_device` instances.

A first per-bit callback implementation exposed a real emulation-layer ordering bug: an atomic PCDATA write was being presented to the KS0066 as separate bit changes, allowing R/W to fall before E. The resulting false LCD write produced command `0x8F` and an infinite busy loop.

The bridge was corrected locally in `alphasma3k_state` without changing the MC68EZ328 or HD44780/KS0066 cores. It accumulates one Port C byte and commits it atomically: falling E edges observe the old DB/RW/RS state, then the new DB/RW/RS state is applied, then rising E edges observe the new state.

With that bridge, AlphaWord observes a real finite busy flag from the emulated controller (`0x80` was observed once and cleared on the next poll), with no false `0x8F` and no E1/E2 contention.

Key implementation commit: `0b414444` (`as3k: make LCD Port C bridge atomic`).

### 40×4 display composition

The physical display is modeled as two independent 2×40 character controllers:

- physical rows 1–2: E1 / `ks0066_0`, DDRAM lines `0x00` and `0x40`;
- physical rows 3–4: E2 / `ks0066_1`, DDRAM lines `0x00` and `0x40`.

The MAME screen is composed by the driver as one 240×36 LCD using 6×9 cells with a 5×8 active glyph area.

Both KS0066 devices are configured as 2×40. The screen renderer uses each controller's `render()` output and does not alter controller protocol state.

The palette and refresh/vblank values are still provisional rather than measured from original AS3K hardware.

Key implementation commit: `38528ee8ffca9ff718d87e88145cae92d9e0134a`.

### Exception vectors

`SystemInstallExceptionVectors` at `0x00430504` has been correlated and executed successfully.

It installs only these four vectors:

| Vector | RAM entry | Handler |
| --- | --- | --- |
| 2 — bus error | `0x00000008` | `0x00430526` |
| 3 — address error | `0x0000000C` | `0x004305B0` |
| 4 — illegal instruction | `0x00000010` | `0x0043063A` |
| 5 — divide by zero | `0x00000014` | `0x004306C4` |

On the MC68EZ328's 16-bit bus, all four longword stores appeared as the expected eight big-endian 16-bit transactions.

Previously installed TRAP 0 and level-4/5/6 vectors remained unchanged, no exception handler executed, and normal initialization reached `KeyboardInitializeModule` before timeout.

Validation commit: `72b9935edfafe270e91becdd30fb7459c6f07a03` (`as3k: validate exception vector installation`).

### Keyboard Phase 1 — static hardware contract

`KeyboardInitializeModule` has now been correlated exactly against `KeyboardModule.c`, `KeyboardModule.o.lst`, `AWordRAM01.out`, and the Flash-linked `AWordApplet02.bin`.

The routine is a 72-byte leaf at `0x0042E2A8`–`0x0042E2EF`, with RTS at `0x0042E2EE`. It has no direct calls, no RAM-global accesses, no loop or wait, no interrupt installation, and returns `D0 = 0` if its ordinary hardware accesses complete.

Its exact Phase-1 hardware setup is:

- PASEL `0xFFFFF403`: set PA0–PA6 to GPIO, preserve PA7;
- PADIR `0xFFFFF400`: set PA0–PA6 as outputs, preserve PA7;
- PAPUEN `0xFFFFF402`: disable internal pull-ups on PA0–PA6, preserve PA7;
- external latch `0x00600000`: write `0xFF`;
- PADATA `0xFFFFF401`: drive PA0–PA6 high, preserve PA7;
- PDSEL `0xFFFFF41B`: write `0xFF`, selecting GPIO on all Port D pins;
- PDDIR `0xFFFFF418`: write `0x00`, all Port D pins inputs;
- PDPUEN `0xFFFFF41A`: write `0x00`, internal row pull-ups off.

The keyboard electrical/software contract is now substantially defined:

- 15 scanned columns × 8 rows;
- the separate logical column code `0x8` is the power switch and is not part of the scanned matrix;
- external latch bits drive X1–X8; PA0–PA6 drive X9–X15; PD0–PD7 read Y1–Y8;
- normal scanning selects one column active-low at a time;
- no-key rows are expected externally pulled high (`PDDATA = 0xFF`); row-low means pressed;
- `Keyboard_GetNewKeyStates` complements PDDATA so the stored key bitmap is active-high.

Later helper inspection established that `KeyboardEnableKeyboardInterrupt` uses interrupt source `0x40`, drives every column low, and writes `PKBDINT = 0xFF`; the handler disables source `0x40`, clears `PKBDINT`, and sets the keyboard interrupt flag. These later helpers are not called by Phase 1.

`KeyboardInitializeModulePhase2` is separately linked at `0x0042E2F0`–`0x0042E3C3`. It initializes keyboard software globals and installs handler `0x0042EA74` through `InterruptInstallHandler(0x40, ...)`, but does not enable the keyboard interrupt. Phase 2 has not yet been executed.

Static study commit: `eb42b589a18644eeb02e5e1f131e6962f12768c4` (`as3k: correlate keyboard phase 1 statically`).

## Current machine model

The branch currently models or assumes:

- Motorola MC68EZ328 DragonBall EZ CPU;
- CPU clock configured as 16 MHz; hardware clock validation is still pending;
- 256 KiB RAM at `0x00000000`;
- 1 MiB Flash window at `0x00400000`;
- two KS0066-compatible LCD controllers connected through Port C;
- one composite 40×4 LCD screen;
- MAME software-list support inherited from the existing skeleton.

Historical AlphaSmart development material supports the production memory map of 256 KiB SRAM, 1 MiB Flash, and an external write-latch window at `0x00600000`. The current MAME driver still uses a static final memory map and a reset-time vector-copy workaround rather than dynamically reproducing all DragonBall chip-select remapping.

For keyboard-related MC68EZ328 state, PADIR/PADATA/PASEL and PDDIR/PDDATA/PDPUEN already have useful core support. PA output and PD input callbacks exist but are not connected by the AS3K driver. Static inspection found no core map entries for PAPUEN `0xFFFFF402`, PDSEL `0xFFFFF41B`, or PKBDINT `0xFFFFF41E`; exact consequences must be demonstrated dynamically before changing a core.

## Diagnostic systems

The branch contains ROM definitions for:

- `asma3k` — original AlphaSmart 3000 MAME system;
- `asma3kdi` — local invalid-applet startup diagnostic;
- `asma3kdv` — local valid-AlphaWord diagnostic without LCD devices;
- `asma3kdvl` — valid-AlphaWord diagnostic with the LCD bridge and both controller devices.

The corresponding diagnostic ROM/fixture files are **not distributed in this repository**. No AlphaSmart firmware, physical ROM dump, proprietary AlphaWord binary, or real KS0066 CGROM is committed here.

For controller-protocol testing, development used a locally generated 4096-byte all-zero `ks0066_f05.bin`. MAME intentionally reports a checksum warning for this synthetic file. It is useful only for controller/protocol tests, not character-shape fidelity.

## Reduced build

The development build used on macOS is:

```sh
make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2
```

The resulting executable is `./alphasma3k`.

The current subtarget contains four drivers: `asma3k`, `asma3kdi`, `asma3kdv`, and `asma3kdvl`.

## Known gaps

The emulator is not yet a complete usable AlphaSmart 3000. Important remaining work includes:

- executing and validating keyboard Phase 1;
- implementing the external byte-write latch at `0x00600000` once its dynamic necessity is demonstrated;
- connecting PA0–PA6 outputs and PD0–PD7 row inputs to an actual 15×8 matrix/input-port model;
- evaluating PAPUEN, PDSEL and PKBDINT core omissions only where execution evidence requires them;
- validating and then executing `KeyboardInitializeModulePhase2`;
- actual AlphaSmart key mapping (`INPUT_PORTS` is currently empty);
- later AlphaWord initialization modules and main-loop execution;
- dynamic fidelity of DragonBall chip selects/remapping;
- Port C pull-up register (`PCPUEN`) fidelity in the MC68EZ328 core if later behavior requires it;
- power/battery behavior;
- UART/RS-232, ADB, PS/2, IrDA and USB/PDIUSBD11D behavior;
- Flash update/write behavior;
- measured LCD timing/colors and confirmation of the exact original controller variant;
- synthetic visual fixture for explicit four-row rendering validation.

## Next development step

Execute only `KeyboardInitializeModule` Phase 1 on `asma3kdv` under the debugger. Observe the exact byte transactions to Port A, Port D and `0x00600000`, then stop at the caller return `0x00420168` before any later keyboard work.

The critical question is whether the currently unmapped write to `0x00600000` merely logs/continues or becomes the first hard execution dependency. No latch, input matrix, driver callback, or MC68EZ328 core change should be made until that narrow test establishes what is actually required.

The evidence-first method remains:

1. historical source/listing;
2. exact linked AlphaWord bytes and relocations;
3. narrow debugger test;
4. only then modify the driver when a demonstrated hardware gap requires it.

## Reproducibility and contribution notes

Development coordination files live in `docs/as3k/`:

- `STATUS.md` — this public project state/handoff;
- `CODEX_NEXT.md` — the next narrowly scoped local Codex task;
- `CODEX_RESULT.md` — the factual result of the most recently completed task.

`AGENTS.md` contains repository-specific development and safety rules.

Please keep proprietary ROMs, firmware, historical binaries and locally generated diagnostic ROM files out of Git. Source changes, public documentation, reproducible scripts, and factual test results are appropriate for this branch.
