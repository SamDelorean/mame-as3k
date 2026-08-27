# Codex result — static correlation of `KeyboardInitializeModule` Phase 1

Date: 2026-08-26

Status: **complete; static stop condition met**. Keyboard code was not executed, the emulator was not rebuilt, and no MAME source/core, ROM definition, fixture, binary, generated file, or proprietary artifact was changed.

## Repository gate

- Required pull: already up to date; branch `as3k-mame0289-dev`; initial tracked status clean.
- Commit `72b9935edfafe270e91becdd30fb7459c6f07a03` is an ancestor of `HEAD`.
- `docs/as3k/STATUS.md` exists; initial `git diff --check` passed.

## Historical artifacts used

All paths are within `/Users/sperezc/Downloads/AlphaSmart.iso` (temporary extraction was under `/tmp/as3k-keyboard.xJ1bRo`, outside Git):

- `AS3000/Software/ModuleSources/{KeyboardModule.c,KeyboardModule.h,M68328EZ.h,InterruptModule.h,SystemModule.h,ProjectIncludes.h}`
- `AS3000/Software/BuiltApplet/{KeyboardModule.o.lst,KeyboardModule.h,AlphaWord.o.lst,InterruptModule.o.lst,AWordApplet02.bin}`
- `AS3000/Software/BuiltAlphaWordRamUSA/AWordRAM01.out`

The USA Flash applet was selected because its linked entry matches the dynamically established address. The `.out` supplies linked symbols/data addresses, the object listing supplies source-correlated code and relocation sites, and the Flash file supplies final relocated bytes.

## Exact linked Phase 1

`KeyboardInitializeModule` occupies `0x0042e2a8`–`0x0042e2ef`, inclusive: 72 bytes. Its `RTS` is at `0x0042e2ee`; `0x0042e2f0` is both its end boundary and Phase 2's entry.

```text
707f8038f40311c0f403707f8038f40011c0f4001038f4020200008011c0f402
13fc00ff00600000707f8038f40111c0f40111fc00fff41b11fc0000f41811fc
0000f41a70004e75
```

These bytes match object offsets `0x000000`–`0x000047` after relocation. The applet load bias is `0x00420030`, so file offset `0x0000e278` is linked address `0x0042e2a8`. The caller's `JBSR` is `0x00420162`; normal return is `0x00420168`, the caller's `RTS`.

Phase 1 is a leaf: **no direct calls, no RAM-global reads/writes, no SR/interrupt-mask change, and no loop, wait, assert, error path, debounce/repeat/timer work, or key-state dependency**. It returns `D0 = 0` if its ordinary accesses complete.

All accesses are bytes:

| Linked instruction(s) | Address | Operation/result |
| --- | --- | --- |
| `0x0042e2aa`, `0x0042e2ae` | `0xfffff403` PASEL | read, OR `0x7f`, write: bits 0–6 set, bit 7 preserved |
| `0x0042e2b4`, `0x0042e2b8` | `0xfffff400` PADIR | read, OR `0x7f`, write: bits 0–6 outputs, bit 7 preserved |
| `0x0042e2bc`, `0x0042e2c4` | `0xfffff402` PAPUEN | read, AND `0x80`, write: pull-ups 0–6 off, bit 7 preserved |
| `0x0042e2c8` | `0x00600000` | write `0xff` to external column latch |
| `0x0042e2d0`, `0x0042e2d4` | `0xfffff401` PADATA | read, OR `0x7f`, write: PA0–PA6 high, PA7 preserved |
| `0x0042e2d8` | `0xfffff41b` PDSEL | write `0xff`, all Port D pins GPIO |
| `0x0042e2de` | `0xfffff418` PDDIR | write `0x00`, all rows inputs |
| `0x0042e2e4` | `0xfffff41a` PDPUEN | write `0x00`, internal row pull-ups off |

No other hardware address is accessed. Phase 1 does not read PDDATA, touch `PKBDINT`, configure interrupt polarity/edge/enable, or call an interrupt API.

