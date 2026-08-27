# AlphaSmart 3000 emulator — development status

Last updated: 2026-08-26

This document is the public handoff for AlphaSmart 3000 emulator development on branch `as3k-mame0289-dev` of `SamDelorean/mame-as3k`.

The branch is based on exact MAME 0.289 commit `f34f02505e32c1993c6a782b6814232cbfc74e36` and is intentionally separate from `master`.

## Current execution frontier

The valid-AlphaWord diagnostic path has executed through **`KeyboardInitializeModule` Phase 1** and returned normally to its caller at `0x00420168` with `D0 = 0`.

`KeyboardInitializeModulePhase2 = 0x0042E2F0` has **not** been executed.

The MC68EZ328 keyboard-related GPIO audit and the first driver-only implementation stage are complete. The AS3K board's external 8-bit keyboard column latch at `0x00600000` is mapped, saved, deterministically reset, and validated through the existing Phase-1 path. The matrix, key mappings, Phase 2, and CPU-core changes remain deferred.

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

## Known gaps

The emulator is not yet a complete usable AlphaSmart 3000. Remaining work includes EZ PAPUEN/Port-A fidelity, PA/latch-to-PD matrix plumbing, PDSEL, later PDKBEN keyboard interrupt behavior, actual MAME input-port/key mapping, Phase 2 and later AlphaWord modules, dynamic chip-select/remapping fidelity, power/battery behavior, UART/RS-232, ADB, PS/2, IrDA, USB/PDIUSBD11D, Flash update/write behavior, measured LCD timing/colors, exact controller confirmation, and an explicit synthetic four-row visual fixture.

## Next development step

Implement **only** the next EZ Port-A core-fidelity stage:

- add PAPUEN state, reset/save handling, mapping, and Port-A input pull-up fallback;
- make the existing `SCR.WDTH8` Port-A selection state accurate and saveable without adding a fake EZ PASEL register at reserved address `0xFFFFF403`;
- validate this core change independently before adding driver matrix callbacks or key mappings;
- do not execute Phase 2 or add PDSEL/PDKBEN interrupt behavior in the same stage;
- preserve the validated external latch, `asma3kdi`/`asma3kdv` startup behavior, and LCD bridge behavior.

The evidence-first development method remains: original source/listings → exact linked behavior → narrow dynamic test → static core audit where needed → minimum demonstrated implementation → regression test → public documentation.

## Reproducibility and contribution notes

Development coordination files live in `docs/as3k/`: `STATUS.md`, `CODEX_NEXT.md`, and `CODEX_RESULT.md`. `AGENTS.md` contains repository-specific scope and safety rules.

Keep proprietary ROMs, firmware, historical binaries and generated diagnostic ROMs out of Git. Public source changes, documentation, reproducible scripts and factual test results are appropriate for this branch.
