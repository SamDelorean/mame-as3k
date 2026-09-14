# Current Codex task — downward cursor scrolling to the hidden fifth line

Date: 2026-09-13. Branch: as2k-mame0289-dev. Attempt 1.
Read AGENTS.md and mandatory private AS2K_KNOWLEDGE.md first.
Upward recovery/insertion/top-viewport recall accepted in LOCAL 8z4GGYPr;
see CODEX_RESULT.md. Preserve closed findings and their evidence limits.

## One narrow task

Observe downward cursor scrolling after upward recovery. This is an unresolved
coverage gap, not a demonstrated emulator defect. With fresh private NVRAM,
ordinary asma2k input and established idle gating, enter ab, Return, cd,
Return, ef, Return, gh, Return, ij. Press Up four times, then Down four times
with separate observations. Insert x at the resulting cursor. No insertion
at the top in this sequence. Exclude automatic wrapping, sixth-line entry
and unrelated peripherals.

## Pass criteria and LOCAL validation

- Preserve exact accepted fifth-line and fourth-Up checkpoints. Capture ordered
  all-four-row raw DDRAM after each Down and insertion, 40 cells per row.
  Proposed Down 1-3 viewport: ab<B5> / cd<B5> / ef<B5> / gh<B5>;
  proposed Down 4: cd<B5> / ef<B5> / gh<B5> / ij;
  proposed insertion: cd<B5> / ef<B5> / gh<B5> / ijx.
  These are hypotheses, including cursor column. Preregister expectations;
  retain mismatches and distinguish observation/input/firmware behavior before
  any driver change. Never silently learn gate expectations from the same run.
- Observe F2/F1, separate-process saved-NVRAM recall and another F2/F1;
  determine viewport restoration policy for this cursor position.
- Require keyboard transitions, ordered LCD writes (commands can suffice for
  cursor-only steps), both displays enabled, exact four rows and completion.
- Retain shift detection from boot. Shared decoder does not model shifts;
  any shift requires separate visible-screen analysis before raw DDRAM can
  establish viewport behavior. Preserve workflow and trace format.
- ROM-free negative fixtures: stale top viewport, lost fifth line, wrong
  insertion column, wrong controller/row, stale recall, missing/unordered
  evidence and shift commands. Bounded external LOCAL validation stops at
  first unexplained regression. No CPU/video core changes.
- Retain upward-scroll/recall, fifth/fourth/three-line entry/recall, boundary
  join/resplit, vertical/horizontal traversal, newline/join, interior editing,
  all-eight-file isolation/restart, production kKzZ=+, Send, diagnostic
  keyboard/LCD, six-character pixels, input fixtures, HC11 harness, both
  focused builds, -validate and both BIOS audits.
- REVIEW runs no builds or long runtime. Record commands, values, limits,
  files and final status in CODEX_RESULT.md; git diff --check. Commit validated
  redistributable changes only; push origin/as2k-mame0289-dev.

Preserve blank-LCD provenance, physical B5/F05 NEEDS REDUMP, decoder-shift,
production recall, timing/XIRQ and hardware limits. No exhaustive editing,
RAM/decode or physical display claim. Exclude proprietary/local artifacts.
Unrelated malformed gpl_renderer.h bytes remain excluded; LOCAL substitutes
committed header only in disposable snapshot. Prior ICE cause is unproven.
Untracked EMULATION_FINDINGS.md is separate work; do not fold it into this task.
After at most three attempts without materially new evidence/progress, mark
OPEN/DEFERRED, record missing evidence and select one different narrow task,
unless a demonstrated blocking defect warrants continuation.
