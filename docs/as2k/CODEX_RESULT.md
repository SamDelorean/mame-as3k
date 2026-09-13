# AS2000 REVIEW result — PASS

Date: 2026-09-13. Branch: as2k-mame0289-dev; parent a1ae56707a1.
Attempt 2 accepted: bounded Left/Right traversal across an explicit newline.
Read AGENTS.md, CODEX_NEXT.md, mandatory private AS2K_KNOWLEDGE.md,
LOCAL summary, candidate scripts, external validator and LCD_RENDERING.md.
No demonstrated emulator defect or driver/core correction.

LOCAL summary 2026-09-13T01:21:04-06:00 exited 0.
Private artifacts: external state/as2k-validation.Ifdikbk6.
Log: as2k-local-validation-20260913-002748.log; not read in REVIEW.
The prior parallel-build compiler ICE did not recur in this serial run;
its root cause remains unproven. The external validator records bounded
make -j1 SUBTARGET=as2kdiag SOURCES=src/mame/skeleton/alphasma.cpp
REGENIE=1 USE_QTDEBUG=0 for production and CI instrumentation, both -validate,
and v314/v308 -verifyroms. Both audit files report one set OK / best available,
with F05 NEEDS REDUMP. No build or MAME process was launched in REVIEW.

Firmware runtime (distinct from earlier synthetic fixture output) passed:
- original abcd / blank;
- Left twice, Return: ab<B5> / cd, cursor before c;
- Left once, x: abx<B5> / cd;
- Right once, y: abx<B5> / ycd;
- F2/F1: abx<B5> / ycd;
- separate-process restart and its F2/F1: abx<B5> / ycd.
All rows are exact raw DDRAM values space padded to 40 cells. The checker
requires ordered observations, keyboard transitions and LCD writes between
checkpoints, display enabled and ordered completion. Traversal starts with
fresh private NVRAM; recall reuses only its saved NVRAM. Each process is bounded
at 180 emulated / 300 wall seconds. Five-frame presses and 60 idle frames at
PC 87D7 are test parameters, not measured hardware timing. No CPU/RAM/IRQ injection.

Retained gates passed: newline/join and recall; interior editing and recall;
F1-F8 isolation/restart and canonical bank/file bounds; production kKzZ=+
persisted bytes and recall completion; Send COL.7/10 released 10 -> pressed 00
-> released 10; diagnostic az09=+ and restart; six F05 glyphs (240 pixels);
input fixtures (16 combinations, 15 rejected fixtures); isolated HC11 harness,
including 148 synthetic STOP/wake cycles (not firmware auto-off evidence).

No contradiction with canonical derived engineering evidence was found.
The earlier blank-LCD checkpoint remains a separate unresolved observation.
B5 is a bus marker; its physical glyph remains unverified. Preserve F05 NEEDS
REDUMP and decoder-shift limits. No exhaustive editing, production LCD bus,
RAM/decode, physical LCD equivalence, new firmware-backed unmasked-XIRQ/auto-off,
physical wake-source/period or Send host-transfer claim.

Cheap REVIEW checks:
- Candidate Lua/checker/fixtures, shared decoder and workflow byte-match LOCAL snapshot.
- PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_newline_edit.py: exit 0.
- bash -n external state/as2k-local-validator.sh: PASS.
- git diff --check: PASS.
No make, rebuild, MAME launch, long test, polling or complete validation-log read.
LOCAL substituted committed gpl_renderer.h in its disposable snapshot; unrelated
local malformed bytes remain excluded. No whole-workspace identity claim.

Files selected: scripts/as2k_newline_edit.lua, scripts/as2k_check_newline_edit.py,
scripts/as2k_test_newline_edit.py, docs/as2k/CODEX_RESULT.md, docs/as2k/CODEX_NEXT.md.
External validator, ROMs, logs and generated artifacts are excluded.
Next task: bounded Up/Down cursor motion between two short explicit-newline rows.
Publication targets only origin/as2k-mame0289-dev; PREPARE follows successful push.

Final git status --short before staging/publication:
```
 M docs/as2k/CODEX_NEXT.md
 M docs/as2k/CODEX_RESULT.md
 M scripts/as2k_check_newline_edit.py
 M scripts/as2k_newline_edit.lua
 M scripts/as2k_test_newline_edit.py
```
