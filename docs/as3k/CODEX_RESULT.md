# Codex result — AS3K 40×4 composite LCD screen

Date: 2026-08-26

Status: **complete**. The 240×36 composite LCD infrastructure is implemented and regression-tested. No later firmware module, keyboard work, patterned CGROM, visual fixture, or screenshot work was attempted.

## Implementation

Only `src/mame/skeleton/alphasma3k.cpp` was changed for the implementation:

- Added a driver-owned `screen_update` that first fills the clipped area with pen 0 and tolerates either or both optional KS0066 devices being absent.
- The renderer calls `render()` only for present devices and maps physical rows as follows: row 0 from `ks0066_0` line 0, row 1 from `ks0066_0` line 1, row 2 from `ks0066_1` line 0, and row 3 from `ks0066_1` line 1.
- Each character starts at `16 * (controller_line * 40 + column)`. Glyph bytes 0–7 and bits 4–0 are drawn into a 6×9 cell; column 5 and row 8 remain the background spacing. All pixel writes are clipped.
- Added one 240×36 `SCREEN_TYPE_LCD`, full visible area, 50 Hz refresh, provisional 2500 µs vblank, and a two-entry palette.
- Palette colors are background `rgb_t(138, 146, 148)` and active pixel `rgb_t(92, 83, 88)`. They are provisionally reused from `alphasma.cpp`, not claimed as measured AS3K calibration.
- Both KS0066 devices now use visual geometry `set_lcd_size(2, 40)`.
- Removed the unused `m_tmp_bitmap`; no replacement temporary bitmap was introduced.
- The atomic Port C bridge implementation is unchanged from validated commit `0b414444`.

`-listxml` reports exactly one display for `asma3kdvl`:

```xml
<display tag="screen" type="lcd" rotate="0" width="240" height="36" refresh="50.000000" />
```

## Build and static checks

- Pre-edit `git pull --ff-only as3k-project as3k-mame0289-dev`: already up to date.
- Branch confirmed as `as3k-mame0289-dev`; tracked status was clean before editing.
- Incremental reduced build completed successfully with 1 source and 4 drivers and linked `alphasma3k`.
- Build log: `../diagnostic/rebuild_lcd_composite_screen.log` (local, not staged).
- `./alphasma3k asma3kdi -verifyroms`: good.
- `./alphasma3k asma3kdv -verifyroms`: good.
- `git diff --check`: passed before publication.

## No-LCD regression

Command:

```sh
./alphasma3k asma3kdv -debug -debugscript ../diagnostic/as3kdv_lcd_nolcd.cmd -log -seconds_to_run 8
```

Result: passed unchanged. Both LCD resets completed; counts were 11 WriteByte, 11 ReadByte, 22 PCDATA reads, and 11 busy branches. All 11 reconstructed reads were `0x00`, every busy branch exited, and the scripted breakpoint at `0x0043079e` fired (the log records post-break PC `0x004307a0`). No crash or screen-update error occurred with both KS0066 devices removed.

Local logs, not staged: `../diagnostic/as3kdv_lcd_composite_nolcd_regression.log` and `../diagnostic/as3kdv_lcd_composite_nolcd_error.log`.

## KS0066 protocol regression with screen present

The local-only `roms/asma3kdvl/ks0066_f05.bin` remained 4096 zero bytes, SHA1 `1ceaf73df40e531df3bfb26b4fb7cd95fb7bff1d` (MAME-reported CRC `c71c0011`). It was not staged.

Command:

```sh
./alphasma3k asma3kdvl -debug -debugscript ../diagnostic/as3kdvl_lcd_controller.cmd -log -seconds_to_run 8
```

Result: passed. The only ROM warnings were the two expected synthetic F05 WRONG CHECKSUMS reports. Counts were 2 LCD resets, 11 WriteByte, 12 ReadByte, 24 PCDATA reads, and 12 busy branches. Reconstructed `LCD_ReadByte` values were all `0x00`; busy-branch values were eleven `0x00` and one finite `0x80`, which cleared on the next poll. These counts and values exactly match `../diagnostic/as3kdvl_lcd_atomic_bridge.log`, so screen scheduling caused no observed protocol difference. There was no `0x8f`, E1/E2 contention, required-file failure, crash, or screen/palette/rendering error. Both reset calls and `LCDMoveCursor` completed, and the scripted `0x0043079e` breakpoint fired before timeout (logged post-break as `0x004307a0`). Execution did not continue beyond the LCD return.

Local logs, not staged: `../diagnostic/as3kdvl_lcd_composite_console.log` and `../diagnostic/as3kdvl_lcd_composite.log`.

## Publication

Safe tracked files: `src/mame/skeleton/alphasma3k.cpp` and `docs/as3k/CODEX_RESULT.md`. No ROM, diagnostic log, screenshot, generated binary, proprietary artifact, MAME core, or driver-list change is included.

The implementation/publication commit SHA and push result are recorded in the final task response because a commit cannot include its own SHA. Final `git status --short` is expected to be clean after publication and is verified there.
