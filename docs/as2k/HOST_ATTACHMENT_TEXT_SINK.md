# AS2000 host attachment and plain-text output — design note

Status: `PLANNING_ONLY_NOT_WORKER_AUTHORIZATION`

This document is a design/consolidation note only. It is **not** an automation task, is not authorization to start or retarget a worker, and must not be treated as an instruction to modify source code. Worker task/control files remain authoritative until the user explicitly authorizes a new task.

## Design objective

Define the smallest useful emulation of the AlphaSmart 2000 host-attachment and Send behavior without making MAME act as an operating-system keyboard.

Target user-visible flow:

```text
stock AS2000 ROM
    -> emulated PC attachment condition
    -> stock firmware enters "Attached to PC, emulating keyboard."
    -> user presses Send through the normal matrix/IRQ path
    -> stock ROM generates the same logical keyboard events it would send to a PC
    -> emulator decodes those events
    -> plain text is written to a fixed salida.txt
```

The central design principle is to preserve the original firmware decision-making. MAME should provide the missing external hardware condition and observe the resulting output; it should not bypass the ROM's attachment or Send logic.

## Non-goals for this stage

This design intentionally does **not** require:

- USB HID enumeration;
- a host operating-system keyboard driver;
- Linux `uinput`;
- Windows `SendInput`;
- macOS `CGEvent`;
- foreground-window/focus handling;
- making MAME itself appear to the host OS as a physical keyboard;
- MC68HC11 SCI implementation solely for the purpose of creating the text file;
- changes to the separate V3.14.x firmware project.

Those are separate future fidelity or hardware-modernization topics.

## Consolidated evidence already available

The active AS2000 MAME branch already models Send through the normal keyboard path:

- firmware keycode `$47`;
- matrix `COL.7 / 0x10`;
- normal `kb_irq` behavior;
- no direct firmware-call shortcut.

Stock v3.1.4 runtime evidence in the current no-host model shows a real Send press reaching `$9716 -> $D2DC`, not the wired Send routine `$8606`. Two passive PORTA-sense observations recorded 77 reads in each run with PA0/PA2 remaining low. The current driver has no external PC/ADB attachment source, so the absence of connected-host behavior is consistent with a missing hardware attachment model rather than a missing Send key.

The separate IRLESS validation has already shown that the derived firmware can route Send through `$9716 -> $8606` and Print through `$9804 -> $ABC9` without the removed IrDA handlers. This is useful control-flow evidence but is not a substitute for emulating stock host attachment.

## Firmware layers to keep distinct

Current reverse engineering supports treating the stock behavior as three separate layers.

### 1. Host presence / attachment detection

`$85A1-$85C5` is the leading attachment-probe candidate. PA0/PA2 are current sense/handshake candidates. The exact active polarity, temporal behavior and truth table are still open and must be derived from ROM/hardware evidence before implementation.

### 2. Host classification / connected state

RAM `$008A bit0` participates in the PC/Mac distinction. `$A3AC` uses that state to select the stock strings:

- `$DDF2`: `Attached to PC, emulating keyboard.`
- `$DDCD`: `Attached to Mac, emulating keyboard.`

For a faithful model, MAME must not write `$008A` directly. That state should be a firmware consequence of the emulated electrical condition.

### 3. Keyboard-event transport

`$8606` is the high-level Send/document-stream engine. It is not host detection. It translates the current document into host keyboard events and feeds the transport service around `$AA26`.

`$AA26/$AA52` is also used outside Send by normal connected-keyboard behavior, so it should be treated as a host-output transport boundary rather than as a Send-only function.

## Proposed emulator-side connection model

The minimal UI concept is a single configuration state:

```text
PC connection: Disconnected / Connected
```

This is a design proposal, not yet an implementation instruction.

### Disconnected

Preserve the current electrical state presented by MAME. The stock ROM remains in its normal editor/no-host behavior. Pressing Send is allowed to follow whatever disconnected path the stock firmware selects, including the observed IrDA route.

### Connected

Present only the external electrical condition that a real PC connection would create once that condition has been proven. The stock ROM should then detect the host and autonomously enter its PC keyboard-emulation state.

The following shortcuts are specifically outside the design:

