# AS2000 host attachment and text-output emulation

Status: `HOST_ATTACHMENT_TEXT_SINK_PLAN_V1`

## Scope

This document defines the next MAME fidelity task for the AlphaSmart 2000. It is intentionally narrower than the separate physical USB-modernization work in `AS2K-V3.14.x`.

The emulator objective is:

`stock AS2000 ROM -> host attachment detection -> stock wired keyboard mode -> stock Send/key events -> plain-text sink -> salida.txt`

No operating-system keyboard driver, USB HID enumeration, `uinput`, `SendInput`, `CGEvent`, or host-window focus integration is required for this stage.

## Evidence already closed

The active branch already maps Send through the normal emulated keyboard path:

- firmware keycode `$47`;
- matrix `COL.7 / 0x10`;
- normal `kb_irq` behavior;
- no direct firmware-call shortcut.

Stock v3.1.4 runtime evidence with no emulated host attached shows:

- a real Send matrix event reaches `$9716 -> $D2DC`;
- it does not reach wired Send `$8606`;
- two passive PORTA-sense runs observed 77 reads each with PA0/PA2 low throughout.

The current driver explains that result: `alphasmart_state::port_a_r()` has no external PC/ADB attachment source. The absent connected-host behavior is therefore a hardware-model gap, not a Send-key, firmware-routing, or SCI defect.

The integrated IRLESS runner independently validates the derived-firmware routing contract:

- Send: `$9716 -> $8606`;
- Print: `$9804 -> $ABC9`;
- no forbidden `$D2DC/$D099/$D437` markers in the validated run.

That proves IRLESS routing only. It does not emulate stock cable attachment or external host transfer.

## Firmware attachment model under investigation

Current ROM reverse engineering identifies `$85A1-$85C5` as the leading attachment-probe candidate. The wired transport around `$AA26/$AA52` reads PORTA bits 0 and 2, and PA0/PA2 are the current candidate sense/handshake inputs.

PA0 is shared with Print and must not be treated as a host-only/free signal. PA2 is the leading candidate for a host-present/ready role, but its active polarity and exact temporal protocol are not yet proven.

The firmware later distinguishes PC/Mac state through RAM `$008A bit0`; `$A3AC` uses that state to select the stock messages at `$DDF2` (`Attached to PC, emulating keyboard.`) and `$DDCD` (`Attached to Mac, emulating keyboard.`).

The next implementation must not guess the PA0/PA2 truth table. First close the exact boolean and temporal condition from ROM/control-flow evidence.

## MAME configuration model

Add one user-facing configuration state after the attachment condition is proven:

`PC connection: Disconnected / Connected`

`Disconnected` must preserve the current stock no-host electrical state. A stock Send press must remain free to follow the firmware's ordinary disconnected behavior, including the observed `$9716 -> $D2DC` IrDA path.

`Connected` must inject only the proven external electrical condition that a real PC connection presents to the AS2000. The ROM must detect the condition and enter its connected keyboard-emulation mode by itself.

Forbidden shortcuts:

- no direct call to `$8606`;
- no forced CPU PC;
- no poke of RAM `$008A` or other classification state;
- no rewriting Send dispatch merely to obtain wired output.

The first positive attachment gate is the stock ROM autonomously displaying/entering `Attached to PC, emulating keyboard.` after the MAME connection state changes.

## Plain-text output sink

Once stock connected mode is working, capture the logical outgoing keyboard-event boundary rather than emulating a physical host keyboard device.

The preferred initial observation boundary is the logical service around `$AA26`, or an equivalent driver/device sink that still allows the original ROM to execute its normal translation and Send logic.

The sink must maintain enough state to decode:

- key make events;
- key release/break events;
- Shift and other modifiers used by the stock ROM;
- printable characters;
- useful plain-text controls such as Return/Enter and Tab where applicable.

Break-only events are not written as characters. Repeated printable make events must remain repeatable; the sink must not collapse identical adjacent characters.

The resulting character stream is written to a fixed file named `salida.txt`. The exact MAME-managed path may be chosen during implementation, preferably alongside the machine's persistent/NVRAM state if that integrates cleanly. No path-selection UI is required for the first implementation.

## Validation gates

1. Close the exact `$85A1-$85C5` attachment truth table/polarity/timing over PA0/PA2 from ROM evidence. Do not discover it by weakening runtime expectations or random bit forcing.
2. Add the manual `PC connection` state and prove that Disconnected retains stock no-host behavior.
3. Set Connected and prove that the stock ROM autonomously enters the PC-attached keyboard-emulation state.
4. With Connected active, exercise Send through the normal matrix/IRQ path and prove the wired path is selected without synthetic control-flow injection.
5. Add the logical event decoder and fixed `salida.txt` sink.
6. Type a known file such as `ABC 123`, press Send, and require `salida.txt` to contain exactly `ABC 123` with the expected line-ending semantics.
7. Retain existing editor/LCD/NVRAM/F1-F8/Send/Print regressions and the disconnected IrDA-route observation.

## Relationship to SCI/USB work

MC68HC11D0 SCI emulation is not a prerequisite for this MAME text-output objective. SCI remains relevant to the separate future physical USB bridge design and to any later emulator effort that deliberately models that modified firmware.

Do not implement SCI merely to produce `salida.txt`. The present task is to emulate stock host attachment faithfully enough that the stock ROM reaches its own wired keyboard path, then capture the resulting logical events.
