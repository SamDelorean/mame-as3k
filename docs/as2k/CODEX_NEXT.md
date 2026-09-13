# Current Codex task — F1–F8 file isolation and restart regression

Date: 2026-09-12. Branch: as2k-mame0289-dev. Attempt 1.
Read AGENTS.md and mandatory private AS2K_KNOWLEDGE.md first.
Production K/Z/+ and Send mappings are accepted. Preserve LCD_RENDERING.md,
the earlier blank-LCD checkpoint and all existing XIRQ/charset coverage limits.

## One narrow task

Extend the current F1/F8 smoke to all eight firmware files: unique typed
content must survive file switching and restart without cross-file corruption.
Use the established fn_9486 bank mapping and ROM file bounds from the private
evidence base; do not rediscover them. This closes a regression coverage gap,
not a demonstrated emulator defect or exhaustive physical RAM/decode proof.
Do not touch unrelated peripherals, CPU/video cores or speculate about hardware.

## Pass criteria and LOCAL validation

- Add a reproducible bounded idle-gated F1–F8 typing/recall regression with
  distinct per-file tokens, verify each file's content after switching away
  and back, and verify all eight again after restart with persisted NVRAM.
- Assert file-specific content and absence of other files' tokens in the
  observed file; completion markers or a global NVRAM substring alone are
  insufficient evidence of isolation. Use diagnostic LCD bus observations
  and document precisely any production coverage provided.
- Retain production kKzZ=+, explicit Send transition, diagnostic keyboard/LCD,
  six-character pixel consistency and existing build/validation gates.
- Keep all private ROMs, NVRAM, executables and logs outside Git; preserve CI
  workflow and trace format. Provide an external bounded LOCAL validator.
- Stop at the first unexplained regression; establish observation versus
  emulator failure before any minimal source correction.
- Update CODEX_RESULT.md with commands, values, files and evidence limits;
  git diff --check must pass. REVIEW performs no build or long runtime.
  Commit/push accepted redistributable changes only to origin/as2k-mame0289-dev.

After at most three attempts without materially new evidence or progress,
apply the user's OPEN/DEFERRED rule and select one different narrow task.
Do not claim exhaustive RAM-bank/address or physical ZPSD decode coverage.
