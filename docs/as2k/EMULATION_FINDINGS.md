# AlphaSmart 2000 Emulation Findings

This document consolidates reverse-engineering findings relevant to the
AlphaSmart 2000 MAME driver.

It is a hardware/firmware interaction reference for emulator development.
It does not redefine the emulator roadmap or firmware project objectives.

It complements:

- `docs/as2k/SEND_KEY_FINDING.md`
- `docs/as2k/GATE1A_RUNTIME_HARNESS.md`

## 1. Firmware reference

Current reverse-engineering findings in this document refer primarily to:

- AlphaSmart 2000 firmware v3.1.4

Historical firmware comparison has also been useful for separating original
wired-host functionality from functionality added later for IrDA.

The IR-detached experimental firmware is derived from v3.1.4 by a differential
patch. Proprietary ROM images are not part of the emulator repository.

## 2. Send key

Current Send mapping on branch `as2k-mame0289-dev`:

- AS2000 keycode: `$47`
- keyboard matrix position: `COL.7 / 0x10`
- current MAME host key: `F12`
- MAME label: `Send`
- keyboard transition uses the normal `kb_irq` mechanism

The older `master` branch may still show `COL.7 / 0x10` as `IPT_UNUSED`.

Send must be generated through the normal keyboard matrix. It should not be
implemented as a direct firmware call.

## 3. Historical separation of wired Send and IrDA Send

Historical firmware comparison establishes that Send existed before the IrDA
extension.

In the older firmware image, keycode `$47` reaches the wired keyboard-host Send
implementation at approximately `$8611`.

In v3.1.4 the corresponding retained wired implementation is at:

- `$8606`

The normal main-loop Send path includes a call from approximately:

- `$80F2` -> `$8606`

The later IrDA extension is separately reachable through the editor dispatcher:

- `$9777` -> `$9716` -> `$D2DC`

This distinction is important for emulation because `$D2DC` is not the normal
wired Send implementation.

## 4. Wired host communication regions

Firmware analysis identifies these retained wired communication areas:

- `$8606–$8709` — Send / wired host path
- `$887E–$8F04` — Macintosh / ADB transport
- `$AA26–$AB09` — PC / two-wire transport

These regions are protected retained functionality and are distinct from the
IrDA backend.

Emulator changes related to IrDA must not assume that host communication as a
whole has been removed.

## 5. IrDA-related firmware regions

Current firmware analysis identifies the following candidate IrDA-only ROM
regions:

- `$D098–$D487`
- `$D499–$D517`
- `$E103–$FFBF`

Important retained exception:

- `$D488–$D498` — shared non-IrDA keyboard/matrix helper

Current candidate reclaim size after separating this shared helper is:

- approximately 9,004 bytes

For emulator purposes, these ranges are useful primarily as execution
classification ranges.

## 6. IrDA Send backend

Useful firmware landmarks in the IrDA Send path include:

- `$9716` — editor-side dispatch point
- `$D2DC` — IrDA Send backend
- `$D3F3` — IrDA-related control/detection path
- `$D099` — IrDA initialization/timer path
- `$E104` — entry into the high IrDA stack

Firmware analysis indicates that `$D3F3` manipulates control value `0x14` and
tests PA7 before the subsequent IrDA initialization/stack path.

These addresses should not be confused with the retained wired Send routine at
`$8606`.

## 7. Print paths

Firmware analysis identifies:

- `$ABC9` — retained Print path
- `$D437` — IrDA-related Print path

Relevant patched/caller landmarks include:

- `$962D`
- `$9804`

A trace of the IR-detached firmware should demonstrate execution of the retained
Print path without entering `$D437`.

## 8. Keyboard IRQ behavior

Current MAME implementation and firmware analysis indicate:

- keyboard transitions assert `MC68HC11_IRQ_LINE`
- writing the low keyboard matrix selector at `$9000` clears or acknowledges
  the keyboard IRQ in the current emulator implementation
- `$2000` participates in keyboard read / high matrix selector behavior

Send must reach the firmware through this normal matrix/IRQ mechanism.

Directly forcing the PC to `$8606` is not equivalent to pressing Send and does
not validate keyboard matrix or IRQ behavior.

## 9. Keyboard MMIO

Two particularly important addresses in the current hardware model are:

- `$2000`
- `$9000`

Current observations indicate:

