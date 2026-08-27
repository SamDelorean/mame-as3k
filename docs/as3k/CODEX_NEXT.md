# Current Codex task — recover local result, publish it, then continue if needed

Date: 2026-08-26

Read `AGENTS.md` first and obey it.

## Situation

The previous Codex session reportedly finished locally, but GitHub still shows this branch at the pre-task handoff commit and `docs/as3k/CODEX_RESULT.md` still says `waiting for local Codex bootstrap`.

Therefore **do not blindly rerun the experiment first**. Recover the local state and publish the factual result if it already exists.

Remote repository:
`https://github.com/SamDelorean/mame-as3k.git`

Required branch:
`as3k-mame0289-dev`

Exact MAME base:
`f34f02505e32c1993c6a782b6814232cbfc74e36`

## A. Recover the local worktree safely

From `~/Projects/alphasmart-as3k/mame0289` inspect:

- `pwd`
- `git status --short`
- `git branch --show-current`
- `git rev-parse HEAD`
- `git log --oneline --decorate -8`
- `git remote -v`
- `git diff --check`

Do not use `reset --hard`, do not discard any local diff, do not push to `master`, and do not add anything from `roms/` or any proprietary firmware/dumps.

Determine whether the previous session already:

- created `scripts/as3k/codex-next.sh`;
- added `asma3kdvl`;
- generated the local synthetic `ks0066_f05.bin`;
- built the 4-driver subtarget;
- ran the `asma3kdvl` LCD-controller test;
- wrote a local `docs/as3k/CODEX_RESULT.md`;
- created one or more local commits that were never pushed.

## B. Reconcile with GitHub without losing local work

Ensure a remote points to `SamDelorean/mame-as3k` and fetch `as3k-mame0289-dev`.

If the local branch already contains completed work, preserve it and integrate only the newer remote handoff file(s) as needed. Do not overwrite a completed local `CODEX_RESULT.md` with the remote placeholder.

If the local work is committed but not pushed, inspect the commit contents before pushing.

If the local work is uncommitted, inspect it carefully before staging.

## C. If the previous experiment already completed

Do **not** repeat it unless necessary to obtain missing factual evidence.

Replace `docs/as3k/CODEX_RESULT.md` with a complete factual report containing at least:

- Codex CLI version;
- whether `codex exec` exists and the exact safe automation syntax found locally;
- whether `scripts/as3k/codex-next.sh` was created and the exact future command;
- branch/remote state;
- whether `asma3kdvl` was added;
- synthetic CGROM size and hashes;
- build result and whether 4 drivers were found;
- whether the AlphaWord fixture was resolved from parent `asma3kdv`;
- ROM-loader warning/error behavior for the synthetic F05;
- LCD test counts and distinct PCDATA values;
- reconstructed LCD_ReadByte values;
- whether busy=1 was ever observed;
- BUSY_BRANCH destinations;
- whether E1/E2 contention occurred;
- whether both LCD_Reset calls completed;
- whether breakpoint `0x0043079E` was reached before timeout;
- `git diff --check` result;
- final `git status --short`;
- commit SHA and push result.

Then stage only safe tracked project files. Never stage `roms/`, generated ROM binaries, historical firmware, `AWordApplet02.bin`, `UpdaterROMWithStartup02.bin`, or other proprietary artifacts.

Commit with a clear AS3K message if needed and push only to `as3k-mame0289-dev`.

## D. If the previous experiment did NOT actually complete

Continue the pending task exactly as originally planned:

1. Add diagnostic system `asma3kdvl` using the same valid AlphaWord fixture as `asma3kdv`, parent `asma3kdv`, machine config `alphasmart3k`.
2. Add `asma3kdvl` to `src/mame/mame.lst`.
3. Create local-only `roms/asma3kdvl/ks0066_f05.bin` as 4096 zero bytes and verify:
   - CRC32 `c71c0011`
   - SHA1 `1ceaf73df40e531df3bfb26b4fb7cd95fb7bff1d`
   - SHA256 `ad7facb2586fc6e966c004d7d1d16b024f5805ff7cb47c7a85dabd8b48892ca7`
4. Do not copy the AlphaWord fixture into the child set initially; confirm it resolves from parent `asma3kdv`.
5. Incremental build only:
   `make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2`
6. Verify 4 drivers.
7. Create `../diagnostic/as3kdvl_lcd_controller.cmd` from the existing no-LCD script with marker prefix `AS3KDVL_`.
8. Run only:
   `./alphasma3k asma3kdvl -debug -debugscript ../diagnostic/as3kdvl_lcd_controller.cmd -log -seconds_to_run 8`
9. Stop without speculative fixes if there is required-file failure, LCD contention, or failure to reach `0x0043079E`.
10. Write the factual result to `docs/as3k/CODEX_RESULT.md`, commit safe files, and push only to `as3k-mame0289-dev`.

## E. Automation wrapper

If the installed CLI supports safe non-interactive `codex exec`, ensure `scripts/as3k/codex-next.sh` exists and is safe. It must:

- verify branch `as3k-mame0289-dev`;
- fetch/pull the task handoff safely without discarding local work;
- read `AGENTS.md` and `docs/as3k/CODEX_NEXT.md`;
- invoke the locally supported non-interactive Codex syntax;
- keep normal safeguards/approvals;
- exit non-zero on failure;
- print `docs/as3k/CODEX_RESULT.md` and `git status --short` at the end.

Do not recursively run the wrapper from inside this Codex session.

## Finish

The essential deliverable is that GitHub branch `as3k-mame0289-dev` is advanced with the safe source/handoff/result files and that `docs/as3k/CODEX_RESULT.md` contains the actual result rather than the placeholder.
