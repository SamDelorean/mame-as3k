# AS2000 REVIEW result — PASS

Date: 2026-09-13. Branch: as2k-mame0289-dev; parent 27b2a5b029e.
Attempt 1 accepted: bounded Up/Down motion between two explicit-newline rows.
Read AGENTS.md, CODEX_NEXT.md, mandatory private AS2K_KNOWLEDGE.md,
LOCAL summary, candidate scripts, external validator and LCD_RENDERING.md.
No demonstrated emulator defect or driver/core correction.

LOCAL summary 2026-09-13T02:29:18-06:00 exited 0.
Private artifacts: external state/as2k-validation.OvsfXgYu.
Log: as2k-local-validation-20260913-013751.log; not read in REVIEW.
The validator runs bounded make -j1 SUBTARGET=as2kdiag
SOURCES=src/mame/skeleton/alphasma.cpp REGENIE=1 USE_QTDEBUG=0 for production
and CI instrumentation, both -validate, and v314/v308 -verifyroms.
Both small audit files report one set OK / best available, F05 NEEDS REDUMP.
The earlier parallel-build compiler ICE did not recur; its cause is unproven.
No build, MAME launch, long test or polling was performed in REVIEW.

Firmware runtime (summary lines 3819–3827, distinct from synthetic fixtures):
- original abcd / blank;
- Left twice, Return: ab<B5> / cd, cursor before c;
- Up once, x: xab<B5> / cd;
- Down once, y: xab<B5> / cyd (insertion at column one);
- F2/F1: xab<B5> / cyd;
- separate-process restart and its F2/F1: xab<B5> / cyd.
Rows are exact raw DDRAM values padded with spaces to 40 cells. The checker
requires ordered checkpoints, keyboard transitions and LCD writes between
checkpoints, display enabled and ordered completion. Negative fixtures reject
ignored/horizontal/overwrite/wrong-column behavior and stale recall.
Fresh private NVRAM is shared only with the separate recall process.
Each phase is bounded at 180 emulated / 300 wall seconds. Five-frame presses
and 60 idle frames at PC 87D7 are test parameters, not hardware timing.
No CPU/RAM/IRQ injection. Candidate comments saying expectations await LOCAL
record their pre-validation status; this report supplies acceptance evidence.

Retained gates passed: Left/Right traversal and recall; newline/join and recall;
interior editing and recall; F1-F8 isolation/restart and canonical bank/file
bounds; production kKzZ=+ persisted bytes and recall completion; Send COL.7/10
released 10 -> pressed 00 -> released 10; diagnostic az09=+ and restart;
six F05 glyphs (240 pixels); input fixtures (16 combinations, 15 rejected
fixtures); isolated HC11 harness including 148 synthetic STOP/wake cycles.
These cycles are not new firmware auto-off evidence.

No contradiction with canonical derived evidence was found. Preserve the
separate earlier blank-LCD checkpoint, unverified physical B5 glyph, F05
NEEDS REDUMP and decoder-shift limits. No unequal-length clamping, wrapping,
scrolling, exhaustive editing, production LCD bus, RAM/decode, physical LCD
equivalence, new firmware-backed unmasked-XIRQ/auto-off, physical wake-source/
period or Send host-transfer claim.

Cheap REVIEW checks:
- Candidate Lua/checker/fixtures, shared decoder and workflow byte-match LOCAL snapshot.
- PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_newline_edit.py: 7 tests PASS.
- bash -n external state/as2k-local-validator.sh: PASS.
- git diff --check: PASS.
LOCAL substituted committed gpl_renderer.h only in its disposable snapshot;
unrelated malformed local bytes remain excluded. No whole-workspace identity claim.

Files changed: scripts/as2k_newline_edit.lua, scripts/as2k_check_newline_edit.py,
scripts/as2k_test_newline_edit.py, docs/as2k/CODEX_RESULT.md, docs/as2k/CODEX_NEXT.md.
External validator, ROMs, logs and generated artifacts are excluded.
Next task: explicit-newline text across the boundary into the third LCD row.
Publication targets only origin/as2k-mame0289-dev; PREPARE follows successful push.

Final git status --short before staging/publication:
```
 M docs/as2k/CODEX_NEXT.md
 M docs/as2k/CODEX_RESULT.md
 M scripts/as2k_check_newline_edit.py
 M scripts/as2k_newline_edit.lua
 M scripts/as2k_test_newline_edit.py
```
