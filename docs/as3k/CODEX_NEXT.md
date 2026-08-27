# Current Codex task — implement the AS3K 40×4 composite LCD screen

Date: 2026-08-26

Read `AGENTS.md` and the current `docs/as3k/CODEX_RESULT.md` first.

## Established evidence

The previous static study is complete and published. Treat these points as established for this task:

- E1 / `ks0066_0` drives physical rows 1–2.
- E2 / `ks0066_1` drives physical rows 3–4.
- Within each controller, line 0 is DDRAM base `0x00`; line 1 is DDRAM base `0x40`.
- AlphaWord configures the LCD controllers for two-line mode.
- Each controller therefore provides two meaningful 40-character rows.
- `hd44780_base_device::render()` returns 80 character slots in this mode, with 16 bytes reserved per character; bytes 0–7 contain the 5×8 glyph rows and bits 4..0 are the five pixels.
- Final physical geometry is 40×4 character cells, each 6×9 pixels: **240×36 pixels**.
- The current atomic GPIO↔KS0066 bridge is validated and must remain semantically unchanged.
- The existing `set_lcd_size(4,40)` on each controller is visually incorrect and should become `set_lcd_size(2,40)`.

This task implements only the display infrastructure. Do **not** yet create the patterned CGROM or visual-test firmware/fixture.

## Goal

Add a real composite LCD screen to `src/mame/skeleton/alphasma3k.cpp`, preserving all validated protocol behavior.

At the end we must have:

1. one 240×36 `SCREEN_TYPE_LCD` owned by the AS3K driver;
2. rows 1–2 drawn from `ks0066_0->render()`;
3. rows 3–4 drawn from `ks0066_1->render()`;
4. both KS0066 configured visually as 2×40;
5. existing no-LCD diagnostics still working when the optional KS0066 devices are removed;
6. `asma3kdvl` still completing `LCDInitializeModule` with finite real busy polling and no `0x8f` loop.

Do not proceed to later AlphaWord modules, keyboard, or opening-screen execution.

---

## A. Pre-edit gate

From `~/Projects/alphasmart-as3k/mame0289`:

1. `git pull --ff-only as3k-project as3k-mame0289-dev`.
2. Confirm branch is `as3k-mame0289-dev` and tracked status is clean.
3. Read the current:
   - `src/mame/skeleton/alphasma3k.cpp`;
   - `src/mame/skeleton/alphasma.cpp` rendering/palette/configuration;
   - relevant `hd44780.cpp/.h` `render()` semantics if needed.
4. Confirm the current atomic bridge code is exactly the validated implementation from commit `0b414444` or its unchanged descendant.

If the bridge has changed unexpectedly, stop and report before editing.

---

## B. Implement the composite renderer only in `alphasma3k.cpp`

### 1. Remove unused temporary bitmap state

The current `m_tmp_bitmap` member in `alphasmart3k_state` is unused. Remove it if it is still present. Do not introduce a replacement temporary bitmap.

### 2. Add palette function

Add a driver palette initializer in MAME style, using the existing AlphaSmart palette provisionally:

- pen 0 / LCD background: `rgb_t(138, 146, 148)`
- pen 1 / active pixel: `rgb_t(92, 83, 88)`

Document briefly that these colors are reused provisionally from `alphasma.cpp`, not claimed as a measured AS3K color calibration.

### 3. Add driver-owned `screen_update`

Add a `uint32_t screen_update(screen_device &, bitmap_ind16 &, const rectangle &cliprect)` member.

Required behavior:

- fill `cliprect` with palette pen 0 first;
- tolerate either/both optional KS0066 devices being absent;
- call `render()` only on a device that exists;
- compose the physical rows exactly:
  - physical row 0 <- `ks0066_0`, controller line 0
  - physical row 1 <- `ks0066_0`, controller line 1
  - physical row 2 <- `ks0066_1`, controller line 0
  - physical row 3 <- `ks0066_1`, controller line 1
- for each physical row `r=0..3`, column `c=0..39`, glyph row `gy=0..7`, glyph pixel `gx=0..4`:
  - source character offset = `16 * (controller_line * 40 + c)`;
  - source byte = `render_buffer[offset + gy]`;
  - screen position = `(x = c*6 + gx, y = r*9 + gy)`;
  - active pixel is `BIT(source_byte, 4-gx)`;
- leave cell column `x=5` and cell row `y=8` as background spacing by virtue of the initial fill;
- respect `cliprect` when writing pixels;
- return `0`.

Do not alter DDRAM, CGRAM, AC, busy state, cursor state, or bridge state. The renderer must be read-only with respect to emulated LCD protocol state.

Prefer a small local helper/lambda for drawing one controller line if it improves clarity, but keep the implementation simple and MAME-style.

### 4. Correct controller visual geometry

Change both:

```cpp
set_lcd_size(4, 40)
```

to:

```cpp
set_lcd_size(2, 40)
```

Do not change clock, BIOS selection, hashes, or device type.

### 5. Add screen and palette devices

In `alphasmart3k(machine_config &config)`, add one LCD screen with:

- `SCREEN_TYPE_LCD`;
- logical size `240 × 36` (`6*40`, `9*4` is fine);
- full visible area;
- driver `screen_update` callback;
- palette bound to the new two-entry palette;
- refresh/vblank values consistent with `alphasma.cpp` unless the exact MAME 0.289 API requires equivalent syntax. Use 50 Hz and the same provisional vblank if directly reusable.

Add a two-entry `PALETTE` using the driver palette initializer.

The derived `alphasmart3k_diag` configuration removes only the two KS0066 devices. The screen/palette may remain; the renderer must then produce a blank background without crashing. Do not remove the screen from diagnostic configs unless there is a proven MAME configuration reason to do so.

### 6. Do not touch

Do not modify:

- `src/devices/machine/mc68328.cpp/.h`;
- `src/devices/video/hd44780.cpp/.h`;
- the atomic Port C bridge semantics;
- ROM definitions/hashes;
- `mame.lst` unless no semantic change is required;
- diagnostic fixture binaries;
- anything under `roms/`;
- keyboard, USB, serial, interrupts, later firmware modules.

---

## C. Static checks and build

1. Run `git diff --check`.
2. Inspect the complete `alphasma3k.cpp` diff before build and confirm it contains only:
   - palette/screen renderer;
   - screen/palette machine configuration;
   - 2×40 geometry correction;
   - removal of unused temporary bitmap if applicable.
3. Incremental build, no clean:

```sh
make SUBTARGET=alphasma3k \
  SOURCES=src/mame/skeleton/alphasma3k.cpp \
  OSD=sdl \
  REGENIE=1 \
  -j2
```

Save output to:

`../diagnostic/rebuild_lcd_composite_screen.log`

Expect 1 source, 4 drivers, successful link.

4. Confirm:

```sh
./alphasma3k asma3kdi -verifyroms
./alphasma3k asma3kdv -verifyroms
```

remain good.

5. Use `-listxml` (or another non-emulating metadata command if MAME syntax differs) to confirm `asma3kdvl` exposes exactly one LCD display with width 240 and height 36. Record the exact display metadata/refresh value. Do not use `asma3kdvl -verifyroms` as a success criterion because its synthetic F05 intentionally has the wrong checksum.

---

## D. No-LCD regression

Run the established no-LCD diagnostic unchanged:

```sh
./alphasma3k asma3kdv \
  -debug \
  -debugscript ../diagnostic/as3kdv_lcd_nolcd.cmd \
  -log \
  -seconds_to_run 8
```

Save the result separately as, for example:

`../diagnostic/as3kdv_lcd_composite_nolcd_regression.log`

Required behavior:

- both LCD resets reached;
- 11 WriteByte / 11 ReadByte / 22 PCDATA reads / 11 busy branches as before;
- reconstructed reads remain `0x00`;
- all busy branches exit rather than loop;
- final breakpoint at `0x0043079e` reached;
- no crash or screen-update error despite the two LCD devices being absent.

If this regression changes, stop and report. Do not attempt a second fix.

---

## E. KS0066 protocol regression with screen present

Use the existing local-only zero CGROM:

`roms/asma3kdvl/ks0066_f05.bin`

It must remain exactly 4096 zero bytes with the previously recorded hashes. Regenerate only if missing; never stage it.

Run the existing controller script:

```sh
./alphasma3k asma3kdvl \
  -debug \
  -debugscript ../diagnostic/as3kdvl_lcd_controller.cmd \
  -log \
  -seconds_to_run 8
```

Preserve under new names, e.g.:

- `../diagnostic/as3kdvl_lcd_composite_console.log`
- `../diagnostic/as3kdvl_lcd_composite.log`

Success criteria:

- only expected synthetic F05 WRONG CHECKSUMS warnings;
- no required-file failure;
- finite busy polling is acceptable and desirable;
- any busy=1 condition must clear;
- `0x8f` must not reappear;
- no E1/E2 contention;
- both `LCD_Reset` calls and `LCDMoveCursor` complete;
- final `0x0043079e` breakpoint reached before timeout;
- no screen/palette/rendering error or crash.

Report counts and distinct `LCD_ReadByte` values, but do not require them to be byte-for-byte identical if screen scheduling slightly changes finite busy timing. Explain any difference.

Do not continue beyond the LCD RTS.

---

## F. Publication

If all gates pass:

1. `git diff --check`.
2. `git status --short`.
3. Ensure no ROM, diagnostic log, screenshot, generated binary, or proprietary artifact is staged.
4. Replace `docs/as3k/CODEX_RESULT.md` with a factual report containing:
   - exact renderer implementation and row map;
   - pixel/cell geometry;
   - palette values and provisional status;
   - confirmation both KS0066 are now 2×40;
   - exact screen metadata from `-listxml`;
   - build result;
   - no-LCD regression result;
   - KS0066 protocol regression result;
   - confirmation atomic bridge semantics were not changed;
   - `git diff --check` and final status;
   - commit SHA and push result.
5. Commit only safe tracked source/docs changes.
6. Push only to `as3k-project/as3k-mame0289-dev`.

## Stop condition

Stop after the **240×36 composite screen infrastructure is implemented and regression-tested**.

Do **not** yet generate the patterned synthetic CGROM, create the four-row visual test fixture, take visual screenshots, continue to the AlphaWord opening screen, or implement keyboard/later modules.