- `$2000` participates in keyboard read and high matrix-selector behavior
- `$9000` participates in the low matrix selector
- writing the low selector at `$9000` acknowledges/clears the keyboard IRQ in
  the current emulator model

These semantics should be preserved while instrumenting firmware execution.

## 10. PA6 memory-view behavior

PA6 controls the visibility of I/O versus RAM in the lower address space:

- `$0000–$7FFF`

Therefore PA6 is part of the fundamental AS2000 memory model and must not be
treated as an IrDA-specific signal.

Instrumentation and debugging must take the active memory view into account
when interpreting accesses in this address range.

## 11. RAM banking

PA4–PA5 participate in RAM-bank selection.

The original AS2000 memory model uses these bits as part of the banked RAM
mechanism.

Removing IrDA does not change this behavior.

## 12. DictROM banking

PA4–PA5 also participate in DictROM-related bank selection.

Current emulator/firmware analysis additionally indicates that DictROM
selection interacts with bit 7 of the LCD-control state.

Consequently PA4–PA5 cannot be interpreted solely as RAM-selection signals.

Any future refinement of DictROM emulation should preserve the observed
interaction between:

- PA4–PA5
- LCD control bit 7
- DictROM bank selection

## 13. HC11 timer and capture/compare ownership

IrDA is not the only subsystem using HC11 timer resources.

Retained wired-host communication, particularly Macintosh/ADB-related code,
also appears to use timer/capture-compare facilities.

Observed MMIO addresses associated with retained behavior include:

- `$000E`
- `$0014`
- `$001A`
- `$0021`
- `$0023`

Therefore accesses to these registers cannot be classified as IrDA solely
because they involve HC11 timer/capture-compare hardware.

The emulator must not simulate IR removal by globally disabling HC11 timers,
capture, or output-compare facilities.

## 14. IrDA interrupt vectors

Firmware analysis associates five interrupt vectors with the IrDA subsystem:

- PAI
- OC4
- OC3
- OC2
- OC1

Original vector destinations identified during reverse engineering include:

- `FFDA -> D0E0`
- `FFE2 -> D194`
- `FFE4 -> D107`
- `FFE6 -> D2AB`
- `FFE8 -> D26A`

The current IR-detach firmware redirects these five vectors to the existing
generic RTI at:

- `$895B`

This is a firmware modification and must not be emulated by disabling the HC11
timer subsystem globally.

Other retained firmware still uses timer/capture-compare resources.

## 15. Timer interrupt masks

The original IrDA initialization path at approximately `$D099` manipulates HC11
timer interrupt masks including:

- TMSK1
- TMSK2

Static analysis of the IR-detached firmware found no retained direct accesses
attributable to the removed IrDA path to these masks.

This does not mean TMSK1/TMSK2 or the timer hardware are globally unused.

The distinction is between removal of the IrDA users of these resources and
removal of the resources themselves.

## 16. PORTA and PORTD ownership

Current reverse-engineering findings indicate retained wired communication
uses:

- PORTD bits 0–1
- PORTA bits 0 and 2

These pins must not be classified as IrDA-only based on serial activity,
timing activity, or proximity to IrDA code.

They remain part of the retained wired-host hardware model.

## 17. PA7

PA7 is strongly associated with the wireless detection/control path in current
firmware analysis.

In particular, the IrDA path around `$D3F3` tests PA7.

However, the physical PCB/net role of PA7 has not yet been established well
enough to declare:

- that PA7 is exclusively IrDA hardware
- that it becomes free GPIO after IrDA removal
- that its external electrical behavior can be omitted from the emulator

PA7 should therefore remain conservatively classified as unresolved /
IrDA-associated until hardware evidence closes its ownership.

## 18. Runtime tracing landmarks

Useful retained-code landmarks:

- `$8606` — wired Send implementation
- `$ABC9` — retained Print implementation
- `$887E–$8F04` — Macintosh/ADB transport
- `$AA26–$AB09` — PC/two-wire transport

Useful Send dispatch landmark:

- `$9716`

Useful Print caller landmarks:

- `$962D`
- `$9804`

Useful IrDA failure landmarks:

- `$D099`
- `$D2DC`
- `$D437`

Additional IrDA-path landmarks:

- `$D3F3`
- `$E104`

Candidate IrDA exclusion ranges:

- `$D098–$D487`
- `$D499–$D517`
- `$E103–$FFBF`

Shared retained exception:

- `$D488–$D498`

A generic exclusion check over `$D098–$D517` is therefore incorrect unless it
explicitly permits `$D488–$D498`.

