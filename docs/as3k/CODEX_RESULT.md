# Codex result — atomic LCD Port C bridge

Date: 2026-08-26

Status: **passed**. The driver-local atomic bridge fixes the false KS0066 write and `LCDInitializeModule` returns with both controllers present.

## Implementation

- `alphasmart3k_state` now keeps two compact saved/reset bytes: `m_lcd_port_c_pending`, assembled by the eight Port C callbacks, and `m_lcd_port_c_applied`, representing the state visible to the LCD devices.
- PC0 through PC6 only update their pending bit. PC7 updates its bit and commits the complete byte.
- PC7 is a safe commit point for this path because the exact MC68EZ328 core loops selected callbacks deterministically from bit 0 through bit 7, while the linked AlphaWord LCD path selects all Port C bits and drives PC7 as the E2 output. Runtime E2 low/high transitions also confirm PC7 is not being treated as an input.
- Each commit applies falling E1/E2 edges first against OLD DB/RW/RS, then presents NEW DB/RW/RS to both available controllers, then applies rising E1/E2 edges against the new controls, and finally records NEW as applied.
- `lcd_data_r()` multiplexes from the applied E1/E2 bits. Missing-device behavior and the existing deterministic contention diagnostic are preserved.
- No MC68EZ328 or HD44780/KS0066 core, ROM definition/hash, fixture, rendering, or driver-list change was made.

## Build and static checks

- `git diff --check`: passed before build and at the final gate.
- Incremental build (no clean): `make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2`.
- Result: passed; 1 source file and 4 drivers found; `alphasma3k` linked successfully.
- Build log: `../diagnostic/rebuild_lcd_atomic_bridge.log`.
- `./alphasma3k asma3kdi -verifyroms`: good.
- `./alphasma3k asma3kdv -verifyroms`: good.

## No-LCD regression

Command: `./alphasma3k asma3kdv -debug -debugscript ../diagnostic/as3kdv_lcd_nolcd.cmd -log -seconds_to_run 8`

- LCD_RESET_ENTRY: 2; WRITEBYTE_ENTRY: 11; READBYTE_ENTRY: 11; PCDATA_READ: 22; BUSY_BRANCH: 11.
- E1 PCDATA: only `0x50`; E2 PCDATA: only `0x90`; reconstructed byte only `0x00`.
- Busy was always 0; all 11 branches exited to `0x00430e40` and none looped to `0x00430e28`.
- Final breakpoint at RTS `0x0043079e` was reached.
- Preserved log: `../diagnostic/as3kdv_lcd_atomic_bridge_nolcd_regression.log`.

## KS0066 controller test

- Local-only `roms/asma3kdvl/ks0066_f05.bin` was exactly 4096 bytes with CRC32 `c71c0011`, SHA1 `1ceaf73df40e531df3bfb26b4fb7cd95fb7bff1d`, and SHA256 `ad7facb2586fc6e966c004d7d1d16b024f5805ff7cb47c7a85dabd8b48892ca7`.
- Command: `./alphasma3k asma3kdvl -debug -debugscript ../diagnostic/as3kdvl_lcd_controller.cmd -log -seconds_to_run 8`.
- Loader output contained only the two expected `WRONG CHECKSUMS` warnings for synthetic F05 and no required-file failure.
- LCD_RESET_ENTRY: 2; WRITEBYTE_ENTRY: 11; READBYTE_ENTRY: 12; PCDATA_READ: 24; BUSY_BRANCH: 12.
- Distinct E1/TOP PCDATA: `0x50`, `0x58`; distinct E2/BOTTOM PCDATA: `0x90`.
- Distinct reconstructed `LCD_ReadByte` values: `0x00`, `0x80`.
- Busy=1 was observed once as `0x80`; it looped once to `0x00430e28`, then cleared to `0x00`. Totals: 1 loop and 11 exits to `0x00430e40`.
- `0x8f` did not appear; the repeated false-write busy loop is gone.
- No E1/E2 contention occurred.
- Both `LCD_Reset` calls and `LCDMoveCursor` completed; final RTS `0x0043079e` was reached before timeout.
- Preserved logs: `../diagnostic/as3kdvl_lcd_atomic_bridge_console.log` and `../diagnostic/as3kdvl_lcd_atomic_bridge.log`.

## Files and publication

- Safe tracked changes: `src/mame/skeleton/alphasma3k.cpp`, `docs/as3k/CODEX_RESULT.md`.
- Nothing under `roms/`, diagnostics, generated binaries, or proprietary historical artifacts was staged.
- Implementation/result commit: pending final commit.
- Push target: `as3k-project/as3k-mame0289-dev`.
- Push result: pending final push.
- Final `git status --short`: pending publication.
