# AS2000 REVIEW — upward scrolling and recall PASS

2026-09-13; as2k-mame0289-dev; attempt 1.
Read AGENTS.md, CODEX_NEXT.md, mandatory private AS2K_KNOWLEDGE.md,
LOCAL summary, candidate scripts/diff and external validator.
No demonstrated emulator defect; no driver or core correction required.

LOCAL summary 2026-09-13T08:24:24-06:00 exits 0.
Private artifacts: external state/as2k-validation.8z4GGYPr.
Log: external logs/as2k-local-validation-20260913-072602.log.
Twelve ordered entry/Up/insertion/switch checkpoints and two separate-process
recall checkpoints pass. Accepted fifth-line viewport is preserved:
cd<B5> / ef<B5> / gh<B5> / ij. Up 1-3 retain those rows; Up 4 yields
ab<B5> / cd<B5> / ef<B5> / gh<B5>. Inserting x yields
abx<B5> / cd<B5> / ef<B5> / gh<B5>. F2/F1, saved-NVRAM restart with F1,
and another F2/F1 preserve this top viewport for this sequence.
Each row is exactly 40 space-padded raw DDRAM cells. Keyboard transitions,
LCD bus writes between ordered observations, both displays enabled and
completion are required. Cursor-only steps may issue commands without data
rewrites. No display-shift or automatic-entry-shift commands observed from
boot through these checkpoints. Preregistered expectations were not changed;
script hypothesis comments predate acceptance here. Intermediate cursor
columns are not independently established by unchanged DDRAM rows.

LOCAL uses ordinary asma2k v314 input, fresh private NVRAM shared only with
recall, five-frame presses and 60 idle frames at PC 87D7. These are test
parameters, not hardware timing. Each editing phase is bounded at 180 emulated /
300 wall seconds; no CPU/RAM/IRQ injection in these editing phases.

Retained gates pass: fifth/fourth/three-line entry/recall, boundary join/resplit,
vertical/horizontal traversal, newline/join, interior editing, all-eight-file
isolation/restart with canonical bank bounds, production kKzZ=+ persisted bytes
and recall completion, Send COL.7/10 (10 -> 00 -> 10), diagnostic az09=+ and
recall, six F05 glyphs (240 pixels), input fixtures, isolated HC11 harness
including 148 synthetic STOP/wake cycles. Validator exit 0 establishes both
focused serial builds and -validate gates. Both concise BIOS audits report
one ROM set OK / best available and F05 NEEDS REDUMP for both controllers;
audits do not establish v308 editing behavior.

No genuine contradiction with canonical evidence identified. Preserve the
2026-09-12 blank-LCD observation and subsequent accepted runtime checkpoints
with separate provenance; this run does not explain the earlier extraction.
Physical B5 glyph, complete charset, decoder shifts, production recall without
LCD bus evidence, firmware-backed unmasked XIRQ, real wake source/period and
Send host transfer remain limited/unverified. Synthetic cycles are not new
firmware auto-off evidence. No wrap, sixth-line, exhaustive editing, RAM/decode
or physical display claim; no hardware assumption promoted.

Cheap REVIEW checks:
- Three candidate scripts, decoder and workflow match LOCAL snapshot bytes.
  HC11 source also matches; driver matches after exact CI instrumentation
  (direct driver comparison initially differed by its expected 13 added lines).
- PYTHONDONTWRITEBYTECODE=1 python3 scripts/as2k_test_newline_edit.py:
  12 ROM-free tests PASS (1.246s), including stale/lost/misplaced rows, insertion
  column, recall, shift and missing/unordered evidence negatives.
- bash -n external state/as2k-local-validator.sh: PASS.
- git diff --check: PASS.
No make, rebuild, MAME launch, long tests, polling or full validation-log read.
LOCAL committed-header substitution is disposable-snapshot-only; unrelated
malformed gpl_renderer.h bytes excluded; prior compiler ICE cause unproven.

Publication: three newline scripts, CODEX_RESULT.md and CODEX_NEXT.md.
Next task: downward cursor scrolling back to the hidden fifth line after
upward recovery. External validator unchanged. ROMs, logs, binaries and local
artifacts excluded. EMULATION_FINDINGS.md is separate work, untouched.

Final git status --short before publication:
```text
 M docs/as2k/CODEX_NEXT.md
 M docs/as2k/CODEX_RESULT.md
 M scripts/as2k_check_newline_edit.py
 M scripts/as2k_newline_edit.lua
 M scripts/as2k_test_newline_edit.py
?? docs/as2k/EMULATION_FINDINGS.md
```
Expected post-publication status: only the same untracked EMULATION_FINDINGS.md.

Publication integration: first push rejected because origin gained four
documentation-only commits through 76672732c12 (Gate 1A, host USB and SCI
findings). Reviewed and rebased cleanly; validated scripts, decoder and
workflow still match LOCAL bytes. No executable changes arrived.
Post-rebase git diff --check passed. Final git status --short after commit:
```text
?? docs/as2k/EMULATION_FINDINGS.md
```
