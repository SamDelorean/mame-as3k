# AS2000 Emulation Evidence Audit — 2026-09-16

This document is a factual supplement to `docs/as2k/EMULATION_FINDINGS.md`.
It consolidates reproducible AS2000 hardware/firmware observations that are useful to emulator development but were not yet stated there with sufficient precision.

It does not change the emulator worker, its control contract, task order, firmware objectives, or Gate 1A state. Proprietary ROM/NVRAM contents are not included.

## Baseline machine model

Current AS2000 work uses AlphaSmart 2000 firmware v3.1.4 as the primary behavioral reference.

The machine model relevant to current evidence is:

- MC68HC11D0-class CPU at 8 MHz in the current MAME model.
- 128 KiB banked NVRAM/SRAM, exposed as four 32 KiB banks.
- 128 KiB dictionary ROM (M27C1001-class organization in current hardware documentation), exposed as eight 16 KiB logical banks.
- main firmware remains visible in `$8000-$FFFF`.
- lower `$0000-$7FFF` behavior depends on PA6 and therefore cannot be treated as one flat address space.
- the current display model uses two KS0066-compatible LCD controllers.

These are emulator-model facts/working hardware findings; where physical PCB continuity has not yet been measured, the logical behavior proven by firmware/runtime tests should be preserved without overstating physical net ownership.

## Lower memory view and bank geometry

PA6 selects the lower memory view:

- PA6 RAM view: `$0000-$7FFF` exposes the selected 32 KiB SRAM bank.
- PA6 I/O/ROM view: decoded peripherals and DictROM occupy portions of the same CPU range.

PA4/PA5 select the four legacy 32 KiB RAM banks. End-to-end runtime tests have exercised all eight legacy files and produced persistent sentinels at the expected physical offsets:

- F1: `0x069D`
- F2: `0x8100`
- F3: `0xC000`
- F4: `0x10100`
- F5: `0x14000`
- F6: `0x18100`
- F7: `0x1B000`
- F8: `0x1E000`

This validates more than key bindings: it jointly exercises File-key behavior, PA4/PA5 RAM banking, the legacy allocator/layout and NVRAM persistence.

Additional legacy layout landmarks useful for emulator regression work include:

- bank 0 workspace: `$0100-$069C`
- bank 3 signature/tail: `$7FDF-$7FFF`

These locations should be treated as observed legacy behavior, not as a proposal for a future filesystem.

## MMIO read/write overlays

The AS2000 decode is not adequately represented by a flat RAM/ROM map.

Important current semantics include:

- `$2000`: keyboard read / high matrix-selector behavior; the same logical latch also participates in LCD control.
- `$4000`: LCD/Dict control latch.
- `$9000`: low keyboard-selector write overlay; ROM remains visible for the corresponding read behavior in the current model.

Therefore emulator refactoring must preserve access direction and side effects. In particular, a write-only overlay must not accidentally replace the ROM-visible read path.

## LCD interface details

Current logical LCD behavior gives the emulator several concrete signals to preserve:

- CPU PORTD bits PD2-PD5 carry the four-bit LCD data bus D4-D7 in the current model.
- `$2000` high-latch state participates in LCD RS selection.
- the same high-latch model includes the LCD read/write selection state.
- `$4000` bit 0 is the logical enable for LCD controller 0.
- `$4000` bit 1 is the logical enable for LCD controller 1.
- rising enable behavior is significant in the current `lcd_ctrl_w` path.
- `$4000` bit 7 participates in DictROM bank selection.

The existing emulator has already demonstrated editing across the boundary between both LCD controllers and persistent recovery of that state. This supports preserving the current in-project LCD architecture rather than replacing it speculatively.

Physical PCB continuity for every LCD control net remains a separate hardware-verification question.

## DictROM functional evidence

The dictionary path is functionally exercised, not merely inferred from bank writes.

With fresh NVRAM, entering `teh` and invoking Spell Check produced real suggestions including:

- `eh`
- `tee`
- `tea`
- `ten`
- `tech`
- `the`

This is end-to-end evidence that the firmware path, DictROM banking/content and LCD presentation cooperate in the current emulator.

DictROM uses eight logical 16 KiB banks in `$4000-$7FFF`, selected by the interaction of PA4/PA5 and `$4000` bit 7. Any future emulator cleanup should preserve this functional result, not merely reproduce a bank number.

## Keyboard matrix evidence

The AS2000 driver models 16 active-low keyboard columns. The keyboard read combines selected columns according to the matrix selector; normal key transitions assert the HC11 IRQ path.

Existing runtime tests have demonstrated real IRQ transitions for ordinary keyboard combinations, and the Send key has now been resolved as:

- keycode `$47`
- `COL.7 / 0x10`
- current host binding `F12`

Recent Gate 1A harness work also resolved normal automated selection for Send, Print, F1-F8, Find, Clear/Recover and SpellCheck. This is useful emulator evidence because future failures after successful field resolution should not automatically be classified as missing MAME input bindings.

The new harness surrounds physical input operations with `INPUT_START` / `INPUT_END` temporal markers. Those markers are diagnostic boundaries only; they must not inject a firmware PC or bypass matrix/IRQ behavior.

## HC11 STOP / wake evidence

A separate emulator-fidelity issue has been isolated around HC11 STOP wake behavior.

