# AS2000 ZPSD Payload-1 / Payload-1b diagnostic model

Status: **PASS — bounded CPBF programming model**

The AS2000 diagnostic driver now models the minimum recovered ZPSD211R state
needed to execute Payload-1 and Payload-1b against one coherent device state.

Modeled state:

- `CPBF_raw`, stock `FF`, target `EF`;
- PB/PC programming bus values;
- PD control state;
- AS falling-edge address latch;
- SPECIAL acceptance on A19/CSI LOW->HIGH->LOW;
- ordered SPECIAL commands `04/80`, `00/08`;
- selected PORT index `0003`;
- exactly one external PSEN event supplied by the fixture;
- CPBF readback for Payload-1 verify;
- read-only CPBF diagnostic alias for Payload-1b.

The model never mutates the program ROM or the PAD/fuse grid. It is not a full
ZPSD emulator and does not model analog VPP amplitude or timing.

## Payload-1

Machines `asma2kp1` and `asma2kp1f` execute the same canonical 180-byte HC11
image. The PASS fixture accepts the sole programming event and changes CPBF
`FF->EF`; the FAIL fixture rejects it and leaves CPBF=`FF`.

Expected/observed terminal records:

`AS2K_P1 PASS special=2 addr=0003 data=EF pulses=1 cpbf=EF`

`AS2K_P1 FAIL special=2 addr=0003 data=EF pulses=1 cpbf=FF`

## Payload-1b

The current Payload-1b is the single-pass functional PB4 verifier/display
image from `AS2K-V3.14.x:zpsd-programmer-software`. It no longer reads the
raw CPBF diagnostic alias. Instead it exercises the recovered runtime Port-B
registers at `$3003/$3005/$3007` and then drives the real emulated KS0066 LCD.

Canonical corrected image:

- length: 185 bytes, `$0040-$00F8`;
- SHA256: `a8d03fb419f8bcb82e4f7332583de92a34faea1edf59c773e5d2658cb39a28c0`;
- PASS fixture: CPBF raw `EF`, PB4 follows DATA4 while DIR4=1, LCD displays `OK`;
- FAIL fixture: CPBF raw `FF`, PB4 does not follow DATA4 HIGH, LCD displays `ER`;
- both paths restore DATA4=0 and DIR4=0 before display hold.

Regression testing found and fixed a result-register lifetime bug in the first
183-byte single-pass image: `LCD_BYTE` clobbered register B. The corrected
185-byte image preserves B with `PSHB/PULB` around LCD initialization.

`asma2k1bp` and `asma2k1bu` run the exact same 185-byte payload with only the
modeled persistent CPBF state differing.
