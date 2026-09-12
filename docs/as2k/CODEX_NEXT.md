# Current Codex task — AS2000 keyboard matrix regression gate

Date: 2026-09-11

Read `AGENTS.md`, this file, the current driver
`src/mame/skeleton/alphasma.cpp`, the diagnostic workflow
`.github/workflows/as2k-diagnostic.yml`, and
`scripts/as2k_decode_trace.py` first.

## Established state

Treat the following commits as established:

- `491a00067df` — portable AS2000 diagnostic runtime.
- `9863db37a4b` — diagnostic instrumentation for control latch and PMR.
- `6a29953324b` — LCD bus and keyboard wake-path tracing.
- `94e13021e23` — LCD trace decoder.

Current keyboard implementation:

- 16 matrix columns: `COL.0` through `COL.15`;
- matrix selection is active low;
- `kb_r()` ANDs the input data of every selected column;
- low matrix byte is written at `0x9000`;
- high matrix byte is written at `0x2000`;
- every key transition asserts MC68HC11 IRQ;
- writing the low matrix byte clears that IRQ.

Current AS2000 RAM bank selection in `asma2k_state::port_a_w()` uses only
PA4–PA5:

`m_rambank->set_entry((data >> 4) & 0x03);`

Preserve this. Do not reinterpret PA3 as an additional RAM-bank bit.

The existing LCD path and reconstructed 40x4 text behavior are also regression
gates. Do not redesign them in this task.

---

## A. Repository gate

Work from:

`~/Projects/alphasmart/mame-as2k`

1. Pull only `origin/as2k-mame0289-dev` using `--ff-only`.
2. Confirm branch `as2k-mame0289-dev`.
3. Confirm tracked status is clean before edits.
4. Run `git diff --check`.
5. Do not touch `master`.

If the repository is not in this state, stop and report.

---

## B. Static keyboard-matrix audit

Build a factual table for every defined AS2000 key containing at minimum:

- `COL.n`;
- bit mask within that column;
- MAME `KEYCODE_*`;
- unshifted character/function;
- shifted character/function where defined;
- whether the position is intentionally unused.

Verify:

1. all 16 columns exist;
2. every active key occupies exactly one matrix position;
3. no accidental duplicate host-key mapping causes an ambiguity;
4. active-low semantics in the input definitions agree with `kb_r()`;
5. column selection agrees with the 16-bit matrix composed from
   `m_matrix[1] << 8 | m_matrix[0]`;
6. IRQ assertion/clear behavior is internally consistent with the scan path.

Do not change a key mapping merely because it looks unusual. Change mappings
only if there is concrete evidence that the current definition contradicts the
known matrix or validated runtime behavior.

Record the audit results in `docs/as2k/CODEX_RESULT.md`.

---

## C. PA3 / RAM-bank regression gate

Confirm statically that:

- AS2000 RAM banking still has exactly four entries selected by PA4–PA5;
- PA3 is not used to select an extra RAM bank;
- dictionary banking and I/O-view selection remain independent of that rule.

Do not expand RAM banking in this task.

If any existing code contradicts these requirements, stop and report before
making a speculative correction.

---

## D. Diagnostic build

Use the existing focused diagnostic target and workflow semantics.

At minimum run:

`make -j3 SUBTARGET=as2kdiag SOURCES=src/mame/skeleton/alphasma.cpp REGENIE=1 USE_QTDEBUG=0`

Then run:

`./as2kdiag -validate`

Also run:

`git diff --check`

Do not add proprietary ROMs or firmware to the repository.

---

## E. Dynamic keyboard regression

Use the narrowest reliable MAME mechanism available locally to exercise
keyboard input without modifying proprietary firmware.

The goal is to validate the matrix implementation, not to redesign it.

Exercise representative keys first, then expand to every defined matrix
position if the mechanism is reliable.

For each exercised key verify, where observable:

1. the key transition produces `AS2KTRACE KEY`;
2. firmware scan selects the expected matrix column;
3. `kb_r()` returns the expected active-low row bit;
4. IRQ assertion occurs on transition;
5. scan activity clears the IRQ through the established low-matrix write path;
6. the machine returns to normal execution rather than entering a new failure;
7. printable keys that reach the editor produce the expected character.

If reliable automated injection of every key cannot be established with the
current MAME interfaces, do not invent results. Validate the maximum defensible
subset and document exactly what prevented exhaustive dynamic coverage.

Temporary local-only diagnostic logging is permitted if required to observe
matrix selection/readback, but remove it before the final build and commit.

Do not commit generated logs or local input recordings.

---

## F. LCD regression

For any run that produces LCD trace data, decode it with:

`python3 scripts/as2k_decode_trace.py <trace-file>`

Use the reconstructed text view to confirm that keyboard testing did not
regress established LCD behavior.

Do not modify KS0066/HD44780 core behavior in this task.

---

## G. Scope restrictions

Do not implement or redesign:

- new RAM banking;
- PA3 banking semantics;
- LCD controller core behavior;
- MC68HC11 core behavior;
- dictionary hardware;
- serial/USB hardware;
- power-management behavior unrelated to keyboard wake/scan;
- AlphaSmart 3000 or NEO behavior.

Do not add ROMs, firmware, proprietary source, dumps, logs, generated
executables, or runtime artifacts to Git.

---

## H. Result and publication gate

Replace `docs/as2k/CODEX_RESULT.md` with a concise factual report containing:

- repository/branch gate result;
- complete static matrix audit summary;
- any duplicate/ambiguous mappings found;
- confirmation of active-low scan semantics;
- PA3/RAM-bank regression result;
- exact build and validation commands/results;
- dynamic keys exercised and observed matrix positions;
- IRQ assertion/clear evidence;
- LCD regression result where available;
- limitations preventing exhaustive dynamic coverage, if any;
- files changed;
- final `git diff --check`;
- final `git status --short`;
- commit SHA and push result if a commit was justified.

If the audit finds no defect requiring source changes, it is acceptable for the
only tracked changes to be documentation/handoff updates.

Commit only safe tracked changes and push only to:

`origin/as2k-mame0289-dev`

## Pass criteria

Pass means:

- all 16 keyboard columns and their active positions are accounted for;
- scan polarity and `kb_r()` selection logic are internally consistent;
- no unexplained key-matrix collision is found;
- representative dynamic keyboard scanning is demonstrated, or any inability
  to automate exhaustive injection is explicitly bounded and documented;
- PA3 remains excluded from RAM-bank selection;
- all four existing RAM banks remain selected only by PA4–PA5;
- diagnostic build and validation pass;
- existing LCD behavior is not regressed;
- no unrelated peripheral work is introduced.

## Stop condition

Stop after this keyboard-matrix regression gate is audited, tested to the
maximum reliable dynamic coverage, documented, and safely committed/pushed if
appropriate.

Do not proceed to a new peripheral or broader AS2000 hardware redesign.
