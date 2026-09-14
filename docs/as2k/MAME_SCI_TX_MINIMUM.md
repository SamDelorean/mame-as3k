# AS2000 HC11 SCI-TX minimum gate

## Context

The future AS2000 USB-host derivative intends to preserve the existing host-output ABI initially at `$AA26` and replace the legacy physical serializer with an HC11 SCI link to an external USB-capable companion.

The current AS2000 ROM does not use SCI, so this is not a blocker for stock emulation or Phase 1 IR removal. It is a prerequisite only for future USB-host firmware testing.

## Gate

`MAME_SCI_TX_MINIMUM`

Before any AS2000 USB firmware test is accepted, the generic MC68HC11D0 SCI transmitter must provide:

- stateful BAUD/SCCR1/SCCR2 handling sufficient for TX configuration;
- SCCR2 transmitter enable semantics;
- non-constant TDRE and TC behavior in SCSR;
- SCDR writes that enqueue a byte for real emulated transmission;
- scheduled byte completion with polling-visible timing;
- a completed-byte callback/sink from the HC11 core;
- normal PD1 GPIO behavior whenever SCI TX is disabled.

The gate does not require SCI receive, bit-accurate TxD, USB packets/electrical behavior, or any change to the current AS2000 IR-detach ROM patch.

## Required synthetic regression

Use a non-proprietary HC11 test program to configure SCI TX, send a deterministic byte sequence, poll TDRE/TC, and verify the machine-side sink receives the same bytes exactly once and in order. Observing only SCDR writes is not a pass; completion through the callback is required.

## AS2000 follow-on

After the generic gate passes, the AS2000 driver may attach an abstract companion to the SCI TX callback and test:

```text
AA26-compatible shim -> SCI TX / PD1 -> byte sink -> virtual USB/HID companion
```

The first profile remains output-only. PD0/RxD and PA0 stay reserved while the legacy Print path remains active. PA0/PA2 must not be used as RAM banking outputs.

## Do not disturb the current IR gate

The following remain protected and unrelated to this SCI work:

- Send keycode `$47` and COL.7/0x10 keyboard path;
- `$8606-$8709` cable Send;
- `$AA26-$AB09` PC/two-wire until a later USB-specific firmware gate;
- `$887E-$8F04` Macintosh/ADB;
- keyboard IRQ behavior and `$2000/$9000` MMIO;
- PA6 and PA4-PA5 banking;
- timer/capture-compare behavior retained by ADB/host paths.

Phase 1 Gate 1A still requires runtime evidence from `E0_IR_PURGE_DETACH_v0.1`. This document does not authorize DynFS or any USB firmware patch.

Status: `SPECIFIED_NOT_IMPLEMENTED`.
