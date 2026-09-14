# AS2000 Gate 1A runtime readiness checkpoint

Date: 2026-09-13
Branch: `as2k-mame0289-dev`
Reference commit: `86762fee099b2e16ce37457499f01894f775b3f5`

This note feeds current emulator regression evidence back into the AS2000 v3.1.4 IR-removal gate. It does not declare Phase 1A complete and does not authorize DynFS work.

## What is now demonstrated in MAME

The current AS2000 validation suite completes normal editor operation with fresh private NVRAM and separate-process recall. The retained regression set includes:

- keyboard transitions and ordinary editor input;
- F1-F8 file switching, isolation and restart recall;
- LCD controller activity and diagnostic text/pixel checks;
- persisted NVRAM text and recall;
- `Send` matrix input on `COL.7 / 0x10`, observed as released `0x10` -> pressed `0x00` -> released `0x10`;
- no demonstrated emulator defect requiring a CPU, video or keyboard-driver correction for these gates.

The normal keyboard IRQ/MMIO/banking contract remains unchanged. Do not simplify `$2000/$9000`, PA6, PA4-PA5, timer capture/compare or wired-host resources to make the IR test easier.

## What is not yet demonstrated

The above is a runtime prerequisite only. It is not evidence that the locally derived Phase 1A ROM has executed the required firmware detach path.

Phase 1A still requires a trace of the derived image proving, in order:

- Send: `$9716 -> $8606`;
- Print: `$962D` or `$9804` -> `$ABC9`;
- no execution of `$D2DC`, `$D099` or `$D437`;
- no PC in `$D098-$D487`, `$D499-$D517` or `$E103-$FFBF` during the required profile;
- `$D488-$D498` remains allowed as the shared keyboard/matrix helper.

Host-side PC/Mac keyboard transfer remains untested by the current Send matrix gate. Do not promote matrix recognition to a claim that external host transfer works.

## Firmware contract that must remain protected

- Send firmware keycode: `$47`.
- Retained cable/keyboard Send entry: `$8606`.
- Send must remain a user-visible retained function.
- Only the IrDA backend and its negotiation/init/ISR dependencies are being removed.
- Macintosh/ADB and PC/two-wire host paths remain protected.
- HC11 timer capture/compare remains shared retained hardware.

## Next MAME action

Run the strict Gate 1A trace workflow against the locally derived ROM with the firmware repository command file and classifier. The existing broad editor/NVRAM regression can then serve as smoke-test evidence around that focused trace rather than being rerun as proof of the detach itself.
