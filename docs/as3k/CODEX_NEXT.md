# Current Codex task — execute `KeyboardInitializeModule` Phase 1 only

Date: 2026-08-26

Read `AGENTS.md`, `docs/as3k/STATUS.md`, and the current `docs/as3k/CODEX_RESULT.md` first.

## Established evidence

The valid-AlphaWord path is dynamically validated through the entry of:

`KeyboardInitializeModule = 0x0042E2A8`

The static keyboard study is complete. Treat these facts as established:

- Phase 1 occupies `0x0042E2A8`–`0x0042E2EF` inclusive;
- RTS is `0x0042E2EE`;
- the next byte boundary `0x0042E2F0` is `KeyboardInitializeModulePhase2`, but Phase 2 is called later from a different call site and must **not** be executed in this task;
- caller JBSR is at `0x00420162`; normal return is `0x00420168`;
- Phase 1 is a leaf: no direct calls, no RAM globals, no loops/waits, no interrupt install/enable, and it should return `D0=0` if ordinary accesses complete;
- exact Phase-1 hardware accesses are:
  - PASEL `0xFFFFF403`: read, OR `0x7F`, write;
  - PADIR `0xFFFFF400`: read, OR `0x7F`, write;
  - PAPUEN `0xFFFFF402`: read, AND `0x80`, write;
  - external latch `0x00600000`: write `0xFF`;
  - PADATA `0xFFFFF401`: read, OR `0x7F`, write;
  - PDSEL `0xFFFFF41B`: write `0xFF`;
  - PDDIR `0xFFFFF418`: write `0x00`;
  - PDPUEN `0xFFFFF41A`: write `0x00`.

Static MAME 0.289 inspection established:

- PADIR, PADATA, PASEL, PDDIR, PDDATA and PDPUEN have core support;
- PA0–PA6 output and PD0–PD7 input callbacks exist but the AS3K driver does not connect them;
- no AS3K driver mapping/device currently exists for the external byte-write latch `0x00600000`;
- core map entries appear absent for PAPUEN `0xFFFFF402` and PDSEL `0xFFFFF41B`;
- `PKBDINT 0xFFFFF41E` is a later concern and is not touched by Phase 1.

This task is a **narrow dynamic validation only**.

**Do not modify MAME source or cores.**
**Do not implement the latch.**
**Do not add input ports or keyboard callbacks.**
**Do not execute Phase 2.**

---

## A. Repository gate

From `~/Projects/alphasmart-as3k/mame0289`:

1. `git pull --ff-only as3k-project as3k-mame0289-dev`.
2. Confirm branch is `as3k-mame0289-dev` and tracked status is clean.
3. Confirm static keyboard commit `eb42b589a18644eeb02e5e1f131e6962f12768c4` is an ancestor of HEAD.
4. Confirm `docs/as3k/STATUS.md` contains the keyboard Phase-1 contract.
5. Run `git diff --check`.
6. Confirm existing `./alphasma3k` executable is present. Do not rebuild; if it is missing, stop and report.

Do not edit `src/mame/skeleton/alphasma3k.cpp`, `src/mame/mame.lst`, any MAME core, ROM definition, fixture, or generated file.

---

## B. Create one local debugger script

Create local-only:

`../diagnostic/as3kdv_keyboard_phase1.cmd`

Use explicit `0x...` literals everywhere.

The script must:

1. mark entry at `0x0042E2A8`;
2. watch the exact Phase-1 hardware locations as byte accesses;
3. record address, data, read/write direction where supported, and PC;
4. break at the Phase-1 RTS `0x0042E2EE` and record D0 plus relevant GPIO register values using proven debugger memory expressions;
5. continue through RTS and quit at caller return `0x00420168`;
6. include safety breakpoints for locations that Phase 1 must not call/enter:
   - `InterruptInstallHandler = 0x004318A4`;
   - `Keyboard_InterruptHandler = 0x0042EA74`;
   - `KeyboardInitializeModulePhase2 = 0x0042E2F0` should not be reached by fall-through; if reached before caller return, treat as failure and quit.

Watch these exact byte addresses:

