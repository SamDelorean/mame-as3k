# R7 — Genuine System 3 AlphaWord Plus boot and typing, 2026-10-31

STATUS: CLOSED / ORIGINAL_APPLET_DISCOVERY_PASS / ALPHAWORD_LAUNCH_PASS /
EDITOR_TYPING_PASS / BUILD_PASS / REGRESSION_PASS.

One and only AS3K-DIAG: canonical mame-as3k-dev MAME 0.289 reduced
single executable alphasma3k. No new branch/clone/alternate emulator,
no ROM patch, no fake PC, no new applet, no Codex, no push.

## Original source and Flash evidence
The two 1-MiB private System3 images remain unchanged.
At Flash 00408000 a real MAME CPU program-space READ watchpoint
at PC=004C3254 showed the header FFFFFFFF for BOOT and the
original C0FFEEAD SmartApplet signature for WRITER.
The WRITER image contains the 101596-byte manufacturer
AlphaWordPlus.OS3KApp installed at offset 0x8000 and authentic
applet name AlphaWord Plus. Firmware is System3 2005.
Thus a genuine firmware scan distinguishes installed applet
from blank Flash, not driver logic hardcoded for a ROM release.

## First-start integrity dialog and restart
Original System3 warns 'An unexpected data change occurred.'
MAME natural keyboard sent Y, firmware returned native code 0F,
and System3 displayed an actual 'Your AlphaSmart needs to be
restarted... Press the enter key to restart' message on the
emulated dual KS0066 screen. Natural Return/Enter triggered
the ROM's authentic restart path. The original AlphaWord Plus
3.0 / System 3.01 splash appeared, then native editor display.
The underlying first-boot integrity condition is not proven
permanently resolved; do not assert persistent Flash/RAM repair.

## Hardware blocker: AS3000 battery detect, not firmware patch
The applet initially displayed genuine 'Low Battery / Install new
batteries or recharge' then stopped progressing.
Manufacturer original Software/ModuleSources/PowerModule.c,
PowerIsBatteryLow reads PGDATA bit4 (POWER_LOW_BATTERY_BIT=0x10):
a LOW input means battery fault, HIGH means good power.
The MC68EZ328 lacked an external board signal on PG4.
Added a real MAME BATTERY configuration field with default
Normal (PG4=1) and selectable Low (PG4=0), using the existing
MC68EZ328 in_port_g<4> callback. No ROM patches or bypassed
firmware condition. With normal PG4, low-battery alert ceased.

## Hardware blocker: keyboard interrupt wakeup
After entering the editor with a flashing cursor, CPU was stopped
at original System3 STOP around 004D12BE, with observed PC
004D12C4 on periodic Lua sampling from emulated seconds 10..20.
Natural 'hello' was queued but did not render.
A real CPU write watchpoint verified System3 writes PDKBEN=FF
to 0xFFFFF41E at PC=004CE98C; old EZ core lacked that register.
Recovered original KeyboardModule.c:
KeyboardEnableKeyboardInterrupt enables interrupt source 4,
forces all columns low and writes M328_PKBDINT=FF; handler
disables interrupt and writes 00 to return to matrix polling.

Implemented EZ-only PDKBEN R/W at 0xFFFFF41E, reset 00,
save-state, active-low keyboard OR source as MC68EZ328 INT_KB
mapped to CPU IRQ level 4. PDKBEN masks rows, row input
direction gates and current physical levels determine assertion.
No per-row pseudo-interrupts from the original MC68328 are
used for the EZ new route. The AS3K board now invokes
port_d_in_w whenever any of its 77 mapped host keys changes,
latch columns change or PA outputs change, allowing a
STOPped CPU to wake on a genuine host key contact.
No firmware-specific key injection or synthesized text in
the LCD bridge.

## Functional end-to-end result
With unchanged WRITER ROM and default normal PG4:
first-boot original integrity prompt -> Y -> restart prompt ->
Return -> original 'AlphaWord Plus 3.0 / System 3.01' splash ->
native blank editor/cursor -> host natural keyboard 'hello' ->
real KS0066 LCD screenshot with text 'hello' followed by cursor.
Saved private screenshots r7_before_typing.png and
r7_after_typing.png are both 240x36.
Pixel evidence: 5 active non-background pixels before typing,
100 afterward, with visible readable 'hello'.
No text is embedded by the emulator; MAME keypost passes through
the genuine board GPIO matrix, EZ masked interrupt, original
firmware and LCD controller.

## Validation and explicit exclusions
- Same minimal MAME target compiled and linked: return 0.
- MAME -validate: return 0.
- Four ROM audits: asma3kdi, asma3kdv, asma3ksb,
  asma3ksw, 4/4 OK, exit 0. Known real KS0066 F05
  NEEDS REDUMP warning remains.
- Historical R3 KeyboardInitializeModule Phase1 regression:
  SCR/PAPUEN, latch FF and RTS D0=0, caller D0=0: PASS.
- No-key LCD snapshots from both original BOOT and WRITER
  remain pixel-identical to previously credited R5 snapshots.
- Firmware-independent hardware change only in existing
  alphasma3k.cpp and EZ subclass mc68328.cpp/.h.
- PDSEL upper-nibble mux not implemented as part of this
  direct path, and not credited as PASS. No claim of complete
  keyboard IRQ edge mode, all 77 typed keys tested, persistent
  documents, Flash writes, saving/printing, mouse or USB.
- First-boot data-change prompt may recur on a fresh RAM
  image until persistence is implemented; do not assert a
  permanent solution. R8 is the established storage milestone.

## Original source provenance, private evidence
Manufacturer sources already on mounted Google Drive:
ALPHASMART/Soruce code/Software/ModuleSources/PowerModule.c,
KeyboardModule.c; canonical SmartApplets3000/OS3KROM.OS3KOS,
SMALLOS3KROM.OS3KOS and AlphaWordPlus.OS3KApp.
Runtime logs/scripts/screenshots remain private in
../private/roms/as3k/mame_sets/:
r7_applet_asma3ksb.log, r7_applet_asma3ksw.log,
r7_y_flow_with_oslog.log, r7_after_y.png,
r7_after_restart.png, r7_before_typing.png,
r7_after_typing.png, r7_keyboard_irq_probe.cmd,
r7_pdkben_probe.log, r7_editor_type.lua,
r7_after_irq_run.log, r7_pc_timeline.log,
r7_keyboard_irq_build.log, r7_validate.log,
r7_rom_audit.log, r7_r3_phase1_regression.log,
r7_regression_images/.
No proprietary binary/screenshots committed to public Git.

NEXT: R8 original storage/persistence under existing plan,
in the SAME emulator. R8 was not started in R7.
