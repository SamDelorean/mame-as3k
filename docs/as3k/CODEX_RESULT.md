# Codex result — AS3K dual-controller 40×4 rendering study

Date: 2026-08-26

Status: **complete (design/research only)**. No source, ROM, fixture, or generated binary was changed; no build or emulated system was run.

## Evidence and physical row mapping

The named archived `LCDModule.c`/`.h` files were not present as standalone files in the local project tree. Their linked AlphaWord equivalent is preserved in `../diagnostic/lcd_lowlevel_disasm.txt`, generated previously from the local synthetic diagnostic. It gives unambiguous row evidence in `LCDMoveCursor` at `0x00430c76`: rows/columns are validated as 1–4/1–40; rows 1–4 select command/controller pairs (`0x80`,1), (`0xc0`,1), (`0x80`,0), (`0xc0`,0); and the emitted command adds `column - 1`. The low-level write at `0x00430dd8` converts selector 1 to Port C enable `0x40` (E1), and selector 0 to `0x80` (E2).

The validated driver maps E1 to `ks0066_0` and E2 to `ks0066_1`. Firmware TOP therefore means physical rows 1–2 and BOTTOM means rows 3–4; rows are not interleaved.

| Physical row | Controller | Firmware selection | Controller line | DDRAM base | Column-1 command |
|---|---|---|---:|---:|---:|
| 1 | E1 / `ks0066_0` | TOP / selector 1 | 0 | `0x00` | `0x80` |
| 2 | E1 / `ks0066_0` | TOP / selector 1 | 1 | `0x40` | `0xc0` |
| 3 | E2 / `ks0066_1` | BOTTOM / selector 0 | 0 | `0x00` | `0x80` |
| 4 | E2 / `ks0066_1` | BOTTOM / selector 0 | 1 | `0x40` | `0xc0` |

## MAME 0.289 HD44780/KS0066 semantics

Evidence is in `src/devices/video/hd44780.h` and `hd44780.cpp`, especially `set_lcd_size`, `pixel_update`, Function Set handling, `render`, and `screen_update`.

- `set_lcd_size(lines, chars)` only sets visual module geometry (`m_lines`, `m_chars`) used by the default `pixel_update` mapper to clip/rearrange positions into a bitmap. It does not configure DDRAM, Function Set state, `m_num_line`, character height, `render()` iteration, or render-buffer layout.
- Firmware Function Set controls `m_num_line` and `m_char_size`. Bit 3 makes `m_num_line = 2` and forces 5×8 characters. AlphaWord sends two-line Function Set, so `line_size = 80 / m_num_line = 40`.
- `render()` clears `m_render_buf`, then emits `m_num_line × line_size` characters when display is on. Character `(line,pos)` begins at `m_render_buf + 16 * (line * line_size + pos)`. Two-line mode has 80 meaningful slots: 0–39 from DDRAM `0x00`–`0x27`, and 40–79 from `0x40`–`0x67`, subject to display shift.
- Each character reserves 16 bytes. In 5×8 mode bytes 0–7 are glyph rows and bytes 8–15 remain zero. Bits 4 through 0 in each row are pixels left through right; `screen_update` ignores the upper three bits.
- `render()` copies CGRAM/CGROM pixels, then folds in cursor/blink: cursor replaces the last active glyph row with `0x1f`; active blink replaces all active rows with `0x1f` during its applicable phase.
- Calling `render()` from an owner screen update clears/repopulates only the derived buffer and reads state. It does not alter AC, busy state, DDRAM/CGRAM, shift, or protocol state. Blink phase changes independently by timer.

Current `set_lcd_size(4,40)` is provisional and semantically wrong for two independent 2×40 controllers. It asks the default mapper to treat each KS0066 as a four-row module; with two-line controller state, positions 40–79 are remapped to visual lines 2–3. Each controller supplies two 40-character rows, so implementation should use `set_lcd_size(2,40)`. Direct composition from `render()` does not technically depend on it, but correction describes each device accurately and prevents incorrect native screen-update use.

## Existing MAME examples

`src/mame/skeleton/alphasma.cpp` is closest:

- `alphasmart_palette`: background `(138,146,148)`, pixels `(92,83,88)`;
- `machine_start`: allocates a 240×36 temporary indexed bitmap;
- `screen_update`: renders controller 0 and copies at y=0, then controller 1 and copies at y=18;
- `alphasmart`: configures both as 2×40 and a `SCREEN_TYPE_LCD` of `6*40 × 9*4` (240×36), full visible area, 50 Hz, provisional 2500 µs vblank.

