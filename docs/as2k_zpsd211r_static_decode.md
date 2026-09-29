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
- AS2K fuse-row map is now closed: pair0=A16, pair1=A12, pair2=A13, pair3=A14, pair4=A15, pair5=A11, pair6=A17, pair7=A18, pair8=A19/CSI, pair9=R/W, pair10=E, pair11=AS.

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


## v1.1: exact AS2K fuse-row map from PSDsoft calibration

A real PSDsoft/PSDabel ZPSD311 project with known source equations and compiled .FOB/.OBJ data was used as a calibration vector. It directly confirms the control/address row positions and exposes one device-family difference: ZPSD311 places A11 at pair0 and A16 at pair5, while the AS2K ZPSD211R CSIOPORT decode proves pair5=A11.

Use this AS2K-specific table in any diagnostic decoder:

```text
pair0  = A16
pair1  = A12
pair2  = A13
pair3  = A14
pair4  = A15
pair5  = A11
pair6  = A17
pair7  = A18
pair8  = A19/CSI
pair9  = R/W
pair10 = E
pair11 = AS
```

Port-config object fields are now directly calibrated:
- 0x81E1 = CPAF1, complementary encoding;
- 0x81E3 = CPBF, complementary encoding;
- 0x81E4 low3 = CPCF, complementary encoding.

For the stock AS2K this means PA0-PA7 are latched A0-A7, PB0-PB7 are CS0-CS7, and PC0-PC2 are A16/A17/A18-class PAD inputs.

The JSON companion in this directory is intended as machine-readable input for future ZPSD diagnostic instrumentation. Do not hard-code the ZPSD311 A11/A16 row order into generic tooling.


## PB4/CS4 Shared-EXT static contract

Static reverse engineering closes PB4 as the preferred internal EXT candidate without changing the current emulator.

- stock PB4 mode: CS4
- CS4 PAD slots: 26 and 27
- both slots are CONTRADICT across all 12 literal pairs
- CS4 therefore never asserts
- stock CPBF=0x00, raw object 0x81E3=0xFF
- target CPBF=0x10, raw object 0x81E3=0xEF
- only CPBF4 changes; the PAD fuse grid does not
- runtime PB4 mask is 0x10
- PB Direction=0x3005, PB Data=0x3007
- both registers reset to 0 in MCU-I/O mode
- EXT must be externally biased low during reset

Future banking contract:
- RAM bank = (EXT<<2)|(PA5<<1)|PA4
- AppROM page = (EXT<<3)|(PA5<<2)|(PA4<<1)|CTRL7

No MAME runtime behavior is changed now. Dynamic PB4/EXT validation is deferred until October 2026 by project decision. See docs/as2k_pb4_ext_test_plan.md.


## PB4/EXT canonical object payload

The static configuration payload is closed without changing MAME runtime behavior:

- object address 0x81E3: FF -> EF;
- stock record: `:0581E000FF00FFFF0796`;
- target record: `:0581E000FF00FFEF07A6`;
- all program bytes, PAD fuse-grid bytes, global config bytes and other port config bytes remain invariant.

This is a canonical Intel-HEX payload contract, not proof of a programmer-valid PSDsoft .FOB wrapper. The machine-readable contract is `docs/as2k_zpsd_pb4_ext_object_contract.json`.

Dynamic MAME validation remains deferred until October 2026.


## PB4/EXT deferred runtime contract — 2026-09-28

Static work has now closed the PB4/EXT object and electrical contract, but runtime testing is intentionally deferred until October 2026.

When testing resumes, use this exact contract:

- reset: PB4 is input/Hi-Z and external pull-down keeps EXT=0;
- initialization: DATA4=0 before DIR4=1;
- EXT=0 preserves RAM banks 0-3 and DictROM/AppROM pages 0-7;
- EXT=1 selects RAM banks 4-7 and DictROM/AppROM pages 8-15;
- no ZPSD PAD equation changes are required for PB4 conversion;
- physical decode mirrors remain diagnostic-only and must not change the stable map during the first EXT tests.

No runtime PASS should be recorded before those tests execute.
