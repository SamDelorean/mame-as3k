# AS2000 bootstrap takeover — bounded MAME diagnostic design

Status: **DESIGN FROZEN / IMPLEMENTATION OPTIONAL**

This document defines the smallest MAME adaptation needed to exercise the
real BT7 stage-0 and BT8/DebugTool DictROM path without implementing a full
MC68HC11D0 SCI bootstrap ROM.

The goal is diagnostic fidelity, not a general expansion of the HC11 core.

## 1. What the current core already provides

Current `MC68HC11D0` in the AS2K branch already provides:

- 192 bytes internal RAM;
- the normal register block at $0000-$003F;
- CONFIG and INIT;
- PORTA behavior used by the AS2K driver;
- normal HC11 instruction execution;
- instruction callback tracing;
- the AS2K lower-half memory view;
- PA6 switching between DictROM/I/O and external RAM;
- PA4/PA5 + CTRL bit7 DictROM bank selection.

The AS2K driver already maps:

```
PA6=0:
  $4000-$7FFF -> DictROM selected bank

PA6=1:
  $0000-$7FFF -> external RAM selected bank

$8000-$FFFF -> stock Z
```

## 2. Missing pieces that block the real stage-0

The current D0 core does not provide:

1. HPRIO at $003C;
2. reset mode state for RBOOT/SMOD/MDA;
3. Special Bootstrap entry behavior;
4. functional SCI download;
5. special-mode vector behavior.

BT-G0 does not need items 4 or 5 to validate the post-download stage-0.

## 3. Diagnostic strategy

Do **not** emulate the serial bootstrap bitstream inside MAME.

Instead add a diagnostic bootstrap injection mode that represents the state
*after* the documented HC11 boot ROM has successfully received and copied the
stage-0 image.

MAME then executes the exact stage-0 bytes.

Conceptual flow:

```
machine reset
 -> diagnostic bootstrap selected
 -> copy exact stage0.bin to internal RAM $0040
 -> initialize D0 bootstrap state
      PC=$0040
      HPRIO=RBOOT|SMOD
      MDA=0
      CONFIG.NOCOP=1
      CCR I=1, X=1
 -> release CPU
 -> execute the real 27-byte stage-0
 -> stage-0 writes HPRIO itself
 -> MDA=1
 -> CTRL=$04 / bank0
 -> RBOOT=0
 -> SMOD=0
 -> JMP $4000
 -> execute test DictROM
```

This intentionally bypasses only the serial transfer mechanism. It does **not**
bypass the stage-0 mode transition.

## 4. Minimal D0 core patch

### 4.1 New state

Add to `mc68hc11d0_device`:

```cpp
uint8_t m_hprio;
bool    m_diag_bootstrap;
```

Optional diagnostic configuration interface:

```cpp
void set_diag_bootstrap(bool enable);
bool diag_bootstrap() const;
uint8_t hprio() const;
```

Save `m_hprio` for save states.

### 4.2 HPRIO register

Map D0 register $003C:

```cpp
block(base + 0x3c, base + 0x3c)
    .rw(FUNC(mc68hc11d0_device::hprio_r),
        FUNC(mc68hc11d0_device::hprio_w));
```

Diagnostic semantics required by BT7:

```
reset diagnostic bootstrap: RBOOT=1 SMOD=1 MDA=0

while SMOD=1:
  MDA may be set/cleared;
  RBOOT may be cleared;
  SMOD may be cleared.

once SMOD becomes 0:
  stage-0 cannot set SMOD back to 1 until reset.
```

A conservative write implementation is sufficient:

```cpp
void mc68hc11d0_device::hprio_w(uint8_t data)
{
    if (!(m_hprio & 0x40))
        return; // special privilege already lost

    const uint8_t old = m_hprio;

    // Only bits needed by BT-G0 are modeled here.
    // Permit MDA changes while special.
    m_hprio = (m_hprio & ~0x20) | (data & 0x20);

    // RBOOT and SMOD may transition 1->0.
    if (!(data & 0x80)) m_hprio &= ~0x80;
    if (!(data & 0x40)) m_hprio &= ~0x40;

    logerror("AS2K_BOOT HPRIO %02X -> %02X\n", old, m_hprio);
}
```

This should be clearly marked diagnostic/incomplete rather than silently
claiming complete HPRIO emulation.

## 5. Bootstrap image injection

Preferred implementation: AS2K-driver diagnostic helper, not a fake SCI
peripheral.

Add a small ROM region or file-backed region:

```cpp
ROM_REGION(0x60, "stage0", ROMREGION_ERASEFF)
ROM_LOAD("as2k_stage0.bin", 0x0000, 0x001b, ...)
```

On diagnostic bootstrap reset:

1. copy exactly 27 bytes from `stage0` into CPU program space
   `$0040-$005A`;
2. leave `$005B-$009F` untouched/zero as appropriate;
3. set D0 diagnostic bootstrap state;
4. set PC=$0040;
5. run normally.

