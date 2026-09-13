# AS2000 Send key and IR-detach runtime contract

This note records firmware-derived input, transport, MMIO, timer, IRQ, and banking constraints needed by the AlphaSmart 2000 V3.14.x Phase 1A runtime gate. It is intentionally separate from the AS3000 driver work.

## Send mapping

AlphaSmart 2000 v3.1.4 firmware identifies **Send** as:

- matrix column: `COL.7`
- bit: `0x10`
- firmware keycode: `$47`

On the active AS2000 development branch `as2k-mame0289-dev`, `src/mame/skeleton/alphasma.cpp` already maps this position as a keyboard input using host `F12`, with `PORT_NAME("Send")` and the normal keyboard IRQ callback. `COL.7 / 0x40` remains Find. The default `master` branch still has `COL.7 / 0x10` unused, so Phase 1A runtime work must use the AS2000 development branch until this correction is integrated upstream.

The host F12 assignment is only an emulator convenience. The hardware/firmware invariant is `COL.7 / 0x10 -> keycode $47`.

## Firmware transport split

The high-level Send action is not an IR-only function.

Known v3.1.4 paths:

- main-loop PC/two-wire Send: `$80EE -> $80F2 -> $8606`
- Macintosh/ADB Send trigger while attached: `$888A`, feeding the retained state machine around `$8B33-$8F04`
- editor-side IrDA extension: `$9777 -> $9716 -> $D2DC`

The V3.14.x IR-free profile preserves the wired-host paths while removing only the IrDA backend. The Phase 1A patch redirects `$9716` to `$8606`.

## Keyboard IRQ and MMIO behavior in the driver

The current driver asserts `MC68HC11_IRQ_LINE` on every keyboard input transition through `kb_irq`. The IRQ is cleared when firmware writes the low keyboard matrix selector through `kb_matrixl_w`.

For `asma2k`, the externally mapped keyboard interface is:

- `$2000` in I/O view 0: keyboard read / high matrix-select write;
- `$9000`: low matrix-select write, which also clears the keyboard IRQ in the current model.

This behavior is part of the Phase 1A observable contract. A Send press should therefore produce the same matrix/IRQ sequence as other keys rather than a special synthetic firmware call.

## Banking/MMIO constraint

AS2000 `PORTA` writes select more than one function in the current model:

- PA6 selects the `$0000-$7fff` I/O view versus RAM view;
- PA4-PA5 select the RAM bank;
- PA4-PA5 also contribute to DictROM bank selection together with LCD-control bit 7.

The IR purge must not repurpose or globally suppress these bits. Any runtime trace around Send/Print should distinguish IrDA-specific accesses from normal bank/view switching.

## Timer and wired-host ownership

Redirecting the old IrDA interrupt vectors does **not** mean the HC11 timer block can be disabled.

The Phase 1A firmware gate redirects the IR-owned PAI and OC1-OC4 vectors and removes retained IrDA-origin accesses to `TMSK1/TMSK2` (`$22/$24`). However, the retained Macintosh/ADB transport still relies on HC11 capture/compare timing, with firmware accesses observed at `$000E`, `$0014`, `$001A`, `$0021`, and `$0023`.

Retained wired-host signal use also includes:

- PORTD bits 0-1: PC/two-wire drive; bit 1 also participates in Macintosh/ADB;
- PORTA bit 0: wired-host sense, including Macintosh/ADB;
- PORTA bit 2: PC/two-wire sense;
- PORTA bit 7: IrDA hardware-detect candidate only; do not treat it as free GPIO until PCB/net ownership is physically confirmed.

The firmware repository explicitly protects `$8606-$8709`, `$887E-$8F04`, and `$AA26-$AB09` during the IR purge.

## Phase 1A runtime checks

Using the AS2000 development branch and a locally generated patched ROM, tracing should confirm:

1. Send is recognized through `COL.7 / 0x10` and the normal keyboard IRQ path;
2. Send reaches `$8606` for the PC/two-wire path;
3. Send does not reach `$D2DC` or `$D099`;
4. Print reaches `$ABC9` and does not reach `$D437`;
5. no program counter enters `$D098-$D487`, `$D499-$D517`, or `$E103-$FFBF` during normal operation, Send, or Print, except the preserved `$D488-$D498` helper where applicable;
6. no unexpected IRQ/timer storm occurs;
7. normal PA6/PA4-PA5 view/bank behavior remains intact;
8. retained timer/MMIO behavior is not globally disabled merely because the IrDA vectors were detached.

If Macintosh/ADB attachment is modeled later, tracing must also preserve `$888A` and the `$8B33-$8F04` state machine as a distinct wired-host backend.

## Scope rule

Do not use these findings to alter the AS3000 keyboard/display work. They apply specifically to the `asma2k` runtime gate. DynFS behavior is out of scope until the IR purge is closed and validated.