- directly calling `$8606`;
- forcing the HC11 program counter;
- poking `$008A` or another host-state variable;
- changing the Send dispatcher merely to force the wired path.

A successful attachment model should be recognizable because the **unmodified stock ROM** reaches its own `Attached to PC, emulating keyboard.` state.

## Proposed plain-text sink

Once the stock ROM is genuinely in connected-PC mode, the emulator does not need to reproduce an operating-system keyboard device. It only needs to observe the outgoing logical keyboard-event stream at an appropriate boundary and convert it to text.

The preferred first observation point is around `$AA26`, or an equivalent device-level boundary that still allows the original ROM to perform its normal document traversal, key translation and Send sequencing.

The decoder will probably need state for:

- make/press events;
- break/release events;
- Shift and any other modifier used by stock Send;
- printable key mapping;
- Return/Enter;
- Tab if emitted;
- repeated identical characters.

Break/release events should update state but should not themselves append text. Adjacent repeated characters such as `ll` must remain two characters and must not be collapsed by an output-change mechanism.

## Output-file model

First-version output is intentionally fixed and simple:

```text
salida.txt
```

No save dialog and no user-selectable path are required in the initial design. A practical location is alongside the machine's MAME-managed persistent/NVRAM state if that can be done cleanly and portably.

The preferred transaction semantics are still to be decided explicitly before implementation. The simplest candidate is:

- when a new Send transaction begins, create/truncate `salida.txt`;
- append decoded text as the ROM emits it;
- flush/close when the Send transaction ends or when the emulation session closes cleanly.

This avoids ambiguity about whether multiple independent Send operations should concatenate into one file.

## Questions that must be closed before implementation instructions are written

1. What exact read/control-flow sequence in `$85A1-$85C5` identifies host attachment?
2. Which of PA0 and PA2 represent presence, ready/handshake or host classification, and with what polarity?
3. Is attachment a static level, a timed handshake, or a sequence of both?
4. What exact condition distinguishes PC from Macintosh attachment?
5. At what logical point can MAME observe the outgoing event without bypassing stock translation/transport semantics?
6. What keycode table/encoding reaches that boundary for the PC mode selected by the ROM?
7. How are Shift, break/release, Return, Tab, Backspace and repeated keys represented?
8. What event unambiguously marks the beginning and end of one Send transaction for file truncation/flush purposes?
9. What exact MAME-managed directory should contain `salida.txt` so behavior remains portable across platforms?

## Future validation concept

After the design questions above are closed and the user explicitly authorizes implementation, an eventual end-to-end validation can be structured around a simple known document such as:

```text
ABC 123
```

Expected behavior would be:

1. boot stock AS2000 ROM in the normal editor;
2. enter the known text;
3. change the proposed PC-connection state to Connected;
4. observe the stock firmware enter its PC keyboard-emulation state;
5. press Send through the ordinary emulated matrix/IRQ path;
6. obtain `salida.txt` containing exactly the expected text and agreed line-ending semantics;
7. return to Disconnected and confirm that stock no-host behavior remains intact.

This future validation outline is documentation only. It does not authorize source changes or worker execution.

## Relationship to SCI and USB modernization

The existing USB-modernization work in the separate firmware project addresses a different problem: replacing legacy physical PC/ADB transport on real hardware with an external USB-capable bridge. That design may eventually require HC11 SCI support in MAME for testing modified firmware.

The plain-text sink described here does not require that path. Its purpose is to make the stock AS2000 emulator useful and observable while keeping the implementation platform-neutral and small.

## Current planning state

`HOST_ATTACHMENT_TEXT_SINK_DESIGN_DRAFT_V2`

Closed enough to retain as design constraints:

- Send key/matrix/IRQ mapping is known;
- stock no-host Send routing is observed;
- `$8606` is Send/document-stream logic, not attachment detection;
- `$AA26` is a shared host-output transport boundary candidate;
- `$85A1-$85C5` is the leading attachment-probe range;
- PA0/PA2 are candidate host-sense inputs;
- the desired output is a fixed platform-neutral `salida.txt`, not a host keyboard device.

Still deliberately open before any worker instruction is issued:

- exact attachment truth table and timing;
- exact PC/Mac selection condition;
- exact event-decoding boundary and key encoding;
- Send transaction start/end semantics for the file;
- final persistent-file location in MAME.