A core helper is cleaner than manipulating private CPU state from the driver:

```cpp
void mc68hc11d0_device::diag_bootstrap_load(
    const uint8_t *data, size_t size, uint16_t address = 0x0040);
```

It should:

- reject sizes >192 bytes;
- copy through the CPU program space so normal internal-RAM mapping is used;
- set PC only after copy completes;
- initialize `m_hprio=0xC0`;
- ensure CONFIG.NOCOP is set;
- keep I/X masked.

This models the *completed bootstrap transfer*, not its serial timing.

## 6. External-bus guard

Current AS2K memory mapping is visible even before MDA is set.

For a stronger diagnostic proof, guard AS2K external accesses while
`HPRIO.MDA=0`.

Minimum useful rule in diagnostic mode:

```
before MDA=1:
  stage-0 fetches from internal RAM allowed;
  external $2000/$4000-$7FFF accesses are rejected/logged;
  any PC in stock Z is a failure.

after MDA=1:
  normal AS2K memory handlers become active.
```

This can be implemented in the AS2K driver's lower-half handlers by querying
`m_maincpu->hprio()`.

It is not necessary to reproduce every special-mode bus timing detail.

## 7. Zero-Z execution assertion

While diagnostic takeover mode is armed, reuse the existing instruction
callback and add:

```cpp
if (pc >= 0x8000)
    fatalerror("AS2K_BOOT forbidden Z fetch at %04X", pc);
```

Also log the first instruction fetch in the DictROM window:

```cpp
if (!m_takeover_seen && pc >= 0x4000 && pc <= 0x7fff)
{
    m_takeover_seen = true;
    logerror("AS2K_BOOT FIRST_DICT_FETCH pc=%04X bank=%u\n",
             pc, selected_dict_bank);
}
```

Expected first external PC:

```
$4000
```

## 8. BT8 signature observation

The existing BT8 landing writes:

```
$00A0-$00A3 = "BT8!"
```

Diagnostic completion condition:

```
PC=$4010
RAM[$00A0..$00A3] == "BT8!"
zero Z fetches
```

A Lua or driver-side regression can stop after N visits to $4010 and report
PASS.

## 9. Test DictROM ROM set

Use a dedicated diagnostic clone instead of replacing the normal AS2K
DictROM definition.

Suggested machine name:

```
asma2kbt
```

It can reuse the same AS2K machine configuration and main Z BIOS but replace
only the `spellcheck` region with:

```
as2k_bt8_dictrom.bin
size   0x20000
CRC32  c5bd89df
SHA1   6e84689da9e0705920bca24137c88a387dedfac1
```

The presence of stock Z in the machine remains useful as a negative control:
the instruction callback must prove it was never executed.

Do not commit private stock Z bytes.

## 10. Suggested diagnostic ROM definition

Conceptually:

```cpp
ROM_START(as2kbt)
    ROM_REGION(0x10000, "maincpu", 0)
    // same private/known AS2000 BIOS requirement as asma2k

    ROM_REGION(0x20000, "spellcheck", 0)
    ROM_LOAD("as2k_bt8_dictrom.bin", 0x00000, 0x20000,
             CRC(c5bd89df)
             SHA1(6e84689da9e0705920bca24137c88a387dedfac1))

    ROM_REGION(0x60, "stage0", ROMREGION_ERASEFF)
    ROM_LOAD("as2k_stage0.bin", 0x0000, 0x001b,
             CRC(...) SHA1(...))
ROM_END
```

The stage-0 raw-image SHA-1 is already frozen by BT7:

```
ab76eafa386311b2ab70ea644345fb15767e908f
```

## 11. Why this is preferable to full SCI emulation

The real hardware proof still exercises:

- MODA/MODB;
- RESET;
- serial boot ROM;
- echo;
- timing;
- the physical external bus.

MAME is being used for a different purpose:

- execute the exact stage-0 machine code;
- model HPRIO transitions;
- verify lower-half banking;
- execute the replacement DictROM;
- catch accidental stock-Z execution.

Implementing SCI bootstrap in MAME would duplicate BT9 rather than improve
pre-BT9 confidence.

## 12. Later DebugTool use

After the BT8 diagnostic machine works, the same `asma2kbt` mode can load a
future no-Z DebugTool DictROM image instead of the 18-byte landing ROM.

Nothing about the bootstrap injector needs to change:

```
same stage0.bin
same HPRIO model
same $4000 entry
different 128 KiB test DictROM
```

This makes the diagnostic MAME target directly reusable for DebugTool
integration.

## 13. Implementation order

Bounded order if/when code is applied:

1. D0 HPRIO state + read/write;
2. diagnostic stage-0 RAM injection + PC=$0040;
3. AS2K zero-Z instruction guard;
4. first-DictROM-fetch logging;
5. `asma2kbt` test ROM definition;
6. BT8 signature regression;
7. only then replace the BT8 landing image with a no-Z DebugTool test image.

No SCI emulation is required for this diagnostic target.
