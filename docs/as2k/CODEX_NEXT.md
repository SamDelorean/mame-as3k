# Current Codex task — cursor insertion/backspace and restart regression

Date: 2026-09-12. Branch: as2k-mame0289-dev. Attempt 1.
Read AGENTS.md and mandatory private AS2K_KNOWLEDGE.md first.
F1–F8 short-token isolation/restart coverage is accepted. Preserve existing
production input, Send, XIRQ and LCD_RENDERING.md evidence limits, including
the earlier blank-LCD checkpoint. Do not rediscover closed bank findings.

## One narrow task

Add a reproducible bounded regression for editing within an existing short
F1 token using cursor-left, insertion and backspace, then switching away/back
and restarting with persisted NVRAM. This is an unresolved coverage gap,
not a demonstrated emulator defect. Use established input definitions and
idle gating; document exact keys and expected text at each checkpoint.
Keep the token within one LCD row to avoid expanding into scrolling behavior.

## Pass criteria and LOCAL validation

- Start with fresh private NVRAM. Type a short ASCII token, move the cursor
  into its interior, insert a character, then backspace that character.
  Assert exact expected row content at each edit checkpoint through ordered
  diagnostic LCD bus observations, distinguishing insertion from overwrite
  and confirming backspace removes the character before the cursor.
- Make a final interior edit that changes the original token; verify edited
  content after switching away/back and in a new process using saved NVRAM.
  Completion markers or global NVRAM substrings alone are insufficient.
- Retain the accepted all-eight-file isolation/restart checks, production
  kKzZ=+, Send transition, diagnostic keyboard/LCD, six-character pixel gate,
  input fixtures, HC11 harness, focused builds, -validate and BIOS audits.
- Provide an external bounded LOCAL validator; no proprietary/local artifacts
  in Git. Preserve CI workflow/trace format and CPU/video cores.
- Stop at the first unexplained regression. Distinguish key injection or
  observation limitations from an emulator defect before any minimal fix.
- Update CODEX_RESULT.md factually with commands, values, files, status and
  coverage limits; git diff --check must pass. REVIEW runs no build or long
  runtime. Commit/push validated redistributable changes only to
  origin/as2k-mame0289-dev.

Do not claim exhaustive keyboard, editing, RAM/decode or physical LCD proof.
After at most three attempts without materially new evidence or progress,
apply OPEN/DEFERRED with the exact missing evidence and select one different
narrow task, unless a demonstrated blocking defect warrants continued work.
