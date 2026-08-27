# Current Codex task — execute and validate `SystemInstallExceptionVectors`

Date: 2026-08-26

Read `AGENTS.md` and the current `docs/as3k/CODEX_RESULT.md` first.

## Established evidence

The static study of `SystemInstallExceptionVectors` is complete and published.

Treat these facts as established:

- entry: `0x00430504`;
- RTS: `0x00430524`;
- the routine performs exactly four architectural `MOVE.L` stores and no helper calls;
- vector 2 at `0x00000008` must become `0x00430526` (`SystemBusError`);
- vector 3 at `0x0000000c` must become `0x004305b0` (`SystemAddressError`);
- vector 4 at `0x00000010` must become `0x0043063a` (`SystemIllegalInstruction`);
- vector 5 at `0x00000014` must become `0x004306c4` (`SystemDivideByZeroError`);
- each 32-bit write is expected to appear in the MAME debugger as two big-endian 16-bit transactions;
- the four installed handlers are diagnostic/error paths and must **not** execute during normal initialization;
- Startup Manager TRAP 0 at `0x00000080` and InterruptInitializeModule vectors at `0x00000110`, `0x00000114`, `0x00000118` must remain intact;
- the next primary-module call is `KeyboardInitializeModule = 0x0042e2a8`.

This task is a **narrow dynamic validation only**. Do not enter keyboard code and do not modify MAME source.

## Goal

Execute `SystemInstallExceptionVectors` on `asma3kdv`, prove the exact four vector values written to low RAM, verify none of the installed exception handlers fires, and stop at the first instruction boundary of `KeyboardInitializeModule` before any keyboard instruction executes.

---

## A. Repository gate

From `~/Projects/alphasmart-as3k/mame0289`:

1. `git pull --ff-only as3k-project as3k-mame0289-dev`.
2. Confirm branch is `as3k-mame0289-dev`.
3. Confirm tracked `git status --short` is clean.
4. Confirm current branch contains static-study commit `4b3f5e065fdf8e694dbe86d1cafb819a67ec4cb0` or a clean descendant.
5. Run `git diff --check`.

Do not edit `src/mame/skeleton/alphasma3k.cpp`, `mame.lst`, any MAME core, ROM definition, or fixture.

No rebuild should be necessary unless the existing `./alphasma3k` executable is missing. If a rebuild is unexpectedly required, stop and report rather than broadening the task.

---

## B. Create one local debugger script

Create local-only:

`../diagnostic/as3kdv_exception_vectors.cmd`

Use explicit `0x...` literals everywhere.

Start from this plan, adapting only if MAME 0.289 debugger syntax demonstrably requires a small correction:

```text
temp0 = 0
bp 0x00430504,1,{ temp0 = 1 ; logerror "AS3KDV_EXCEPTION_INSTALL_ENTRY PC=%08X A7=%08X SR=%04X\n",pc,sp,sr ; g }
wp 0x00000008,0x4,w,temp0==1,{ logerror "AS3KDV_VECTOR2 ADDR=%08X DATA=%08X PC=%08X\n",wpaddr,wpdata,pc ; g }
wp 0x0000000c,0x4,w,temp0==1,{ logerror "AS3KDV_VECTOR3 ADDR=%08X DATA=%08X PC=%08X\n",wpaddr,wpdata,pc ; g }
wp 0x00000010,0x4,w,temp0==1,{ logerror "AS3KDV_VECTOR4 ADDR=%08X DATA=%08X PC=%08X\n",wpaddr,wpdata,pc ; g }
wp 0x00000014,0x4,w,temp0==1,{ logerror "AS3KDV_VECTOR5 ADDR=%08X DATA=%08X PC=%08X\n",wpaddr,wpdata,pc ; g }
bp 0x00430526,1,{ logerror "AS3KDV_UNEXPECTED_BUS_ERROR PC=%08X\n",pc ; quit }
bp 0x004305b0,1,{ logerror "AS3KDV_UNEXPECTED_ADDRESS_ERROR PC=%08X\n",pc ; quit }
bp 0x0043063a,1,{ logerror "AS3KDV_UNEXPECTED_ILLEGAL_INSTRUCTION PC=%08X\n",pc ; quit }
bp 0x004306c4,1,{ logerror "AS3KDV_UNEXPECTED_DIVIDE_BY_ZERO PC=%08X\n",pc ; quit }
bp 0x00430524,1,{ logerror "AS3KDV_EXCEPTION_INSTALL_RTS V2=%08X V3=%08X V4=%08X V5=%08X TRAP0=%08X LV4=%08X LV5=%08X LV6=%08X\n",d@0x00000008,d@0x0000000c,d@0x00000010,d@0x00000014,d@0x00000080,d@0x00000110,d@0x00000114,d@0x00000118 ; g }
bp 0x0042e2a8,1,{ logerror "AS3KDV_KEYBOARD_ENTRY_STOP PC=%08X V2=%08X V3=%08X V4=%08X V5=%08X TRAP0=%08X LV4=%08X LV5=%08X LV6=%08X\n",pc,d@0x00000008,d@0x0000000c,d@0x00000010,d@0x00000014,d@0x00000080,d@0x00000110,d@0x00000114,d@0x00000118 ; quit }
g
```

