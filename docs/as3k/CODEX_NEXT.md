# Current Codex task — audit MC68EZ328 keyboard GPIO gaps before implementation

Date: 2026-08-26

Read `AGENTS.md`, `docs/as3k/STATUS.md`, and the current `docs/as3k/CODEX_RESULT.md` first.

## Established dynamic result

`KeyboardInitializeModule` Phase 1 has now executed completely on `asma3kdv` and returned normally to `0x00420168` with `D0 = 0`.

Observed transactions matched the historical routine exactly:

- PASEL `0xFFFFF403`: read `0x00`, write `0x7F`;
- PADIR `0xFFFFF400`: read `0x00`, write `0x7F`;
- PAPUEN `0xFFFFF402`: read `0x00`, write `0x00`;
- external latch `0x00600000`: write `0xFF`;
- PADATA `0xFFFFF401`: read `0x00`, write `0x7F`;
- PDSEL `0xFFFFF41B`: write `0xFF`;
- PDDIR `0xFFFFF418`: write `0x00`;
- PDPUEN `0xFFFFF41A`: write `0x00`.

The latch write and PAPUEN accesses were logged as unmapped but were nonfatal. PASEL accesses also produced aligned unmapped diagnostics at `0xFFFFF402`; PDSEL did not produce an unmapped message but direct debugger readback remained zero. Direct debugger byte readbacks at the Phase-1 RTS were zero for the listed GPIO registers even where watchpoints had observed writes.

No exception handler, keyboard interrupt handler, `InterruptInstallHandler`, or Phase-2 entry fired.

A direct source check after the run has already shown an important point that must now be audited carefully: `base_internal_map()` contains PADIR/PADATA and the Port-D registers, while `PASEL` is explicitly added in `mc68328_device::internal_map()` but not obviously in `mc68ez328_device::internal_map()`. Do not assume the previous static classification of PASEL support was correct.

This task is **static analysis only**. Do not run MAME, rebuild, or modify source.

## Goal

Determine exactly what the MAME 0.289 MC68EZ328 core implements or omits for the keyboard-related Port A/Port D registers, explain the observed Phase-1 diagnostics/readbacks, and design the minimum technically correct next implementation step.

The audit must distinguish:

1. a driver-only missing component (`0x00600000` external latch / matrix callbacks);
2. MC68EZ328 core mapping/state omissions;
3. registers whose behavior can reuse existing base-class handlers/state;
4. registers that require genuinely new state/interrupt semantics.

Do not patch anything in this task.

---

## A. Repository gate

From `~/Projects/alphasmart-as3k/mame0289`:

1. `git pull --ff-only as3k-project as3k-mame0289-dev`.
2. Confirm branch `as3k-mame0289-dev`, clean tracked status, and `git diff --check`.
3. Confirm the latest `CODEX_RESULT.md` contains the successful dynamic Phase-1 test.
4. Do not rebuild and do not execute any system.

---

## B. Audit the exact MAME 0.289 core implementation

Inspect at minimum:

- `src/devices/machine/mc68328.cpp`
- `src/devices/machine/mc68328.h`

Trace all relevant declarations, state variables, save-state registration, reset defaults, handlers, callback gating, and map entries for:

### Port A

- PADIR `0xFFFFF400`
- PADATA `0xFFFFF401`
- PAPUEN `0xFFFFF402`
- PASEL `0xFFFFF403`

### Port D

- PDDIR `0xFFFFF418`
- PDDATA `0xFFFFF419`
- PDPUEN `0xFFFFF41A`
- PDSEL `0xFFFFF41B`
- PDPOL `0xFFFFF41C`
- PDIRQEN `0xFFFFF41D`
- PKBDINT `0xFFFFF41E`
- PDIRQEDGE `0xFFFFF41F`

For every register, report:

- whether it is mapped for `mc68ez328_device`, not merely for the original `mc68328_device`;
- handler name if present;
- backing state variable if present;
- reset value;
- whether state is registered for save-state;
- effect on output callbacks or input reads;
- interrupt behavior if applicable.

Pay special attention to inheritance: `base_internal_map(0xFFFFF000, map)` versus additions made only by `mc68328_device::internal_map()`.

## C. Explain the Phase-1 observations

Using the actual handler code, explain why the dynamic run could observe bus writes yet debugger readback at RTS showed zero for PADIR/PADATA/etc.

Do not guess. Determine whether the cause is one or more of:

- register genuinely unmapped;
- read handler masks/gates with select/direction state;
- debugger `b@` access behavior;
- callbacks unset;
- another implementation detail visible in the core.

If static source alone cannot prove the readback explanation, state exactly what remains unproven and design one future narrow debugger check; do not execute it now.

## D. Verify against AlphaSmart/DragonBall definitions

Use the local historical AS3000 material and available Motorola register definitions already archived in the project, especially `M68328EZ.h` and any locally available MC68EZ328 manual/documentation.

For PASEL, PAPUEN, PDSEL and PKBDINT, establish:

- documented address;
- bit meaning relevant to the AS3K;
- reset value if available;
- read/write semantics;
- whether the existing MAME base-class model has an analogous implementation that can be reused safely.

Do not broaden into unrelated DragonBall peripherals.

## E. Determine the minimum implementation boundary

Produce a concrete implementation plan, but do not implement it.

Classify each needed change as one of:

- **driver-only** — belongs in `alphasma3k_state` / AS3K address map or input model;
- **MC68EZ328 core mapping fix** — existing state/handler can be mapped for EZ;
- **MC68EZ328 core state/behavior addition** — new register state/semantics required;
- **defer** — not required until later keyboard interrupt behavior.

The plan must specifically answer:

1. Is adding `PASEL` to `mc68ez328_device::internal_map()` sufficient for Port-A select behavior, or is more required?
2. Does PAPUEN need new state/handlers, and does AS3K Phase 1 functionally depend on it beyond fidelity?
3. Does PDSEL need new state/handlers, or is Port D effectively hardwired GPIO in the current core?
4. What exact role should PKBDINT have, and can it be deferred until `KeyboardEnableKeyboardInterrupt` is reached?
5. Can the external `0x00600000` latch be modeled entirely in the AS3K driver as an 8-bit write latch with saved state?
6. What callback/matrix structure will eventually be required for PA0–PA6 output columns and PD0–PD7 input rows, without implementing key mappings yet?

Prefer the smallest correct patch sequence. Do not combine core fixes, latch, matrix, and Phase 2 into one future implementation if they can be validated independently.

## F. Publication

Replace `docs/as3k/CODEX_RESULT.md` with a factual report containing:

- exact files and historical references inspected;
- register-by-register MC68EZ328 support table;
- explanation of the dynamic Phase-1 unmapped messages/readbacks;
- confirmed inaccuracies, if any, in the previous static support classification;
- minimum patch plan by layer (driver vs core);
- which issues can safely be deferred;
- proposed next single implementation/test stage;
- `git diff --check` and final status.

Commit only the documentation result and push only to `as3k-project/as3k-mame0289-dev`.

Do not change MAME source, `mame.lst`, ROM definitions, fixtures, binaries, or diagnostic files.

## Stop condition

Stop after the static core audit and implementation plan.

**Do not execute keyboard Phase 2 and do not implement any keyboard/core change in this task.**
