#!/usr/bin/env python3
# license:BSD-3-Clause
"""Decode AS2000 diagnostic LCD traces emitted by as2k-diagnostic.yml.

Consumes AS2KTRACE LCD lines and reconstructs KS0066 4-bit command/data
traffic plus a text-only 40x4 view of the two 2x40 controllers.  Printable
ASCII can be validated without the KS0066F05 bitmap CGROM.
"""
from __future__ import annotations

import argparse
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

LCD_RE = re.compile(
    r"AS2KTRACE LCD pc=([0-9A-Fa-f]+) e=(\d+) rs=(\d+) rw=(\d+) "
    r"nibble=([0-9A-Fa-f]) matrixh=([0-9A-Fa-f]{2}) pd=([0-9A-Fa-f]{2})"
)


@dataclass
class BusEvent:
    seq: int
    pc: int
    e: int
    rs: int
    rw: int
    nibble: int


@dataclass
class DecodedByte:
    seq: int
    pc: int
    controller: int
    rs: int
    rw: int
    value: int
    phase: str


@dataclass
class Controller:
    ident: int
    four_bit: bool = False
    last_rs: int | None = None
    last_rw: int | None = None
    first_nibble: bool = False
    high: int = 0
    active: str = "ddram"
    ac: int = 0
    increment: bool = True
    display_on: bool = False
    ddram: list[int] = field(default_factory=lambda: [0x20] * 0x80)
    cgram: list[int] = field(default_factory=lambda: [0x00] * 0x40)

    def feed(self, event: BusEvent) -> list[DecodedByte]:
        """Feed one E-rising-edge event and return completed bus bytes."""
        out: list[DecodedByte] = []

        if not self.four_bit:
            value = event.nibble << 4
            out.append(DecodedByte(event.seq, event.pc, self.ident, event.rs, event.rw, value, "8bit"))
            if event.rw == 0 and event.rs == 0 and (value & 0xF0) == 0x20:
                self.four_bit = True
                self.last_rs = event.rs
                self.last_rw = event.rw
                self.first_nibble = False
            return out

        # Mirror hd44780_base_device::update_nibble(): an RS/RW transition
        # resets nibble phase before the edge toggles it.
        if self.last_rs != event.rs or self.last_rw != event.rw:
            self.first_nibble = False
            self.last_rs = event.rs
            self.last_rw = event.rw

        self.first_nibble = not self.first_nibble
        if self.first_nibble:
            self.high = event.nibble
        else:
            value = (self.high << 4) | event.nibble
            out.append(DecodedByte(event.seq, event.pc, self.ident, event.rs, event.rw, value, "4bit"))
        return out

    def apply(self, byte: DecodedByte) -> bool:
        """Apply a completed write. Return True when visible DDRAM may change."""
        if byte.rw:
            return False
        value = byte.value
        if byte.rs:
            if self.active == "cgram":
                self.cgram[self.ac & 0x3F] = value
                self.ac = (self.ac + (1 if self.increment else -1)) & 0x3F
                return False
            self.ddram[self.ac & 0x7F] = value
            self.ac = (self.ac + (1 if self.increment else -1)) & 0x7F
            return True

        if value & 0x80:
            self.active = "ddram"
            self.ac = value & 0x7F
        elif value & 0x40:
            self.active = "cgram"
            self.ac = value & 0x3F
        elif value & 0x20:
            pass  # Function Set
        elif value & 0x10:
            pass  # Cursor/display shift does not alter DDRAM bytes.
        elif value & 0x08:
            self.display_on = bool(value & 0x04)
        elif value & 0x04:
            self.increment = bool(value & 0x02)
        elif value & 0x02:
            self.active = "ddram"
            self.ac = 0
        elif value & 0x01:
            self.ddram[:] = [0x20] * 0x80
            self.active = "ddram"
            self.ac = 0
            return True
        return False

    def lines(self) -> tuple[str, str]:
        def row(start: int) -> str:
            data = self.ddram[start:start + 40]
            return ''.join(chr(b) if 0x20 <= b <= 0x7E else (f"<{b:02X}>" if b else " ") for b in data)
        return row(0x00), row(0x40)


def parse_events(lines: Iterable[str]) -> list[BusEvent]:
    events: list[BusEvent] = []
    for line in lines:
        match = LCD_RE.search(line)
        if not match:
            continue
        pc, e, rs, rw, nibble, _matrixh, _pd = match.groups()
        events.append(BusEvent(len(events), int(pc, 16), int(e), int(rs), int(rw), int(nibble, 16)))
    return events


