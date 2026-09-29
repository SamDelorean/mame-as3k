# AlphaSmart 2000 ZPSD211R static decode notes

Date: 2026-09-28
Branch target: as2k-mame0289-dev

## Why this exists

The current AS2000 driver models the functional result of the board glue rather than the ZPSD211R itself. Static reverse engineering of the v3.1.4 ZPSD programming trailer now provides enough information to define a future diagnostic/fidelity layer without changing the stable map yet.

## Stable driver-visible contract

Keep these functional behaviors unchanged:

- PA6=1: RAM view at 0x0000-0x7fff.
- PA6=0: I/O + DictROM view.
- PA4/PA5 select four 32 KiB RAM banks.
- PA5/PA4/CTRL7 select eight 16 KiB DictROM banks.
- 0x2000: keyboard read / matrix-high write.
- 0x4000 write: LCD/control latch.
- 0x4000-0x7fff read: DictROM.
- 0x9000 write: matrix-low selector.
- 0x8000-0xffff read: main ZPSD program ROM.

## Static ZPSD conclusions useful to MAME

- CSIOPORT base is strongly anchored at 0x3000.
- PB0-PB7 are stock CS0-CS7 outputs, not MCU-I/O.
- PC0-PC2 are stock PAD inputs, not external CS8-CS10 outputs.
- Port C does not participate in any recovered stock decode that feeds ES0-ES7, CSIOPORT, or CS0-CS7.
- pair6 ~= A17 and pair7 ~= A18 are strong structural assignments; pair0/pair8/pair11 remain an unordered {A16,A19/CSI,AS} set.

## Physical decode/mirror model

The simplified driver hides address mirrors produced by the ZPSD equations. A diagnostic layer may eventually expose/log them:

- keyboard-high class: approximately 0x2000-0x27ff.
- keyboard-low write class: approximately 0x9000-0x9fff.
- 0x4000 control write class: repeated 0x800-byte subwindows inside 0x4000-0x7fff.
- DictROM select class: full 0x4000-0x7fff read window plus lower read aliases.
- global read-enable class: CS7 = R/W * E.

These are physical-decode hypotheses from the recovered fusemap. Do not replace the existing functional handlers with mirror mappings until regression behavior is tested.

## Memory-control hypotheses

- PA6 -> SRAM CE2 (direct or through glue) is a near-closed functional hypothesis. Stock firmware clears PA6 immediately before dropping the 0x4000 power-hold, matching CE2-low standby.
- PB7/CS7 is a strong candidate for common read-enable (/OE SRAM, G DictROM).
- PB1/CS1 is a strong candidate for DictROM E(/CE) or its immediate gate.
- SRAM /WE must be generated from E + write phase of R/W outside the recovered PAD CS terms.
- SRAM /CE1 is not any single CS0-CS7 general output.

## Future diagnostic implementation

When implementing a fidelity mode, prefer instrumentation before remapping:

1. calculate candidate CS0-CS7 assertions from address/RW/E;
2. log discrepancies between simplified handlers and physical-decode mirrors;
3. keep Port C inputs observable but inert for stock decode;
4. log PA6 transitions around sleep/power-off;
5. retain the stable map as the default until tests close.

No proprietary ROM or trailer data should be committed here.
