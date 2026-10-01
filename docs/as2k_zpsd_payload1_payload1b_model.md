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

Machines `asma2k1bp` and `asma2k1bu` use the same `CPBF_raw` representation.
They run the same 31-byte verifier with initial state `EF` or `FF`.

Expected/observed records:

`AS2K_P1B_CPBF PASS cpbf=EF result=79 pc=0056`

`AS2K_P1B_CPBF FAIL cpbf=FF result=1F pc=005D`

The `$3011` CPBF alias is emulator-only and must not be interpreted as a real
HC11-visible ZPSD register.
