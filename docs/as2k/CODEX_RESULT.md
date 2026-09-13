# AS2000 REVIEW result — PASS

Date: 2026-09-12. Branch: as2k-mame0289-dev; parent 08b1daf80f1.
Attempt 2 accepted: bounded newline insertion/Backspace join and restart coverage.
Read AGENTS.md, CODEX_NEXT.md, canonical private AS2K_KNOWLEDGE.md,
LOCAL summary, candidate scripts, external validator and LCD_RENDERING.md.
No emulator defect demonstrated; no driver, CPU/video core, shared decoder,
workflow or trace-format changes.

LOCAL summary 2026-09-12T23:49:15-06:00 exited 0.
Artifacts: /home/spc/Projects/alphasmart/tools/automation/state/as2k-validation.eQ3vbLtJ
Log: as2k-local-validation-20260912-231548.log; not read in REVIEW.
External state/as2k-local-validator.sh records commands: focused
make -j3 SUBTARGET=as2kdiag SOURCES=src/mame/skeleton/alphasma.cpp
REGENIE=1 USE_QTDEBUG=0 for production and CI instrumentation; both -validate;
v314/v308 -verifyroms; fixtures, HC11 harness and retained runtime gates.
Newline phases each have 180 emulated-second / 300 wall-second bounds,
fresh private NVRAM for write and shared saved NVRAM in a separate recall process.

Five ordered write observations passed (both rows padded to 40 cells):
- original: abcd / blank;
- Left twice, Return: ab<B5> / cd;
- Backspace at second-line start: abcd / blank;
- Return again: ab<B5> / cd;
- F2/F1: ab<B5> / cd.
Separate-process restart and its F2/F1 both passed ab<B5> / cd.
Checker requires keyboard transitions and LCD writes between checkpoints,
exact raw DDRAM cells, display enabled and ordered completion.
Prior LOCAL IUI9Jp1C measured saved F1 bytes 61 62 0D 63 64 at bank0:069D;
that bounded stored-CR observation is retained, not independently remeasured here.
B5 denotes a bus byte; physical marker glyph and general encoding remain unverified.
Five-frame presses and 60 idle frames at PC 87D7 are test parameters,
not hardware timing. No CPU/RAM/interrupt injection.

Retained gates passed: production kKzZ=+ persisted bytes and recall completion;
Send COL.7/10 released 10 -> pressed 00 -> released 10; diagnostic az09=+
and restart content; six F05 glyphs (240 pixels); F1-F8 isolation/restart
and canonical bank/file bounds; interior editing/restart; input fixtures;
HC11 harness including 148 synthetic STOP/wake cycles.
No contradiction with canonical derived engineering evidence was found.
The earlier blank-LCD checkpoint remains a separate unresolved observation.
No exhaustive editing, production LCD bus, RAM/decode, physical LCD equivalence,
new firmware-backed unmasked-XIRQ/auto-off, wake-source/period or Send host-transfer
claim. LCD_RENDERING.md decoder-shift and F05 NEEDS REDUMP limits remain.

Cheap REVIEW checks:
- Three candidate scripts, shared decoder and workflow match LOCAL snapshot bytes.
- PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_newline_edit.py: 4 tests PASS.
- bash -n external state/as2k-local-validator.sh: PASS.
- git diff --check: PASS.
No make, rebuild, MAME launch, polling or complete validation-log read in REVIEW.
LOCAL deliberately used HEAD gpl_renderer.h in its disposable snapshot;
unrelated malformed workspace bytes remain untouched and excluded from publication.
No whole-workspace identity claim is made.

Files selected: scripts/as2k_newline_edit.lua, scripts/as2k_check_newline_edit.py,
scripts/as2k_test_newline_edit.py, docs/as2k/CODEX_RESULT.md, docs/as2k/CODEX_NEXT.md.
External validator, private ROMs, logs and generated artifacts are excluded.
Next task: bounded Left/Right cursor traversal across an explicit newline.
Publication targets only origin/as2k-mame0289-dev; PREPARE follows successful push.

Final git status --short before staging/publication:
```
 M docs/as2k/CODEX_NEXT.md
 M docs/as2k/CODEX_RESULT.md
?? scripts/as2k_check_newline_edit.py
?? scripts/as2k_newline_edit.lua
?? scripts/as2k_test_newline_edit.py
```
