# AlphaSmart 3000 emulator — Codex instructions

This branch is the working branch for the AlphaSmart 3000 emulator effort on MAME 0.289.

## Base and scope
- Branch: `as3k-mame0289-dev`
- Base commit: `f34f02505e32c1993c6a782b6814232cbfc74e36` (MAME 0.289)
- Primary driver: `src/mame/skeleton/alphasma3k.cpp`
- Driver list: `src/mame/mame.lst`
- Reduced build command:
  `make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2`

## Working rules
1. Work one diagnostic stage at a time. Do not jump ahead to unrelated peripherals.
2. Do not modify MAME cores (`mc68328.*`, `hd44780.*`, etc.) unless the task file explicitly authorizes it.
3. Never add proprietary ROMs, firmware dumps, historical proprietary binaries/source archives, or user dumps to Git. Keep `roms/` and local diagnostic binaries outside version control.
4. Synthetic test data may be generated locally when explicitly requested, but prefer scripts/instructions over committing generated binary blobs.
5. Preserve the validated `asma3kdi` and `asma3kdv` diagnostic behavior unless a task explicitly changes them.
6. Use explicit `0x...` literals in MAME debugger scripts; unprefixed numerals are interpreted as hexadecimal.
7. Before source edits run `git status --short`; after edits run `git diff --check` and the task-specific tests.
8. Do not run the original `asma3k` system unless the task explicitly authorizes it.
9. On failure, stop at the first unexplained regression and report it instead of stacking speculative fixes.
10. Do not push directly to `master`. Work only on `as3k-mame0289-dev` unless explicitly instructed otherwise.

## Task handoff protocol
- Read `docs/as3k/CODEX_NEXT.md` for the current task.
- After completing or blocking on the task, replace `docs/as3k/CODEX_RESULT.md` with a concise factual report containing commands/tests, observed values, files changed, and final `git status --short`.
- If the task authorizes committing/pushing and all stated gates pass, commit the safe tracked changes and push `as3k-mame0289-dev`.
- Never commit anything under `roms/` or local proprietary artifacts even if `git add -A` would ignore them today.
