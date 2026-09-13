# Current Codex task — fourth-row entry and recall

Date: 2026-09-13. Branch: as2k-mame0289-dev. Attempt 1.
Read AGENTS.md and mandatory private AS2K_KNOWLEDGE.md first.
Third-line join/resplit and recall accepted in LOCAL KeEi5rma.
Preserve established findings; do not rediscover closed behavior.

## One narrow task

Cover explicit-newline entry into the fourth LCD row and persisted recall.
This is an unresolved coverage gap, not a demonstrated emulator defect.
Use fresh private NVRAM, ordinary asma2k input and established idle gating.
Exclude fifth-line entry, wrapping, scrolling and unrelated peripherals.

## Pass criteria and LOCAL validation

- Enter ab, Return, cd, Return, ef, Return, gh.
- Require ordered exact raw four-row DDRAM checkpoints after each Return:
  ab<B5> / blank / blank / blank;
  ab<B5> / cd<B5> / blank / blank;
  ab<B5> / cd<B5> / ef<B5> / blank.
- Require final ab<B5> / cd<B5> / ef<B5> / gh, each row padded to 40 cells.
  These are proposed firmware expectations, not physical hardware facts.
- Require the same final rows after F2/F1 and separate-process saved-NVRAM
  recall, including another F2/F1.
- Require keyboard transitions, ordered LCD writes, both displays enabled,
  exact all-four-row content and completion. Substrings alone are insufficient.
  Reuse raw B5 checks; preserve shared decoder and workflow/trace format.
- Add ROM-free negative fixtures rejecting missing/wrong fourth-row text,
  stale third-row marker, wrong controller/row placement, stale recall and
  missing/unordered evidence. On mismatch retain traces and distinguish
  observation, input and firmware behavior before attributing an emulator bug.
- Extend bounded external LOCAL validator, retaining boundary join/resplit,
  three-line entry/recall, vertical/horizontal traversal, newline/join,
  interior editing, all-eight-file isolation/restart, production kKzZ=+, Send,
  diagnostic keyboard/LCD, six-character pixels, input fixtures, HC11 harness,
  focused production/diagnostic builds, -validate and both BIOS audits.
  Stop on first unexplained regression. Do not modify CPU/video cores.
- REVIEW runs no builds or long runtime. Record commands, observations, limits,
  changed files and final status in CODEX_RESULT.md; require git diff --check.
  Commit only validated redistributable changes; push origin/as2k-mame0289-dev.

Preserve earlier blank-LCD provenance, unverified physical B5 glyph, F05 NEEDS
REDUMP, decoder-shift, timing/XIRQ and hardware limits. No exhaustive editing,
RAM/decode or physical display claim. Exclude proprietary/local artifacts.
Unrelated malformed gpl_renderer.h bytes remain excluded; LOCAL substitutes
committed header only in its disposable snapshot. Prior ICE cause is unproven.
Untracked EMULATION_FINDINGS.md is separate work; do not fold it into this task.
After at most three attempts without materially new evidence/progress, mark
OPEN/DEFERRED, record missing evidence and select one different narrow task,
unless a demonstrated blocking defect warrants continuation.
