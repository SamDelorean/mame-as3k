# AS2K Emulator Automation Task

Status: AUTHORIZED

## Immediate objective

Resume productive AS2000 emulator development from the current repository state. The next objective is faithful stock host attachment followed by a platform-neutral plain-text wired-output sink. Read `HOST_ATTACHMENT_TEXT_SINK.md` as the current task contract.

Do not repeat the already-closed input/Send/Print routing work unless it is required as a regression check.

## Required startup audit

Before editing:
- read all control/handoff/findings documents named in `EMULATOR_AUTOMATION_CONTROL.md`, including `HOST_ATTACHMENT_TEXT_SINK.md`;
- inspect `git status --short`, recent commits and the relevant source/tests;
- preserve the already validated normal matrix/IRQ paths for typing, Send and Print;
- distinguish ROM reverse-engineering gaps from MAME hardware-model gaps before changing code.

## Current evidence to preserve

The emulator baseline already has useful AS2000 execution, LCD/editor/NVRAM behavior and reliable normal input. Send is available as keycode `$47` at `COL.7 / 0x10` through the normal `kb_irq` path. Stock v3.1.4 with no emulated host reaches `$9716 -> $D2DC` after a real Send press. A passive host-sense probe observed 77 PORTA reads in each of two stock runs with PA0/PA2 remaining low, and source inspection established that the current driver does not provide an external PC/ADB attachment source.

The integrated IRLESS runner has separately validated Send `$9716 -> $8606` and Print `$9804 -> $ABC9` without forbidden IrDA handlers. That closes the routing prerequisite; it does not prove stock host attachment or external wired transfer.

## Productive sequence

1. Consume the ROM-side attachment analysis around `$85A1-$85C5` and PA0/PA2. Do not guess active polarity or invent a handshake. If the exact condition is still unresolved, improve diagnostic/observation support without changing machine behavior and record the blocker precisely.
2. Once the condition is proven, implement the smallest MAME configuration input needed for `PC connection: Disconnected / Connected`. It must modify only the modeled external electrical state.
3. Prove that Disconnected preserves current stock no-host behavior and that Connected makes the stock ROM autonomously enter `Attached to PC, emulating keyboard.`. Do not force PC, poke firmware RAM, or call `$8606` directly.
4. With Connected active, exercise Send through the normal matrix/IRQ path and demonstrate that the stock wired path is selected.
5. Implement a logical outgoing-key decoder at/near the `$AA26` transport boundary and write the decoded stream to a fixed `salida.txt`. Maintain make/break and modifier state; do not collapse repeated identical characters.
6. Validate a known text payload such as `ABC 123` end-to-end into `salida.txt`, then rerun the retained editor/LCD/NVRAM/F1-F8/Send/Print regressions.
7. Only after the text-sink gate is closed should lower-level physical transport fidelity be reconsidered. MC68HC11D0 SCI is not a prerequisite for this emulator objective; it remains relevant to separate future modified-firmware/USB work.

## Completion rule for each task

A task is complete only when the change is compiled if necessary, the relevant automated test passes or produces a deterministic classified result, `git diff --check` is clean, no unexplained regression remains, and the result is recorded factually.

Do not use emulator progress as authorization to modify the separate `AS2K-V3.14.x` firmware patch. Keep proprietary ROM/NVRAM artifacts local and publish only redistributable code, tests and documentation.
