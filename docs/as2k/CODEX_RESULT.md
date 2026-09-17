# AS2000 automation — wired Send host-sense diagnosis

2026-09-16; branch as2k-mame0289-dev; baseline d73a6b6937d.

Read AGENTS.md, automation control/inbox/task, CODEX_NEXT/RESULT, findings and
private knowledge/as2k/AS2K_KNOWLEDGE.md. Startup clean; fetch succeeded;
HEAD/origin divergence 0/0. Continued higher-priority wired Send prerequisites.
Separate firmware checkout untouched.

Changes and evidence:
- Added a read-only PORTA tap to the existing ordinary matrix Send probe.
  Observe from Send press to completion; no CPU/memory/signal injection.
- Source-derived hypothesis before testing: PA0/PA2 stay low because the
  driver feeds back masked output-latch state and has no external host source.
- Two fresh runs: 77 PORTA reads each, PA0/PA2 OR mask 00. Ordered normal
  Send -> $9716 -> $D2DC, release/completion/PROBE_STOP; MAME exit 0. No $8606.
- Classifier now requires ordered, unique, positive read evidence; rejects
  missing/malformed/zero/duplicate/boot-only observations and high sense bits.
  Count 77 is not hard-coded as a runtime requirement.
- Classification: emulator host-interface modeling gap; no demonstrated input
  harness defect, firmware regression, SCI defect or infrastructure blocker.
  Wired Send remains OPEN/EVIDENCE: attachment polarity/sequence/handshakes
  are unverified. No speculative driver/core change is justified.

Validation:
- `PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_send_probe.py`: 10 PASS.
- Same command with `--runtime`: VALIDATED stock route and low host senses.
  Private runs: ~/.local/state/as2k-emulator-automation/send-probe/
  run.uojvmqid/ (initial observation), run.sd0yzbmg/ (classifier validation).
- `PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_boot_editor_harness.py`:
  4 PASS.
- `bash tools/automation/test_as2k_boot_editor.sh`: PASS, PC=87D7, 60 frames.
- `git diff --check`: PASS.
- ROM SHA1 e0b777dc68c671c31ba808e214fb9d2573b9a853 (runner verified).
- Existing as2kdiag SHA256
  0720c1475f4ed01edcf20748c3810bd633b4b4271c24e3936e3dbae947579d7b.
  Lua/Python only; compilation unnecessary. F05 NEEDS REDUMP remains.

No unexplained regression. No transport, LCD-content, physical timing,
patched-firmware or Gate 1A completion claim. Next narrow stage: establish
host attachment evidence before wired modeling; Print remains separate.
Only derived observations and redistributable scripts/docs are published.

Final pre-publication `git status --short`:
```text
 M docs/as2k/CODEX_RESULT.md
 M docs/as2k/EMULATION_FINDINGS.md
 M scripts/as2k_send_probe.lua
 M scripts/as2k_test_send_probe.py
```
Expected post-publication status: clean.