## Electrical/software contract (bounded helper inspection)

The build has 16 logical column codes, but `0x8` is the separate power-switch column and is not scanned: the matrix is **15 scanned columns × 8 rows**.

- Byte-write-only latch `0x00600000`: D0–D6 drive X1–X7/logical `0x9`–`0xf`; D7 drives X8/logical `0x0`.
- PA0–PA6 drive X9–X15/logical `0x1`–`0x7`; PA7 is unrelated and preserved. PD0–PD7 are the rows.
- Phase 1 drives all attached columns high. Since it writes PDPUEN `0x00`, external board pull-ups must make no-key PDDATA read `0xff`.
- Normal scan is one-column-at-a-time active low. `Keyboard_SetColumnLow` (`0x0042e9fe`) uses two 16-byte masks, preserving PA7. Other helpers can drive all columns high or low, so multiple active columns are possible outside normal scan.
- `Keyboard_GetNewKeyStates` (`0x0042e6f0`) scans `0x0`–`0x7` and `0x9`–`0xf`, reads PDDATA (`0xfffff419`) once per column, and complements it. Thus row low means pressed and the stored key bitmap is active high. `KeyboardScanKeypad` is `0x0042e69e`.

The later `KeyboardEnableKeyboardInterrupt` (`0x0042ea48`) clears its flag, calls `InterruptEnableSource(0x40)`, drives every column low (latch `0x00`, PA0–PA6 low), and writes `PKBDINT` (`0xfffff41e`) `0xff`. `INTERRUPT_KEYBOARD_4 = 0x00000040`. Handler `Keyboard_InterruptHandler` (`0x0042ea74`) disables source `0x40`, writes `PKBDINT = 0x00`, and sets the flag. These helpers were inspected only to establish the next-stage hardware contract; neither initialization phase invokes scan/debounce/repeat/timer helpers.

## Phase 1 versus Phase 2

`KeyboardInitializeModulePhase2` is linked at `0x0042e2f0`; its `RTS` is `0x0042e3c2` (212 bytes, end `0x0042e3c4`). `_AWord_Initialize` calls it at `0x004200e2`, returning to `0x004200e8`, after `SystemCheckBootSignature` and any required repetition of primary initialization.

Phase 1 performs only electrical GPIO/latch setup and can complete independently. Phase 2 performs boot-dependent software setup and installs—but does not enable—the handler. Its only calls are `SystemGetBootMethod` (`0x0042fd16`) from `0x0042e2f2` and `InterruptInstallHandler` (`0x004318a4`) from `0x0042e356`, with source `0x00000040` and handler `0x0042ea74`.

Phase-2 linked RAM globals (access widths shown) are:

- `0x00038f78`, `0x00038f7a`, `0x00038f7c`: queue count/in/out, words `0`;
- `0x00038f9a`–`0x00038fa9` and `0x00038faa`–`0x00038fb9`: old/new column tables, 16 bytes each, cleared;
- `0x00038fba`: auto-repeat delay, word copied from `0x00038fc6` (`500`);
- `0x00038fbc`: last scan code, byte `0xff`; `0x00038fbe`: last event, word `0x00ff`;
- `0x00038fc0`, `0x00038fc2`: modifier keys/key-ups, words `0`;
- `0x00038fc4`: interrupt flag byte `0`; `0x00038fc5`: up-key-mode byte `0`;
- `0x00038fc6`, `0x00038fc8`, `0x00038fca`: repeat-delay words `500`, `20`, `40`;
- cold boot: sticky/temp-sticky bytes `0x00038fcc/0x00038fce = 0`, auto/temp-auto bytes `0x00038fcd/0x00038fcf = 1`;
- warm boot: copy `0x00038fce` to `0x00038fcc` and `0x00038fcf` to `0x00038fcd`.

Phase 2 has one bounded 16-iteration RAM-clear loop, no key/hardware wait, and no Port D interrupt configuration.

