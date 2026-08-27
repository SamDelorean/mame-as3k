# Current Codex task — implement only MC68EZ328 Port-A PAPUEN / WDTH8 fidelity

Date: 2026-08-26

Read `AGENTS.md`, `docs/as3k/STATUS.md`, and the current `docs/as3k/CODEX_RESULT.md` first.

## Established state

The AS3K external keyboard latch is now implemented and dynamically validated in commit:

`c5e6eb1ba51694e672d133667f86c2d3c1f71be1`

Keyboard Phase 1 still returns normally to caller `0x00420168` with `D0 = 0`.

The static MC68EZ328 keyboard GPIO audit is commit:

`08816104002f27365ace1798eddd06b78550c764`

Treat these audit conclusions as established:

- `PADIR` / `PADATA` are mapped for MC68EZ328 through `base_internal_map()`;
- `PAPUEN = 0xFFFFF402` is genuinely missing and needs real state/semantics;
- `0xFFFFF403` is reserved on MC68EZ328 and must **not** be implemented as a fake PASEL register;
- Port-A GPIO selection on EZ is controlled by `SCR.WDTH8`;
- the current EZ core changes internal `m_pasel` from `SCR.WDTH8`, but the audit found that this internal selection state is not saved on the EZ path and the clear/set behavior must be checked for exact fidelity;
- `PDSEL`, `PDKBEN/PKBDINT`, keyboard matrix plumbing, input keys, and Phase 2 are separate later stages.

This task explicitly authorizes a **narrow MC68EZ328 core change** in `src/devices/machine/mc68328.cpp` and `.h` only as required for this Port-A stage.

Do not broaden into Port D or other peripherals.

---

## A. Repository gate

From `~/Projects/alphasmart-as3k/mame0289`:

1. `git pull --ff-only as3k-project as3k-mame0289-dev`.
2. Confirm branch `as3k-mame0289-dev`, clean tracked status, and `git diff --check`.
3. Confirm latch commit `c5e6eb1ba51694e672d133667f86c2d3c1f71be1` is an ancestor of HEAD.
4. Confirm `docs/as3k/STATUS.md` identifies EZ Port-A fidelity as the next stage.
5. Do not touch `master`.

Before editing, inspect the exact current implementations of:

- `base_internal_map()`;
- `mc68ez328_device::internal_map()`;
- `padata_r/w`;
- `padir_r/w`;
- `scr_w` and SCR reset/save behavior;
- `m_pasel` declaration/reset/save registration across base/original/EZ classes.

Also re-check the MC68EZ328 manual semantics for PAPUEN and `SCR.WDTH8`.

If the smallest correct implementation would require a broad redesign or would alter original `mc68328_device` behavior in a way you cannot prove safe, stop and report before coding.

---

## B. Required behavior — PAPUEN

Implement PAPUEN for **MC68EZ328** with the documented semantics:

- address: `0xFFFFF402`;
- eight read/write pull-up-enable bits;
- reset value: `0xFF`;
- saved state;
- read returns current state;
- write stores the byte.

For Port-A pins configured as inputs and selected as GPIO, `PADATA` must use the input callback when one is connected; when no callback is connected, the documented pull-up state must provide the deterministic fallback instead of hardwired zero.

Preserve existing output-latch behavior for Port-A outputs.

Do not silently change original-MC68328 behavior. If shared base helpers/state are the cleanest implementation, ensure the original device retains its previous externally visible behavior unless the processor documentation independently justifies a correction.

Do **not** map anything at `0xFFFFF403` for EZ.

---

## C. Required behavior — SCR.WDTH8 / internal Port-A selection

Audit the current `mc68ez328_device::scr_w()` implementation against the manual and correct only the demonstrated selection-state defect.

Required end state:

- when `SCR.WDTH8` selects 8-bit operation / Port-A GPIO, internal Port-A selection must become the corresponding GPIO-selected state;
- when that bit is cleared, internal selection must return to the documented non-GPIO/data-bus state rather than remaining latched from an earlier set, if that is what the current code incorrectly does;
- the EZ save-state path must preserve the internal Port-A selection state, or it must be reliably derived from another saved register on restore in a way consistent with MAME save-state conventions.

Do not create an externally visible EZ PASEL register at `0xFFFFF403`.

Do not change unrelated SCR bits or chip-select behavior.

---

## D. Scope restrictions

Do not implement in this task:

- PDSEL;
- PDKBEN / PKBDINT;
- PDIRQEN/PDPOL/PDIQEG fixes;
- keyboard matrix callbacks;
- PA0–PA6 driver column plumbing;
- PD0–PD7 row callbacks;
- `INPUT_PORTS` or key mappings;
- Keyboard Phase 2;
- new LCD behavior;
- dynamic chip-select remapping;
- any new ROM definition or fixture.

Do not modify `src/mame/skeleton/alphasma3k.cpp` unless a build-only include/context adjustment is absolutely unavoidable. The already validated latch and LCD bridge should remain byte-for-byte untouched if possible.

