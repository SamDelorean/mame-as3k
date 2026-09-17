# AS2K Emulator Automation Task

Status: AUTHORIZED

## Immediate objective
Resume productive AS2000 emulator development from the current repository state. The first objective is to make normal automated keyboard interaction and diagnostics sufficiently reliable to exercise the firmware paths currently needed for regression work, especially Send and Print, without debugger-only shortcuts.

## Required startup audit
Before editing:
- read all control/handoff/findings documents named in `EMULATOR_AUTOMATION_CONTROL.md`;
- inspect `git status --short`, recent commits and the relevant source/tests;
- determine what was already fixed or proven in prior runs;
- do not repeat a completed experiment unless it is a regression check or required to validate a repair.

## Current evidence to preserve
The emulator baseline already has useful AS2000 execution, LCD/editor behavior, upward cursor scrolling/recall work, diagnostic instrumentation, and accumulated findings. Recent Gate 1A work exposed a Lua/input-resolution problem rather than proving a firmware regression. The next emulator work must use measured behavior to separate input-harness problems from machine-model problems.

## Productive sequence
1. Audit current input-field discovery/normalization and reproduce the latest failure if still present.
2. Repair the harness if the failure is demonstrably in the harness.
3. Prove that Send, Print and ordinary typing can be selected/injected through the intended MAME input path.
4. Run focused traces/probes that make the resulting firmware path observable.
5. If wired Send reaches an unimplemented/incorrect HC11 SCI behavior, treat SCI as the next emulator fidelity task and implement only the minimum measured behavior needed, with regression coverage.
6. Continue through the priority list in `EMULATOR_AUTOMATION_CONTROL.md` as each preceding blocker is resolved.

## Completion rule for each task
A task is complete only when the change is compiled if necessary, the relevant automated test passes or produces a deterministic classified result, `git diff --check` is clean, no unexplained regression remains, and the result is recorded factually.

Do not wait for Gate 1A to close before improving the emulator. Do not use emulator progress as authorization to modify the Gate 1A firmware project.
