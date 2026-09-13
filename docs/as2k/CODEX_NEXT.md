# Current Codex task — explicit newline into the third LCD row

Date: 2026-09-13. Branch: as2k-mame0289-dev. Attempt 1.
Read AGENTS.md and mandatory private AS2K_KNOWLEDGE.md first.
Two-row newline/join, horizontal traversal, Up/Down insertion, F2/F1 and
separate-process recall are accepted. Preserve their evidence and all existing
input, Send, XIRQ, banking and LCD_RENDERING.md limits. Do not rediscover them.

## One narrow task

Add bounded diagnostic coverage for a short three-line F1 document crossing
from the first LCD controller into the second. This is an unresolved coverage
gap, not a demonstrated emulator defect. Use fresh private NVRAM, asma2k input
definitions and established idle gating. Exclude wrapping and scrolling.

## Pass criteria and LOCAL validation

- Enter ab, Return, cd, Return, ef with ordinary keyboard input.
- Require ordered exact raw DDRAM observations after each newline and final
  text. Proposed final rows: ab<B5> / cd<B5> / ef / blank, each padded to 40
  cells. Check both controllers, including the untouched fourth row.
  Proposed firmware expectations are not verified physical behavior.
- Verify the exact four-row final content after F2/F1 and in a separate
  process using saved NVRAM, including another F2/F1.
- Require keyboard transitions, ordered LCD writes, enabled displays and
  completion; completion or NVRAM substrings alone are insufficient.
- Add ROM-free negative fixtures rejecting missing/wrong controller-two
  content, stale recall, incorrect row placement and missing evidence.
- Reuse raw B5 comparisons without changing shared decoder semantics.
  On mismatch preserve traces and distinguish observation/input limitations
  and firmware behavior from an emulator defect before changing code.
- Retain vertical and horizontal traversal, newline/join, interior-edit,
  all-eight-file isolation/restart, production kKzZ=+, Send, diagnostic
  keyboard/LCD, six-character pixel, input fixture, HC11 harness, focused
  build, -validate and BIOS audit gates.
- Extend the external bounded LOCAL validator. Preserve workflow/trace format;
  do not modify CPU/video cores or add proprietary/local artifacts to Git.
- Stop at the first unexplained regression. REVIEW performs no build or long
  runtime. Record commands, observations, limits, changed files and status in
  CODEX_RESULT.md; require git diff --check before publishing validated changes
  only to origin/as2k-mame0289-dev.

Preserve the earlier blank-LCD checkpoint, unverified physical B5 glyph,
F05 NEEDS REDUMP, decoder-shift, timing and hardware limits. No exhaustive
editing, RAM/decode or physical display claim.
Workspace note: unrelated malformed local gpl_renderer.h bytes remain excluded;
LOCAL substitutes the committed version in its disposable snapshot. Serial
builds passed after an earlier compiler ICE; its cause remains unproven.
After at most three attempts without new evidence/progress, mark OPEN/DEFERRED,
record missing evidence and select one different task unless a demonstrated
blocking defect warrants continuation.
