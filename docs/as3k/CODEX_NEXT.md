# Current Codex task — implement and validate only the AS3K external keyboard latch

Date: 2026-08-26

Read `AGENTS.md`, `docs/as3k/STATUS.md`, and the current `docs/as3k/CODEX_RESULT.md` first.

## Established evidence

The valid-AlphaWord path has executed through `KeyboardInitializeModule` Phase 1 and returned normally to caller `0x00420168` with `D0 = 0`.

The exact Phase-1 transaction at the board latch is:

`write byte 0xFF to 0x00600000`

The previous dynamic run showed this access as unmapped but nonfatal:

`unmapped program memory write to 00600000 = FFFF & FF00`

Historical AS3K material and the production chip-select values establish the external write-latch region at `0x00600000` with a 32 KiB chip-select window. The physical component is an 8-bit latch (74HC574-class board logic) whose eight outputs are keyboard columns X1–X8. PA0–PA6 provide X9–X15; PD0–PD7 are rows Y1–Y8.

The MC68EZ328 audit is complete. Treat these conclusions as established:

- `0x00600000` is **driver/board logic**, not DragonBall core state;
- PAPUEN, PDSEL and later PDKBEN/PKBDINT are separate core-fidelity stages;
- EZ address `0xFFFFF403` is reserved in the processor manual and must **not** be implemented as a fake PASEL register;
- no keyboard matrix plumbing or key mappings should be added in this stage;
- Phase 2 must not be executed in this stage.

This task is the **first single implementation stage from the audit plan**: external latch only.

---

## A. Repository gate

From `~/Projects/alphasmart-as3k/mame0289`:

1. `git pull --ff-only as3k-project as3k-mame0289-dev`.
2. Confirm branch is `as3k-mame0289-dev`.
3. Confirm tracked status is clean.
4. Confirm HEAD contains audit commit `08816104002f27365ace1798eddd06b78550c764` and the current public `STATUS.md` says the next stage is the external latch.
5. Run `git diff --check`.

Do not touch `master`.

## B. Inspect the existing driver/map before editing

Inspect `src/mame/skeleton/alphasma3k.cpp` and determine the exact program-space bus width and existing address-map style.

Also inspect the historical AS3K source/material already available locally only as needed to confirm how the write latch is addressed. Do not publish proprietary files.

Important mapping constraint:

- the board chip-select window is `0x00600000–0x00607FFF`;
- the actual peripheral is an 8-bit write latch;
- the observed Phase-1 byte transaction at `0x00600000` appears on the high byte lane (`mem_mask 0xFF00`) of the 16-bit CPU bus.

Implement the smallest mapping that correctly represents the proven board decode and byte-lane behavior. Do not invent readable register semantics if the hardware/source evidence only establishes a write latch.

If source evidence shows the latch is mirrored throughout the 32 KiB CS window, map the window accordingly. If only address `0x00600000` is demonstrably decoded, use the narrowest supported mapping and state the uncertainty in the result. Do not guess silently.

## C. Implement only the external latch in `alphasma3k_state`

Add only what is needed for this board component:

1. one 8-bit latch state variable;
2. `save_item()` registration;
3. deterministic initialization/reset;
4. a write handler and program-map entry at the supported decode/window;
5. optional narrow logging only if useful for validation and not noisy.

Reset-value rule:

- the physical power-on value has not been established;
- choose a deterministic emulator reset value appropriate for safe/no-column-active behavior, preferably `0xFF` if consistent with the scan polarity and existing Phase-1 behavior;
- explicitly document it as an **emulator initialization choice**, not a measured hardware reset fact.

The handler must retain the written byte so the Phase-1 `0xFF` write can be demonstrated as mapped and stored.

Do **not** in this task:

- connect latch bits to a keyboard matrix;
- add PA callbacks;
- add PD callbacks;
- add `INPUT_PORTS` keys;
- implement PAPUEN;
- implement PDSEL;
- implement PDKBEN/PKBDINT;
- modify `mc68328.cpp` or `mc68328.h`;
- execute `KeyboardInitializeModulePhase2`;
- modify LCD behavior;
- modify diagnostic ROM definitions unless absolutely required (it should not be).

## D. Build and static checks

Run:

```sh
make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2
```

No clean build.

Then verify:

- build succeeds;
- four drivers are still present;
- `git diff --check` passes;
- `asma3kdi` and `asma3kdv` ROM audits remain good with the existing local fixtures.

Do not run original `asma3k`.

## E. Narrow dynamic validation

Create a local-only debugger script under `../diagnostic/`, for example:

`as3kdv_keyboard_latch.cmd`

Use explicit `0x...` literals.

Run only `asma3kdv` with an 8-second safety limit.

The script must:

1. mark Phase-1 entry `0x0042E2A8`;
2. watch the exact latch transaction at `0x00600000`;
3. verify the write data is `0xFF`;
4. immediately after the write, prove the driver's saved latch state has become `0xFF` using the narrowest practical evidence available (handler log, debugger-visible mapping/state if exposed, or a temporary local diagnostic observation that is removed before commit);
5. confirm the access no longer produces the previous unmapped `0x00600000` message;
6. continue through the Phase-1 RTS and caller return `0x00420168` with `D0 = 0`;
7. include safety breaks for Phase 2, keyboard interrupt handler, `InterruptInstallHandler`, and the established exception handlers.

Do not scan keys and do not continue beyond the caller return.

If the new mapping causes an exception, wrong byte-lane behavior, repeated/mirrored writes inconsistent with evidence, or regression before caller return, stop and report before broadening the patch.

## F. Regression boundary

Confirm that this driver-only latch change does not alter previously validated unrelated behavior:

- `asma3kdi` startup diagnostic remains valid/auditable;
- `asma3kdv` valid-AlphaWord path still reaches and returns from Phase 1;
- no MAME core file changed;
- LCD bridge source is unchanged except unavoidable context lines.

A full re-run of every LCD debugger script is not required unless the code diff unexpectedly touches LCD-related logic.

## G. Publication and public handoff

At the end:

1. run `git diff --check`;
2. inspect `git status --short`;
3. inspect the exact diff;
4. ensure no `roms/`, proprietary firmware, historical binary/source archive, CGROM, diagnostic fixture, local debugger script/log, generated binary, or build artifact is staged;
5. replace `docs/as3k/CODEX_RESULT.md` with a factual report containing:
   - exact driver changes;
   - exact map range and byte-lane handling chosen, with evidence;
   - reset value and explicit statement that it is an emulator choice if hardware reset is unknown;
   - build/audit result;
   - observed latch write and retained value;
   - whether the old unmapped message disappeared;
   - Phase-1 RTS/caller-return result and D0;
   - safety-break result;
   - final `git diff --check` and status;
6. update `docs/as3k/STATUS.md` only if necessary to keep the public handoff factual after the implementation;
7. commit only safe source/documentation changes;
8. push only to `as3k-project/as3k-mame0289-dev`.

## Pass criteria

Pass means all of the following are true:

- the external board latch is modeled in `alphasma3k_state` without a core modification;
- Phase 1 writes `0xFF` through the correct byte lane and the latch retains `0xFF`;
- the old unmapped `0x00600000` message is gone;
- Phase 1 still returns normally to `0x00420168` with `D0 = 0`;
- no Phase 2/interrupt/error handler is entered;
- build and existing diagnostic audits pass;
- only safe driver/documentation changes are committed and pushed.

## Stop condition

Stop after validating the external latch and publishing the result.

**Do not add the keyboard matrix, key mappings, PAPUEN, PDSEL, PDKBEN/PKBDINT, or execute Phase 2 in this task.**
