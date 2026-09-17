# AS2000 automation — reproducible boot readiness harness

2026-09-16; branch as2k-mame0289-dev; baseline fa02f95f541.

Read AGENTS.md, automation control/inbox/task, CODEX_NEXT.md, previous
CODEX_RESULT.md, EMULATION_FINDINGS.md and private AS2K_KNOWLEDGE.md.
This cycle inherited the four staged repair/report files listed below and an
unrelated unstaged worker sandbox change; all were inspected. The latter is
preserved and excluded from publication. Newer automation directive prioritizes reliable
normal input diagnostics over the older downward-scroll coverage task.

Diagnosed a harness portability defect: BOOT_EDITOR copied its required Lua
probe from /tmp/as2k_editor_ready.lua, which is not a repository dependency.
Added the existing idle observation logic as a repository Lua probe and copied
from that location with an explicit missing-probe classification. State-directory
creation now checks failure. A readiness marker cannot override a nonzero
MAME exit. No driver, CPU/video core, firmware or build changes.

Validation:
- Before and after repair:
  XDG_STATE_HOME=/tmp/as2k-worker-validation bash tools/automation/test_as2k_boot_editor.sh
  Both exit 0: VALIDATED BOOT_EDITOR; PC=87D7, stable_frames=60.
  Existing as2kdiag binary, private v3.1.4 SHA1
  e0b777dc68c671c31ba808e214fb9d2573b9a853; fresh per-run state.
- bash -n tools/automation/test_as2k_boot_editor.sh: PASS.
- PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_boot_editor_harness.py:
  4 ROM-free tests PASS: success, missing repository probe,
  missing readiness marker, and marker followed by runtime failure.
  Fake MAME/hash commands exercise classification only, not firmware behavior.
- git diff --check: PASS.

Scope: editor readiness only; no new Send/Print routing, typing, LCD, SCI,
scrolling or physical hardware claim. No unexplained regression observed.
EMULATION_FINDINGS.md unchanged: no new reusable hardware/firmware finding.

Publication recovery (this cycle):
- `git fetch origin as2k-mame0289-dev`: PASS; previous HOST read-only
  metadata restriction no longer occurs.
- `git rev-list --left-right --count HEAD...origin/as2k-mame0289-dev`:
  1 / 0 at startup; no incoming commits or merge required.
- Re-ran the runtime, four ROM-free tests, shell syntax and both staged and
  unstaged diff checks above: all PASS. Runtime again reported PC=87D7,
  stable_frames=60. No new source changes or build required.
- Reviewed the existing unpublished fa02f95f541 harness commit before
  publishing it together with this repair. No proprietary artifacts included.

This cycle closes the interrupted boot-readiness repair/publication stage.
Next authorized stage: normal matrix Send/input path evidence; no Send/Print
or SCI result is claimed here. No unexplained regression observed.

Safe publication files:
- docs/as2k/CODEX_RESULT.md
- scripts/as2k_editor_ready.lua
- scripts/as2k_test_boot_editor_harness.py
- tools/automation/test_as2k_boot_editor.sh

Preserved unrelated local change / expected post-publication status:
```text
 M tools/automation/run_as2k_emulator_cycle.sh
```
