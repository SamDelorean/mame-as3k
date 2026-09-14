# AS2000 Gate 1A local execution handoff

This note mirrors the runtime contract maintained by `SamDelorean/AS2K-V3.14.x` for the IR-detach gate.

No new keyboard, IRQ, MMIO, timer or banking change is required before the trace run.

Current emulator prerequisites already established on this branch:

- Send is `COL.7 / 0x10` and follows the normal keyboard IRQ path.
- The observed matrix transition is released `0x10` -> pressed `0x00` -> released `0x10`.
- Editor input, LCD diagnostics, F1-F8 switching and NVRAM recall pass the local regression suite.
- `$2000/$9000`, PA6, PA4-PA5 and retained timer/capture-compare behavior remain part of the observable contract.

The next operation is a firmware trace using a locally derived `E0_IR_PURGE_DETACH_v0.1` ROM. Do not force the CPU PC or bypass normal key input.

Required ordered evidence:

- Send: `$9716` -> later `$8606`
- Print: `$962D` or `$9804` -> later `$ABC9`

Forbidden execution:

- `$D2DC`
- `$D099`
- `$D437`
- `$D098-$D487`
- `$D499-$D517`
- `$E103-$FFBF`

`$D488-$D498` remains an allowed shared keyboard/matrix helper.

A firmware routing pass at `$8606` does not claim that external PC/Mac host transfer is completely modeled. Host keyboard-transfer coverage must remain a separately reported emulator capability; it must not be approximated by re-enabling or simplifying IrDA paths.

The canonical command file and trace classifier live in `SamDelorean/AS2K-V3.14.x`:

- `tools/mame_gate1a.cmd`
- `tools/check_mame_gate1a_trace.py`
- `docs/GATE1A_LOCAL_RUNBOOK.md`

Phase 1B ROM overwrite safety is a separate gate; in particular, the `$E102-$E104` high-boundary ownership question must not be inferred from a successful runtime trace alone.
