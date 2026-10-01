# AS2000 Payload-1b CPBF static diagnostic

Machines: `asma2k1bp`, `asma2k1bu`

Both machines execute the same independently built 31-byte HC11 verifier.
The only fixture difference is the diagnostic CPBF byte exposed at `$3011`:

- `asma2k1bp`: `0xEF` -> PASS (`0x79` in `$00BF`);
- `asma2k1bu`: `0xFF` -> FAIL (`0x1F` in `$00BF`).

`$3011` is an emulator-only alias. It is not a claimed physical ZPSD211R
register. The recovered object-level fact is CPBF `FF -> EF` at object offset
`0x81E3`, with the programming-interface PORT selector using internal index 3.

This diagnostic performs no ZPSD write and does not model VPP, PSEN, SPECIAL
programming pulses, or Payload-1. Its purpose is only to execute and validate
the Payload-1b CPBF compare/decision logic against deterministic programmed and
unprogrammed states.

Expected logs:

`AS2K_P1B_CPBF PASS cpbf=EF result=79 pc=0056`

`AS2K_P1B_CPBF FAIL cpbf=FF result=1F pc=005D`

## Executed result

The instrumented `as2kdiag` build completed successfully. The same 31-byte
verifier image produced:

- PASS fixture: `AS2K_P1B_CPBF PASS cpbf=EF result=79 pc=0056`;
- FAIL fixture: `AS2K_P1B_CPBF FAIL cpbf=FF result=1F pc=005D`.

Both runs returned 0.
