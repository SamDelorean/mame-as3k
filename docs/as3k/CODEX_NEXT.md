# Current Codex task — bootstrap GitHub handoff and run `asma3kdvl`

Date: 2026-08-26

Read `AGENTS.md` first and obey it.

## A. Safely attach the existing local MAME 0.289 worktree to GitHub

The local worktree is expected at `~/Projects/alphasmart-as3k/mame0289`, based on MAME 0.289 commit `f34f02505e32c1993c6a782b6814232cbfc74e36`, and currently has intentional uncommitted changes in `src/mame/mame.lst` and `src/mame/skeleton/alphasma3k.cpp`.

The GitHub repository is `https://github.com/SamDelorean/mame-as3k.git` and the remote branch `as3k-mame0289-dev` already exists from the exact MAME 0.289 base. **Do not push to `master`.**

1. Inspect `pwd`, `git status --short`, `git rev-parse HEAD`, `git remote -v`, and `git diff --check`.
2. If the GitHub repo is not already a remote, add it under a non-conflicting name such as `alphasmart`.
3. Fetch `as3k-mame0289-dev`.
4. Safely switch the local worktree onto a local tracking branch `as3k-mame0289-dev`, preserving the existing uncommitted source changes. Do not reset, stash-drop, or overwrite them.
5. Pull/merge only the remote handoff files (`AGENTS.md`, `docs/as3k/...`) from the branch as needed. Because the remote branch has the same 0.289 base and only handoff files, no source conflict is expected.
6. Confirm that the source diff still contains the previously validated diagnostic systems and GPIO↔KS0066 bridge.

## B. Prepare no-copy task execution for future iterations

Inspect the **local installed** Codex CLI help (`codex --help` and, if present, `codex exec --help`). Do not assume flags from another version.

If a supported non-interactive `codex exec` mode exists, create `scripts/as3k/codex-next.sh` that, when run by the user from this worktree:
- verifies it is on `as3k-mame0289-dev`;
- runs `git pull --ff-only` from the GitHub remote;
- reads `AGENTS.md` and `docs/as3k/CODEX_NEXT.md`;
- invokes the locally installed Codex CLI non-interactively using the exact syntax supported by this installation;
- does **not** bypass sandbox/approval safeguards beyond the normal safe automation mode exposed by the installed CLI;
- exits non-zero if Codex fails;
- prints the resulting `docs/as3k/CODEX_RESULT.md` and `git status --short`.

Do not run this wrapper recursively from inside the current Codex session. Just create it, make it executable, and report the exact future command the user should run.

If no supported non-interactive mode exists, do not invent one; report that and create no wrapper.

## C. Continue the pending LCD-controller experiment: add `asma3kdvl`

Create a fourth diagnostic system `asma3kdvl` meaning valid AlphaWord + LCD bridge + two KS0066 controllers.

### Source changes
1. In `src/mame/skeleton/alphasma3k.cpp`, add `ROM_START( asma3kdvl )` using exactly the same IPL region and `as3k_diag_valid_alphaword.u1` CRC/SHA1 as `asma3kdv`.
2. Add a `COMP` entry for `asma3kdvl` with:
   - parent `asma3kdv`;
   - machine config `alphasmart3k` (not `alphasmart3k_diag`), so both KS0066 devices are present;
   - description clearly indicating valid AlphaWord + LCD/bridge diagnostic;
   - flags/style coherent with the existing diagnostic entries.
3. Add `asma3kdvl` under `@source:skeleton/alphasma3k.cpp` in `src/mame/mame.lst`.
4. Do not conceptually change `asma3k`, `asma3kdi`, or `asma3kdv`.

Run `git diff --check` and inspect the source/list diff before compiling.

### Synthetic CGROM — local only, never commit
Create `roms/asma3kdvl/ks0066_f05.bin` as exactly 4096 zero bytes. This is an artificial diagnostic CGROM, not an original ROM.

Verify:
- size 4096;
- CRC32 `c71c0011`;
- SHA1 `1ceaf73df40e531df3bfb26b4fb7cd95fb7bff1d`;
- SHA256 `ad7facb2586fc6e966c004d7d1d16b024f5805ff7cb47c7a85dabd8b48892ca7`.

Do not copy the AlphaWord fixture into the child set initially. Confirm runtime can find `as3k_diag_valid_alphaword.u1` from parent `asma3kdv`. If it cannot, stop and report before copying anything.

### Build
Incremental only, no clean:

`make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2`

Save build output to `../diagnostic/rebuild_asma3kdvl.log`.

Verify 4 drivers and `-listfull` shows `asma3k`, `asma3kdi`, `asma3kdv`, `asma3kdvl`. Verify via `-listxml` or equivalent that `asma3kdvl` is a clone of `asma3kdv`.

`-listroms` may list both F00/F05 alternatives for both KS0066. Do not use `-verifyroms` as the success gate for `asma3kdvl`, because the synthetic F05 intentionally has a wrong checksum and F00 is absent.

### Debug script and execution
Copy `../diagnostic/as3kdv_lcd_nolcd.cmd` to `../diagnostic/as3kdvl_lcd_controller.cmd`, preserving all addresses/watchpoints but changing marker prefix `AS3KDV_` → `AS3KDVL_`.

It must still `quit` on the breakpoint over the RTS at `0x0043079E`, before `SystemInstallExceptionVectors` executes.

Run only:

`./alphasma3k asma3kdvl -debug -debugscript ../diagnostic/as3kdvl_lcd_controller.cmd -log -seconds_to_run 8`

Capture console output in `../diagnostic/as3kdvl_lcd_controller_console.log` and copy `error.log` to `../diagnostic/as3kdvl_lcd_controller.log`.

Expected ROM-loader behavior: `ks0066_f05.bin` is found and produces `WRONG CHECKSUMS` warning, but **not** `Required files are missing` and not a fatal abort.

Report:
- counts of LCD_RESET_ENTRY, WRITEBYTE_ENTRY, READBYTE_ENTRY, PCDATA_READ, BUSY_BRANCH;
- every distinct PCDATA value for E1/TOP and E2/BOTTOM;
- reconstructed high nibble, low nibble and byte for each distinct LCD_ReadByte behavior;
- whether any `byte & 0x80` is non-zero;
- whether any BUSY_BRANCH loops to `0x00430E28` versus exits to `0x00430E40`;
- whether any `LCD bus contention` occurs;
- whether both LCD_Reset calls complete;
- whether the final breakpoint at `0x0043079E` is reached before the time limit.

If contention occurs, a required file is missing, or the test does not reach the final breakpoint, stop and do not attempt speculative fixes.

## D. Result, commit and push

Replace `docs/as3k/CODEX_RESULT.md` with a factual report for this task.

Never add anything from `roms/` or proprietary diagnostic binaries. Before commit, inspect `git status --short` and staged files explicitly.

If the build/test gates pass, commit the safe tracked changes (source/list + handoff/wrapper/result files) with a clear AS3K message and push only to `as3k-mame0289-dev`.

If the experiment fails, still update `CODEX_RESULT.md`; do not commit speculative source fixes. Push only safe handoff/result changes if doing so will not hide the failing source state.

Finish by reporting:
- Codex CLI version and supported automation syntax discovered locally;
- remote/branch state;
- exact future wrapper command if created;
- test result;
- commit SHA if committed;
- push result;
- final `git status --short`.
