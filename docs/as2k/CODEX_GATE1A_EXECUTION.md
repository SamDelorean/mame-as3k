# Codex Handoff — AS2000 Gate 1A Runtime Execution

This MAME branch is the execution target for the AS2000 v3.1.4 IR-detach Gate 1A. The firmware-side task is defined in `SamDelorean/AS2K-V3.14.x` as `docs/CODEX_GATE1A_TASK.md`.

## Do not change these observables to force the gate

- Send remains `COL.7 / 0x10` and must enter through the normal keyboard IRQ path.
- `$9000` selector-low writes clear the keyboard IRQ as currently modeled.
- `$2000` remains the keyboard read / selector-high access in the I/O view.
- PA6 remains the I/O/RAM view selector.
- PA4-PA5 remain part of RAM/DictROM banking.
- HC11 timer/capture-compare behavior remains available to retained wired transports.
- PC/two-wire and Macintosh/ADB behavior must not be disabled to simplify IR removal.

## Required firmware trace evidence

Run the locally derived `E0_IR_PURGE_DETACH_v0.1` image and collect the full CPU trace required by the firmware project. A valid control-flow result must show:

- `$9716 -> $8606` after a normal Send press;
- `$962D` or `$9804` -> `$ABC9` for Print;
- no `$D2DC`, `$D099`, or `$D437`;
- no PC in `$D098-$D487`, `$D499-$D517`, or `$E103-$FFBF`;
- `$D488-$D498` is the retained shared-helper exception.

Reaching `$8606` proves firmware routing only. It must not be reported as proof that complete PC/Mac host-keyboard output is already modeled.

If a new emulator-specific observation is needed to explain the trace, document it here or in a dedicated `docs/as2k/` note and feed it back to `SamDelorean/AS2K-V3.14.x`. Do not begin DynFS work in this repository as part of Gate 1A.