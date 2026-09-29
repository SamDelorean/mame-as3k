# AS2000 bootstrap takeover — MAME preparation increment 3

Status: **CLOSED / STATIC PASS**

Scope: expose the bounded bootstrap diagnostic as a runnable MAME system and
freeze the exact ROM inputs for the first native compile/runtime attempt.

## Runnable diagnostic system

Added:

```
asma2kbt
```

Parent:

```
asma2k
```

Machine configuration:

```
asma2kbt
```

Input definition:

```
asma2k
```

Display name:

```
AlphaSmart 2000 (Bootstrap Takeover Diagnostic)
```

## ROM regions

### maincpu — stock Z negative control

The diagnostic clone declares the same stock v3.1.4 Z file/hash already used
by the normal AS2000 driver:

```
alphasmart__2000__v3.1.4__h4.zpsd211r.plcc44.bin
CRC32 49487f6d
SHA1  e0b777dc68c671c31ba808e214fb9d2573b9a853
```

The file is not generated, copied, or distributed by this work.

Because `asma2kbt` is a clone of `asma2k`, MAME may resolve the shared stock
file from the parent set.

Its purpose here is negative control only: the runtime guard aborts if PC ever
reaches $8000-$FFFF.

### stage0 — exact BT7 image

```
filename as2k_stage0.bin
size     27 bytes
CRC32    ff5dedf9
SHA1     ab76eafa386311b2ab70ea644345fb15767e908f
SHA256   b46282c155c6656eb21cca8994213622e824e2619fa18d023ce5c108f35a4039
```

### spellcheck — BT8 test DictROM

```
filename as2k_bt8_dictrom.bin
size     131072 bytes
CRC32    c5bd89df
SHA1     6e84689da9e0705920bca24137c88a387dedfac1
SHA256   d76819d7f04b842a48cb3ed62ac4cd78fde9599bc9fdcf76fb704ea3850de4b4
```

Bank 0 begins with the 18-byte BT8 landing payload; the remainder of the
128 KiB image is $FF.

## Deterministic ROM preparation

Added:

```
scripts/as2k_prepare_bootstrap_roms.py
```

Run from the MAME source root:

```
python3 scripts/as2k_prepare_bootstrap_roms.py
```

It creates:

```
roms/asma2kbt/as2k_stage0.bin
roms/asma2kbt/as2k_bt8_dictrom.bin
```

The script validates CRC32/SHA1/SHA256 before writing the files.

It deliberately does **not** handle the stock Z ROM.

## Parent ROM requirement

Keep the existing normal AS2000 parent ROM set available through the MAME ROM
path. For example:

```
roms/asma2k/
  alphasmart__2000__v3.1.4__h4.zpsd211r.plcc44.bin
```

No stock ROM bytes are committed by this diagnostic work.

## Static regression

Added:

```
scripts/as2k_test_bootstrap_diag_romset.py
```

It verifies:

- one `ROM_START(asma2kbt)`;
- correct stock negative-control declaration;
- exact stage-0 CRC/SHA1;
- exact BT8 DictROM CRC/SHA1;
- one diagnostic COMP entry;
- parent remains `asma2k`.

## First native compile procedure

The exact build command depends on the already-used local MAME build setup.
The minimal validation after rebuilding the existing AS2K target is:

```
<rebuilt-mame> -listfull asma2kbt
```

Expected: the diagnostic system is listed.

Then audit ROM resolution:

```
<rebuilt-mame> -verifyroms asma2kbt
```

Expected:

- stock v3.1.4 Z resolved from the available parent/ROM path;
- `as2k_stage0.bin` GOOD;
- `as2k_bt8_dictrom.bin` GOOD.

## First runtime procedure

Run:

```
<rebuilt-mame> asma2kbt -window -verbose
```

Required log sequence:

```
AS2K_BOOT INJECT stage0 size=27 entry=0040 HPRIO=C0
HC11D0_DIAG HPRIO C0 -> E0
...
AS2K_BOOT FIRST_DICT_FETCH pc=4000 bank=0 PA=00 CTRL=04
```

No line matching:

```
AS2K_BOOT forbidden Z fetch
```

may occur.

The BT8 landing then should remain at $4010 and internal RAM $00A0-$00A3
should contain:

```
42 54 38 21  = "BT8!"
```

A later regression helper may automate observation of that RAM signature after
the first native runtime succeeds.

## Current evidence level

- ROM declaration: **STATIC PASS**
- hashes/layout: **PASS**
- runtime ROM files: reproducibly generatable
- native C++ compile: **PENDING**
- MAME runtime: **PENDING**
- BT8 signature in native MAME: **PENDING**

No physical BT9 claim is made.

## Next bounded increment

Perform the **first native MAME compile and runtime attempt** on the existing
MAME build machine.

If compile errors are found, fix only errors attributable to this bounded
bootstrap diagnostic patch.

If the machine starts, verify:

1. injection at $0040;
2. HPRIO C0->E0->60->20;
3. first DictROM fetch $4000 bank0;
4. zero Z fetches;
5. PC stabilizes at $4010;
6. internal RAM signature = `BT8!`.

Do not integrate DebugTool until this diagnostic path runs successfully.
