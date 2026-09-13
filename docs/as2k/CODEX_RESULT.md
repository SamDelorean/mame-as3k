# AS2000 REVIEW result — fourth-row entry and recall PASS

2026-09-13; as2k-mame0289-dev; attempt 1.
Read AGENTS.md, CODEX_NEXT.md, mandatory private AS2K_KNOWLEDGE.md,
LOCAL summary, candidate scripts/diff, external validator and LCD_RENDERING.md.
No demonstrated emulator defect; no driver or core correction needed.

LOCAL summary 2026-09-13T06:01:05-06:00 exited 0.
Private artifacts: external state/as2k-validation.3oLxC3fU.
Log: external logs/as2k-local-validation-20260913-050527.log.
Five ordered entry checkpoints and two separate-process recall checkpoints pass:
- first Return: ab<B5> / blank / blank / blank;
- second Return: ab<B5> / cd<B5> / blank / blank;
- third Return: ab<B5> / cd<B5> / ef<B5> / blank;
- final entry, F2/F1, saved-NVRAM restart and another F2/F1:
  ab<B5> / cd<B5> / ef<B5> / gh.
All four rows are exactly 40 raw DDRAM cells, space padded. Checker requires
both displays enabled, ordered content, keyboard transitions and LCD writes
between checkpoints, and completion. Fresh private NVRAM is shared only with
its recall process. Bounds are 180 emulated / 300 wall seconds per phase.
Five-frame presses and 60 idle frames at PC 87D7 are test parameters, not
hardware timing. No CPU/RAM/IRQ injection. Candidate comments marking proposed
expectations predate LOCAL; this report records their runtime acceptance.

Retained LOCAL gates passed: third-line boundary join/resplit, three-line entry,
vertical/horizontal traversal, newline/join, interior editing, all with recall;
F1-F8 isolation/restart with canonical bank/file bounds; production kKzZ=+
persisted bytes and recall completion; Send COL.7/10 released 10 -> pressed 00
-> released 10; diagnostic az09=+ and recall; six F05 glyphs (240 pixels);
input fixtures; isolated HC11 harness including 148 synthetic STOP/wake cycles.
Validator exit 0 establishes both focused serial builds and -validate gates.
Both concise v314/v308 audit files report one ROM set OK / best available,
with F05 NEEDS REDUMP for both controllers.

No genuine contradiction with canonical evidence identified for this task.
Preserve the earlier blank-LCD observation separately from later accepted
runtime checkpoints. No physical B5 glyph, complete charset/display,
wrapping/scrolling, exhaustive editing or RAM/decode claim. Preserve
LCD_RENDERING.md and decoder-shift limits, production recall without LCD bus
evidence, firmware-backed unmasked-XIRQ limits, unmeasured wake source/period,
and untested Send host transfer. Synthetic HC11 tests are not new firmware
auto-off evidence. No new hardware assumptions promoted.

Cheap REVIEW checks:
- Byte comparisons: three candidate scripts, shared decoder and workflow match
  LOCAL snapshot exactly; no whole-workspace identity claim.
- PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_newline_edit.py:
  10 tests PASS, including missing/wrong fourth-row text, stale third-row B5,
  wrong controller/row, stale recall and missing/unordered evidence rejection.
- bash -n external state/as2k-local-validator.sh: PASS.
- git diff --check: PASS.
No make, rebuild, MAME launch, long test, command polling or full large log read.
LOCAL substituted committed gpl_renderer.h only in its disposable snapshot;
unrelated malformed bytes excluded; prior compiler ICE cause remains unproven.

Publication files: three newline scripts, CODEX_RESULT.md and CODEX_NEXT.md.
Untracked EMULATION_FINDINGS.md is separate work and excluded. External
validator/state, ROMs, logs and generated artifacts remain outside Git.
Next task: fifth explicit line and one-step vertical scrolling/recall.

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
commits 4c6a0a52722 and 5601cbce062 (Gate 1A ordering and IR boundary handoffs).
Reviewed and rebased cleanly; validated scripts unchanged. Post-rebase
git diff --check passed. Final post-rebase git status --short:
```text
?? docs/as2k/EMULATION_FINDINGS.md
```
