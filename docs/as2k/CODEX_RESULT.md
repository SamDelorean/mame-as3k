# AS2000 automation — normal matrix Send evidence

2026-09-16; branch as2k-mame0289-dev; baseline d3a81002a5f.

Read AGENTS.md, all requested automation/handoff/findings documents and private
AS2K_KNOWLEDGE.md. Startup status clean. Fetch succeeded; HEAD/origin divergence
0/0. Newer automation priority selects normal Send input ahead of the older
scrolling coverage task. Separate firmware checkout untouched.

Diagnosis and change:
- Existing Send mapping is correct. Existing editor scripts resolve fields by
  port/mask; no label normalization or driver repair was needed.
- Added a bounded stock-v3.1.4 Lua probe and Python runner/classifier. Fresh
  private run directory, ROM hash check, existing binary, ordinary input only.
- Preregistered expected stock route: $9716 -> $D2DC, not a wired-path PASS.
  Wait 60 frames at idle $87D7, queue `a`, wait for idle again, press Send at
  :COL.7/0x10 for five frames, release, observe 120 frames, exit.
- Both exploratory and repository-runner runs observed ordered field/ready/
  press/$9716/$D2DC/release/completion/probe-stop evidence; MAME exit 0.
  No $8606. This is documented stock firmware routing, not an emulator failure.
  KS0066 F05 still reports NEEDS REDUMP.

Reproduction/validation:
- `PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_send_probe.py`: 5 tests PASS
  (ordered route; every missing marker; boot-only landmarks; runtime failure;
  unexpected wired entry). Fixtures are classification tests only.
- Same command with `--runtime`: exit 0, VALIDATED stock_Send_matrix_to_9716_D2DC.
  Private evidence: ~/.local/state/as2k-emulator-automation/send-probe/run.nyrd6yol/
- `PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_boot_editor_harness.py`:
  4 tests PASS.
- `bash tools/automation/test_as2k_boot_editor.sh`: PASS, PC=87D7, 60 frames.
- `git diff --check`: PASS.
- Stock ROM SHA1: e0b777dc68c671c31ba808e214fb9d2573b9a853.
- Existing as2kdiag SHA256:
  0720c1475f4ed01edcf20748c3810bd633b4b4271c24e3936e3dbae947579d7b.
  No C++/core changes; compilation unnecessary.

Files changed: scripts/as2k_send_probe.lua, scripts/as2k_test_send_probe.py,
docs/as2k/EMULATION_FINDINGS.md, docs/as2k/CODEX_RESULT.md.
No ROM, NVRAM, binary or raw trace included. No unexplained regression.
Limits: no LCD-content, electrical IRQ, wired Send, Print, SCI, patched-firmware
or physical hardware claim. Next stage: establish the authorized normal wired
Send prerequisites; stock editor dispatch alone cannot prove $8606. Keep Print
and remaining input coverage separate from this completed diagnostic stage.

Final pre-publication `git status --short`:
```text
 M docs/as2k/CODEX_RESULT.md
 M docs/as2k/EMULATION_FINDINGS.md
?? scripts/as2k_send_probe.lua
?? scripts/as2k_test_send_probe.py
```
Expected post-publication status: clean.
