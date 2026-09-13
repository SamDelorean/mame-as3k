# Current Codex task — upward scrolling to the hidden first line

Date: 2026-09-13. Branch: as2k-mame0289-dev. Attempt 1.
Read AGENTS.md and mandatory private AS2K_KNOWLEDGE.md first.
Fifth-line scrolling/recall accepted in LOCAL mJusNQ7G; see CODEX_RESULT.md.
Preserve established findings; do not rediscover closed behavior.

## One narrow task

Observe cursor-driven upward scrolling after five explicit lines, recovering
the first line hidden by the accepted downward scroll. This is an unresolved
coverage gap, not a demonstrated emulator defect. Use fresh private NVRAM,
ordinary asma2k input and established idle gating. Enter ab, Return, cd,
Return, ef, Return, gh, Return, ij, then press Up four times with observations
between presses. Insert x at the resulting cursor to make its position
observable. Exclude automatic wrapping, sixth-line entry and other peripherals.

## Pass criteria and LOCAL validation

- Preserve the accepted fifth-line entry checkpoint:
  cd<B5> / ef<B5> / gh<B5> / ij, padded to 40 cells per row.
- Capture ordered exact all-four-row raw DDRAM after each Up and insertion.
  Proposed fourth-Up viewport: ab<B5> / cd<B5> / ef<B5> / gh<B5>;
  proposed insertion: abx<B5> / cd<B5> / ef<B5> / gh<B5>.
  These cursor-column and viewport expectations are hypotheses. Determine
  intermediate viewport behavior from retained evidence, without silently
  learning gate expectations from a passing run. No change to driver on a
  mismatch until observation, input and firmware behavior are distinguished.
- Observe F2/F1 and separate-process saved-NVRAM recall including another
  F2/F1; determine viewport restoration policy for this cursor position.
- Require keyboard transitions, ordered LCD writes, both displays enabled,
  exact four-row content and completion. Substrings alone are insufficient.
- Retain shift-command detection. Shared decoder does not model shifts;
  any shift requires separate visible-screen analysis before accepting raw
  DDRAM as viewport evidence. Preserve decoder and workflow/trace format.
- Add ROM-free negative fixtures for a stale bottom viewport, lost first line,
  wrong insertion column, controller/row placement, stale recall, missing or
  unordered evidence. Use bounded external LOCAL validation, stopping at the
  first unexplained regression. No CPU/video core changes.
- Retain fifth/fourth-row entry/recall, boundary join/resplit, three-line
  entry/recall, vertical/horizontal traversal, newline/join, interior editing,
  all-eight-file isolation/restart, production kKzZ=+, Send, diagnostic
  keyboard/LCD, six-character pixels, input fixtures, HC11 harness, both
  focused builds, -validate and both BIOS audits.
- REVIEW runs no builds or long runtime. Record commands, observations,
  limits, changed files and final status in CODEX_RESULT.md; git diff --check.
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
