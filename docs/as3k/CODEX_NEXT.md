# Current Codex task — study `KeyboardInitializeModule` before execution

Date: 2026-08-26

Read `AGENTS.md`, `docs/as3k/STATUS.md`, and the current `docs/as3k/CODEX_RESULT.md` first.

## Established state

The normal valid-AlphaWord path is now dynamically validated through the entry of:

`KeyboardInitializeModule = 0x0042E2A8`

The immediately preceding primary-module sequence has passed:

- `InterruptInitializeModule = 0x0043177E`;
- `TimerInitializeModule = 0x004310BE`;
- `LCDInitializeModule = 0x0043074E`;
- `SystemInstallExceptionVectors = 0x00430504`.

The latest dynamic test installed the expected bus/address/illegal/divide-by-zero vectors, preserved TRAP 0 and level-4/5/6 vectors, and reached `0x0042E2A8` without entering keyboard code.

Treat these hardware facts from the project evidence as hypotheses to verify against the keyboard source/listing, not as substitutes for that source:

- AS3K keyboard matrix is expected to be 15 columns × 8 rows;
- external write latch window is expected at `0x00600000` and supplies columns X1–X8;
- Port A lines are expected to supply columns X9–X15;
- Port D lines are expected to read rows Y1–Y8;
- `INTERRUPT_KEYBOARD_4` was previously identified from `InterruptModule.o.lst` as mask/value `0x40`.

The exact initialization behavior, polarity, GPIO register programming, interrupt setup, globals, and linked call graph must now be derived from the historical keyboard source/listing and exact Flash-linked AlphaWord bytes.

This task is **static only**.

**Do not execute `KeyboardInitializeModule`.**
**Do not modify MAME source.**
**Do not rebuild.**
**Do not implement input ports or the external latch yet.**

---

## A. Repository gate

From:

`~/Projects/alphasmart-as3k/mame0289`

1. `git pull --ff-only as3k-project as3k-mame0289-dev`.
2. Confirm branch is `as3k-mame0289-dev` and tracked status is clean.
3. Confirm the branch contains exception-vector validation commit `72b9935edfafe270e91becdd30fb7459c6f07a03` or a clean descendant.
4. Confirm public handoff `docs/as3k/STATUS.md` exists.
5. Run `git diff --check`.

Do not edit `src/mame/skeleton/alphasma3k.cpp`, `src/mame/mame.lst`, any MAME core, ROM definition, fixture, or generated file.

---

## B. Locate the original keyboard source and listings

Search the local archived AS3000 development material, especially `/Users/sperezc/Downloads/AlphaSmart.iso`, for the artifacts corresponding to keyboard initialization.

Prioritize original files such as:

- `Software/ModuleSources/KeyboardModule.c` and related headers;
- `KeyboardModule.o.lst` or equivalent object listing;
- `AWordRAM01.out`, map/symbol output, or linked listings used in prior stages;
- any hardware-definition headers referenced by the keyboard module, including DragonBall GPIO/register constants and write-latch definitions.

Report exact file paths used.

Identify and distinguish at minimum:

- `KeyboardInitializeModule`;
- `KeyboardInitializeModulePhase2` if present;
- keyboard interrupt handler(s);
- low-level matrix scan/read helper(s) directly relevant to initialization;
- any debounce/repeat/timer helper directly invoked during initialization.

Do not recursively reverse engineer the entire keyboard subsystem.

---

## C. Correlate the exact Flash-linked `KeyboardInitializeModule`

Using `AWordApplet02.bin` and the historical listing/symbol material, determine precisely:

1. entry address, expected `0x0042E2A8`;
2. exact end/return address and byte length;
3. every direct call made by the routine;
4. every global RAM variable written/read during initialization;
5. every MC68EZ328 internal register accessed;
6. every external hardware address accessed, especially the `0x0060xxxx` write-latch window if present;
7. every interrupt API call and exact interrupt constant/mask;
8. whether the routine changes SR/interrupt mask directly;
9. whether it contains loops, waits, asserts, or error paths that could hang on missing hardware;
10. whether linked Flash bytes match the historical source/listing after accounting for relocations.

Provide exact addresses and access widths (byte/word/long), not just symbolic names.

---

## D. Reconstruct the keyboard electrical/software model needed by initialization

From source/listing evidence, determine exactly how the firmware expects the AS3K keyboard interface to behave.

Document:

- number of matrix columns and rows actually referenced by this build;
- which columns are driven by the external latch and which by Port A;
- exact Port A bits used;
- exact Port D bits used for rows;
- direction/select/pull-up register values written for Port A and Port D;
- active polarity of column drive;
- active polarity of row/key detection;
- idle values expected with no key pressed;
- whether multiple columns can be active simultaneously;
- whether scanning uses one-hot, active-low, active-high, or another pattern;
- exact external latch write address(es) and values used by initialization, if any;
- whether the latch is write-only from firmware's perspective;
- whether keyboard input relies on Port D data, edge/interrupt status, both, or something else;
- which DragonBall interrupt source is used and how it is enabled/cleared/configured.

If some of these behaviors belong only to scan helpers and are not exercised during `KeyboardInitializeModule`, inspect only enough helper code to establish the hardware contract for the next emulator stage and state that boundary clearly.

---

## E. Distinguish Phase 1 from `KeyboardInitializeModulePhase2`

AlphaWord calls `KeyboardInitializeModulePhase2` later in `_AWord_Initialize`, after `SystemCheckBootSignature`.

Determine, from source/listing only:

- exact linked address of Phase 2;
- what hardware/software work is deferred to Phase 2;
- whether Phase 1 can complete independently of Phase 2;
- whether Phase 1 installs the interrupt handler or Phase 2 does;
- whether Phase 1 expects any key state or hardware event before returning.

Do not execute or fully reverse engineer Phase 2 in this task.

---

## F. Check current MAME support against the exact Phase-1 requirements

Inspect MAME 0.289 source only as needed. Do not edit it.

Compare the static keyboard requirements with the current driver/core:

- `src/mame/skeleton/alphasma3k.cpp` currently has empty `INPUT_PORTS`;
- the production write-latch behavior is not yet implemented in the driver;
- the MC68EZ328 core exposes Port A and Port D GPIO callbacks/register handlers.

For each hardware operation actually required by `KeyboardInitializeModule`, classify it as:

1. already modeled correctly;
2. modeled by the MC68EZ328 core but not connected in the AS3K driver;
3. absent from the AS3K driver and requiring a local device/latch/input implementation;
4. potentially absent/incorrect in the MC68EZ328 core itself.

Do not infer a core bug merely because the AS3K driver has not connected a callback.

If the keyboard initializer touches a register the core does not implement, identify the exact register and source evidence.

---

## G. Design the next narrow dynamic test, but do not run it

Based on the static result, specify an exact debugger plan for the following task.

The next test should execute **only `KeyboardInitializeModule` Phase 1**, observe its hardware/global writes, and stop immediately after it returns, before the next higher-level initialization work proceeds.

Design breakpoints/watchpoints for:

- entry `0x0042E2A8`;
- exact Port A/Port D registers written/read by Phase 1;
- exact external latch range if Phase 1 touches it;
- exact keyboard globals initialized;
- `InterruptInstallHandler` call/result if used;
- keyboard interrupt handler breakpoint as a fail/safety condition if it should not fire during initialization;
- assert/error helper if one exists;
- exact return/next-call boundary determined from the linked caller.

Use explicit `0x...` numeric literals throughout.

State the exact expected values for every watched register/global where source/listing evidence makes them deterministic.

If Phase 1 is predicted to hit missing latch/input hardware before returning, design the test to stop on that first demonstrated dependency rather than adding an emulator fix preemptively.

---

## H. Public-documentation implications

Do not edit `STATUS.md` in this task, but include in `CODEX_RESULT.md` a concise section titled `Public handoff implications` containing:

- the new execution/hardware facts another developer would need;
- the exact first missing emulator component, if one is identified;
- what remains unproven;
- the recommended next dynamic test.

This lets the coordinating step update the public status document without relying on chat history.

---

## I. Publication

This is a no-source-change task.

At the end:

1. `git diff --check`.
2. `git status --short`.
3. Confirm no MAME source, ROM, fixture, proprietary artifact, diagnostic binary, or generated file changed.
4. Replace `docs/as3k/CODEX_RESULT.md` with a factual report containing at least:
   - exact source/listing artifacts used;
   - linked address range and return;
   - direct calls and relevant helper symbols/addresses;
   - all RAM globals initialized;
   - all Port A/Port D/internal-register accesses;
   - external-latch accesses;
   - matrix geometry and polarities supported by evidence;
   - interrupt setup and handler address;
   - Phase-1 versus Phase-2 responsibilities;
   - current MAME support/gaps classified as above;
   - exact next debugger test plan;
   - `Public handoff implications` section;
   - `git diff --check` and final status.
5. Commit only the safe documentation result.
6. Push only to `as3k-project/as3k-mame0289-dev`.

## Stop condition

Stop after `KeyboardInitializeModule` Phase 1 is fully correlated statically and the next dynamic test is designed.

**Do not execute keyboard code. Do not implement keyboard/latch/input ports. Do not modify MAME source or cores.**
