# Current Codex task — study `SystemInstallExceptionVectors` before execution

Date: 2026-08-26

Read `AGENTS.md` and the current `docs/as3k/CODEX_RESULT.md` first.

## Established state

The AS3K LCD path is now validated through `LCDInitializeModule` and the composite 40×4 screen infrastructure is present:

- atomic Port C bridge validated;
- two KS0066 devices complete LCD initialization with finite busy polling;
- composite screen is 240×36, rows 1–2 from E1/`ks0066_0`, rows 3–4 from E2/`ks0066_1`;
- both controllers are configured visually as 2×40;
- no-LCD and KS0066 regressions pass;
- execution is still intentionally stopped at the `LCDInitializeModule` RTS `0x0043079e`.

The next real AlphaWord primary-module call is:

`SystemInstallExceptionVectors = 0x00430504`

followed by:

`KeyboardInitializeModule = 0x0042e2a8`.

The synthetic four-row visual fixture is deliberately deferred; do not create it in this task.

## Goal

Perform an **evidence-first static study** of `SystemInstallExceptionVectors` and any directly called exception handlers/helpers so the following task can execute it with exact watchpoints and expected RAM/vector values.

This task is static only.

**Do not modify source code.**
**Do not rebuild.**
**Do not run an emulated AS3K system.**
**Do not continue into keyboard.**

---

## A. Repository / source gate

From:

`~/Projects/alphasmart-as3k/mame0289`

1. `git pull --ff-only as3k-project as3k-mame0289-dev`.
2. Confirm branch is `as3k-mame0289-dev` and tracked status is clean.
3. Confirm current branch includes composite-LCD implementation commit `38528ee8ffca9ff718d87e88145cae92d9e0134a` or a clean descendant.
4. Do not edit `alphasma3k.cpp` or any MAME source.

---

## B. Locate the original AlphaSmart source/listing

Search the local archived AS3000 material for the source and linked listing corresponding to `SystemInstallExceptionVectors`.

Prioritize original artifacts such as:

- `ModuleSources/SystemModule.c` / related headers;
- `SystemModule.o.lst` or equivalent linked listing;
- `AWordRAM01.out` / symbol map;
- AlphaWord `.map`, `.lst`, or linker outputs already used in prior tasks.

Report the exact source function and any constants/types it depends on.

If the standalone source is absent, use the linked listing and symbol map and state that explicitly.

---

## C. Correlate the exact Flash-linked function

Using the local AlphaWord fixture/listing artifacts, determine precisely:

1. start address: expected `0x00430504`;
2. exact end/RTS address and byte length;
3. every direct call made by the function;
4. every RAM address written;
5. every exception-vector entry written;
6. exact handler address stored in each vector;
7. whether writes are byte/word/long and expected big-endian debugger transaction behavior;
8. whether the function touches any MC68EZ328 internal hardware register (`0xffffxxxx`) or external hardware window (`0x0060xxxx`);
9. whether it modifies SR, stack pointer, VBR-like state, interrupt mask, or any CPU-specific control state;
10. whether any helper can fail/assert/loop.

Compare the linked Flash bytes against the historical source/listing and account for relocations exactly as done for Interrupt/Timer/LCD.

Do not infer names from addresses if the symbol/listing evidence is available.

---

## D. Reconstruct the exception-vector table changes

Build a precise table with at least:

`vector number -> vector-table RAM address -> handler symbol -> handler Flash address -> write size`

Use 68k vector numbering correctly: vector N occupies `N * 4` in the vector table unless the AlphaSmart source explicitly does something different.

Determine whether the function installs handlers for, for example, bus error, address error, illegal instruction, divide-by-zero, CHK, TRAPV, privilege violation, trace, line-A/F, spurious interrupt, traps, or only a subset. Do not assume the list; derive it from the source/bininary.

Also distinguish any vectors already written by earlier modules:

- Startup Manager TRAP 0 setup at RAM `0x00000080` if still relevant;
- InterruptInitializeModule level 4/5/6 vectors at RAM `0x00000110`, `0x00000114`, `0x00000118`.

Report whether `SystemInstallExceptionVectors` overwrites any previously established vector or leaves them intact.

---

## E. Inspect the installed handlers just enough for execution planning

For each handler installed by `SystemInstallExceptionVectors`, inspect only enough static code/source to answer:

- does it immediately return (`RTE`) or enter a diagnostic/reset/assert path?
- does it reference hardware not yet emulated?
- does it write a recognizable RAM diagnostic/signature?
- does it intentionally loop?
- is any handler expected to execute during normal initialization, or are these only safety vectors?

Do **not** recursively reverse engineer entire error subsystems. Stop once normal-path implications are clear.

---

## F. Check MAME 68000/MC68EZ328 compatibility relevant to this routine

Inspect MAME 0.289 source only as needed to determine whether the exact operations used by `SystemInstallExceptionVectors` require anything beyond ordinary RAM vector writes and normal 68k execution.

In particular verify whether the MC68EZ328/68000 model uses the base vector table at address 0 (no relocatable VBR in this CPU generation) for the vectors being installed, unless source/core evidence shows otherwise.

Identify any credible emulator limitation that could affect this function itself. Do not propose fixes without evidence.

---

## G. Design the next narrow dynamic test, but do not run it

Based on the static result, specify an exact debugger plan for the next task:

- entry breakpoint at `0x00430504`;
- watchpoints on the exact vector-table RAM ranges written by the function;
- any helper/return breakpoint needed;
- final breakpoint at `KeyboardInitializeModule = 0x0042e2a8`, stopping before keyboard executes;
- expected final values at every changed vector entry.

Use explicit `0x...` literals in all proposed MAME debugger commands/lengths.

If long writes will appear as two 16-bit debugger transactions, state the expected high/low halves.

The dynamic test should prove only that `SystemInstallExceptionVectors` writes the expected vectors and returns into `KeyboardInitializeModule` without exception or hang.

Do not run this test now.

---

## H. Publication

This is a no-source-change task.

At the end:

1. `git diff --check`.
2. `git status --short`.
3. Confirm no MAME source changed.
4. Replace `docs/as3k/CODEX_RESULT.md` with a factual report containing:
   - exact source/listing artifacts found;
   - exact linked range and RTS;
   - every direct call/helper;
   - complete vector-write table;
   - whether earlier Startup/Interrupt vectors are preserved or overwritten;
   - handler addresses and concise normal-path behavior;
   - any hardware accesses or emulator risks;
   - exact next debugger/watchpoint plan;
   - `git diff --check` and final Git status.
5. Commit only the safe documentation result.
6. Push only to `as3k-project/as3k-mame0289-dev`.

## Stop condition

Stop after `SystemInstallExceptionVectors` is fully correlated statically and the next dynamic test is specified.

**Do not execute the function yet. Do not enter `KeyboardInitializeModule`. Do not modify MAME source.**