## 19. Send runtime evidence

A useful Send trace for the IR-detached firmware should be generated by an
actual Send/F12 key transition through the emulated keyboard matrix.

Expected positive execution evidence:

- `$9716`
- `$8606`

Expected negative evidence:

- no `$D2DC`
- no `$D099`
- no execution in the IrDA exclusion ranges

The shared `$D488–$D498` helper remains permitted.

Reaching `$8606` by directly manipulating the CPU state is not valid evidence
for keyboard/IRQ correctness.

## 20. Print runtime evidence

A useful Print trace should contain positive evidence that the Print operation
was actually exercised.

Expected positive evidence:

- `$ABC9`
- at least one applicable caller/patch landmark:
  - `$962D`
  - `$9804`

Expected negative evidence:

- no `$D437`
- no execution in the removed IrDA regions

This distinction prevents a trace with no Print activity from being
misclassified as successful merely because it contains no IrDA execution.

## 21. Positive-evidence rule

Absence of IrDA execution alone is insufficient to demonstrate correct
emulation.

Runtime validation should distinguish:

1. expected retained code that was actually executed
2. forbidden IrDA code that was not executed

This is particularly important for automated trace classification.

A trace in which neither the retained path nor the IrDA path executes must not
be interpreted as evidence that the operation works.

## 22. Firmware control-flow versus peripheral emulation

Firmware control-flow validation and complete peripheral emulation are separate
questions.

For example, if a real Send key event:

- enters the normal keyboard/IRQ path
- reaches `$9716`
- reaches wired Send `$8606`
- does not enter `$D2DC` or `$D099`

then the firmware-side wired/IrDA routing has been demonstrated even if the
emulator does not yet reproduce every physical PC/Mac signaling detail.

Complete wired host transport emulation remains a separate hardware-model
requirement.

This distinction is useful when diagnosing whether a failure originates in:

- firmware dispatch
- keyboard emulation
- HC11 peripheral emulation
- PC/two-wire transport
- Macintosh/ADB transport
- host-side MAME integration

## 23. Runtime instrumentation constraints

Tracing must not alter the hardware behavior being tested.

In particular, instrumentation should preserve normal behavior of:

- keyboard matrix scanning
- keyboard IRQ generation
- keyboard IRQ acknowledgement
- `$2000`
- `$9000`
- PA4–PA6
- PORTA
- PORTD
- RAM banking
- DictROM banking
- HC11 timers
- input capture
- output compare

IrDA absence must be established by firmware execution evidence rather than by
artificially disabling shared HC11 resources.

## 24. MAME trace capture

MAME 0.289 debugger tracing can be used together with breakpoint-generated
logging.

The debugger trace command supports trace flags including:

- `noloop`
- `logerror`

This makes it possible to keep instruction tracing and selected breakpoint
markers in the same trace evidence.

For AS2000 work this is useful for correlating:

- Send key execution
- Print execution
- retained wired-path landmarks
- accidental IrDA execution
- IRQ/peripheral activity

Trace tooling should remain diagnostic and should not modify emulated hardware
state merely to obtain a passing result.

## 25. IR-detach firmware reference

The current experimental IR-detach patch changes exactly:

- 34 bytes relative to the AS2000 v3.1.4 baseline

Expected derived-ROM SHA-1:

- `f40b4008604536b9bfa8d343456ffaa523c789dc`

The patch performs firmware-side control-flow detachment while retaining wired
communication.

Relevant effects include:

- Send routing retains `$8606`
- Print routing retains `$ABC9`
- five IrDA-associated vectors are redirected to `$895B`

This hash is useful for ensuring that emulator traces correspond to the exact
firmware build being analyzed.

The derived proprietary ROM itself should not be distributed.

## 26. Current emulator branch reference

AS2000 development findings in this document correspond to:

- repository: `SamDelorean/mame-as3k`
- branch: `as2k-mame0289-dev`

The Send mapping is already present on this branch.

Do not re-add or bypass the Send mapping unless source inspection shows that
the branch has changed.

## 27. Emulator implementation cautions

When implementing or refining the AS2000 driver:

- do not bypass the keyboard matrix for Send
- do not synthesize a direct call to `$8606`
- do not disable timers globally to represent absent IrDA
- do not classify all capture/compare activity as wireless
- do not classify PORTA or PORTD globally as wireless
- do not treat PA4–PA5 as single-purpose RAM-bank outputs
- do not treat PA6 as wireless-related
- do not declare PA7 free until hardware ownership is confirmed
- do not flag `$D488–$D498` as IrDA execution
- do not infer successful Send/Print operation from absence of IrDA execution
- do not alter shared peripheral behavior merely to make firmware traces pass

