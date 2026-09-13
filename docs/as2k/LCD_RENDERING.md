# AS2000 LCD rendering evidence

2026-09-12 REVIEW: LOCAL passed bus-to-F05 pixel consistency; no demonstrated
hardware glyph mismatch and no production correction proposed.

`alphasma.cpp` configures two Samsung KS0066 controllers at an unmeasured
270 kHz, each 2 x 40, explicitly selecting BIOS `f05` over device default
`f00`. Their outputs occupy rows 0–17 and 18–35 of a 240 x 36 screen.
Character cells are 6 x 9 with 5 x 8 glyph pixels in two-line mode.

`src/devices/video/hd44780.cpp` declares these 4096-byte device ROMs:

| ROM | CRC32 | SHA1 | Source annotation |
| --- | --- | --- | --- |
| ks0066_f05.bin (selected) | af9e7bd6 | 0196e871584ee5d370856e7307c0f9d1466e3e51 | BAD_DUMP; datasheet page 51 |
| ks0066_f00.bin | e459877c | 65cf075a988cdcbb316b9afdd0529b374a1a65ec | datasheet page 61; validated on PSR540 |

The private `cgrom/ks0066.zip` contains both files with exactly these lengths
and hashes (checked during PREPARE). This verifies local asset identity, not
physical AS2000 provenance. Fresh v314/v308 audits each reported one ROM set OK / best available, with
NEEDS REDUMP for both F05 controllers. BAD_DUMP and the
driver's wrong-charset TODO alone do not demonstrate a pixel defect.

Bus decoding interprets DDRAM character values as text without consulting
CGROM pixels. For `az09=+`, values are $61 $7A $30 $39 $3D $2B; the renderer
uses CGROM offset code * 16, eight rows, bits 4 through 0. Codes below $10
instead use CGRAM. Display enable, shifts and cursor/blink can affect actual
pixels independently of decoded text.

## Reproducible local evidence

Run executable `/home/spc/Projects/alphasmart/tools/automation/state/as2k-local-validator.sh`.
It snapshots tracked candidate files plus the new checker outside Git,
builds production and existing CI-instrumented variants, validates MAME,
audits both firmware BIOSes and preserves the established v314 keyboard,
F1/F8, NVRAM and restart-recall gates. No register investigation is repeated.

Input and final observation wait for 60 consecutive frames at idle PC $87D7
with an empty natural-keyboard queue. After returning to F1, the validator
saves a native PNG and full screen RGB values beside private logs. SDL's
dummy video driver with software output permits headless rendering.
`scripts/as2k_check_lcd_pixels.py` correlates final bus text with all 240
glyph pixels of the six selected characters and the verified F05 asset.
The cursor should be beyond the six-character token; any mismatch fails
for investigation rather than triggering a speculative fix. The decoder
does not model display shifts; a mismatch must therefore be reviewed for
observation/shift/cursor effects before attribution to rendering.

Each build has a 7200-second deadline; runtime smoke phases have 180-second
wall/45-second emulated bounds. Runs use fresh private directories, sharing
NVRAM only between input and recall. No commit/push or proprietary output
is performed in the repository. LOCAL summary 2026-09-12T18:56:08-06:00 exited 0. Private artifacts are in
external state/as2k-validation.DjV21oOB. The input phase completed at 20
emulated seconds and recall at 5. All 240 tested glyph pixels matched;
keyboard, F1/F8, saved NVRAM and restart recall passed. Runtime evidence uses
the CI-instrumented driver with the existing input-fix helper applied; the
unchanged production variant was built, validated and audited, not smoke-tested.

## Limits and disposition

This demonstrates bus-to-selected-CGROM rendering consistency for
six ASCII characters, not a complete charset or verified physical display.
No verified AS2000 CGROM/reference display was supplied by this evidence.
Hardware charset correctness is OPEN/DEFERRED; minimum follow-up is an
independent physical glyph comparison before any CGROM-selection correction.
Do not replace F05 merely because F00 has a stronger generic source annotation.

Canonical private notes retain an earlier blank-LCD observation; the later
accepted smoke PASS is a separate checkpoint, not grounds to erase it.
This task adds pixel evidence to that distinction. Banking, keyboard,
wake timing, cores, CGROM contents and workflow/trace format are unchanged.
