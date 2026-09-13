# AS2000 REVIEW result — PASS

Date: 2026-09-12. Branch: as2k-mame0289-dev; reviewed parent cd7152c57c7.
Read AGENTS.md, current task, canonical private AS2K_KNOWLEDGE.md, status/diff,
concise LOCAL summary, validator, fixtures and LCD_RENDERING.md.

Accepted exactly four established AS2000 input corrections: K k/K (COL.0
mask 04), equals =/+ (COL.0 mask 10), Z z/Z (COL.12 mask 40), and Send/F12
(COL.7 mask 10, firmware keycode 47). AlphaSmart Pro is unchanged. Helper
accepts legacy, corrected and mixed states, rejects unexpected fields before
writing, and is idempotent. No core, workflow, trace or hardware-model changes.

LOCAL summary 2026-09-12T19:51:26-06:00 exited 0. Private artifacts:
external state/as2k-validation.v5pzZUGV; log as2k-local-validation-20260912-191158.log.
LOCAL commands were focused make -j3 SUBTARGET=as2kdiag
SOURCES=src/mame/skeleton/alphasma.cpp REGENIE=1 USE_QTDEBUG=0,
-validate for production and CI diagnostic variants, production -verifyroms
for v314/v308, input fixtures, HC11 harness, bounded v314 production and
diagnostic input/recall, explicit Send input and six-character pixel checker.
Exact commands are in external state/as2k-local-validator.sh.

Observed PASS: production idle-gated natural typing kKzZ=+, F1/F8 sequence,
131072-byte persisted NVRAM containing typed tokens and completed recall
sequence. Input completed at 20 emulated seconds, recall at 5. Send COL.7
mask 10 transitioned released=10, pressed=00, released=10 (7 seconds).
Diagnostic az09=+, F1/F8, decoded LCD text, saved NVRAM and restart content
recall passed. Bus codes 61 7A 30 39 3D 2B matched all 240 selected F05 glyph
pixels. Nine HC11 harness checks passed, including 148 synthetic wake cycles.

Limits: production recall completion is not a content/display assertion;
content recall is checked through diagnostic bus traces. Send input does not
prove host transfer. Six glyphs establish selected-ROM pixel consistency,
not physical charset correctness (OPEN/DEFERRED; F05 NEEDS REDUMP remains).
No exhaustive bank/address coverage, new firmware-backed unmasked XIRQ proof,
or physical wake source/period measurement. Synthetic cycles are not firmware
auto-off evidence. Canonical input corrections and idle PC 87D7 agree. The
private blank-LCD symptom remains an earlier checkpoint; later successful
idle-gated observations are preserved separately, without erasing that result.

REVIEW cheap checks: python3 scripts/as2k_test_input_fixes.py passed all 16
legacy/corrected combinations, idempotence/outside preservation and 15 rejected
fixtures; bash -n external validator and git diff --check passed. All
non-document tracked files plus the new fixture match the tested snapshot
after reproducing exact CI instrumentation in a temporary directory.
No build, MAME launch, polling or full validation-log read during REVIEW.

Files changed: src/mame/skeleton/alphasma.cpp, scripts/as2k_apply_input_fixes.py,
scripts/as2k_test_input_fixes.py, CODEX_RESULT.md and CODEX_NEXT.md.
Next task: bounded F1–F8 file-isolation and restart-content regression,
using the canonical bank map without rediscovering it. No defect presumed.

Final git status --short before staging/publication:
```
 M docs/as2k/CODEX_NEXT.md
 M docs/as2k/CODEX_RESULT.md
 M scripts/as2k_apply_input_fixes.py
 M src/mame/skeleton/alphasma.cpp
?? scripts/as2k_test_input_fixes.py
```
Only these redistributable changes are selected for publication.
