# AS2000 Send key mapping finding

This note records a firmware-derived input-map correction needed by the AlphaSmart 2000 runtime work. It is intentionally separate from the AS3000 driver changes.

## Confirmed mapping

AlphaSmart 2000 v3.1.4 firmware reverse engineering identifies the **Send** key as:

- matrix column: `COL.7`
- bit: `0x10`
- firmware keycode: `$47`

The current `asma2k` input map in `src/mame/skeleton/alphasma.cpp` marks `COL.7 / 0x10` as `IPT_UNUSED`.

Current entry:

```cpp
PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
```

A suitable mapping is:

```cpp
PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F12) PORT_NAME("Send") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(alphasmart_state::kb_irq), 0)
```

`F12` is only a convenient host-key choice. The hardware/firmware fact to preserve is `COL.7 / 0x10`.

## Firmware evidence

The high-level Send action is not an IR-only function.

Known v3.1.4 paths:

- main-loop cable/keyboard Send: `$80EE -> $80F2 -> $8606`
- editor-side IrDA extension: `$9777 -> $9716 -> $D2DC`

Historical comparison shows the cable Send implementation existed before the later IrDA extension. The V3.14.x firmware work therefore preserves `$8606` while removing only the IrDA backend.

## Runtime use

This mapping is required for the V3.14.x Phase 1A IR-detach runtime gate.

After the key is mapped, tracing should confirm:

1. pressing Send is recognized by the matrix;
2. the IR-detached test ROM reaches `$8606`;
3. it does not reach `$D2DC`;
4. it does not enter `$E103-$FFBF`;
5. no unexpected timer/IRQ storm occurs.

The firmware-side gate is documented in `SamDelorean/AS2K-V3.14.x` under:

- `docs/SEND.md`
- `docs/EMULATOR_HANDOFF.md`
- `patches/ir_removal/E0_IR_PURGE_DETACH_v0.1.json`
- issue #1 in that repository.

## Scope rule

Do not use this finding to alter the AS3000 keyboard/display work. It applies specifically to the `asma2k` input map in `alphasma.cpp`.
