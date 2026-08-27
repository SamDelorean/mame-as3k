# Current Codex task — study the AS3K dual-controller 40×4 rendering path

Date: 2026-08-26

Read `AGENTS.md` and the current `docs/as3k/CODEX_RESULT.md` first.

## Established state

The GPIO↔KS0066 protocol is now validated through `LCDInitializeModule`.

The current atomic bridge in `src/mame/skeleton/alphasma3k.cpp` is known-good for this stage:

- no-LCD regression passes unchanged;
- `asma3kdvl` with two KS0066 devices completes both `LCD_Reset` calls and `LCDMoveCursor`;
- one real finite busy=1 poll (`0x80`) was observed and cleared;
- the false `0x8f` loop is gone;
- final RTS `0x0043079e` is reached.

Do **not** modify that bridge in this task.

## Goal

Perform an **evidence-first static study** of how to render the physical AlphaSmart 3000 40×4 LCD from the two independent KS0066 devices.

This task is design/research only.

**Do not modify source code.**
**Do not rebuild.**
**Do not run an emulated AS3K system.**
**Do not generate or download any real CGROM.**

The output must be precise enough that the following task can implement the display without guessing row mapping or geometry.

---

## A. Verify the physical/firmware row mapping

Locate the original AlphaSmart LCD source/listings already available locally, especially `LCDModule.c`, `LCDModule.h`, linked listing/disassembly, or equivalent archived AS3000 material.

Determine exactly how logical cursor rows 1–4 map to:

- E1 / `ks0066_0` (TOP);
- E2 / `ks0066_1` (BOTTOM);
- DDRAM line/address inside that controller.

Inspect at minimum:

- `LCDMoveCursor`;
- any row/address tables or switch statements;
- constants for TOP/BOTTOM display selection;
- the command byte used for each physical row.

Report a table conceptually equivalent to:

`physical row -> controller -> controller logical line -> DDRAM base address`

but fill it only from evidence.

Do not assume the conventional 40×4 mapping unless the AlphaSmart source confirms it.

Also determine whether the firmware meaning of TOP/BOTTOM is:

- rows 1–2 vs rows 3–4,
- alternating rows,
- or another arrangement.

---

## B. Inspect MAME 0.289 HD44780/KS0066 rendering semantics

Inspect:

- `src/devices/video/hd44780.h`
- `src/devices/video/hd44780.cpp`

Answer precisely:

1. What does `set_lcd_size(lines, chars)` affect?
2. What does it **not** affect?
3. How does `render()` lay out its `m_render_buf`?
4. What is `line_size` after AlphaWord has sent Function Set for 2-line mode?
5. How many character positions from each KS0066 are meaningful for one AS3K controller?
6. How are 5×8 glyph rows stored in the returned render buffer?
7. How are cursor/blink effects already folded into `render()`?
8. Does calling `render()` from an owner driver's `screen_update` have side effects relevant to emulation state?

Explicitly evaluate the current AS3K configuration:

```cpp
m_lcdc0->set_lcd_size(4, 40);
m_lcdc1->set_lcd_size(4, 40);
```

Determine whether `4,40` is semantically correct for two independent controllers or only a provisional visual setting.

If each physical KS0066 should instead be treated visually as `2,40`, explain why from MAME code and the AS3K hardware/firmware mapping.

Do not change it yet.

---

## C. Study real MAME rendering examples

Inspect at least these if present in MAME 0.289:

- `src/mame/skeleton/alphasma.cpp`
- `src/mame/omron/luna_88k.cpp`

and one additional useful HD44780/KS0066 screen example if needed.

For `alphasma.cpp`, locate the exact:

- palette setup;
- `screen_update` implementation;
- temporary bitmap usage, if any;
- LCD screen dimensions and visible area;
- way its two KS0066 devices are combined.

Determine whether that implementation can be reused directly for AS3K or whether its older hardware/controller wiring makes only the rendering portion reusable.

Report exact source functions and the relevant geometry.

