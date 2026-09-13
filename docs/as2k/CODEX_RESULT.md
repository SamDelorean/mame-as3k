# AS2000 REVIEW result — PASS

Date: 2026-09-12. Branch: as2k-mame0289-dev; reviewed parent ab1eddb9a6e.
Read AGENTS.md, CODEX_NEXT.md, mandatory private AS2K_KNOWLEDGE.md,
LOCAL summary, current status/diff, candidate scripts, external validator and
LCD_RENDERING.md. Accepted the F1–F8 isolation/restart regression coverage.
No emulator, CPU/video core, workflow or existing trace-format changes.

LOCAL summary 2026-09-12T20:39:02-06:00 exited 0. Private artifacts:
/home/spc/Projects/alphasmart/tools/automation/state/as2k-validation.issmyjDh
Log: as2k-local-validation-20260912-200653.log (not read during REVIEW).
Exact LOCAL commands remain in external state/as2k-local-validator.sh:
focused make -j3 SUBTARGET=as2kdiag SOURCES=src/mame/skeleton/alphasma.cpp
REGENIE=1 USE_QTDEBUG=0, production/CI diagnostic -validate, v314/v308
-verifyroms, input fixtures, HC11 harness, bounded production/diagnostic
keyboard/recall, Send, pixel checker and all-eight-file write/recall runs.
New phases have 180-second emulated / 300-second wall bounds each.

Observed PASS: file1aa through file8hh each appeared exactly once on its
selected diagnostic LCD checkpoint, with all seven other tokens absent,
after switching away/back and again in a separate process with persisted
NVRAM. All eight ordered checkpoints, completion and keyboard transitions
were required in each phase. Input/observation waits for 60 consecutive
frames at PC 87D7 with an empty natural-keyboard queue; no CPU/RAM/interrupt
injection. The 131072-byte saved RAM also passed per-file bounds checks.
Canonical fn_9486 banks agree: F1 bank0, F2/F3 bank1, F4/F5 bank2,
F6/F7/F8 bank3. Bounds agree with ROM D96A: 069D–7FFF, 0100–3FFF,
4000–7FFF, 0100–3FFF, 4000–7FFF, 0100–2FFF, 3000–5FFF, 6000–7FDE.
No contradiction with canonical derived engineering evidence was found.

Retained gates passed: production kKzZ=+, F1/F8 sequence and persisted
bytes; Send COL.7 mask 10 transitioned 10 -> 00 -> 10; diagnostic az09=+
and restart content recall; six bus codes 61 7A 30 39 3D 2B matched all
240 selected F05 glyph pixels; input-fix fixtures and nine HC11 harness
checks including 148 synthetic STOP/wake cycles.

Evidence limits: new all-eight-file content assertions are diagnostic v314
only, using reconstructed LCD bus state. Production recall completion is
not a content/display assertion. Short tokens and canonical storage ranges
do not prove exhaustive RAM addressing, physical ZPSD decode or long-file
editing. Decoder display-shift limitations remain. LCD_RENDERING.md and
F05 NEEDS REDUMP/physical charset OPEN/DEFERRED remain unchanged. Preserve
the earlier private blank-LCD checkpoint separately from later successful
idle-gated observations. No new firmware-backed unmasked-XIRQ, physical
wake source/period, firmware auto-off or host Send-transfer proof is claimed.

REVIEW cheap checks: all non-document candidate files matched the tested
LOCAL snapshot, reproducing exact CI instrumentation on a temporary copy
of the driver; bash -n external validator and git diff --check passed.
No build, MAME launch, polling or complete validation-log read in REVIEW.

Files changed: scripts/as2k_file_isolation.lua,
scripts/as2k_check_file_isolation.py, CODEX_RESULT.md and CODEX_NEXT.md.
Only these redistributable files are selected for commit/publication to
origin/as2k-mame0289-dev. External validator and artifacts stay outside Git.
Next task: bounded cursor insertion/backspace content and restart regression.

Final git status --short before staging/publication:
```
 M docs/as2k/CODEX_NEXT.md
 M docs/as2k/CODEX_RESULT.md
?? scripts/as2k_check_file_isolation.py
?? scripts/as2k_file_isolation.lua
```
Publication is followed by a clean-worktree check and external PREPARE decision.
