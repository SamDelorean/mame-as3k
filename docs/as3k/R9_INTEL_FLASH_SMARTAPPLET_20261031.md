# R9 — Intel 28F008B3 Flash / original SmartApplet programming — 2026-10-31

STATUS: CLOSED / FLASH_PROGRAM_PASS / MAIN_ERASE_PASS / PARAMETER_ERASE_PASS /
SMARTAPPLET_INSTALL_AND_LAUNCH_PASS / FLASH_NVRAM_PASS / BUILD_PASS.
Canonical single AS3K-DIAG: mame-as3k-dev, MAME 0.289 reduced.
No isolated emulator, clone, worktree, alternative executable, firmware
patch, new proprietary ROM, Codex or push.

## Physical device and reused MAME controller
Original AS3000 board identifies an 8-Mbit Intel 28F008B3 Flash at
0x00400000–0x004FFFFF. Manufacturer documentation describes an 8-bit
data bus, ID 0x89 and 28F008B3B bottom-boot device ID 0xD3
(top-boot alternative ID 0xD2), eight 8-KiB parameter blocks
and fifteen 64-KiB main blocks.
Technical source: Intel 3-Volt Advanced Boot Block Flash:
https://media.digikey.com/pdf/Data%20Sheets/Intel%20PDFs/28F008%2C016%2C160%2C320%2C640%2C800B3.pdf

Reused src/devices/machine/intelfsh.cpp / .h. Added ONLY Intel B3B
variant (1 MiB, 0x89/0xD3, NOR bits 1->0, correct bottom-boot
8-KiB / main 64-KiB erase geometry). Existing E28F008SA
model is NOT equivalent: it reports device ID 0xA2 and uses
different erase geometry. Existing unrelated flash chips unchanged.
Physical PCB part suffix top/bottom remains to be conclusively
verified; this implementation models B3B bottom-boot. Do not
represent either suffix or lock/VPP timing as proven on real PCB.

## Same emulation, same data inputs
Driver src/mame/skeleton/alphasma3k.cpp uses a required Intel
B3B flash device and replaces direct read-only ROM mapping
at 0x00400000–0x004FFFFF with 68k 16-bit bus wrappers around
the real 8-bit chip command interface, respecting selected byte
lanes. Existing 1-MiB original manufacturer ROM regions 'ipl'
remain unchanged for verification and reset-vector copy.
For each of six existing registered ROM sets, ROM_COPY of
that region initializes the flash device's own internal 'flash'
region on first boot. The built-in MAME Flash NVRAM interface
thereafter saves its actual 1-MiB programmed bytes per device.
Original external 256-KiB SRAM/R8 NVRAM is separate and unchanged.
ROM sets are alternative DATA INPUTS to the ONE AS3K-DIAG,
not separate emulator versions. No firmware-dependent behavior.

## Targeted CPU-bus command tests (private test NVRAM only)
On original WRITER hardware map, previously blank byte 0x004A1000:
initial 0xFF; command 0x90 -> manufacturer ID 0x89,
device ID 0xD3; 0xFF returns read-array mode.
Command 0x40 then data 0xA5 -> status 0x80 and stored 0xA5.
A second programming attempt with 0xFF kept it 0xA5,
proving real NOR bit restriction rather than synthetic RAM writes.
Command 0x20 / 0xD0 -> busy status 0x00, followed by read-array
return; target erased to 0xFF, outside-erase startup byte
was unchanged. These operations ran through the MAME CPU
address space's actual Flash mapping.

On separate private BOOT Flash test, erase 0x00401000:
before 0x42, after 0xFF; adjacent 0x00402000 remained 0x08.
Full saved Flash byte comparison: exactly 8,192 bytes of bottom
parameter block 0x000000–0x001FFF became 0xFF; all other
bytes in the 1,048,576-byte image remained IDENTICAL to the
original BOOT image (8,136 actual byte differences).
This verifies the specific 8-KiB parameter-block geometry
without altering any original ROM source file.

