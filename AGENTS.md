# AlphaSmart 2000 emulator — Codex instructions

This branch is the working branch for the AlphaSmart 2000 emulator effort on MAME 0.289.

## Base and scope
- Branch: `as2k-mame0289-dev`
- Primary driver: `src/mame/skeleton/alphasma.cpp`
- Driver list: `src/mame/mame.lst`
- Diagnostic workflow: `.github/workflows/as2k-diagnostic.yml`
- LCD trace decoder: `scripts/as2k_decode_trace.py`
- Focused diagnostic build:
  `make -j3 SUBTARGET=as2kdiag SOURCES=src/mame/skeleton/alphasma.cpp REGENIE=1 USE_QTDEBUG=0`

## Working rules
1. Work one diagnostic stage at a time. Do not jump ahead to unrelated peripherals.
2. Do not modify MAME CPU/video cores unless the current task explicitly authorizes it.
3. Never add proprietary ROMs, firmware dumps, historical proprietary binaries/source archives, or user dumps to Git.
4. Preserve the existing AS2000 diagnostic workflow and trace format unless a task explicitly changes them.
5. Prefer measured behavior and trace evidence over speculative emulation changes.
6. Repository cleanliness is gated by the outer wrapper; after edits run `git diff --check` and task-specific tests.
7. Stop at the first unexplained regression rather than stacking speculative fixes.
8. Do not push directly to `master`. Work only on `as2k-mame0289-dev` unless explicitly instructed otherwise.
9. Keep changes narrowly scoped and document any hardware assumption that is not yet verified.
10. Information useful to the wider AlphaSmart effort should be recorded factually so it can be reused by the AS3000, NEO, BetaWise, and hardware-analysis fronts.

## Task handoff protocol
- Read `docs/as2k/CODEX_NEXT.md` for the current task.
- After completing or blocking on the task, replace `docs/as2k/CODEX_RESULT.md` with a concise factual report containing commands/tests, observed values, files changed, and final outer-wrapper repository-cleanliness result.
- If the task authorizes committing/pushing and all stated gates pass, commit the safe tracked changes and push `as2k-mame0289-dev`.
- Never commit ROMs, user dumps, generated executables, diagnostic logs, or other proprietary/local artifacts.

## Codex CLI workaround
- Do not run `git status` from inside Codex CLI on this workstation; Codex 0.154.0 can abort when `git status` is executed after earlier tool calls in the same session.
- Repository cleanliness is checked by the outer automation wrapper before Codex starts and after it finishes.
- Use `git diff --check`, `git diff --name-only`, and other specific Git commands when needed; do not substitute another broad status command.
- This is an execution-environment workaround, not a project or emulator requirement.
