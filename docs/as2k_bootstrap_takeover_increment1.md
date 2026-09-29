# AS2000 bootstrap takeover — MAME preparation increment 1

Status: **CLOSED / STATIC PASS**

Scope: implement only the HC11D0 core support required by the bounded
post-bootstrap diagnostic design. Do not activate it in the AS2000 driver yet.

## Implemented

### HPRIO $003C

The D0 register map now exposes a diagnostic subset of HPRIO.

Diagnostic reset state:

```
RBOOT=1
SMOD=1
MDA=0
HPRIO=$C0
```

Supported transition semantics while SMOD=1:

- MDA may change;
- RBOOT may transition 1->0;
- SMOD may transition 1->0;
- RBOOT/SMOD cannot be reasserted after being cleared.

Once SMOD=0, further diagnostic HPRIO writes are ignored until reset.

Outside explicitly enabled diagnostic bootstrap mode, HPRIO writes are inert
and reset value remains the legacy-neutral $00.

### Post-bootstrap RAM injection API

New D0 API:

```cpp
set_diag_bootstrap(bool)
diag_bootstrap_load(data, size, address=$0040)
hprio()
```

The loader:

1. checks that the requested image fits HC11 internal RAM;
2. writes it through the CPU program space;
3. clears the core's pending RESET interrupt;
4. leaves WAIT/STOP state;
5. restores reset-style I/X masking;
6. starts PC/PPC at the injected address.

This represents the state **after** successful Special Bootstrap SCI download.
It does not emulate serial timing or the bootstrap ROM.

### NOCOP

Diagnostic load forces:

```
CONFIG.NOCOP=1
```

before starting the injected code.

### Save state

HPRIO state is registered with MAME save-state infrastructure.

## Normal-mode isolation

Increment 1 deliberately does not modify `alphasma.cpp`.

Therefore neither:

```
set_diag_bootstrap(...)
diag_bootstrap_load(...)
```

is called by the normal AS2000 machine.

No diagnostic bootstrap behavior is active unless a later dedicated machine
configuration opts in.

## Static regression

Added:

```
scripts/as2k_test_bootstrap_diag_core.py
```

It asserts:

- HPRIO mapping exists;
- diagnostic mode defaults inactive;
- exact MDA/RBOOT/SMOD transition code exists;
- post-bootstrap entry clears RESET wait state;
- PC is set to injected RAM entry;
- normal AS2000 driver does not activate the feature.

This is a source-level guard only. A native MAME build remains required after
the driver-side increment is applied.

## Files changed

```
src/devices/cpu/mc68hc11/mc68hc11.h
src/devices/cpu/mc68hc11/mc68hc11.cpp
scripts/as2k_test_bootstrap_diag_core.py
```

## Next bounded increment

**MAME preparation increment 2 — AS2000 diagnostic driver activation**

Only:

1. add a dedicated diagnostic AS2000 machine configuration;
2. load the exact 27-byte stage-0 region;
3. call `set_diag_bootstrap(true)`;
4. inject stage-0 at $0040 on reset;
5. add zero-Z fetch guard and first-DictROM-fetch logging.

Do not yet add the final DebugTool image.
