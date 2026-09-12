# AS2000 REVIEW result — PASS

Date: 2026-09-12. Repository `/home/spc/Projects/alphasmart/mame-as2k`,
branch `as2k-mame0289-dev`, reviewed parent `b3d6947a11f`.
Read AGENTS.md, current task, history, diff/status, summary and local validator.
`git pull --ff-only origin as2k-mame0289-dev`: already up to date.
Dirty files were the expected task candidates.

## Change and static audit

Production `check_irq_lines()` consumes masked pending XIRQ only in STOP state 1
and changes STOP to state 2. The existing STOP handler advances past STOP;
no XIRQ stack/vector service or stale pending request is created by that wake.
Unmasked XIRQ, masked XIRQ outside STOP, WAI and IRQ paths remain unchanged.
No AS2000 banking, PA3, keyboard matrix or LCD implementation changed.
CI runs the isolated production-function harness instead of applying the retired
STOP/XIRQ helper. Independent synthetic-XIRQ builds remain supported.
The forced-resume Lua control is labelled historical and is not production-fix proof.

## Local validation evidence

Summary dated `2026-09-12T16:29:26-06:00`, exit code 0:
`/home/spc/Projects/alphasmart/tools/automation/state/as2k-validation-summary.txt`.
Artifacts: external `state/as2k-validation.TwloaRLY`.
The complete validation log was not read. REVIEW did not run make, rebuild MAME,
launch MAME tests or poll commands.

The local validator ran under `set -euo pipefail`:
- `python3 scripts/as2k_test_hc11_stop_xirq.py`: nine checks passed.
- `make -j3 SUBTARGET=as2kdiag SOURCES=src/mame/skeleton/alphasma.cpp REGENIE=1 USE_QTDEBUG=0`
  and `./as2kdiag -validate`: passed for production, instrumented baseline and
  synthetic variants.
- Baseline/synthetic v3.1.4 headless firmware observation and regression/recall
  runs passed, using the external validator's exact commands.

Core checks cover no-source STOP, S mask, masked WAI, masked STOP resume at
$87D8 without stack/vector/pending service after unmask, masked outside STOP,
asserted-before-STOP wake, unmasked STOP XIRQ vector $FFF4 with nine stacked
bytes despite I mask, unmasked outside STOP, IRQ vector $FFF2 and 148 core cycles.
Timing, memory reads and stack operations are stubbed in this isolated harness;
unmasked XIRQ has not been tested through AS2000 firmware.

The synthetic runtime observed 148 masked wakes at STOP $87D7, no XIRQ service,
and firmware auto-off PC $87ED with idle count $94. Observation never forced PC
or wake. The 1.25 s period remains unverified diagnostic timing, not production
hardware behavior. Physical wake source/timing remain unresolved.

Both variants passed typing `az09=+`, F1/F8 switching, decoded LCD text, saved
NVRAM containing both file texts and restart recall of `memory8`; final decoded
screens matched. This resolves the prior premature-input validator failure.
Memory coverage is two firmware files and restart recall, not exhaustive bank or
address coverage. LCD coverage is ASCII bus reconstruction, not CGROM pixels;
the existing CGROM NEEDS REDUMP warning remains.

## Files and cheap review checks

Changed: production MC68HC11 core, diagnostic workflow, historical forced-resume
comment, new `scripts/as2k_test_hc11_stop_xirq.py`, deleted temporary
`scripts/as2k_apply_hc11_stop_xirq_fix.py`, this report and CODEX_NEXT.md.
No proprietary ROMs, dumps, binaries, traces or local artifacts are included.
Snapshot comparison confirmed the candidate core matches the tested core after
removing observer-only logging; workflow, harness and Lua control match exactly.
Validator `bash -n` and `git diff --check` passed.

Next task: investigate the existing AS2000 nonexistent-register $0001 read TODO;
no physical wake-source modeling is authorized by that handoff.
Validated change and next-task handoff committed as `f666348a4c5`.
Push to `origin/as2k-mame0289-dev` succeeded (`b3d6947a11f..f666348a4c5`).
Final `git status --short` after that publication: empty (clean).
This publication receipt is a subsequent documentation-only commit.
