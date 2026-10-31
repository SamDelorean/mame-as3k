# R6 — Original AS3000 keyboard and native MAME host input — 2026-10-31

Status: CLOSED / BUILD_PASS / REAL_CPU_KEY_PASS / MATRIX_PASS.
One canonical AS3K-DIAG, MAME 0.289 reduced target, no clones,
branches, second executable, ROM changes or unrelated peripheral work.

## Genuine source and hardware contract
Recovered historical manufacturer source:
GoogleDrive/ALPHASMART/Soruce code/Software/ModuleSources/KeyboardModule.c.
Used exact gpubKeyboardMatrix9to0, gpubKeyboardMatrix1to7, the
Keyboard_SetColumnLow function, and the 80-entry ICodeToASBitmap map.
The original firmware scan ICode low nibble is logical column 0..15;
its high nibble is 0..7 physical PD input row. Logic 8 denotes
separate power switch (not scanned in the matrix).
Manufacturer logical column order is nonsequential:
0=latch bit7, 1..7=PA0..6, 8=power excluded, 9..F=latch bits0..6.
All selected column outputs and pressed rows are active-low.
The physical eight latch and seven PA columns are preserved.
Original firmware scanning Y at logical F writes latch BF and PA 7F.
An initially attempted sequential interpretation failed CPU tests,
was rejected and was corrected against manufacturer source BEFORE
the final tested build.

## The same existing driver, not another emulator
Only src/mame/skeleton/alphasma3k.cpp changed in R6.
Existing mapped write-only external 74HC574 latch at 00600000
and existing double KS0066 LCD bridge were retained.
Added saved/reset PA0..6 output state (idle 7F), connected the
seven MC68EZ328 out_port_a callbacks, and replaced eight static
idle-high Port-D row callbacks with matrix evaluators.
A row reads zero if ANY active-low column intersects a host-
pressed key, otherwise one thanks to the physical pull-up.
Added original hardware keyboard input matrix as 16 MAME
input ports, 8 rows each; logical column 8 is entirely unused.
Recovered 77 unique identified physical key positions and their
host KEYCODE mapping from manufacturer historical PC table.
Three entries ambiguous in that table stay unassigned instead
of guessing key identities. Natural-keyboard PORT_CHAR for 49
key positions, 75 character declarations (lower/upper letters
and digit, punctuation, common controls), plus native PORT_CODE
for special keys. This is general hardware emulation, not ROM-
specific processing.

## Real firmware CPU key recognition, with key release
On original BOOT asma3ksb, Lua input field held Y at COL15 bit0:
native System3 recognized D5=0F at PC 004D1018 and later released
the same key at PC 004D1024.
On original WRITER asma3ksw, held N at COL15 bit7:
native System3 recognized D5=7F at 004D1018 and subsequently
released it at 004D1024.
MAME built-in natural keyboard -autoboot_command y independently
reached D5=0F in original WRITER System3 (no port injection).

## Physical matrix, two pressed keys, BOTH ROMs
The manufacturer keyboard was stimulated via MAME input fields,
then CPU-visible memory-mapped register selection was performed.
Measured PDDATA at FFFFF419, same in BOOT and WRITER:
No column selected = FF;
Y from selected external latch = FE;
Enter from selected PA5 = EF;
Y + Enter simultaneously, both physical columns active = EE.
No firmware bytes, instruction pointers or exception routes patched.

## Regression and exclusions
- MAME minimized single-target compile and link: exit 0.
- MAME -validate: exit 0.
- ROM audit (asma3kdi/as3kdv/asma3ksb/asma3ksw):
  four ROM sets found, four OK, exit 0; previous CGROM F05
  NEEDS REDUMP remains.
- R3 historical KeyboardInitializeModule Phase1 runs,
  PAPUEN and latch and return to 00420168, D0=0, PASS.
- With no host key, BOOT and WRITER still render LCD
  snapshot 240x36 exactly matching preserved R5 snapshots.
- This proves original System3 *polling* host keyboard paths,
  not independent firmware CPU execution of all 77 keys,
  complete Flash integrity, erased warning, Writer launch,
  PDSEL mux or PDKBEN interrupt fidelity.
- Original CPU CLR.B instruction can generate a read from
  write-only external 00600000; this logs an unmapped read,
  does not block observed key scanning. No undocumented
  latch readback or chip-select mirroring was invented.
- Do not open IRQ work outside R6; PDSEL/PDKBEN and
  actual keyboard interrupt behavior remain uncredited
  hardware fidelity concerns under existing plan.

## Private artifacts and continuation
Private scripts and logs: r6_writer_y_corrected.log,
r6_asma3ksb_y_release.log, r6_asma3ksw_n_release.log,
r6_natural_y_test.log, r6_matrix_asma3ksb.log,
r6_matrix_asma3ksw.log, r6_regression_r3_phase1.log,
r6_final_build_20261031.log, r6_final_validate.log,
r6_final_audit.log, r6_images/ under
../private/roms/as3k/mame_sets.
Proprietary original source and Flash remain outside Git.
Next existing milestone R7: AlphaWord Plus applet discovery/typing
in the SAME AS3K-DIAG. No R7 work begun in R6.