Its stacking geometry/palette are reusable. Its older MCU bus wiring is not; the validated AS3K bridge must remain unchanged. Its temporary-bitmap method also relies on native updates clearing a 240×36 bitmap and copying selected halves. Direct render-buffer composition is clearer and makes AS3K row mapping explicit.

`src/mame/omron/luna_88k.cpp` `common_config` is a conventional single-device example: 2×16 KS0066, monochrome palette, dedicated LCD screen, native `ks0066_device::screen_update`, raw visible geometry 96×16. It confirms native update for one controller, not two-controller composition. Additional 2×40 examples `src/mame/brother/ax145.cpp` and `src/mame/sony/dpsd7.cpp` use 240-pixel width and `set_lcd_size(2,40)`.

## Minimum future AS3K composite design

Use one driver-owned `bitmap_ind16` screen update in `alphasma3k.cpp`: fill the clip rectangle with pen 0, call both controllers' `render()`, and copy mapped glyph bits as pen 1/0.

The default cell is 6×9: 5×8 glyph plus one blank column/row. The physical 40×4 display is exactly **240×36 pixels**. For zero-based coordinates:

`bitmap.pix(physical_row * 9 + glyph_y, column * 6 + glyph_x) = BIT(src[glyph_y], 4 - glyph_x)`

with `column=0..39`, `glyph_y=0..7`, `glyph_x=0..4`. Filling first and never writing cell x=5/y=8 supplies spacing.

| Physical row (zero-based) | Render source | Character source offset |
|---:|---|---|
| 0 | `ks0066_0->render()` line 0 | `16 * (0 * 40 + column)` |
| 1 | `ks0066_0->render()` line 1 | `16 * (1 * 40 + column)` |
| 2 | `ks0066_1->render()` line 0 | `16 * (0 * 40 + column)` |
| 3 | `ks0066_1->render()` line 1 | `16 * (1 * 40 + column)` |

Use the `alphasma.cpp` palette unchanged absent later physical evidence: pen 0 `rgb_t(138,146,148)`, pen 1 `rgb_t(92,83,88)`.

AS3K's current `m_tmp_bitmap` is unused and never allocated. Direct composition needs none, so remove it later. Expected additions, only in `alphasma3k.cpp`: palette function, composite update, 240×36/full-visible-area `SCREEN_TYPE_LCD` configuration/update/palette, two-entry `PALETTE`, both devices corrected to 2×40, and removal of `m_tmp_bitmap`. No bridge/core change.

## Synthetic-only visual validation

The least invasive complete validation is normal protocol writes plus a locally generated deterministic synthetic 4096-byte CGROM and synthetic diagnostic firmware that writes distinguishable DDRAM values through existing LCD commands/data. This tests independent controller state without a rendering back door. Zero CGROM proves only screen existence/dimensions; CGRAM alone has eight slots and needs extra commands; later real AlphaWord writes would be less deterministic.

Recommended uncommitted CGROM rule: start with 4096 zero bytes; for code `c` and row `y=0..7`, store `((c >> (y % 5)) | (c << (5 - (y % 5)))) & 0x1f` at `c*16+y`, leaving bytes 8–15 zero. This simple five-bit cyclic pattern is reproducible, non-copyrightable, and visually distinguishes codes.

A next-task synthetic fixture should use ordinary commands to enable both displays and write four distinct 40-code sequences to E1 `0x00`, E1 `0x40`, E2 `0x00`, E2 `0x40`, including distinct row markers/rightmost cells. Pixel inspection then proves all rows, order, exact 40-column fit, and independence. Do not commit generated CGROM/fixture binaries; commit only generators/instructions if requested.

## Later-module dependency

Screen infrastructure is independent of keyboard, exception vectors, and later firmware. It can be added now: it only visualizes KS0066 state and `render()` has no protocol side effects. At the current stop after `LCDInitializeModule`, DDRAM is spaces and zero CGROM makes pixels blank. Meaningful contents need later firmware or a synthetic LCD-writing diagnostic, but screen creation does not.

## Checks and publication

- Static-only study; no build/emulated system run; no real CGROM generated/downloaded/added.
- `git diff --check`: passed before publication.
- Source files changed: none.
- Safe tracked change: `docs/as3k/CODEX_RESULT.md` only.
- Branch: `as3k-mame0289-dev`; push target: `as3k-project/as3k-mame0289-dev`.
- Documentation commit SHA and push result are in the final handoff (a commit cannot contain its own final SHA).
