# Codex result — `asma3kdvl` LCD-controller experiment

Date: 2026-08-26

Status: experiment stopped at the first unexplained failure. The build and ROM-loading gates passed, but the test remained in the LCD busy loop and did not reach the final breakpoint at `0x0043079e` within 8 seconds. No speculative source fix was attempted.

## Git/bootstrap and Codex automation

- Worktree: `/Users/sperezc/Projects/alphasmart-as3k/mame0289`.
- Base verified before attachment: `f34f02505e32c1993c6a782b6814232cbfc74e36`.
- Added remote `as3k-project` at `https://github.com/SamDelorean/mame-as3k.git`.
- Local branch `as3k-mame0289-dev` tracks `as3k-project/as3k-mame0289-dev`.
- The pre-existing changes in `src/mame/mame.lst` and `src/mame/skeleton/alphasma3k.cpp` were preserved intact while attaching the worktree.
- Local CLI: `codex-cli 0.149.1`. Its supported non-interactive syntax is `codex exec [OPTIONS] [PROMPT]`, with `-` reading the prompt from stdin; `--approve-for-me` provides safe automation in the workspace-write sandbox.
- Created executable `scripts/as3k/codex-next.sh`. Future invocation from this worktree: `./scripts/as3k/codex-next.sh`.
- The wrapper verifies the branch, runs `git pull --ff-only as3k-project as3k-mame0289-dev`, feeds `AGENTS.md` and `docs/as3k/CODEX_NEXT.md` to `codex exec --approve-for-me -C "$repo_root" -`, propagates failure, then prints this result file and `git status --short`.

## Source and local fixture

- Added the `asma3kdvl` ROM definition using the same IPL filename/checksums as `asma3kdv`.
- Added `asma3kdvl` as a clone of `asma3kdv` using the full `alphasmart3k` machine configuration and added it to `src/mame/mame.lst`.
- Created local-only ignored `roms/asma3kdvl/ks0066_f05.bin`; no AlphaWord fixture was copied to the child set.
- Synthetic CGROM verification: 4096 bytes; CRC32 `c71c0011`; SHA1 `1ceaf73df40e531df3bfb26b4fb7cd95fb7bff1d`; SHA256 `ad7facb2586fc6e966c004d7d1d16b024f5805ff7cb47c7a85dabd8b48892ca7`.
- `git diff --check` passed before the build.

## Build and metadata checks

Command:

```text
make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2
```

- Incremental build completed and linked `alphasma3k`; log: `../diagnostic/rebuild_asma3kdvl.log`.
- Build reported `4 driver(s) found`.
- `./alphasma3k -listfull 'asma3k*'` listed `asma3k`, `asma3kdi`, `asma3kdv`, and `asma3kdvl`.
- `-listxml asma3kdvl` reported `cloneof="asma3kdv" romof="asma3kdv"` and the IPL ROM as merged from the parent.

## LCD-controller test

Command:

```text
./alphasma3k asma3kdvl -debug -debugscript ../diagnostic/as3kdvl_lcd_controller.cmd -log -seconds_to_run 8
```

- Console: `../diagnostic/as3kdvl_lcd_controller_console.log`; debugger/error log: `../diagnostic/as3kdvl_lcd_controller.log`.
- Loader found the synthetic F05 twice and emitted two expected `WRONG CHECKSUMS` warnings. It did not report `Required files are missing` or abort fatally, confirming that the IPL was found through parent `asma3kdv`.
- Marker counts: `LCD_RESET_ENTRY=2`, `WRITEBYTE_ENTRY=10`, `READBYTE_ENTRY=345611`, `PCDATA_READ=691220`, `BUSY_BRANCH=345610`.
- E1/TOP distinct PCDATA values: `0x50`, `0x58`, `0x5f`. E2/BOTTOM distinct PCDATA value: `0x90`.
- Distinct read reconstructions:
  - E1/TOP normal: high nibble `0x0`, low nibble `0x0`, byte `0x00`.
  - E1/TOP stuck-busy: high nibble `0x8`, low nibble `0xf`, byte `0x8f`.
  - E2/BOTTOM normal: high nibble `0x0`, low nibble `0x0`, byte `0x00`.
- `byte & 0x80` was non-zero for the repeated E1/TOP `0x8f` behavior.
- Busy branches: 9 exits to `0x00430e40` with byte `0x00`; 345601 loops to `0x00430e28` with byte `0x8f`.
- No `LCD bus contention` and no simultaneous-controller-read warning occurred.
- Both `LCD_Reset` calls completed their four write/read-busy sequences; execution then reached later writes. The subsequent E1/TOP operation remained busy.
- The final breakpoint at `0x0043079e` was not reached before the time limit.

## Recovery, reconciliation, and publication

- Recovered completed local result commit `4c673eb6cf6a412b7f6bd9d11b14598e9885df35`; it contains only `docs/as3k/CODEX_RESULT.md` and `scripts/as3k/codex-next.sh`.
- Fetched remote commit `411e5b78` and merged its current recovery handoff without discarding the completed local result or the uncommitted source diff. The merge commit is `f9807f77`.
- Inspected the source/list diff, staged only those two tracked files, and committed the validated diagnostic systems and GPIO-to-KS0066 bridge as `2e9b5838` (`as3k: add LCD controller diagnostic system`).
- `git diff --check` passed. No file below `roms/`, no diagnostic log, and no proprietary artifact was staged or committed.
- Publication was attempted over HTTPS, but Git reported `could not read Username for 'https://github.com': Device not configured`. SSH was also checked after accepting GitHub's ED25519 host key and reported `Permission denied (publickey)`. Therefore the local branch is ready but cannot be pushed until GitHub credentials are configured in this environment.
- Final tracked worktree status before committing this report: only `docs/as3k/CODEX_RESULT.md` is modified. After this report commit, the expected status is clean and the branch is ahead of `as3k-project/as3k-mame0289-dev` by four commits.
- Local-only `roms/asma3kdvl/ks0066_f05.bin`, generated build products, and diagnostic logs remain ignored and outside Git.
