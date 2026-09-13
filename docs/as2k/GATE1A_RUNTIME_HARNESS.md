# AS2000 V3.14.x Phase 1A runtime harness

This note mirrors the firmware-side runtime gate in `SamDelorean/AS2K-V3.14.x` and keeps the MAME branch aligned with the current IR-purge contract.

Use the AS2000 development branch `as2k-mame0289-dev`, where `COL.7 / 0x10` is already mapped as Send/F12 through the normal `kb_irq` path.

The runtime capture must distinguish retained behavior from IrDA behavior:

- Send positive path: `$9716 -> $8606`;
- Send failure addresses: `$D2DC`, `$D099`;
- Print positive fallback: `$962D` or `$9804` -> `$ABC9`;
- Print failure address: `$D437`;
- forbidden reclaim execution: `$D098-$D487`, `$D499-$D517`, `$E103-$FFBF`;
- retained shared helper allowed: `$D488-$D498`.

Do not model IR removal by disabling the HC11 timer block or bypassing the keyboard matrix. The retained contract still includes keyboard IRQ assertion/clear behavior, `$2000/$9000` keyboard MMIO, PA6 I/O/RAM view selection, PA4-PA5 banking, Macintosh/ADB capture/compare timing, PORTD bits 0-1, and PORTA bits 0 and 2. PA7 remains unproven as reusable GPIO.

The firmware repository now carries a debugger command file and trace classifier so execution evidence can be reviewed reproducibly. A trace-level PASS does not by itself prove complete host-side keyboard emulation; if that external behavior is not yet modeled, report the result as a partial runtime pass rather than treating Send as removed or irrelevant.

No DynFS behavior is part of this emulator gate.
