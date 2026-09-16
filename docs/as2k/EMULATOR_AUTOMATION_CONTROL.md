# AS2K Emulator Unattended Automation Control

Status: ACTIVE CONTROL CONTRACT

## Mission
Continue development and validation of the AlphaSmart 2000 (`asma2k`) emulator autonomously on `as2k-mame0289-dev`, using the existing MAME 0.289 work as the baseline.

This loop is independent from the `AS2K-V3.14.x` Gate 1A firmware worker. It may improve emulator fidelity and diagnostics needed by Gate 1A, but it must not modify the firmware patch or declare Gate 1A closed.

## Cycle contract
Every productive cycle must:
1. synchronize safely with `origin/as2k-mame0289-dev`;
2. read `AGENTS.md`, this file, `EMULATOR_AUTOMATION_INBOX.md`, `EMULATOR_AUTOMATION_TASK.md`, `CODEX_NEXT.md`, `CODEX_RESULT.md`, and `EMULATION_FINDINGS.md`;
3. inspect recent commits and local state before editing;
4. select the highest-priority unresolved authorized emulator task;
5. diagnose before changing code;
6. make the smallest justified implementation or harness repair;
7. compile only when source changes require it, with maximum `-j3`;
8. run focused automated tests/regressions and collect compact evidence;
9. distinguish emulator defects, harness defects, incomplete evidence, infrastructure failures, and firmware behavior;
10. repair recoverable failures in a bounded way and rerun the affected test;
11. stop at unexplained regressions rather than stacking speculative fixes;
12. update factual reusable findings when evidence materially changes;
13. commit/push only verified safe source/docs changes;
14. return to the supervisor; the next cycle begins 10 minutes after completion.

## Engineering priority
Current priority order:
1. reliable keyboard/input automation and observable normal matrix behavior;
2. Send key (`$47`) and normal Send path evidence toward `$8606`;
3. Print path evidence toward `$ABC9`;
4. Find, Clear/Recover, SpellCheck, F1-F8 and normal editor/typing behavior;
5. MMIO/IRQ/timer fidelity needed by observed firmware behavior;
6. MC68HC11D0 SCI behavior required for faithful wired transmission;
7. NVRAM/reset/persistence regression coverage;
8. other AS2000 fidelity gaps supported by measured evidence.

Do not jump to unrelated peripherals merely because they are easier.

## Architectural invariants
- Preserve the current in-project LCD/display approach unless measured evidence requires a change.
- Do not modify MAME CPU/video cores unless the active task explicitly proves that the defect belongs there and authorizes the change.
- Prefer normal emulated input paths over debugger-only injection.
- Preserve diagnostic observability and reproducibility.
- Treat AlphaSmart 2000 v3.1.4 as the primary behavioral baseline for current validation.
- Do not begin AS3000/NEO implementation from this worker.
- Do not implement DynFS or firmware filesystem changes here.

## Private/proprietary boundary
Never commit ROMs, NVRAM/user dumps, generated executables, proprietary traces, historical proprietary source/binaries, secrets, or local machine configuration. Private inputs remain local and should be identified by hash where practical.

## Self-healing ceiling
Automatically allowed, bounded:
- retry transient Git/network failures;
- recreate temporary run state;
- recover stale service state;
- retry an incomplete emulator launch/test;
- rebuild a missing/invalid known local emulator binary when required;
- fix a demonstrated harness/test defect;
- safely fast-forward or rebase worker-owned publication commits when no human work is endangered.

Forbidden:
- `git reset --hard` or destructive `git clean` recovery;
- discarding unknown human changes;
- changing architecture merely to force a test PASS;
- treating deterministic emulator/firmware failure as transient infrastructure noise;
- weakening a regression or classifier to obtain PASS;
- touching the separate Gate 1A firmware checkout.

## Failure classes
Classify each blocked cycle as one of: CONTROL_PLANE, HOST, TOOL_BUILD, PRIVATE_INPUT, EMULATOR_RUNTIME, EVIDENCE, ANALYSIS_PUBLICATION, or UNSAFE_GIT_STATE. Retry only classes that are actually recoverable.

## Publication
Small commits, English messages, factual evidence. Do not create documentation churn when there is no material change. Information useful to AS3000, NEO, BetaWise or hardware analysis should be recorded factually in the appropriate existing findings document.