---

## E. Build and static checks

Run the reduced build without clean:

```sh
make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2
```

Verify:

- build succeeds;
- all four AS3K drivers remain present;
- `./alphasma3k -verifyroms asma3kdi` is good;
- `./alphasma3k -verifyroms asma3kdv` is good;
- `git diff --check` passes.

Do not execute original `asma3k`.

---

## F. Narrow dynamic validation on `asma3kdv`

Create a local-only debugger script under `../diagnostic/`, e.g.:

`as3kdv_keyboard_porta.cmd`

Use explicit `0x...` literals everywhere.

The run must stop before `KeyboardInitializeModulePhase2`.

At minimum prove the following on the real AlphaWord Phase-1 path after the core change:

1. Startup writes SCR with the established value used by this fixture (`0x1D` from the linked initialization path), and the EZ Port-A selection state used by `PADATA` is consistent with `WDTH8` being active.
2. At Phase-1 entry, PAPUEN is mapped and its first read returns the documented reset value `0xFF` unless earlier firmware writes prove otherwise.
3. The Phase-1 `PAPUEN &= 0x80` sequence therefore writes the value dictated by the actual prior read (normally expected `0x80` from reset `0xFF`). Record the observed value; do not force it if earlier firmware changed the register.
4. The former PAPUEN unmapped diagnostic at `0xFFFFF402` disappears for the PAPUEN byte lane.
5. The AlphaSmart access to reserved `0xFFFFF403` remains harmless and is **not** serviced by a newly invented EZ register.
6. PADIR/PADATA writes remain finite and Phase 1 still reaches its RTS and caller return `0x00420168` with `D0 = 0`.
7. The already implemented latch write at `0x00600000 = 0xFF` remains mapped; its old unmapped message must not reappear.
8. No Phase 2, keyboard interrupt handler, `InterruptInstallHandler`, or established exception handler fires.

Because prior debugger byte readback of mapped GPIO registers was ambiguous, use the narrowest reliable method to prove retained Port-A state. Prefer normal debugger program-space reads if they now behave consistently. If they remain ambiguous, use temporary diagnostic logging inside the relevant core handlers to demonstrate state and remove every temporary log before the final build/commit. Do not patch AlphaWord or a ROM fixture to manufacture a readback test.

### Optional micro-check for WDTH8 clearing

If existing debugger commands can safely write/read SCR without patching ROM or broadening execution, you may perform one local-only controlled micro-check that toggles WDTH8 and demonstrates internal Port-A selection follows both set and clear transitions, restoring the original SCR before continuing. Do this only if the syntax and side effects are already understood. Otherwise validate the clear path by source/manual reasoning and report that dynamic AlphaWord coverage exercises only the set state.

---

## G. Regression / publication boundary

After validation:

1. remove all temporary logging/instrumentation;
2. rebuild final source if temporary instrumentation affected compiled files;
3. `git diff --check`;
4. inspect `git status --short` and exact diff;
5. ensure no ROM, firmware, historical binary/source archive, CGROM, diagnostic fixture/script/log, generated executable, or build artifact is staged.

`docs/as3k/CODEX_RESULT.md` must report factually:

- exact core files changed;
- exact PAPUEN state/map/reset/save implementation;
- how PADATA fallback was changed and why original MC68328 behavior is preserved;
- exact `SCR.WDTH8` selection/save-state correction;
- explicit confirmation that no EZ PASEL was mapped at `0xFFFFF403`;
- build and ROM-audit results;
- dynamic PAPUEN read/write values observed in Phase 1;
- whether unmapped PAPUEN diagnostics disappeared;
- retained PADIR/PADATA evidence and any remaining debugger-space ambiguity;
- latch regression result;
- Phase-1 RTS/caller-return and `D0`;
- safety-break results;
- whether WDTH8 clear behavior was dynamically exercised or only statically validated;
- final `git diff --check`, status, commit SHA, and push result.

Update `docs/as3k/STATUS.md` to keep the public handoff factual if the implementation passes.

Commit only safe core/documentation changes and push only to `as3k-project/as3k-mame0289-dev`.

## Pass criteria

Pass means:

- PAPUEN exists for EZ with correct reset/read/write/save semantics;
- Port-A input fallback honors PAPUEN without changing original MC68328 externally visible behavior;
- `SCR.WDTH8` controls the internal EZ Port-A selection state correctly and that state is save-state safe;
- no fake PASEL register exists at `0xFFFFF403`;
- Phase 1 observes mapped PAPUEN behavior and still returns normally with `D0 = 0`;
- the external latch remains mapped and valid;
- no later keyboard/interrupt stage was entered or implemented;
- final build/audits pass and only safe changes are pushed.

## Stop condition

Stop after this **Port-A core-fidelity stage** is implemented, validated, documented, committed, and pushed.

**Do not add the keyboard matrix, PDSEL, PDKBEN/PKBDINT, key mappings, or execute Keyboard Phase 2 in this task.**
