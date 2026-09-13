# Current Codex task — promote established AS2000 input corrections

Date: 2026-09-12. Branch: as2k-mame0289-dev. Attempt 1.

Read AGENTS.md and mandatory private AS2K_KNOWLEDGE.md first.
LCD_RENDERING.md records accepted six-character bus/pixel consistency;
physical charset accuracy is OPEN/DEFERRED pending verified AS2000 reference.
Preserve the earlier blank-LCD checkpoint and all XIRQ timing/coverage limits.

## One narrow task

Promote the already validated AS2000 K/Z uppercase, + and Send mappings from
scripts/as2k_apply_input_fixes.py into src/mame/skeleton/alphasma.cpp.
The production input block still has lowercase K/Z duplicates, underscore in
the + slot and unused COL.7 bit 0x10. Do not rediscover the firmware mapping.
Keep AlphaSmart Pro inputs unchanged. Make the helper safely accept the corrected
state while still rejecting unexpected mappings; preserve diagnostic workflow
behavior and trace format. No CPU/video core or peripheral changes.

## Pass criteria and local validation

- Production AS2000 mappings match the established helper corrections exactly.
- Helper accepts both the legacy input block and corrected block, is idempotent,
  and rejects unexpected mappings; use cheap focused fixture checks.
- Bounded LOCAL production runtime demonstrates lowercase/uppercase K and Z,
  = and + via natural keyboard with idle-gated observation. Verify Send's
  COL.7 bit 0x10 transition via explicit input; do not claim host transfer works.
- Preserve CI-instrumented keyboard/LCD/F1/F8/NVRAM/restart smoke and the
  six-character pixel consistency gate. Keep private artifacts outside Git.
- Build/validate production and diagnostic variants in LOCAL only. Supply a
  bounded external validator; REVIEW performs no build or long runtime test.
- Document production versus diagnostic coverage, run git diff --check, and
  update CODEX_RESULT.md. Commit/push only accepted redistributable changes to
  origin/as2k-mame0289-dev after all gates pass.

Stop on the first unexplained regression. After at most three attempts without
material progress apply the user's OPEN/DEFERRED rule and select one different
narrow unresolved task. Never invent a speculative hardware correction.
