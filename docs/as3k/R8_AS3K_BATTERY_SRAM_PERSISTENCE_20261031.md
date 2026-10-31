# R8 — Battery-backed SRAM / original AlphaWord persistence — 2026-10-31

STATUS: CLOSED / SRAM_PERSISTENCE_PASS / EDITOR_RECOVERY_PASS /
MINIMAL_BUILD_PASS / 4-OF-4_ROM_AUDIT_PASS.
Canonical single AS3K-DIAG MAME 0.289, existing alphasma3k executable.
No branches, clones, proprietary ROM modifications or unrelated work.

## Physical model and original memory address
The AS3000 board has a 256 KiB HY62U8200 external SRAM already mapped
to CPU physical 0x00000000–0x0003FFFF. Previously, the MAME ram_device
RAM_TAG lost its bytes between program invocations. MAME has native
nvram_device in several existing drivers. We reused it rather than
writing an artificial AlphaWord file system or interpreting firmware data.

Only src/mame/skeleton/alphasma3k.cpp changed:
- include machine/nvram.h;
- required nvram_device at tag nvram;
- NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0);
- bind the EXISTING external SRAM pointer and actual size with
  m_nvram->set_base(m_ram->pointer(), m_ram->size()) during machine_start.
The vector reset procedure is unchanged. No additional RAM, ROM
modification, synthetic applet persistence, or new executable.
Normal MAME NVRAM identity keeps ROM-set files separate as data.

## Independent process test: WRITER
First launch (original asma3ksw in an isolated PRIVATE NVRAM directory):
original integrity warning Y -> firmware restart Return ->
original AlphaWord Plus editor -> MAME natural keyboard typed r8proof.
The screenshot r8_after_text.png is a real 240x36 KS0066 LCD output
reading r8proof with cursor. MAME exited normally, rc=0, writing
r8_nvram_test/asma3ksw/nvram, exactly 262144 bytes.
The first saved file SHA256 was
869ef14ad35bef6da5dc94f062f2a2936e365fdee7682edde1ac5a94c2a9881d.

Second launch was a NEW process, same emulator and same original ROM,
same private NVRAM directory, with NO keyboard input. At emulated t=10s,
System3 was executing the editor, PC=004D12C4; screenshot
r8_after_reopen.png showed the identical r8proof plus cursor.
Pixel difference versus before-close r8_after_text.png was EMPTY:
pixel-perfect 240x36 match. The original first-run integrity warning
did not recur in this resumed session. This demonstrates document
recovery, not only that an NVRAM file was created.
No assumption that the raw RAM contains contiguous ASCII r8proof.

Original BOOT asma3ksb independently created its own 262144-byte NVRAM
file in the same private test folder, with zero extra emulator code.

## Regression / remaining scope
- Same reduced MAME build and link exit 0.
- -validate exit 0.
- Audit of asma3kdi, asma3kdv, asma3ksb, asma3ksw: 4/4 OK,
  exit 0; existing KS0066 F05 NEEDS REDUMP notice unchanged.
- Historical R3 KeyboardInitializeModule Phase1 returned normally
  with PAPUEN read FF/write 80, latch FF, D0=0 at 00420168.
- This proves clean-exit external SRAM persistence and real AlphaWord
  text recovery. It does NOT prove crash-safe atomic writes, every file
  management function, raw Flash write/erase, applet installation
  or updating System3 firmware. Those are NOT credited.
- R9 is the existing next milestone: Flash / SmartApplet write fidelity,
  strictly the SAME AS3K-DIAG.

All binary NVRAM, screenshots, scripts and runtime logs stay PRIVATE:
../private/roms/as3k/mame_sets/r8_first_session.lua
../private/roms/as3k/mame_sets/r8_second_session.lua
../private/roms/as3k/mame_sets/r8_first_session.log
../private/roms/as3k/mame_sets/r8_second_session.log
../private/roms/as3k/mame_sets/r8_after_text.png
../private/roms/as3k/mame_sets/r8_after_reopen.png
../private/roms/as3k/mame_sets/r8_nvram_test/
../private/roms/as3k/mame_sets/r8_build_20261031.log
../private/roms/as3k/mame_sets/r8_validate.log
../private/roms/as3k/mame_sets/r8_rom_audit.log
../private/roms/as3k/mame_sets/r8_r3_keyboard_regression.log
Do not commit user documents or external firmware in Git.
