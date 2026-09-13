# Current Codex task — fifth-line scrolling and recall

Date: 2026-09-13. Branch: as2k-mame0289-dev. Attempt 1.
Read AGENTS.md and mandatory private AS2K_KNOWLEDGE.md first.
Fourth-row entry/recall accepted in LOCAL 3oLxC3fU.
Preserve established findings; do not rediscover closed behavior.

## One narrow task

Observe and gate the first vertical scroll when entering a fifth explicit line.
This is an unresolved coverage gap, not a demonstrated emulator defect.
Use fresh private NVRAM, ordinary asma2k input and established idle gating.
Enter ab, Return, cd, Return, ef, Return, gh, Return, ij.
Exclude automatic wrapping, sixth-line entry and unrelated peripherals.

## Pass criteria and LOCAL validation

- Preserve the accepted four-row checkpoints through gh.
- Capture ordered exact all-four-row raw DDRAM after the fourth Return and ij.
  Proposed viewport: cd<B5> / ef<B5> / gh<B5> / blank, then
  cd<B5> / ef<B5> / gh<B5> / ij, each padded to 40 cells.
  These are hypotheses, not established firmware or physical hardware facts.
- Observe F2/F1 and separate-process saved-NVRAM recall including another F2/F1.
  Determine the firmware viewport restoration policy from retained evidence;
  do not assume recall preserves the viewport or change expectations silently.
- Require keyboard transitions, ordered LCD writes, both displays enabled,
  exact four-row content and completion. Substrings alone are insufficient.
- Check whether display-shift commands affect viewport interpretation; the
  shared decoder does not model shifts. Distinguish raw DDRAM from visible
  screen evidence, using existing pixel observations if necessary. Preserve
  the shared decoder and workflow/trace format unless a separate task warrants
  changing them. On mismatch retain traces and distinguish observation, input
  and firmware behavior before attributing an emulator bug.
- Add ROM-free negative fixtures for stale/unscrolled rows, missing/wrong ij,
  row/controller placement, stale recall and missing/unordered evidence.
- Extend bounded external LOCAL validator while retaining fourth-row entry,
  boundary join/resplit, three-line entry/recall, vertical/horizontal traversal,
  newline/join, interior editing, all-eight-file isolation/restart, production
  kKzZ=+, Send, diagnostic keyboard/LCD, six-character pixels, input fixtures,
  HC11 harness, both focused builds, -validate and both BIOS audits.
  Stop at first unexplained regression. Do not modify CPU/video cores.
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