## 28. Useful diagnostic classification

When investigating AS2000 execution, failures can be separated into several
classes.

### Keyboard-path failure

Examples:

- F12 does not generate `COL.7 / 0x10`
- key transition does not assert the expected IRQ
- firmware never observes keycode `$47`

### Firmware-routing failure

Examples:

- Send does not reach `$8606`
- Send reaches `$D2DC`
- Print reaches `$D437`

### Shared-peripheral emulation failure

Examples:

- timer/capture-compare behavior prevents retained ADB execution
- incorrect PORTA/PORTD behavior affects wired transport
- keyboard IRQ acknowledgement is incorrect

### Banking/MMIO failure

Examples:

- incorrect PA6 memory view
- incorrect PA4–PA5 RAM bank
- incorrect DictROM bank selection
- incorrect interaction with LCD control bit 7

### Host-transport emulation failure

Firmware reaches the correct retained wired routine, but the emulated PC or
Mac transport does not yet produce the expected external behavior.

Keeping these categories separate helps avoid incorrectly modifying firmware
to compensate for an emulator hardware-model defect.

## 29. Related documentation

Additional AS2000 emulator documentation:

- `docs/as2k/SEND_KEY_FINDING.md`
- `docs/as2k/GATE1A_RUNTIME_HARNESS.md`

Firmware-side documentation and trace tools in the AS2K firmware repository
provide additional control-flow evidence, but emulator findings should remain
focused on observable hardware/firmware interactions.

Future findings should be added here when they affect:

- keyboard matrix behavior
- IRQ generation or acknowledgement
- MMIO
- RAM or ROM banking
- HC11 timer/capture-compare behavior
- PORTA / PORTD behavior
- host communication timing
- LCD/banking interactions
- hardware ownership of PA7
- observable firmware/hardware execution.

## Three-line LCD controller-boundary validation

Runtime validation on `as2k-mame0289-dev` has demonstrated normal firmware
editing across the boundary between the two AS2000 LCD controllers.

Using ordinary keyboard input with fresh private NVRAM, the sequence:

- `ab`
- Return
- `cd`
- Return
- `ef`

produced the following exact raw four-row DDRAM state:

- controller 1, row 1: `ab<B5>`
- controller 1, row 2: `cd<B5>`
- controller 2, row 1: `ef`
- controller 2, row 2: blank

All rows were checked as exact 40-cell, space-padded DDRAM contents.

The same four-row state was reproduced after:

- F2/F1 file switching
- a separate MAME process using the saved NVRAM
- another F2/F1 cycle after restart

The runtime checker required:

- real keyboard transitions
- ordered LCD writes between checkpoints
- both LCD controllers enabled
- exact row/controller placement
- ordered checkpoint completion

Negative fixtures reject:

- missing controller-two content
- content written to the wrong controller
- incorrect row placement
- stale recall
- missing keyboard/LCD evidence

This provides firmware-backed evidence that the current AS2000 model can write,
retain and recover text across the first-to-second LCD-controller boundary.

It does not by itself establish:

- physical LCD equivalence
- the physical glyph represented by raw `B5`
- complete charset coverage
- wrapping behavior
- scrolling behavior
- exhaustive editing correctness
- hardware timing equivalence

## Stock v3.1.4 normal matrix Send observation (2026-09-16)

The reproducible `scripts/as2k_test_send_probe.py --runtime` probe selects the
Send field directly by `:COL.7`, mask `0x10`, verifies its name and keyboard
codes, and presses/releases it through MAME's ordinary input field API.
With fresh NVRAM and the stock v3.1.4 SHA1 recorded above, it waits for 60 idle
frames at `$87D7`, queues `a`, waits for idle again, then holds Send for five
frames. Existing instruction instrumentation observed `$9716` then `$D2DC`
after the press and before release. The probe completed normally; `$8606`
was not observed. Two fresh runs reproduced this sequence.

This is positive normal-input dispatch evidence for the stock editor's IrDA
extension, consistent with the historical path distinction above. The existing
instrumentation calls `$D2DC` `FAIL_IR_SEND_D2DC`; that label applies to the
IR-detached contract, not to this stock-firmware expectation. No firmware or
emulator change is justified by that marker alone. This probe does not establish
LCD typing contents, electrical IRQ timing, wired transport, Print, or the
IR-detached firmware gate. The KS0066 F05 NEEDS REDUMP limitation remains.

