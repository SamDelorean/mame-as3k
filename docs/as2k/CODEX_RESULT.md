# AS2000 REVIEW result — third-line join/resplit PASS

2026-09-13; as2k-mame0289-dev; attempt 1.
Read AGENTS.md, CODEX_NEXT.md, canonical private AS2K_KNOWLEDGE.md,
LOCAL summary, candidate diff/scripts, external validator and LCD_RENDERING.md.
No demonstrated emulator defect; no driver or core correction needed.

LOCAL summary 2026-09-13T04:51:23-06:00 exited 0.
Private artifacts: external state/as2k-validation.KeEi5rma.
Log: external logs/as2k-local-validation-20260913-034930.log.
Summary establishes six ordered boundary checkpoints and two recall checkpoints:
- first Return: ab<B5> / blank / blank / blank;
- second Return: ab<B5> / cd<B5> / blank / blank;
- final entry: ab<B5> / cd<B5> / ef / blank;
- Left twice, Delete (firmware Backspace): ab<B5> / cdef / blank / blank;
- Return, F2/F1, separate-process restart and its F2/F1:
  ab<B5> / cd<B5> / ef / blank.
All rows are exactly 40 raw DDRAM cells, space padded. Checks require both
controllers enabled, ordered exact rows, keyboard transitions and LCD writes
between checkpoints, and completion. Fresh private NVRAM is shared only with
its separate recall process. Runtime bounds: 180 emulated / 300 wall seconds
per phase. Five-frame presses and 60 idle frames at PC 87D7 are test parameters,
not hardware timing. No CPU/RAM/IRQ injection. Candidate comments describing
proposed expectations predate LOCAL; this report records their acceptance.

Retained LOCAL gates passed: three-line entry/recall, vertical/horizontal
traversal/recall, newline/join/recall, interior editing/recall, F1-F8 isolation
and restart with canonical bank/file bounds, production kKzZ=+ persisted bytes
and recall completion, Send COL.7/10 released 10 -> pressed 00 -> released 10,
diagnostic az09=+ and recall, six F05 glyphs (240 pixels), input fixtures,
and isolated HC11 harness including 148 synthetic STOP/wake cycles.
Validator exit 0 establishes both focused serial builds and -validate gates.
Concise v314/v308 audit files each report one set OK / best available and
F05 NEEDS REDUMP for both controllers.

No contradiction with the canonical evidence identified for this task.
Preserve its earlier blank-LCD checkpoint separately from later accepted
runtime observations. No physical B5 glyph, complete charset/display,
wrapping/scrolling, exhaustive editing or RAM/decode claim. Preserve
LCD_RENDERING.md and decoder-shift limits, production recall without LCD bus
evidence, firmware-backed unmasked-XIRQ limits, unmeasured wake source/period,
and untested Send host transfer. Synthetic HC11 tests are not new firmware
auto-off evidence. No new hardware assumptions promoted.

Cheap REVIEW checks:
- Python byte comparisons: three candidate scripts, shared decoder and workflow
  match LOCAL snapshot exactly; no whole-workspace identity claim.
- PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_newline_edit.py:
  9 tests PASS, including ignored join, wrong deletion, stale/misplaced rows,
  failed resplit/recall, disabled/missing controller and missing/ordered evidence.
- bash -n external state/as2k-local-validator.sh: PASS.
- git diff --check: PASS.
No make, rebuild, MAME launch, long test, command polling or full validation-log
read. LOCAL substituted committed gpl_renderer.h only in its disposable
snapshot; unrelated malformed bytes excluded; prior compiler ICE cause unproven.

Publication files: three newline scripts, CODEX_RESULT.md and CODEX_NEXT.md.
Untracked EMULATION_FINDINGS.md is unrelated pre-existing work, excluded from
this review publication; its broader claims are not validated by these tests.
External validator/state, ROMs, logs and generated artifacts remain outside Git.
Next task: fourth-row explicit-newline entry and separate-process recall.
Push only origin/as2k-mame0289-dev, then set external decision to PREPARE.

Final git status --short before staging/publication:
```text
 M docs/as2k/CODEX_NEXT.md
 M docs/as2k/CODEX_RESULT.md
 M scripts/as2k_check_newline_edit.py
 M scripts/as2k_newline_edit.lua
 M scripts/as2k_test_newline_edit.py
?? docs/as2k/EMULATION_FINDINGS.md
```
Expected post-publication status: only the same untracked EMULATION_FINDINGS.md.

Publication integration: first push rejected because origin added documentation-only
4171c654c4f (GATE1A_RUNTIME_HARNESS.md). Reviewed and rebased cleanly;
validated scripts unchanged. Post-rebase git diff --check passed.
Final post-rebase git status --short: ?? docs/as2k/EMULATION_FINDINGS.md
(before adding this publication note).
