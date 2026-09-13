# Current Codex task — cursor traversal across an explicit newline

Date: 2026-09-12. Branch: as2k-mame0289-dev. Attempt 1.
Read AGENTS.md and mandatory private AS2K_KNOWLEDGE.md first.
Newline insertion, Backspace join, F2/F1 and separate-process recall are accepted.
Preserve existing input, Send, XIRQ, banking and LCD_RENDERING.md limits,
including the earlier blank-LCD checkpoint and unverified physical B5 glyph.

## One narrow task

Add bounded diagnostic coverage for Left/Right traversal across an explicit
newline in F1. This is an unresolved coverage gap, not a demonstrated defect.
Use fresh private NVRAM, asma2k input definitions and established idle gating.
Keep two short visible rows; exclude wrapping, scrolling and other peripherals.

## Pass criteria and LOCAL validation

- Establish ab<CR>cd using the accepted split sequence, with cursor before c.
  Require exact raw DDRAM rows ab<B5> and cd, space padded to 40 cells.
- Left once then insert x. Test the expected boundary behavior: abx<B5> / cd.
  This is a test expectation, not independently verified physical behavior.
- Right once then insert y. Test traversal over the newline: abx<B5> / ycd.
  Distinguish crossing the newline from motion within a row or overwriting text.
- Verify final exact two-row content after F2/F1 and in a separate process
  using saved NVRAM. Require ordered LCD writes and keyboard transitions;
  completion or NVRAM substrings alone are insufficient.
- Reuse raw B5 comparisons without changing shared decoder semantics.
  On unexpected behavior, preserve traces and distinguish input/observation
  limitations and firmware semantics from an emulator defect before changing code.
- Retain newline/join, interior-edit, all-eight-file isolation/restart,
  production kKzZ=+, Send, diagnostic keyboard/LCD, six-character pixel,
  input fixture, HC11 harness, focused build, -validate and BIOS audit gates.
- Extend the external bounded LOCAL validator. Preserve workflow/trace format;
  do not modify CPU/video cores or add proprietary/local artifacts to Git.
- Stop at the first unexplained regression. REVIEW performs no build or long
  runtime. Record commands, observations, limits, changed files and status in
  CODEX_RESULT.md; require git diff --check before publishing validated changes
  only to origin/as2k-mame0289-dev.

Workspace note: unrelated malformed local gpl_renderer.h bytes remain excluded;
the LOCAL validator substitutes its committed version in the disposable snapshot.
No exhaustive editing, RAM/decode, physical display or timing claim.
After at most three attempts without new evidence/progress, mark OPEN/DEFERRED,
record missing evidence and select one different task unless a demonstrated
blocking defect warrants continuation.