## Wired-host sense boundary in the current model (2026-09-16)

The normal Send probe now passively counts CPU PORTA reads at `$0000` from
Send press through five held frames and 120 post-release frames. Two fresh
stock-v3.1.4 runs each observed 77 reads; the OR of PA0/PA2 (`data & 0x05`)
was zero. The existing ordered `$9716 -> $D2DC` path still completed, with
no `$8606`. The tap returns no value and does not inject signal levels.
The classifier requires positive, ordered read evidence and rejects nonzero
sense bits; 77 is an observation, not a fixed timing requirement.

Source explains this limitation: `alphasmart_state::port_a_r()` feeds back
`m_port_a` with only the battery bit replaced. HC11 `port_w`/`ddr_w` callbacks
provide output bits masked by the direction register; D0 resets PORTA direction
to `0x70`. There is no external PC/ADB sense source or attachment input in the
driver. This is an emulator host-interface modeling gap, not evidence of a
broken Send key, firmware regression or SCI failure. Raising a sense bit alone
has not been established as sufficient to reach wired Send. Physical polarity,
attachment sequencing and host handshakes still need evidence before an
implementation; no CPU core or driver behavior was changed.

## Host attachment: PC ready and disconnect validation (2026-09-17)

Stock AS2000 v3.1.4 ROM, SHA-1 `e0b777dc68c671c31ba808e214fb9d2573b9a853`.

- Directed attachment test: presenting PA2=1 at `port_a_r()` makes the stock ROM execute `$8067 -> $806B -> $85A1 -> $85C2 (V=1) -> $8070 -> $80C8 -> $A3AC` without RAM/ROM patching or forced PC state.
- `PC_KEYBOARD_READY` is a control-flow state, not a separate RAM flag. After `$80C8 -> $A3AC` returns, the first hit at `$80D4` proves entry into the PC keyboard service loop; repeated `$80E5` hits with an empty queue (`$008E == $008F`) are the stricter ready signature.
- In that state, normal Send keycode `$47` is consumed by `$938C` and should follow `$80EE -> $80F2 -> $8606`.
- Directed disconnect test at normal throttled timing: PA2=1 at t=1.000 s, held for 3.000 s, PA2=0 at t=4.000 s. The stock ROM reached `$89B6` at t=6.794 s and editor/idle `$87D7` at t=6.797 s.
- Therefore removing PA2 is sufficient for the stock ROM to leave PC mode and return to the editor autonomously. Observed disconnect latency was about 2.8 s.
- Emulator contract: `PC Connected=ON` presents the attachment condition; `OFF` removes it. Firmware remains responsible for PC classification, ready state, disconnect debounce/state cleanup, and editor return.

Next directed runtime gate: hold PA2=1, observe `$80C8 -> $A3AC -> $80D4` (preferably repeated `$80E5` with `$8E==$8F`), then inject Send only through the normal keyboard matrix/IRQ and require `$80EE -> $80F2 -> $8606`.

## Directed PC Send validation (2026-09-17)

A stock-ROM runtime test closed the next gate. PA2 alone reaches `$80D4`, but with PA0 low the PC loop takes `$80DA -> $814D` and does not reach the local keyboard queue consumer `$80E5`. Holding the host data-sense line PA0 high together with PA2 (`PORTA` logical idle `PA0=1, PA2=1`) preserves the PC service path through `$80E5`.

With PA0+PA2 high, the stock ROM produced one `$80C8` PC entry, then repeatedly cycled through `$80D4` and `$80E5`. Send was then pressed and released through the existing MAME `COL.7`/`Send` input field, which uses the normal keyboard IRQ path. The observed causal path was `$80EE -> $80F2 -> $8606`; each marker occurred twice during the press/release test. No ROM/RAM write, forced PC, direct `$8606` call, or Send dispatcher patch was used.

This validates normal wired Send dispatch from the stock firmware once PC keyboard mode is established. It also refines the emulator host-line contract: PC attachment/service requires PA2 asserted and PA0 held at its idle-high level in the absence of host-to-AlphaSmart traffic. The next gate is the `$8606` wired document stream into `$AA26`, followed by decoding the original keyboard transport into `salida.txt`.
