# AS2000 REVIEW result — PASS

Date: 2026-09-12. Branch as2k-mame0289-dev; reviewed parent b1f08e607ec.
Read AGENTS.md, current task, diff/status, history, mandatory private evidence,
concise validation summary, local validator and targeted private trace context.

Local summary 2026-09-12T18:04:09-06:00 exited 0; artifacts are external
state/as2k-validation.4Rf2NAKT. The earlier failed debugger attachment is
superseded by the successful Lua-installed capture plus explicit soft reset.

Both BIOSes produced one $0001 read at INIT=$00, returning $FF:
v314 instruction/access PC $87C6/$87C8; v308 $87D1/$87D3.
Private instruction context and write markers establish immediate transfer of
that value to TFLG1, then idle STOP at $87D7/$87E2 with CCR=$40.
Caller/context and reproducible capture method are in REGISTER_0001.md.
No observable emulator defect attributable to the read was demonstrated.
No production correction is justified; hardware value remains OPEN/DEFERRED.

LOCAL commands/gates passed: python3 scripts/as2k_test_hc11_stop_xirq.py
(nine checks); focused make -j3 SUBTARGET=as2kdiag
SOURCES=src/mame/skeleton/alphasma.cpp REGENIE=1 USE_QTDEBUG=0 and
./as2kdiag -validate for production and CI-instrumented variants; both BIOS
ROM audits and bounded 15-second register captures; baseline v314 keyboard
az09=+, F1/F8, decoded LCD text, saved NVRAM and restart recall of memory8.
Exact runtime commands are in external state/as2k-local-validator.sh.

Limits: soft-reset attachment does not prove initial cold-reset coverage;
no exhaustive firmware paths, RAM-bank/address coverage or CGROM pixel check.
Smoke coverage is v314 only. CGROM NEEDS REDUMP persists. No physical reserved-
register value, physical XIRQ source/period or firmware-backed unmasked XIRQ
was established. Synthetic 1.25 s timing remains diagnostic only.
Canonical idle PC/CCR agrees. Preserve the checkpoint discrepancy between the
private blank-LCD/candidate-core notes and the prior published PASS; this new
smoke PASS corroborates the later result without rewriting older evidence.

Files: REGISTER_0001.md, opt-in scripts/as2k_register_0001.cmd, this report,
and CODEX_NEXT.md. No production source, core, workflow or trace-format change.
Next narrow task: LCD rendered-character evidence, retaining bus/pixel limits.
REVIEW runs only cheap static checks; no make, rebuild, MAME launch, polling or
complete large-log read. No private artifacts are staged.

Cheap REVIEW checks passed: external validator bash -n, git diff --check,
and byte comparison of the diagnostic/core/workflow/harness with the tested
snapshot. git pull --ff-only origin as2k-mame0289-dev was already up to date.

Final git status --short before staging/publication:
```
 M docs/as2k/CODEX_NEXT.md
 M docs/as2k/CODEX_RESULT.md
?? docs/as2k/REGISTER_0001.md
?? scripts/as2k_register_0001.cmd
```
The reviewed diagnostics/documentation and next-task handoff are authorized for
commit/push to origin/as2k-mame0289-dev; automation proceeds with PREPARE only
after successful publication.
