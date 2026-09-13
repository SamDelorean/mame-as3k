# AS2000 REVIEW result — PASS

Date: 2026-09-12. Branch as2k-mame0289-dev; reviewed parent 925fc32f878.
Read AGENTS.md, task, status/diff, canonical private evidence, concise LOCAL
summary, validator, checker and targeted source/snapshot differences.

LOCAL summary 2026-09-12T18:56:08-06:00 exited 0; private artifacts:
external state/as2k-validation.DjV21oOB. Both BIOS audits report one ROM set OK,
best available, with F05 NEEDS REDUMP on both controllers. Selected F05 hash
and LCD configuration are documented in LCD_RENDERING.md.

LOCAL commands: python3 scripts/as2k_test_hc11_stop_xirq.py (nine checks);
focused make -j3 SUBTARGET=as2kdiag SOURCES=src/mame/skeleton/alphasma.cpp
REGENIE=1 USE_QTDEBUG=0 and ./as2kdiag -validate for production and diagnostic
variants; -verifyroms for v314/v308; bounded v314 input/recall; pixel checker.
Exact commands remain in external state/as2k-local-validator.sh.
Input completed at 20 emulated seconds, recall at 5. Keyboard az09=+, F1/F8,
saved NVRAM and restart recall passed. Bus codes 61 7A 30 39 3D 2B matched all
240 glyph pixels from the selected F05 asset on the 240 x 36 rendered screen.

Limits: runtime uses CI instrumentation plus established input corrections;
production was built/validated/audited, not runtime smoke-tested. Six glyphs
prove selected-ROM consistency only, not physical AS2000 correctness or the
whole charset. Hardware charset remains OPEN/DEFERRED pending verified CGROM
or reference display. No production correction is justified by this task.
No exhaustive RAM coverage, new firmware-backed unmasked XIRQ evidence or
physical wake source/period measurement. Synthetic 148 core cycles are not
new firmware auto-off evidence. Canonical idle PC agrees; prior private blank
LCD and candidate-core notes remain earlier checkpoints, distinct from the
published fixes and this later successful observation. No evidence is erased.

REVIEW cheap checks: snapshot comparison (only driver differs, explained by
existing input helper/CI instrumentation; checker and other non-document
tracked files match), Python syntax, external validator bash -n, git diff
--check. No build, MAME launch, polling or complete validation-log read.

Files changed: LCD_RENDERING.md, scripts/as2k_check_lcd_pixels.py, this report
and CODEX_NEXT.md. No production/core/workflow changes or private artifacts.
Next task: promote established diagnostic input corrections into production
and validate production input behavior without rediscovering closed mappings.

Final git status --short before staging/publication:
```
 M docs/as2k/CODEX_NEXT.md
 M docs/as2k/CODEX_RESULT.md
?? docs/as2k/LCD_RENDERING.md
?? scripts/as2k_check_lcd_pixels.py
```
