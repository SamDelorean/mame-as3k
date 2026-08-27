# Codex result — AS3K external keyboard latch

Date: 2026-08-26

Status: **complete; implementation, validation, commit, and push gates passed**.

## Repository gate and evidence

- Pulled `as3k-project/as3k-mame0289-dev` with `--ff-only`; it was already current.
- Confirmed branch `as3k-mame0289-dev`, initially clean tracked status, audit commit `08816104002f27365ace1798eddd06b78550c764` in HEAD, and initial `git diff --check` success.
- Inspected the 16-bit MC68EZ328 program map and local historical `KeyboardModule.c`/listing evidence. The source defines and emits byte writes only at `0x00600000`; it does not prove that the physical latch is mirrored through every address in the established 32 KiB chip-select window.

## Driver change

Only `src/mame/skeleton/alphasma3k.cpp` was changed for implementation:

- added `u8 m_keyboard_column_latch`;
- registered it with `save_item()`;
- reset it deterministically to `0xFF`;
- added a write handler that retains the byte;
- mapped aligned range `0x00600000–0x00600001` with `umask16(0xff00)`, exposing only byte address `0x00600000` on the observed high lane of the 16-bit CPU bus.

The physical power-on latch value is unknown. `0xFF` is explicitly an emulator initialization choice providing safe no-active-low-column output, not a measured hardware reset fact. The mapping is deliberately not mirrored across `0x00600000–0x00607fff` because only exact address `0x00600000` is demonstrably decoded by available source; the 32 KiB board chip-select window remains documented without silently guessing lower address-line decode.

No keyboard matrix, PA/PD callback, input key, PAPUEN, PDSEL, PDKBEN/PKBDINT, Phase 2, LCD, diagnostic ROM definition, or MAME core change was made.

## Build and static validation

- Reduced no-clean build command: `make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2` — passed against final source.
- Driver-list generation reported four drivers; `-listfull` confirmed `asma3k`, `asma3kdi`, `asma3kdv`, and `asma3kdvl`.
- `./alphasma3k -verifyroms asma3kdi` — good.
- `./alphasma3k -verifyroms asma3kdv` — good.
- Original `asma3k` was not executed.

## Bounded dynamic validation

Local-only script: `../diagnostic/as3kdv_keyboard_latch.cmd`, run only with `asma3kdv` under an 8-second alarm.

Observed:

- Phase-1 entry reached at the expected breakpoint (`PC` presentation `0x0042e2aa`).
- Exact latch watchpoint: `ADDR=00600000 DATA=FF PC=0042E2D0`.
- A temporary handler observation immediately reported retained latch value `ff`; that diagnostic log statement was removed before the final build and commit.
- The former `unmapped program memory write to 00600000` message was absent both with the temporary observation and against final source.
- Phase-1 RTS breakpoint was reached (`PC` presentation `0x0042e2f0`) with `D0=00000000`.
- Caller-return breakpoint was reached (`PC` presentation `0x0042016a`) with `D0=00000000`, then the script quit.
- No Phase 2, `InterruptInstallHandler`, keyboard interrupt handler, bus error, address error, illegal-instruction, or divide-by-zero safety break fired.

The final-source rerun repeated the `0xFF` watchpoint, normal RTS/caller return, absence of the old unmapped message, and absence of every failure marker.

## Regression and publication boundary

- `asma3kdi` and `asma3kdv` fixture audits remain good; the valid-AlphaWord path still returns from Phase 1.
- No file under `src/devices/` changed; LCD bridge source is untouched.
- The debugger script and logs remain outside Git. No ROM, proprietary material, fixture, generated binary, or build artifact is included.
- Final `git diff --check`: passed.
- Pre-commit tracked status contains only `src/mame/skeleton/alphasma3k.cpp`, `docs/as3k/STATUS.md`, and `docs/as3k/CODEX_RESULT.md`; post-publication status is clean.