def command_name(value: int) -> str:
    if value & 0x80:
        return f"SET_DDRAM ${value & 0x7f:02X}"
    if value & 0x40:
        return f"SET_CGRAM ${value & 0x3f:02X}"
    if value & 0x20:
        return f"FUNCTION_SET ${value:02X}"
    if value & 0x10:
        return f"SHIFT ${value:02X}"
    if value & 0x08:
        return f"DISPLAY_CTRL ${value:02X}"
    if value & 0x04:
        return f"ENTRY_MODE ${value:02X}"
    if value & 0x02:
        return "RETURN_HOME"
    if value & 0x01:
        return "CLEAR"
    return f"CMD ${value:02X}"


def screen_tuple(ctrls: dict[int, Controller]) -> tuple[str, str, str, str]:
    top = ctrls[1].lines()
    bottom = ctrls[2].lines()
    return top[0], top[1], bottom[0], bottom[1]


def printable_screen(screen: tuple[str, str, str, str]) -> str:
    return "\n".join(f"{i + 1}: {line.rstrip()}" for i, line in enumerate(screen))


def decode(events: list[BusEvent], show_bus: bool = False, show_screens: bool = True) -> None:
    ctrls = {1: Controller(1), 2: Controller(2)}
    last_screen = screen_tuple(ctrls)
    printable_runs: list[tuple[int, int, int, str]] = []
    run_ctrl: int | None = None
    run_pc: int | None = None
    run_seq = 0
    run_chars: list[str] = []

    def flush_run() -> None:
        nonlocal run_ctrl, run_pc, run_seq, run_chars
        if run_chars:
            printable_runs.append((run_ctrl or 0, run_pc or 0, run_seq, ''.join(run_chars)))
        run_ctrl = run_pc = None
        run_chars = []

    for event in events:
        if event.e not in ctrls:
            continue
        ctrl = ctrls[event.e]
        for byte in ctrl.feed(event):
            if show_bus:
                if byte.rw:
                    kind = "READ"
                elif byte.rs:
                    kind = f"DATA {chr(byte.value)!r}" if 0x20 <= byte.value <= 0x7E else f"DATA ${byte.value:02X}"
                else:
                    kind = command_name(byte.value)
                print(f"#{byte.seq:04d} C{byte.controller} PC={byte.pc:04X} {byte.phase:4s} {kind}")

            if byte.rw:
                flush_run()
                continue

            if byte.rs and ctrl.active == "ddram" and 0x20 <= byte.value <= 0x7E:
                if run_ctrl == byte.controller and run_pc == byte.pc:
                    run_chars.append(chr(byte.value))
                else:
                    flush_run()
                    run_ctrl, run_pc, run_seq = byte.controller, byte.pc, byte.seq
                    run_chars = [chr(byte.value)]
            elif not byte.rs:
                flush_run()

            visible = ctrl.apply(byte)
            if show_screens and visible:
                now = screen_tuple(ctrls)
                should_show = not byte.rs and byte.value == 0x01
                if byte.rs and byte.rw == 0 and (
                    byte.value in (ord('.'), ord('!'), ord('?')) or ctrl.ac in (0x00, 0x28, 0x40, 0x68)
                ):
                    should_show = True
                if should_show and now != last_screen:
                    print(f"\n[SCREEN @ event #{byte.seq} PC={byte.pc:04X} C{byte.controller}]\n{printable_screen(now)}")
                    last_screen = now

    flush_run()

    print("\nPrintable DDRAM write runs:")
    for controller, pc, seq, text in printable_runs:
        if text.strip():
            print(f"  event #{seq:04d} C{controller} PC={pc:04X}: {text}")

    print("\nFinal reconstructed 40x4 text view:")
    print(printable_screen(screen_tuple(ctrls)))
    print(f"\nDecoded {len(events)} raw LCD E-edge events.")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path, help="MAME error.log or other AS2KTRACE text file")
    parser.add_argument("--bus", action="store_true", help="print every completed LCD command/data byte")
    parser.add_argument("--no-screens", action="store_true", help="suppress intermediate reconstructed screens")
    args = parser.parse_args()
    text = args.trace.read_text(encoding="utf-8-sig", errors="replace").splitlines()
    events = parse_events(text)
    if not events:
        raise SystemExit("No AS2KTRACE LCD events found")
    decode(events, show_bus=args.bus, show_screens=not args.no_screens)


if __name__ == "__main__":
    main()
