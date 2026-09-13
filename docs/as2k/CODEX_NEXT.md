# Current Codex task — AS2000 LCD character rendering evidence

Date: 2026-09-12. Branch: as2k-mame0289-dev.

## Established state

The $0001 investigation passed: both BIOSes read mapped $FF and immediately
write it to TFLG1 before idle STOP. See REGISTER_0001.md; no production correction
is justified. Hardware reserved-register behavior remains OPEN/DEFERRED.
Preserve the published masked-XIRQ fix and its timing/coverage limits.
Read AGENTS.md and the mandatory private AS2K_KNOWLEDGE.md first. Do not
rediscover established banking, file-switch or keyboard findings.

## One narrow task

Investigate the existing wrong-character-ROM TODO and the gap between decoded
ASCII LCD bus text and actual rendered LCD pixels. Establish the current AS2000
LCD device/CGROM selection and whether locally available CGROM evidence permits
identifying a concrete rendering mismatch. Use existing private assets only;
never publish ROMs, dumps, firmware disassembly or proprietary glyph tables.

Authorized: source inspection, opt-in bounded diagnostics and factual documentation
in docs/as2k/LCD_RENDERING.md. Capture rendered output for the established
`az09=+` sequence and compare its character codes/device mapping with the bus
trace. Keep private captures outside Git. Do not alter CPU/video cores, CGROM,
banking, keyboard, wake timing or the existing workflow/trace format. Describe
any evidence-backed minimum correction as a follow-up, not an implementation.

## Pass criteria and local validation

- Identify LCD configuration, selected ROM identifiers/hashes, ROM audit status
  and the distinction between bus character values and rendered glyphs.
- Obtain bounded runtime pixel evidence for v314's established input sequence
  and correlate it with decoded LCD text; avoid premature input or observation.
- State exactly what is and is not provable without a verified hardware CGROM
  or reference display. A warning/TODO alone is not a demonstrated pixel defect.
- Preserve the existing keyboard/LCD/F1/F8/NVRAM/restart-recall smoke gate.
- Supply a reproducible bounded external local validator. Expensive builds and
  runtime tests belong to LOCAL, never REVIEW. Run git diff --check.

If no defensible correction can be established, document OPEN/DEFERRED with
missing evidence and move on after at most three attempts without material
progress. Do not block the emulator effort on unavailable hardware reference.
Update CODEX_RESULT.md. On accepted REVIEW, commit only redistributable changes,
push only origin/as2k-mame0289-dev, select one next narrow task and write PREPARE
to the external automation decision file.
