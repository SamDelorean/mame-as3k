# Current Codex task — newline insertion and Backspace line join

Date: 2026-09-12. Branch: as2k-mame0289-dev. Attempt 1.
Read AGENTS.md and mandatory private AS2K_KNOWLEDGE.md first.
Interior cursor insertion/backspace and restart coverage is accepted.
Preserve established input, Send, XIRQ, bank and LCD_RENDERING.md limits,
including the earlier blank-LCD checkpoint. Do not rediscover closed findings.

## One narrow task

Add bounded diagnostic coverage for inserting Return within a short F1 token
and using Backspace at the start of the second line to join it again.
This is a coverage gap, not a demonstrated emulator defect. Keep content
within two visible rows, avoiding wrapping, scrolling and unrelated peripherals.
Use the asma2k input definitions and established idle gating.

## Pass criteria and LOCAL validation

- Fresh private NVRAM: type abcd; Left twice; Return. Assert exact visible
  rows ab and cd (space padded), distinguishing newline insertion from overwrite.
- At the start of the second line, Backspace must restore abcd on row 0
  and clear row 1. Require ordered LCD bus observations and key transitions.
- Insert Return again; verify the two-line content after F2/F1 and in a
  separate process using saved NVRAM. Establish/document firmware newline
  representation from these bounded observations if needed; do not guess
  decoder semantics. Completion or NVRAM substrings alone are insufficient.
- Retain accepted interior-edit, all-eight-file isolation/restart, production
  kKzZ=+, Send, diagnostic keyboard/LCD, six-character pixel, input fixture,
  HC11 harness, focused build, -validate and BIOS audit gates.
- Extend external bounded LOCAL validator. Keep proprietary/local artifacts
  outside Git. Preserve workflow/trace format and CPU/video cores.
- Stop at the first unexplained regression; distinguish input/observation
  limitations from emulator defects before proposing a minimal correction.
- Update CODEX_RESULT.md with commands, observations, limits, files and status;
  git diff --check must pass. REVIEW performs no build or long runtime.
  Commit/push validated redistributable changes only to origin/as2k-mame0289-dev.

Workspace note: preceding REVIEW found unrelated malformed local bytes in
src/devices/machine/gpl_renderer.h absent from git status --short, differing
from HEAD and LOCAL snapshot. Do not include it in this task's publication.

No exhaustive editing, RAM/decode or physical LCD claim. After at most three
attempts without materially new evidence/progress, mark OPEN/DEFERRED with
missing evidence and choose one different narrow task unless a demonstrated
blocking defect warrants continuation.