---

## D. Derive the minimum AS3K composite-screen design

Without implementing it, specify the smallest clean change that should later live only in `alphasma3k.cpp`.

Prefer a driver-owned composite `screen_update` unless evidence shows a better MAME-native approach.

Determine:

- final logical character geometry: 40 columns × 4 physical rows;
- pixel cell size implied by MAME's HD44780 renderer (glyph width/height plus spacing);
- resulting bitmap width and height;
- exact source row in `ks0066_0->render()` and `ks0066_1->render()` for each physical display row;
- pixel placement formula for `(physical_row, column, glyph_y, glyph_x)`;
- whether one blank column and one blank row should be inserted between character cells;
- a minimal two-entry LCD palette suitable for MAME style, preferably consistent with `alphasma.cpp` unless AS3K evidence suggests otherwise;
- whether the existing `m_tmp_bitmap` member in `alphasma3k_state` is useful, unnecessary, or should be removed later.

State exactly which future source additions are expected, e.g.:

- palette function;
- composite `screen_update`;
- SCREEN configuration;
- PALETTE configuration;
- possible correction from `set_lcd_size(4,40)` to `set_lcd_size(2,40)` per controller.

Do not add any of them in this task.

---

## E. Design a lawful validation strategy for the next implementation

The current local synthetic `ks0066_f05.bin` is 4096 zero bytes. This is adequate for protocol/busy tests but renders all CGROM characters blank.

Determine the cleanest **synthetic-only** way to validate the future screen composition without any proprietary CGROM.

Evaluate at least these possibilities:

1. keep zero CGROM and validate only screen creation/dimensions initially;
2. generate a deterministic synthetic 4096-byte diagnostic CGROM locally with visible non-copyrightable test patterns;
3. use CGRAM through normal firmware/device commands;
4. temporarily seed distinct DDRAM values in a diagnostic-only mechanism;
5. continue AlphaWord far enough that normal firmware writes visible DDRAM, while using a synthetic patterned CGROM.

Choose the least invasive strategy that can prove:

- all four physical rows render;
- E1 rows and E2 rows are placed in the correct physical order;
- 40 columns fit exactly;
- controller state remains independent;
- no protocol behavior is altered merely to make pixels visible.

No proprietary ROM/CGROM may be added to Git or generated from copyrighted data.

If a synthetic patterned CGROM is recommended, define a simple reproducible pattern-generation rule and explain what it would allow us to observe. Do not generate the file yet.

---

## F. Check whether rendering can be implemented before later firmware modules

We currently stop at `LCDInitializeModule` RTS before `SystemInstallExceptionVectors` and keyboard initialization.

Determine whether a composite screen can be safely added now even though the screen will initially be blank, or whether there is a technical dependency on later modules.

Separate clearly:

- display-device rendering infrastructure;
- firmware actually writing meaningful screen contents.

The expected answer should tell us whether the next implementation can add the 40×4 display **without** yet emulating keyboard or later modules.

---

## G. Repository state and publication

This is a no-source-change task.

At the end:

1. `git diff --check`
2. `git status --short`
3. confirm no source file changed;
4. replace `docs/as3k/CODEX_RESULT.md` with the factual study;
5. commit only that safe documentation result;
6. push only to `as3k-project/as3k-mame0289-dev`.

The result report must include:

- evidence-backed physical row/controller/DDRAM mapping;
- exact `render()` buffer layout;
- meaning of `set_lcd_size` and whether current `4,40` should become `2,40`;
- relevant MAME examples and geometry;
- proposed final screen dimensions;
- exact per-row composition plan;
- palette plan;
- fate of `m_tmp_bitmap`;
- recommended synthetic-only visual validation method;
- whether rendering can be implemented independently of keyboard/later firmware;
- expected minimum future driver changes;
- `git diff --check` and final Git status;
- commit SHA and push result.

## Stop condition

Stop after the rendering design is fully evidenced and documented.

**Do not implement rendering yet.**
