# Current Codex task — AS2000 $0001 register-read investigation

Date: 2026-09-12

## Established state

The production masked-XIRQ STOP fix passed local validation and REVIEW.
Preserve its evidence limits from CODEX_RESULT.md. Synthetic 1.25 s XIRQ timing
is not verified hardware behavior. Do not model the physical wake source.

## One narrow unresolved task

Resolve the evidence gap behind the existing alphasma.cpp TODO that AS2000
reads nonexistent internal register $0001, reportedly in both BIOSes.
Determine the firmware call context, current mapped read value and whether the
read causes an observable emulator defect. Do not assume the TODO is correct.

Read AGENTS.md, history, current source and local evidence first. Work only on
as2k-mame0289-dev; pull only with `git pull --ff-only origin as2k-mame0289-dev`.
Run status and diff checks before edits; preserve unexpected user changes.

## Authorized scope

Use source inspection and narrowly targeted diagnostic instrumentation/Lua to
observe $0001 reads and their surrounding firmware control flow, using locally
available BIOSes. Keep firmware and dumps private. Record factual findings in
`docs/as2k/REGISTER_0001.md` with reproducible commands, BIOS identifiers, PCs,
read values, mapping/configuration and observed downstream behavior. Do not
publish firmware bytes or disassembly excerpts. Separate observed behavior
from hardware assumptions and document unavailable BIOS coverage.

No CPU/video core edits, speculative register implementation, RAM-bank/PA3
changes, keyboard changes, physical wake timers or unrelated peripherals.
If evidence identifies a necessary production correction, describe the minimum
follow-up rather than implementing it in this diagnostic stage. Keep the current
workflow and trace format intact; new diagnostics must be opt-in.

## Validation and pass criteria

- Identify the relevant $0001 mapping and CPU configuration from current source.
- Capture at least one real firmware read with PC, returned value and execution
  context; establish whether it occurs in boot, idle or an explicit user action.
- Explain the observed consequence using evidence, or explicitly bound what the
  trace cannot establish. Do not call hardware behavior verified without a source.
- Check the second BIOS when locally available; report missing coverage honestly.
- Provide a reproducible bounded local validator for any added diagnostics.
- Preserve baseline boot and the established keyboard/LCD/file-recall smoke gate
  if executable diagnostic or production code changes; use the focused build and
  `./as2kdiag -validate` locally when needed, outside REVIEW.
- Run `git diff --check`; publish only redistributable diagnostics/documentation.

A documented benign firmware read is a valid outcome; no speculative fix is
required. Failure to observe a read is incomplete evidence, not proof of absence.
Replace CODEX_RESULT.md with commands, observations, limits, files and status.
After successful REVIEW commit/push only origin/as2k-mame0289-dev and select one
next narrow unresolved task, preserving the autonomous handoff protocol.
