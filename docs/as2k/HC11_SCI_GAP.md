# MC68HC11D0 SCI gap relevant to AS2000 USB-host experiments

## Finding

The current MC68HC11D0 core does not functionally emulate the SCI/UART.

In `src/devices/cpu/mc68hc11/mc68hc11.cpp`, the D0 internal-register map currently provides:

- `0x2B` BAUD: not mapped;
- `0x2C` SCCR1: read helper returns zero, write is `nopw`;
- `0x2D` SCCR2: read helper returns zero, write is `nopw`;
- `0x2E` SCSR: hard-coded read `0xC0`, write is `nopw`;
- `0x2F` SCDR: write is `nopw` and there is no transmit engine.

`mc68hc11.h` has GPIO, analog and SPI callbacks/state, but no SCI TX/RX callback, shifter/timer, or SCI interrupt state machine.

## Current AS2000 impact

None for the stock v3.1.4 ROM: firmware-side aligned reachability found zero accesses to `0x2B-0x2F`.

The stock PC and Macintosh host transports are bit-banged and are separately unmodeled by `alphasma.cpp`; `port_d_w()` currently only latches PORTD.

## Future USB-host impact

The planned AS2000 USB derivative intends to use the unused hardware SCI, initially over PD1/TxD to a small USB-capable companion.

The current core would create a false positive:

- firmware can poll SCSR and see `0xC0` (TDRE/TC apparently ready);
- firmware can write SCDR;
- the SCDR write is silently discarded;
- no byte reaches any device or callback.

Therefore executing past an SCDR write must **not** be treated as proof of a working SCI transport.

## Recommended first emulator implementation gate

Implement generic minimum SCI transmit behavior in the HC11 core before adding an AS2000 USB companion:

1. store BAUD, SCCR1 and SCCR2;
2. model transmitter enable;
3. make TDRE/TC state meaningful instead of permanently ready;
4. accept SCDR writes;
5. complete a transmitted byte after appropriate emulated delay;
6. expose a byte-level TX callback/sink;
7. preserve PORTD/PD1 GPIO behavior when the SCI transmitter is disabled.

Receive/RxD can be deferred because the first AS2000 USB profile is output-only and stock Print still uses PD0.

## Test strategy

Use a synthetic, redistributable HC11 program rather than the proprietary AS2000 ROM. The program should configure SCI, emit a deterministic byte sequence, poll TDRE/TC and prove the callback receives the same bytes.

Only after this core gate should the AS2000 driver connect a virtual USB bridge/HID-event sink to SCI TX.

This work is independent of, and must not block, the current IR Gate 1A runtime validation.

Status: `SCI_NOT_FUNCTIONALLY_EMULATED`.
