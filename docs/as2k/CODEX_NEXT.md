# Current Codex task — join/resplit across the LCD controller boundary

Date: 2026-09-13. Branch: as2k-mame0289-dev. Attempt 1.
Read AGENTS.md and mandatory private AS2K_KNOWLEDGE.md first.
Three-line entry and recall are accepted in LOCAL s7ZH3Aqy. Preserve prior
newline/join, horizontal/vertical traversal, input, Send, XIRQ, banking and
LCD_RENDERING.md evidence and limits; do not rediscover closed findings.

## One narrow task

Cover Backspace joining the third line into the second, then Return restoring
that boundary. This is an unresolved coverage gap, not a demonstrated defect.
Use fresh private NVRAM, ordinary asma2k input and established idle gating.
Exclude wrapping, scrolling and unrelated peripherals.

## Pass criteria and LOCAL validation

- Enter ab, Return, cd, Return, ef. Require exact raw four-row DDRAM content
  ab<B5> / cd<B5> / ef / blank, padded to 40 cells.
- Left twice places the cursor before e; Delete (firmware Backspace) should
  join to ab<B5> / cdef / blank / blank. Require an ordered exact observation.
- Return at the join should restore ab<B5> / cd<B5> / ef / blank.
  These are proposed firmware expectations, not physical hardware facts.
- Require restored exact four-row content after F2/F1 and separate-process
  saved-NVRAM recall, including another F2/F1.
- Require keyboard transitions, ordered LCD writes, both displays enabled,
  all four rows (including cleared third row at join), and completion.
  Completion/NVRAM substrings alone are insufficient. Reuse raw B5 checks
  without changing the shared decoder or diagnostic trace/workflow format.
- Add ROM-free negative fixtures rejecting ignored join, wrong deletion,
  stale third-row text, wrong controller/row placement, stale recall and
  missing evidence. On mismatch retain traces and distinguish observation,
  input and firmware behavior from an emulator defect before changing code.
- Extend the external bounded LOCAL validator; retain three-line entry/recall,
  vertical/horizontal traversal, newline/join, interior-edit, all-eight-file
  isolation/restart, production kKzZ=+, Send, diagnostic keyboard/LCD,
  six-character pixels, input fixtures, HC11 harness, focused production and
  diagnostic builds, -validate and both BIOS audits. Stop on first regression.
- Do not modify CPU/video cores or add proprietary/local artifacts to Git.
  REVIEW runs no build or long runtime; record commands, observations, limits,
  changed files and status in CODEX_RESULT.md. Require git diff --check before
  publication only to origin/as2k-mame0289-dev.

Preserve the earlier blank-LCD checkpoint, unverified physical B5 glyph,
F05 NEEDS REDUMP, decoder-shift, timing and hardware limits. No exhaustive
editing, RAM/decode or physical display claim. Unrelated malformed local
gpl_renderer.h bytes remain excluded; LOCAL substitutes the committed header
in its disposable snapshot. Earlier compiler ICE cause remains unproven.
After at most three attempts without materially new evidence/progress, mark
OPEN/DEFERRED, record missing evidence and select one different narrow task
unless a demonstrated blocking defect warrants continuation.
