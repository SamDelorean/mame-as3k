# AS2000 REVIEW result — three-line controller boundary PASS

2026-09-13; as2k-mame0289-dev; attempt 1.
Read AGENTS.md, CODEX_NEXT.md, canonical private AS2K_KNOWLEDGE.md,
validation summary, current diff, validator, scripts and LCD_RENDERING.md.
No demonstrated emulator defect or defensible driver/core correction needed.

LOCAL summary 2026-09-13T03:35:07-06:00 exited 0.
Private artifacts: external state/as2k-validation.s7ZH3Aqy.
Log: external logs/as2k-local-validation-20260913-024337.log.
Summary runtime lines 3887–3894 establish these ordered raw DDRAM rows:
- first Return: ab<B5> / blank / blank / blank;
- second Return: ab<B5> / cd<B5> / blank / blank;
- final typing: ab<B5> / cd<B5> / ef / blank;
- F2/F1, separate-process restart and its F2/F1: same final four rows.
Every row is exactly 40 cells, space padded. Both controllers are checked,
including untouched row four, with displays enabled, keyboard transitions,
LCD writes between ordered checkpoints and completion. Ordinary keyboard
input uses fresh private NVRAM shared only with its separate recall process.
Each phase is bounded at 180 emulated / 300 wall seconds. Five-frame presses
and 60 idle frames at PC 87D7 are test parameters, not hardware timing.
No CPU/RAM/IRQ injection. Pre-validation expectation comments in candidate
scripts retain their historical wording; this report records runtime acceptance.

Retained LOCAL gates passed: horizontal/vertical traversal and recall;
newline/join and recall; interior editing and recall; F1-F8 isolation/restart
and canonical bank/file bounds; production kKzZ=+ persisted bytes and recall
completion; Send COL.7/10 released 10 -> pressed 00 -> released 10;
diagnostic az09=+ and restart; six F05 glyphs (240 pixels); input fixtures
(16 combinations, 15 rejected fixtures); isolated HC11 harness including
148 synthetic STOP/wake cycles, not new firmware auto-off evidence.
Validator exit 0 establishes both serial focused builds and -validate gates.
Read concise private audit files: v314/v308 each one set OK / best available,
F05 NEEDS REDUMP for both controllers. Earlier compiler ICE cause is unproven.

No contradiction with canonical derived evidence found. Preserve its earlier
blank-LCD checkpoint separately from later accepted runtime observations.
No physical B5-glyph, complete charset/display, wrapping/scrolling, exhaustive
editing or RAM/decode claim. Preserve decoder-shift and LCD_RENDERING.md
limits, production recall completion without LCD bus evidence, unmeasured
hardware timing/wake source/period, firmware-backed unmasked-XIRQ limits,
and untested Send host transfer. No new hardware assumptions promoted.

Cheap REVIEW checks:
- Candidate Lua/checker/fixtures, shared decoder and workflow byte-match LOCAL
  snapshot (Python byte comparisons).
- PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_newline_edit.py:
  8 tests PASS, including missing/wrong controller-two content, stale recall,
  wrong rows and missing evidence negative fixtures.
- bash -n external state/as2k-local-validator.sh: PASS.
- git diff --check: PASS.
No make, rebuild, MAME launch, long test, command polling or large-log read.
LOCAL substituted committed gpl_renderer.h only in its disposable snapshot;
unrelated malformed local bytes remain excluded. No whole-workspace identity claim.

Files changed: three newline scripts below, CODEX_RESULT.md and CODEX_NEXT.md.
External validator, decision, ROMs, logs and generated artifacts stay outside Git.
Next task: Backspace join and Return resplit across the controller boundary.
Publication targets only origin/as2k-mame0289-dev; PREPARE follows successful push.

Publication note: initial push rejected because origin had documentation-only
commit ccc3d81462e (GATE1A_RUNTIME_HARNESS.md). Reviewed and rebased cleanly;
validated scripts unchanged. Post-rebase git diff --check passed and status
was clean before adding this report note.

Final git status --short before staging/publication:
```
 M docs/as2k/CODEX_NEXT.md
 M docs/as2k/CODEX_RESULT.md
 M scripts/as2k_check_newline_edit.py
 M scripts/as2k_newline_edit.lua
 M scripts/as2k_test_newline_edit.py
```