- `0xFFFFF403` PASEL — read/write;
- `0xFFFFF400` PADIR — read/write;
- `0xFFFFF402` PAPUEN — read/write;
- `0x00600000` external latch — write;
- `0xFFFFF401` PADATA — read/write;
- `0xFFFFF41B` PDSEL — write;
- `0xFFFFF418` PDDIR — write;
- `0xFFFFF41A` PDPUEN — write.

If a watchpoint cannot be set because an address is unmapped, document that fact and use the narrowest safe alternative needed to observe the transaction (for example debugger/log output). Do not broaden into code changes.

Because MAME debugger breakpoint PC is already known to report +2 in this workflow, do not treat the presentation offset alone as failure.

The exact script syntax may be adapted only as required by the already-established MAME 0.289 debugger syntax; document any correction.

---

## C. Execute only `asma3kdv`

Run only:

```sh
./alphasma3k asma3kdv \
  -debug \
  -debugscript ../diagnostic/as3kdv_keyboard_phase1.cmd \
  -log \
  -seconds_to_run 8
```

Capture console output and preserve local-only logs under `../diagnostic/`, for example:

- `as3kdv_keyboard_phase1_console.log`
- `as3kdv_keyboard_phase1.log`

Do not stage diagnostic files.

`asma3kdv` intentionally has no KS0066 devices. That is acceptable because LCD initialization and the exception-vector path have already been validated independently and this task begins only at keyboard Phase 1.

---

## D. Exact observations and pass/stop criteria

Record the factual transaction sequence. Compare against the static expectation but do not force expected values where the current core reset state controls preserved bit 7.

Expected semantic sequence:

1. PASEL read then write `old | 0x7F`;
2. PADIR read then write `old | 0x7F`;
3. PAPUEN read then write `old & 0x80`;
4. latch write `0xFF` at exactly `0x00600000`;
5. PADATA read then write `old | 0x7F`;
6. PDSEL write `0xFF`;
7. PDDIR write `0x00`;
8. PDPUEN write `0x00`;
9. Phase-1 RTS reached with `D0=0`;
10. caller return `0x00420168` reached before timeout.

Important decision point: the latch is intentionally unmapped. Determine whether the write to `0x00600000`:

- is observed/logged but execution continues; or
- causes an exception, stop, or other hard dependency.

If the latch write causes a hard stop/exception, **stop the test there and report it as the first demonstrated missing driver dependency. Do not fix it.**

Likewise, if PAPUEN/PDSEL missing core mappings cause a hard execution failure, stop and report the exact first failure. If they merely log as unmapped and execution continues, record this but continue only through caller return.

Pass for this stage means either:

- full Phase 1 reaches caller return with the exact finite sequence and `D0=0`; or
- the test cleanly identifies the first hard missing hardware dependency at one of the expected accesses, with no speculative fix.

Failure means unexplained control flow, an unexpected exception/handler, unexpected Phase-2 entry, or execution outside the bounded routine without a clear hardware cause.

Do not scan keys, do not execute any normal keyboard polling helper, and do not run Phase 2.

---

## E. Publication and public handoff

At the end:

1. `git diff --check`.
2. `git status --short`.
3. Confirm no MAME source/core, ROM, fixture, generated binary, diagnostic script/log, or proprietary artifact is staged.
4. Replace `docs/as3k/CODEX_RESULT.md` with a factual report containing:
   - exact command/script used;
   - every observed Phase-1 hardware transaction in order;
   - actual old/new values for read-modify-write registers;
   - exact behavior of writes to unmapped PAPUEN/PDSEL/latch addresses;
   - whether any CPU exception or error handler fired;
   - D0 and key register state at RTS if reached;
   - whether caller return `0x00420168` was reached;
   - confirmation Phase 2 was not executed;
   - `git diff --check` and final tracked status;
   - documentation commit and push result.
5. Commit only the safe documentation result.
6. Push only to `as3k-project/as3k-mame0289-dev`.

Do not update `STATUS.md` yourself in this run unless the task handoff explicitly changes; ChatGPT will consolidate the public handoff after reading the result.

## Stop condition

Stop immediately after the caller return at `0x00420168`, or earlier at the first demonstrated hard Phase-1 hardware dependency.

**Do not implement or fix keyboard hardware in this task.**
