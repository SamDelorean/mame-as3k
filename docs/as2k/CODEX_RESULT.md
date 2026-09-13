# AS2000 REVIEW result — PASS

Date: 2026-09-12. Branch: as2k-mame0289-dev; parent 047260f28a0.
Read AGENTS.md, CODEX_NEXT.md, canonical private AS2K_KNOWLEDGE.md,
LOCAL summary, candidate scripts/diff, external validator and LCD_RENDERING.md.
Accepted bounded interior insertion/backspace and restart regression coverage.
No emulator or CPU/video core changes, workflow or trace-format changes.

LOCAL summary 2026-09-12T22:16:17-06:00 exited 0.
Artifacts: /home/spc/Projects/alphasmart/tools/automation/state/as2k-validation.XoItIsFW
Log: as2k-local-validation-20260912-213948.log; not read in REVIEW.
External state/as2k-local-validator.sh records exact commands: focused
make -j3 SUBTARGET=as2kdiag SOURCES=src/mame/skeleton/alphasma.cpp
REGENIE=1 USE_QTDEBUG=0 for production and CI instrumentation; both -validate;
v314/v308 -verifyroms; input fixtures, HC11 harness, production/diagnostic
keyboard and recall, Send, pixel, all-eight-file and cursor-edit gates.
Cursor phases each use 180 emulated seconds / 300 wall seconds maximum,
fresh private NVRAM for write and shared saved NVRAM in a separate recall process.

Observed exact row 0, padded to 40 columns, in order:
F1, type abcd -> abcd; Left twice, x -> abxcd;
Backspace -> abcd; y -> abycd; F2 then F1 -> abycd.
Restart F1 -> abycd; F2 then F1 -> abycd.
Five write and two recall checkpoints passed, requiring LCD writes and
keyboard transitions between observations and ordered completion.
Corrected test coordinates Left COL.5/80 and Backspace/Delete COL.9/01
resolved the earlier wrong-input-map failure (abcdx instead of abxcd).
No emulator defect was demonstrated. Five-frame physical presses and
60 consecutive idle frames at PC 87D7 with empty keyboard queue are test
parameters, not physical timing measurements. No CPU/RAM/interrupt injection.

Retained gates passed: production kKzZ=+ persisted bytes and recall completion;
Send COL.7/10 released 10 -> pressed 00 -> released 10; diagnostic az09=+
and restart content; six F05 glyphs (240 pixels); F1–F8 isolation/restart
and canonical bank/file bounds; input fixtures; HC11 harness including
148 synthetic STOP/wake cycles. No contradiction with canonical derived
engineering evidence; preserve the earlier blank-LCD checkpoint separately.

Limits: new content checks are diagnostic v314 bus observations of one short
row, not exhaustive editing, production display proof, RAM/decode proof or
physical LCD equivalence. Decoder display-shift limits, LCD_RENDERING.md,
F05 NEEDS REDUMP and physical charset OPEN/DEFERRED remain. No new
firmware-backed unmasked-XIRQ, auto-off, physical wake period/source or
Send host-transfer evidence is claimed.

REVIEW checks: all three candidate scripts match LOCAL snapshot; production
driver transformed with exact CI instrumentation/input helper matches snapshot.
Other non-document tracked files match except unrelated local
src/devices/machine/gpl_renderer.h: malformed bytes differ from HEAD and
snapshot despite absence from short status. Left untouched and excluded
from staging; no claim of whole-workspace identity. No candidate change uses it.
PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_cursor_edit.py: four tests PASS.
bash -n external validator and git diff --check: PASS.
No build, MAME launch, polling or complete validation-log read in REVIEW.

Files selected: scripts/as2k_cursor_edit.lua, scripts/as2k_check_cursor_edit.py,
scripts/as2k_test_cursor_edit.py, docs/as2k/CODEX_RESULT.md, docs/as2k/CODEX_NEXT.md.
Next task: bounded newline insertion and Backspace line-join/restart coverage.
External validator, ROMs, logs and generated artifacts are excluded from Git.

Final git status --short before staging/publication:
```
 M docs/as2k/CODEX_NEXT.md
 M docs/as2k/CODEX_RESULT.md
?? scripts/as2k_check_cursor_edit.py
?? scripts/as2k_cursor_edit.lua
?? scripts/as2k_test_cursor_edit.py
```
Publication targets only origin/as2k-mame0289-dev; decision PREPARE follows push.
