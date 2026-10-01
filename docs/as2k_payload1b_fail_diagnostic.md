# AS2000 Payload-1b FAIL display diagnostic

Machine: `asma2k1bf`

Purpose: exercise the visible FAIL branch of Payload-1b before physical ZPSD
testing is available.

This fixture does **not** claim to emulate ZPSD211R Port B registers. The current
AS2000 MAME driver does not yet model `$3003/$3005/$3007`.

Instead, it begins at the point where the real Payload-1b orchestrator has
already decided that the PB4 functional test failed. It then executes the exact
LCD code that the real hardware path will use.

## Image identity

Donor: frozen G0 Payload-0, 127 bytes.

Donor SHA256:

`54582354a972848c642b2c87893ae2cc693483eae3d161693edf8f10d26f634a`

Only the final eight bytes change:

`BOOT OK\0 -> FAIL PB\0`

FAIL image:

- size: 127 bytes
- CRC32: `e0e98168`
- SHA1: `ff3136f95aff8f74c77eb968ecb684af2905ce40`
- SHA256: `7469f23dbe03b3a35cfac66fc4be5e107d15498542a892f947b70c900cc288a`

## Expected result

Run:

`mame asma2k1bf -rompath <romroot>`

The LCD should show:

`FAIL PB`

The driver logs exactly once when the Payload-0 hold loop is reached:

`AS2K_P1B_FAIL DISPLAY_READY pc=008D message=FAIL PB`

The machine deliberately remains running at the hold loop so the message can be
visually inspected.

## Scope

This validates:

- the FAIL decision-to-message mapping;
- the patched Payload-0 image;
- bootstrap RAM execution;
- the existing AS2K LCD path;
- visual FAIL presentation.

It does not validate:

- real PB4 electrical behavior;
- ZPSD Port B register semantics;
- VPP/programming;
- the physical ATtiny transport.

Those remain separate gates.