Important:

- Breakpoint/log PC may appear +2 because of the already documented debugger behavior; do not treat that alone as failure.
- If `d@` formatting is unsupported in this context, use the already proven MAME debugger memory-expression syntax and document the exact correction.
- Do not place a breakpoint *inside* `KeyboardInitializeModule`; the stop at its entry must quit before its first instruction executes.

---

## C. Execute only the established no-LCD valid-AlphaWord diagnostic

Run only:

```sh
./alphasma3k asma3kdv \
  -debug \
  -debugscript ../diagnostic/as3kdv_exception_vectors.cmd \
  -log \
  -seconds_to_run 8
```

Capture console output and preserve the resulting MAME log under local diagnostic names, for example:

- `../diagnostic/as3kdv_exception_vectors_console.log`
- `../diagnostic/as3kdv_exception_vectors.log`

Do not stage diagnostic logs.

`asma3kdv` intentionally removes the KS0066 devices. That is acceptable here because the LCD protocol has already been validated separately and this task tests only the exception-vector installer after the known LCD return.

---

## D. Exact pass criteria

The test passes only if all of the following are true:

1. `SystemInstallExceptionVectors` entry is reached.
2. Exactly the expected vector stores occur after entry.
3. Reconstruct the writes as:
   - `0x00000008`: high `0x0043`, low `0x0526` -> `0x00430526`;
   - `0x0000000c`: high `0x0043`, low `0x05b0` -> `0x004305b0`;
   - `0x00000010`: high `0x0043`, low `0x063a` -> `0x0043063a`;
   - `0x00000014`: high `0x0043`, low `0x06c4` -> `0x004306c4`.
4. The debugger shows the expected high-word then low-word behavior for each architectural `MOVE.L`.
5. The RTS breakpoint at `0x00430524` is reached.
6. Final longwords at RTS are exactly the four expected handler addresses.
7. No breakpoint for `SystemBusError`, `SystemAddressError`, `SystemIllegalInstruction`, or `SystemDivideByZeroError` fires.
8. Previously established vectors remain unchanged through the routine:
   - TRAP 0 at `0x00000080`;
   - level 4 at `0x00000110`;
   - level 5 at `0x00000114`;
   - level 6 at `0x00000118`.
   Record their exact observed values and compare with the previously validated values.
9. Execution reaches `KeyboardInitializeModule = 0x0042e2a8` before timeout.
10. Quit at that entry without executing keyboard code.

Previously validated level-vector values are:

- `0x00000110 = 0x00431a04`
- `0x00000114 = 0x00431ab0`
- `0x00000118 = 0x00431ae6`

For TRAP 0, record the factual observed value rather than assuming an address if the prior exact value is not already established in local notes.

If any installed exception handler fires, a vector value differs, the keyboard entry is not reached, or an unexplained exception/hang occurs: **stop and report. Do not modify the driver or attempt a fix in this task.**

---

## E. Publication

This task should not change MAME source.

At the end:

1. `git diff --check`.
2. `git status --short`.
3. Confirm no MAME source, ROM, fixture, generated binary, or diagnostic log is staged.
4. Replace `docs/as3k/CODEX_RESULT.md` with a factual report containing:
   - command and script used;
   - entry/RTS/keyboard-stop observations;
   - every watchpoint transaction with address/data/PC;
   - reconstructed final vector values;
   - confirmation of high-word/low-word order;
   - observed preserved TRAP 0 and level 4/5/6 vectors;
   - whether any unexpected exception handler fired;
   - whether keyboard entry was reached before timeout;
   - confirmation keyboard code was not executed;
   - `git diff --check` and final status;
   - documentation commit SHA and push result.
5. Commit only the safe documentation result.
6. Push only to `as3k-project/as3k-mame0289-dev`.

## Stop condition

Stop immediately after proving `SystemInstallExceptionVectors` and reaching the entry of `KeyboardInitializeModule`.

**Do not execute or reverse engineer keyboard in this task. Do not modify MAME source.**
