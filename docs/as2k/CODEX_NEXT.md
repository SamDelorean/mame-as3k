# Current Codex task — stock PC attachment and text-sink prerequisite

Date: 2026-09-17. Branch: `as2k-mame0289-dev`.

Read `AGENTS.md`, `EMULATOR_AUTOMATION_CONTROL.md`, `EMULATOR_AUTOMATION_TASK.md`, `HOST_ATTACHMENT_TEXT_SINK.md`, `CODEX_RESULT.md`, and `EMULATION_FINDINGS.md` before editing.

## Closed evidence — do not repeat as discovery work

- normal typing/input automation is usable;
- Send is keycode `$47`, matrix `COL.7 / 0x10`, through normal `kb_irq`;
- stock v3.1.4 with no host reaches `$9716 -> $D2DC` on a real Send press;
- two passive PORTA runs observed 77 reads each with PA0/PA2 low;
- source inspection shows the current AS2000 driver has no external PC/ADB attachment source;
- the integrated IRLESS runner validates Send `$9716 -> $8606` and Print `$9804 -> $ABC9` without forbidden IrDA handlers.

These facts identify a missing host-interface model, not a Send-key or SCI defect.

## One narrow task

Advance the exact stock host-attachment model. Use the ROM-side finding that `$85A1-$85C5` is the leading attachment-probe candidate and PA0/PA2 are the candidate sense/handshake inputs.

Do not guess active levels. First determine whether the currently available published evidence is sufficient to specify the exact boolean/temporal condition for PC attachment. If it is not sufficient, add only the smallest passive MAME instrumentation/test support needed to observe the relevant state without changing machine behavior, and record exactly what ROM/hardware evidence is still missing.

If the exact attachment condition is already closed by newer evidence when this task runs, implement the smallest `PC connection: Disconnected / Connected` configuration input that presents only that proven external condition.

## Pass criteria

- `Disconnected` preserves stock no-host behavior and the existing Send route observation.
- `Connected`, when implementation is justified by proven polarity/timing, causes the stock ROM itself to enter `Attached to PC, emulating keyboard.`.
- No direct call to `$8606`, no forced CPU PC and no RAM poke of `$008A` or equivalent state.
- Preserve normal keyboard IRQ/MMIO, Print, PA6 banking, PA4-PA5 banking/DictROM behavior and shared timer/capture resources.
- No SCI, USB HID or operating-system keyboard work is required for this task.
- If blocked on unresolved attachment semantics, finish with a deterministic `EVIDENCE` blocker and the exact missing observation rather than forcing a plausible bit pattern.

## Following gate

After autonomous stock PC attachment works, the next task is the logical wired-output decoder and fixed `salida.txt` sink defined in `HOST_ATTACHMENT_TEXT_SINK.md`, with an end-to-end known-text test such as `ABC 123`.

Run focused regression tests, `git diff --check`, commit only validated redistributable changes and push `origin/as2k-mame0289-dev`. Do not publish ROM/NVRAM/private traces.
