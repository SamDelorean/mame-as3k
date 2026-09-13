# AS2000 Send key mapping finding

This note records firmware-derived input-map and hardware-ownership corrections needed by the AlphaSmart 2000 runtime work. It is intentionally separate from the AS3000 driver changes.

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

- main-loop PC/two-wire cable Send: `$80EE -> $80F2 -> $8606`
- Macintosh/ADB Send trigger while attached: `$888A`, feeding the retained state-machine region around `$8B33-$8F04`
- editor-side IrDA extension: `$9777 -> $9716 -> $D2DC`

Historical comparison shows the cable Send implementation existed before the later IrDA extension. The V3.14.x firmware work therefore preserves the wired-host paths while removing only the IrDA backend.

## IR-detach hardware ownership constraint

Redirecting the old IrDA interrupt vectors does **not** mean the HC11 timer block can be disabled in the AS2000 machine model.

The Phase 1A firmware gate redirects the IR-owned PAI and OC1-OC4 vector entries away from their old handlers, and retained IR-origin direct accesses to `TMSK1/TMSK2` (`$22/$24`) are expected to disappear. However, the retained Macintosh/ADB transport still relies on HC11 capture/compare timing. Firmware analysis observes accesses in that path to `$000E`, `$0014`, `$001A`, `$0021`, and `$0023`.

Retained wired-host signal use also includes:

- PORTD bits 0-1: PC/two-wire drive; bit 1 also participates in the Macintosh/ADB path;
- PORTA bit 0: wired-host sense, including Macintosh/ADB;
- PORTA bit 2: PC/two-wire sense;
- PORTA bit 7: IrDA hardware-detect candidate only; do not treat it as a free GPIO until PCB/net ownership is physically confirmed.

Therefore the IR-free firmware profile must keep the normal HC11 timer, PORTA, and PORTD behavior available in MAME. The firmware-side structured ownership contract is `recon/ir_resource_ownership_v1.json` in `SamDelorean/AS2K-V3.14.x`.

## Runtime use

This mapping is required for the V3.14.x Phase 1A IR-detach runtime gate.

After the key is mapped, tracing should confirm:

1. pressing Send is recognized by the matrix;
2. the IR-detached test ROM reaches `$8606` for the PC/two-wire path;
3. it does not reach `$D2DC`;
4. it does not enter `$E103-$FFBF`;
5. no unexpected timer/IRQ storm occurs;
6. retained timer/MMIO behavior is not globally suppressed merely because the IrDA vectors were detached.

If Macintosh/ADB attachment is modeled later, tracing should also preserve `$888A` and the `$8B33-$8F04` state machine as a distinct wired-host backend.

The firmware-side gate is documented in `SamDelorean/AS2K-V3.14.x` under:

- `docs/SEND.md`
- `docs/EMULATOR_HANDOFF.md`
- `recon/ir_resource_ownership_v1.json`
- `patches/ir_removal/E0_IR_PURGE_DETACH_v0.1.json`
- issue #1 in that repository.

## Scope rule

Do not use this finding to alter the AS3000 keyboard/display work. It applies specifically to the `asma2k` runtime work in `alphasma.cpp`.