Observed firmware/runtime landmarks:

- normal firmware sleep is observable around PC `$87D7`.
- ordinary keyboard IRQ wake works: a key wakes the machine and enters the expected IRQ path (observed around `$8933`).
- the current HC11 core handling of a masked XIRQ is insufficient for the AS2000 periodic-wake behavior when CCR.X is set.
- code inspection showed `check_irq_lines()` discarding XIRQ while CCR.X=1, while the AS2000 behavior under test requires XIRQ to wake STOP even when masked, without taking the XIRQ ISR/vector.

A diagnostic-only synthetic wake experiment produced a strong firmware-side result:

- 148 masked-XIRQ-style resumptions advanced the idle/auto-off state to `$004A = $94`.
- firmware subsequently cleared the `$4000` control state and reached the power-off area near PC `$87ED`.

This indicates that the observed auto-off stall is an emulator wake-source/core-fidelity issue rather than evidence that the firmware auto-off logic is broken.

The synthetic 1.25 s wake period used during diagnosis is **not** established as the physical AS2000 period and must not be promoted to production emulation without hardware measurement. The physical source/timing of the periodic wake remains unresolved.

A correct regression for the core behavior should distinguish:

1. keyboard IRQ wakes STOP and vectors normally;
2. masked XIRQ wakes STOP but does not spuriously vector through `$FFF4`;
3. normal STOP behavior remains unchanged when no wake source is asserted.

## HC11 register `$0001`

An older AS2000 note treated access to HC11 register `$0001` as a possible missing-register blocker.

Current source inspection indicates the MC68HC11D0 implementation already handles this through `reg01_r()` and returns `0xff` in the relevant model. Therefore the historical `$0001` TODO should not be treated as the primary current boot/emulation blocker unless new runtime evidence contradicts the present implementation.

## Boot, NVRAM, sleep and wake baseline

Runtime work has already established a useful minimum regression baseline:

- v3.1.4 boots in the current emulator.
- 128 KiB NVRAM is initialized/used by the firmware.
- the firmware enters its sleep path.
- a normal keyboard event can wake it.
- editor text can be entered and recalled.
- state has survived separate MAME processes in LCD/file-switching tests.

Consequently a future change that only reaches the startup screen but breaks sleep/wake, file banking, recall or persistence is a regression, not an equivalent implementation.

## Wired-host and SCI implications

The retained wired regions remain:

- `$8606-$8709` — Send / wired path
- `$887E-$8F04` — Macintosh/ADB transport
- `$AA26-$AB09` — PC/two-wire transport

Retained communication uses PORTD bits 0-1 and PORTA bits 0/2, and retained host code uses HC11 timer/capture-compare resources. These resources must not be globally disabled as an IrDA-removal shortcut.

For emulator work, reaching `$8606` establishes firmware routing but does not by itself establish faithful external wired transmission. If execution reaches the correct wired routine and then stalls or produces incorrect signaling, HC11 SCI/timer/PORT behavior becomes an emulator-fidelity candidate rather than a reason to alter firmware routing.

## Runtime evidence now available to the emulator worker

The MAME development branch contains debugger-free Gate 1A-oriented PC instrumentation capable of marking execution of:

- `$9716` — Send dispatch
- `$8606` — wired Send
- `$962D` / `$9804` — Print caller landmarks
- `$ABC9` — retained Print
- `$D099`, `$D2DC`, `$D437` — forbidden IrDA-path landmarks for the relevant tests
- `$D488-$D498` — retained shared helper exception
- `$E102-$E103` — protected structural metadata area

The instrumentation is diagnostic. It should remain observational and must not force the CPU into any of these addresses.

Recent input-resolver tests pass their normal and negative fixtures, including ambiguous names, aliases, absent characters, API failure and malformed UTF-8 cases. This narrows the next useful runtime question to actual firmware/peripheral behavior rather than repeatedly rediscovering the same input-field problem.

## Evidence hierarchy for future emulator changes

When evidence conflicts, prefer in this order:

1. reproducible normal-path runtime behavior with the v3.1.4 baseline;
2. reproducible diagnostic runtime behavior that does not alter machine state;
3. source-level behavior of the current MAME driver/core;
4. firmware static analysis and historical comparison;
5. logical hardware interconnection inference;
6. unverified physical-net hypotheses.

Keep physical-net claims explicitly provisional until continuity/schematic evidence exists.

## High-value unresolved emulator questions

The following remain useful emulator questions, without changing the worker priority order:

- demonstrate normal Send input causally reaching `$9716` and then `$8606`;
- demonstrate normal Print input reaching `$962D` or `$9804` and then `$ABC9`;
- determine whether the next wired-Send limitation is SCI, timer/capture-compare, PORT behavior or host-side integration;
- implement/validate the minimum correct masked-XIRQ STOP-wake semantics without encoding the unmeasured synthetic wake period as hardware fact;
- complete broader keyboard-matrix regression coverage beyond the currently proven keys;
- preserve all four legacy RAM banks and eight file regions across reset/restart tests;
- preserve eight-bank DictROM and real SpellCheck behavior;
- retain the current dual-controller LCD behavior while extending regression coverage for wrapping, scrolling and charset behavior;
- distinguish logical emulator-model facts from still-unmeasured physical PCB connectivity.
