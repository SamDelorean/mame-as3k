# AS2000 bootstrap takeover — MAME preparation increment 2

Status: **CLOSED / STATIC PASS**

Scope: activate the bounded post-bootstrap path in the AS2000 driver without
yet exposing a runnable diagnostic system or loading the BT8 DictROM ROM set.

## Implemented in `alphasma.cpp`

### Dedicated diagnostic machine configuration

Added:

```cpp
void asma2k_state::asma2kbt(machine_config &config)
```

It reuses the normal AS2000 machine configuration and only marks the driver
for bootstrap diagnostic behavior.

The normal `asma2k` path remains unarmed.

## Diagnostic reset path

On `asma2kbt` reset:

1. execute normal AS2000 machine reset bookkeeping;
2. locate a ROM region named `stage0`;
3. require that it is exactly 27 bytes;
4. compare every byte against the frozen BT7 image;
5. enable the D0 diagnostic bootstrap mode;
6. inject the image at internal RAM `$0040`;
7. start execution there through the CPU diagnostic API.

Frozen accepted bytes:

```
0F8E00C3143F04143C201500708604B74000153C80153C407E4000
```

Any mismatch is fatal.

## Zero-Z execution guard

While diagnostic bootstrap mode is active:

```
PC >= $8000 -> fatalerror
```

This makes accidental execution of stock Z a hard failure rather than merely a
trace message.

The stock Z image may still remain loaded as a negative control.

## First DictROM fetch marker

The existing CPU instruction callback now records the first PC in:

```
$4000-$7FFF
```

and logs:

- PC;
- selected DictROM bank;
- PORTA;
- CTRL latch.

Expected successful first external instruction:

```
PC=$4000
bank=0
PA6=0
CTRL=$04
```

## Save-state additions

The AS2000 driver now saves:

- diagnostic mode flag;
- takeover-seen flag.

## Normal-machine isolation

No `COMP(as...bt)` entry or `ROM_START(asma2kbt)` exists yet.

Therefore this increment cannot accidentally replace or launch the normal
AS2000 system.

The core's HPRIO read path was also audited:

- diagnostic mode -> modeled HPRIO value;
- normal D0 mode -> `$FF`, preserving the prior unmapped-read behavior;
- writes remain inert outside diagnostic mode.

## Static regression

Added:

```
scripts/as2k_test_bootstrap_diag_driver.py
```

It verifies:

- exact 27-byte BT7 image guard;
- diagnostic config exists;
- stage-0 region injection path exists;
- zero-Z guard exists;
- first-DictROM-fetch logging exists;
- diagnostic system is not exposed before its ROM definition is added.

## Runtime status

This remains a **STATIC PASS**.

No claim is made yet that the modified MAME binary compiles or runs on the
target build environment.

## Next bounded increment

**MAME preparation increment 3 — runnable diagnostic ROM set**

Add only:

1. `ROM_START(asma2kbt)`;
2. exact 27-byte `stage0` ROM region;
3. 128 KiB `as2k_bt8_dictrom.bin` as the `spellcheck` region;
4. diagnostic `COMP` entry using `asma2kbt`;
5. expected CRC/SHA1 declarations;
6. build/run procedure.

After that, perform the first native compile and runtime attempt.

Do not add DebugTool yet.