## Complete native applet installation and execution
MANUFACTURER SOURCE: private local
GoogleDrive/ALPHASMART/SmartApplets3000/AlphaWordPlus.OS3KApp,
101,596 bytes, signature C0FFEEAD.
The existing original WRITER image's payload at offset 0x8000
matches the manufacturer source byte-for-byte.

INSTALL TEST: launch the original BOOT asma3ksb (empty applet region);
via private Lua stimulus through CPU-space Intel 0x40/data byte
commands, program the original manufacturer's 101,596-byte
SmartApplet at physical 0x00408000. Initial header was 0xFF;
after programming the full C0FFEEAD signature was read back.
Normal exit produced an actual 1,048,576-byte Flash NVRAM file.
Full byte comparison against the original source:
- Entire 101,596-byte applet window matches EXACTLY;
- all bytes outside the applet window equal original BOOT;
- no replacement WRITER ROM image was generated or used.

OPEN TEST: start a NEW process with the SAME MAME executable,
the SAME asma3ksb original BOOT ROM and the private programmed
Flash NVRAM file. Original System3 acknowledged its real
first-run integrity warning via host Y and restarted via Return.
The original firmware recognized the installed applet,
displayed 'Opening "File 1"...', then native AlphaWord editor
accepted MAME natural keyboard text 'flash'; actual 240x36
KS0066 snapshot contains readable 'flash' and cursor.
THIRD PROCESS: start again with no input; the original editor
recovered the same text and cursor, with identical screen pixels.
These tests prove installed Flash and R8 SRAM persistence together,
not only raw byte write/read.

## Build and regression
Original first link failed because intelfsh.cpp was absent from
the previous reduced dependency set. Regenerated the SAME reduced
MAME target with REGENIE=1 and PRECOMPILE=0 (old PCH invalid).
Build completed exit code 0 and linked the SAME ./alphasma3k.
No full multi-system MAME build, no alternative emulator binary.
- MAME -validate returned 0.
- -verifyroms asma3kdi/as3kdv/asma3ksb/asma3ksw: 4/4 OK,
  exit 0. Previous KS0066 F05 NEEDS REDUMP still remains.
- Genuine WRITER R4 original System3 boot path and entry
  0x004C7390 PASS with the Flash read wrapper.
- Historical R3 keyboard Phase1 ended with D0=0, PASS.
- R8 original WRITER SRAM state 'r8proof' restored on this
  new Flash-enabled build, pixel-identical to its R8 capture.
- All tests use separate private Flash/SRAM NVRAM directories.
- All original ROM files, R8 initial evidence, non-AS3K
  edits and private user files preserved.

## Scope limits
Install was stimulated directly over the emulated CPU Flash bus
using original manufacturer applet bytes; the original
AlphaSmart Manager installation TRANSFER PROTOCOL (USB/serial)
was not implemented or claimed. Nor are all 28F008B3 lock
features, WP#/VPP electrical protection and timing, top-boot
variant, power-loss transactionality or firmware updater flows
established. These are separate hardware questions, not
evidence of another emulator or a false applet installation.
R10 on the existing plan is external I/O, only if needed.

PRIVATE evidence (not committed): r9_flash_prog.lua/log,
r9_flash_erase.lua/r9_flash_erase_final.log,
r9_param_erase.lua/r9_param_erase.log,
r9_install_original_applet.lua/r9_install.log,
r9_launch_installed.lua/r9_launch_installed.log,
r9_restore_installed.lua/r9_restore_installed.log,
r9_installed_splash.png, r9_installed_text.png,
r9_restored_editor.png, r9_r8_sram_regression.png,
r9_nvram_command_test/, r9_parameter_test/,
r9_flash_install_test/, r9_r8_regression/,
r9_boot_regression.log, r9_r3_keyboard_regression.log.
