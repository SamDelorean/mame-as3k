# AS2000 REVIEW result — fifth-line scrolling and recall PASS

2026-09-13; as2k-mame0289-dev; attempt 1.
Read AGENTS.md, CODEX_NEXT.md, mandatory private AS2K_KNOWLEDGE.md,
LOCAL summary, candidate diff/scripts, external validator and LCD_RENDERING.md.
No demonstrated emulator defect; no driver or core correction required.

LOCAL summary 2026-09-13T07:10:15-06:00 exited 0.
Private artifacts: external state/as2k-validation.mJusNQ7G.
Log: external logs/as2k-local-validation-20260913-061540.log.
Seven ordered entry checkpoints and two separate-process recall checkpoints pass.
The accepted four-row sequence through ab<B5> / cd<B5> / ef<B5> / gh
is preserved. Fourth Return yields cd<B5> / ef<B5> / gh<B5> / blank;
entering ij yields cd<B5> / ef<B5> / gh<B5> / ij.
F2/F1, saved-NVRAM restart with F1, and another F2/F1 restore that same
bottom viewport for this sequence. Every row is exactly 40 raw DDRAM cells,
space padded. Both displays enabled, keyboard transitions and LCD writes
between ordered checkpoints, and completion are required by the checker.
No explicit display-shift or automatic-entry-shift commands were observed
from boot through any of these checkpoints. Raw DDRAM observations do not
establish physical glyph equivalence. No expectations were changed after LOCAL;
script comments describing hypotheses predate their acceptance in this report.

LOCAL used ordinary asma2k input, fresh private NVRAM shared only with recall,
five-frame physical presses and 60 idle frames at PC 87D7. Those are test
parameters, not hardware timing. Each phase is bounded at 180 emulated / 300
wall seconds. No CPU/RAM/IRQ injection in these editing phases.

Retained LOCAL gates passed: fourth-row and three-line entry/recall,
boundary join/resplit, vertical/horizontal traversal, newline/join, interior
editing, F1-F8 isolation/restart with canonical bank/file bounds, production
kKzZ=+ persisted bytes and recall completion, Send COL.7/10 (10 -> 00 -> 10),
diagnostic az09=+ and recall, six F05 glyphs (240 pixels), input fixtures and
isolated HC11 harness including 148 synthetic STOP/wake cycles.
Validator exit 0 establishes both focused serial builds and -validate gates.
Both concise BIOS audit files report one ROM set OK / best available, with
F05 NEEDS REDUMP for both controllers. BIOS audits are not v308 editing tests.

No genuine contradiction with canonical derived evidence identified.
Preserve the earlier blank-LCD observation and later accepted runtime evidence
as distinct checkpoints. Preserve unverified physical B5 glyph, incomplete
charset/display coverage, shared decoder shift limits, production recall
without LCD bus evidence, firmware-backed unmasked-XIRQ limits, unmeasured
wake source/period and untested Send host transfer. Synthetic core cycles are
not new firmware auto-off evidence. No automatic wrap, sixth-line, exhaustive
editing, RAM/decode or physical display claim; no hardware assumption promoted.

Cheap REVIEW checks:
- Byte comparisons: three candidate scripts, shared decoder and diagnostic
  workflow match LOCAL snapshot exactly; no whole-workspace identity claim.
- PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_newline_edit.py:
  11 tests PASS (0.720s), including stale/unscrolled rows, missing/wrong ij,
  controller/row placement, stale recall, shift and missing/unordered evidence.
- bash -n external state/as2k-local-validator.sh: PASS.
- git diff --check: PASS.
No make, rebuild, MAME launch, long test, polling or full validation-log read.
LOCAL substituted the committed gpl_renderer.h only in its disposable snapshot;
unrelated malformed bytes excluded; prior compiler ICE cause remains unproven.

Publication files: three newline scripts, CODEX_RESULT.md and CODEX_NEXT.md.
Untracked EMULATION_FINDINGS.md is separate work, untouched and excluded.
External validator/state, ROMs, logs and generated artifacts remain outside Git.
Next narrow task: upward cursor scrolling to recover the hidden first line.

Final git status --short before publication:
```text
 M docs/as2k/CODEX_NEXT.md
 M docs/as2k/CODEX_RESULT.md
 M scripts/as2k_check_newline_edit.py
 M scripts/as2k_newline_edit.lua
 M scripts/as2k_test_newline_edit.py
?? docs/as2k/EMULATION_FINDINGS.md
```
Expected post-publication status: only the same untracked EMULATION_FINDINGS.md.

Publication integration: initial push rejected because origin added documentation
commit 12c90e4e715 (Gate 1A runtime readiness). Reviewed and rebased cleanly;
three validated scripts, decoder and workflow still match LOCAL byte-for-byte.
Post-rebase git diff --check passed. Final git status --short:
```text
?? docs/as2k/EMULATION_FINDINGS.md
```
