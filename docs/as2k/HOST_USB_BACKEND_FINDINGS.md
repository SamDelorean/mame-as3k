# AS2000 host transport / USB backend findings

This document mirrors firmware-side findings that affect future MAME coverage for the AlphaSmart 2000 host-keyboard interface.

## Scope

The active firmware project is evaluating a later USB-enabled V3.14.x derivative. This does **not** change IR Gate 1A and does not require an immediate MAME device implementation. The purpose here is to preserve emulator-relevant behavior and test boundaries.

## Protected behavior

- Send keycode: `0x47`.
- Matrix location: `COL.7`, bit `0x10`.
- PC/two-wire Send/document routine: `0x8606`.
- Macintosh/ADB Send trigger: `0x888A`.
- IrDA backend: `0x9716 -> 0xD2DC` (removed by the IR-free firmware gate, not by this USB study).

Normal attached-keyboard output and Send must remain observable independently of the physical transport implementation.

## Stock PC transport

The PC/two-wire physical service is not represented by `0xAA26` alone.

Important entries include:

- `0xAA26` — shared transmit wrapper/service;
- `0xAA52` — physical two-wire serializer;
- `0xAAF0` — receive/handshake bit helper;
- `0x8522` — receive-byte/frame engine;
- `0x85A1` — attachment/timing probe;
- `0xAAEC-0xAAEF` — shared delay helper used outside the PC backend.

A hypothetical USB shim at `0xAA26` alone only makes `0xAA27-0xAA4E` unreachable. Direct callers keep `0xAA52` and other physical helpers live.

## Stock Macintosh / ADB transport

The timed state-machine envelope is approximately `0x8B30-0x8F04`.

The helper `0x8CB8-0x8CC3` has retained external callers and must not be treated as ADB-only without relocation.

A replacement entry at `0x8B33`, while preserving `0x8CB8`, makes 966 reachable executable bytes inside the old ADB engine unreachable. The bytes `0x8E94-0x8E95` are excluded because stock aligned reachability does not classify them as instructions.

## Candidate firmware reclaim result

Firmware-side static analysis currently reports:

- conservative PC physical backend candidate: 381 executable bytes;
- conservative ADB candidate: 966 executable bytes;
- combined: 1,347 executable bytes;
- maximum currently identified executable ceiling after relocating compatibility entries/helpers: 1,367 bytes.

These are **not** published free-ROM ranges. Runtime replacement proof remains required.

## MAME model needed for a future USB gate

The simplest emulator-side model should expose an abstract host-output sink above USB electrical details.

Suggested first boundary:

```text
firmware Host_Send / AA26-compatible shim
        -> SCI/bridge model
        -> captured key events
```

The emulator test should compare the resulting key-event stream against the stock transport semantics rather than requiring a full USB stack inside the AS2000 driver.

The future bridge model should be able to record:

- translated keycode / press-release sequence;
- ordering and pacing from Send;
- local keyboard events while attached;
- optional inbound bytes only when a bidirectional profile is implemented.

## Resource constraints that MAME should preserve

- PD1/TxD is the preferred future SCI transmit link.
- Stock reachable firmware has no SCI-register users at `0x2B-0x2F`.
- PD0 remains used by the printer backend, so the first USB profile should not assume SCI RxD is available.
- PA0 remains shared with printer/timer behavior.
- PA2 is input-only and may later serve as bridge status, but not as a RAM bank output.
- `0xAAEC-0xAAEF` remains shared with printer timing until deliberately relocated.

## Regression rule

A future MAME USB-bridge experiment must not alter the current IR Gate 1A expectations. IR routing, host-output modernization and printer behavior remain separate gates so regressions can be localized.