## Current MAME 0.289 support classification

1. **Already modeled correctly:** PADIR, PADATA, PASEL, PDDIR, PDDATA, and PDPUEN have core handlers; Port A output and Port D input callbacks plus Port D interrupt-line logic exist.
2. **Core modeled, driver unconnected:** PA0–PA6 outputs and PD0–PD7 inputs expose callbacks, but `alphasma3k.cpp` connects neither. `INPUT_PORTS` is empty, so later scans cannot obtain externally pulled-high/active-low matrix rows.
3. **Absent from driver:** no mapping/device exists for byte-write-only latch `0x00600000`; this is the first definite Phase-1 driver dependency. A local latch plus matrix/input implementation is ultimately required.
4. **Potential core omissions:** the core map lacks PAPUEN `0xfffff402`, PDSEL `0xfffff41b`, and `PKBDINT` `0xfffff41e`. Phase 1 touches the first two; later interrupt helpers touch the third. This is exact source/map evidence, not an inference from missing callbacks. PAPUEN/PDSEL may not prevent return, but cannot update modeled state. Exact `PKBDINT` semantics remain unproven.

## Next narrow debugger test (designed, not run)

Run only `asma3kdv`. Break at `0x0042e2a8`; stop/quit at caller return `0x00420168`, before later work.

- Byte read/write watchpoints: `0xfffff403`, `0xfffff400`, `0xfffff402`, `0xfffff401`, `0xfffff41b`, `0xfffff418`, `0xfffff41a`.
- Byte write watchpoint: exact `0x00600000` (use `0x00600000`–`0x0060ffff` only if debugger syntax requires the decoded window).
- Break `0x0042e2ee` for pre-RTS state and `0x00420168` for mandatory success/quit.
- Fail/safety breaks: `0x0042ea74` (handler), `0x004318a4` (`InterruptInstallHandler`, never called by Phase 1), and the established project assert/error helper if available.
- No Phase-1 global watchpoint is required: expected accesses are zero. Optional guard: watch `0x00038f78`–`0x00038fcf` and treat any access as mismatch.

Expected order/results: PASEL `| 0x7f` (current EZ reset yields `0xff`); PADIR `| 0x7f` (expected `0x7f`); PAPUEN `& 0x80`; latch `0xff`; PADATA `| 0x7f` (expected `0x7f`); PDSEL `0xff`; PDDIR `0x00`; PDPUEN `0x00`; `D0 = 0`; return. Report preserved bit 7 rather than assuming it except where current reset state is deterministic.

Because `0x00600000` is unmapped, record the first write and whether MAME permits it or raises/stops. Do not add a latch. If execution stops there, end at that first demonstrated dependency; otherwise continue only to `0x00420168`. Do not execute Phase 2.

## Public handoff implications

- Phase 1 is a finite 72-byte leaf configuring PA0–PA6 high, their pull-ups off, latch `0xff`, and all Port D pins as inputs with internal pull-ups off. It initializes no globals and installs/enables no interrupt.
- The interface is 15 active-low scanned columns × 8 active-low rows; externally pulled-high PDDATA `0xff` is idle.
- The first definite missing component is the `0x00600000` latch. Matrix callbacks/input ports are unconnected; PAPUEN/PDSEL are core-map omissions, while PKBDINT is a later potential gap.
- Unproven: whether the unmapped write permits return, actual transaction values/order, physical idle rows, matrix behavior, and interrupt semantics.
- Next step: run only Phase 1 with the exact watches/safety breaks above and quit at `0x00420168`, stopping earlier on the first hard latch dependency.

## Publication checks

- Final `git diff --check`: passed.
- Pre-commit final `git status --short`: ` M docs/as3k/CODEX_RESULT.md` and nothing else.
- Only `docs/as3k/CODEX_RESULT.md` is selected for commit and push to `as3k-project/as3k-mame0289-dev`.
- No MAME source/core, ROM, fixture, proprietary artifact, diagnostic binary, or generated file changed.
