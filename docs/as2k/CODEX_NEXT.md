# Current Codex task — MC68HC11 masked-XIRQ STOP wake fidelity

Date: 2026-09-11

Read `AGENTS.md` and inspect the current branch history before editing.

## Established state

Treat the current branch through `fd523cef411` as established work.

Keyboard work has already advanced beyond the earlier regression gate.
Do not repeat the completed keyboard-binding investigation unless a regression
directly requires it.

Relevant established diagnostics include:

- validated AS2000 keyboard binding fixes;
- diagnostic MC68HC11 STOP/XIRQ correction helper;
- synthetic periodic XIRQ experiment;
- firmware-level auto-off diagnostic.

The existing evidence isolates this behavior:

- AS2000 firmware executes STOP at approximately `$87D7`;
- the firmware expects execution to resume at the instruction after STOP when
  a masked XIRQ wake occurs;
- the current stock MC68HC11 core prevents masked XIRQ from waking STOP;
- forcing only the post-STOP resume allows the firmware idle counter at `$004A`
  to progress to `$94` (148 cycles) and reach the final power-off loop near
  `$87ED`;
- therefore the firmware path is independently demonstrated and the remaining
  defect is in MC68HC11 STOP/XIRQ wake fidelity.

The synthetic 1.25 s XIRQ period is diagnostic only. It is an inferred test
value and MUST NOT be documented or implemented as verified AlphaSmart
hardware timing.

## A. Repository gate

Work only in:

`~/Projects/alphasmart/mame-as2k`

Branch must be:

`as2k-mame0289-dev`

Pull only with:

`git pull --ff-only origin as2k-mame0289-dev`

Run before edits:

Repository cleanliness is already verified by the outer wrapper; do not run `git status` inside Codex.
`git diff --check`

Stop if tracked state is unexpectedly dirty.

## B. Authorized core change

This task explicitly authorizes the minimum necessary change to:

`src/devices/cpu/mc68hc11/mc68hc11.cpp`

Implement correct MC68HC11 XIRQ wake behavior for STOP.

Required semantics:

1. An asserted XIRQ must be capable of waking the processor from STOP even
   when CCR.X masks XIRQ interrupt service.
2. In that masked-XIRQ case:
   - wake from STOP;
   - resume execution after STOP;
   - do not vector into the XIRQ ISR;
   - do not leave a false pending XIRQ solely because it was used as a masked
     wake source.
3. Normal unmasked XIRQ interrupt behavior must remain unchanged.
4. XIRQ behavior outside STOP must remain unchanged.
5. Do not generalize the change to IRQ, timer hardware, or unrelated CPU
   behavior without concrete evidence.

Use the existing
`scripts/as2k_apply_hc11_stop_xirq_fix.py`
only as diagnostic/reference evidence. Do not blindly apply it on top of a
production implementation.

## C. Diagnostic infrastructure cleanup

The current diagnostic workflow may apply the temporary MC68HC11 helper before
building.

After the production core behavior is implemented, ensure CI does NOT attempt
to patch the same core logic a second time.

Update the diagnostic workflow/helper arrangement minimally so that:

- the production source is what is tested;
- no duplicate patch is applied;
- historical diagnostic intent remains understandable;
- the synthetic XIRQ experiment can still be built independently.

It is acceptable to remove or retire a helper that is no longer needed after
the production fix, provided its historical purpose remains recoverable from
Git history.

## D. Static regression audit

Verify before dynamic testing:

- STOP state transition logic is consistent with the new masked-XIRQ wake path;
- unmasked XIRQ still reaches normal interrupt handling;
- masked XIRQ outside STOP remains masked;
- no IRQ semantics are changed;
- no AS2000 RAM-bank logic is changed;
- PA3 is not introduced as an extra RAM-bank bit;
- no keyboard matrix mapping is changed unless a new regression proves it
  necessary.

## E. Build gates

At minimum run:

`make -j3 SUBTARGET=as2kdiag SOURCES=src/mame/skeleton/alphasma.cpp REGENIE=1 USE_QTDEBUG=0`

Then:

`./as2kdiag -validate`

Also run:

`git diff --check`

If the existing workflow builds both the baseline diagnostic runtime and the
synthetic-XIRQ variant, preserve that capability.

## F. Dynamic STOP/XIRQ validation

Use the existing synthetic XIRQ experiment as the primary AS2000 test harness.

Validate, as far as the current harness permits:

1. firmware reaches STOP;
2. XIRQ is asserted while STOPped;
3. with CCR.X masked, CPU wakes and resumes after STOP;
4. it does not incorrectly enter the XIRQ interrupt vector in that masked case;
5. repeated wakes permit the firmware idle counter to advance;
6. the firmware can progress toward the established auto-off path;
7. no new boot, keyboard, LCD, or memory regression appears.

The existing Lua firmware-level forced-resume test may be used as a reference
control, but it MUST NOT be presented as proof that the production CPU-core
fix works because that script bypasses the actual wake mechanism.

If practical, also exercise an unmasked-XIRQ case and confirm that normal XIRQ
interrupt service still occurs.

If a particular dynamic case cannot be automated reliably, document that
limitation rather than inventing a result.

## G. Timing restriction

Do NOT promote the synthetic XIRQ period of 1.25 s into production AS2000
hardware emulation.

Its current purpose is only to stimulate the firmware sufficiently to test
STOP/XIRQ behavior.

Actual AlphaSmart low-power wake source and timing remain separate hardware
reverse-engineering questions.

## H. Scope restrictions

Do not implement:

- a production synthetic XIRQ timer;
- guessed auto-off oscillator hardware;
- new keyboard mappings unrelated to a regression;
- new RAM banking;
- PA3 RAM-bank semantics;
- LCD controller changes;
- dictionary hardware changes;
- serial/USB changes;
- AS3000 or NEO changes.

Do not add ROMs, firmware dumps, binaries, logs, or generated runtime artifacts
to Git.

## I. Result report

Replace:

`docs/as2k/CODEX_RESULT.md`

with a factual report containing:

- repository gate;
- exact core behavior changed;
- explanation of masked-XIRQ STOP semantics;
- files changed;
- CI/helper migration performed;
- build commands/results;
- dynamic STOP/XIRQ evidence;
- masked case result;
- unmasked case result if tested;
- auto-off firmware progression observed;
- explicit statement that 1.25 s remains unverified diagnostic timing;
- regressions checked;
- `git diff --check`;
- final outer-wrapper repository-cleanliness result;
- commit SHA and push result.

## J. Publication gate

If and only if the core change and regressions are defensible:

- commit the safe tracked changes;
- push only to `origin/as2k-mame0289-dev`.

Do not push to `master`.

## Pass criteria

Pass requires:

- masked XIRQ wakes MC68HC11 from STOP;
- masked wake resumes after STOP without false XIRQ service;
- normal unmasked XIRQ behavior is preserved;
- production core is tested directly, not patched twice by CI;
- AS2000 diagnostic build passes;
- established keyboard/LCD/memory behavior is not regressed;
- no synthetic timing value is promoted as real hardware behavior.

## Stop condition

Stop after the MC68HC11 masked-XIRQ STOP wake behavior is implemented,
validated, documented, and safely committed/pushed if all gates pass.

Do not proceed automatically to modeling the physical AlphaSmart wake source.
