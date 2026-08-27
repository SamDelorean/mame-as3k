# Codex result — dynamic validation of `KeyboardInitializeModule` Phase 1

Date: 2026-08-26

Status: **complete; caller return reached**. The full finite Phase-1 sequence executed with `D0 = 0`. Unmapped accesses logged and execution continued; no hard hardware dependency, CPU exception/error handler, keyboard interrupt path, or Phase-2 entry occurred.

## Repository gate

- `git pull --ff-only as3k-project as3k-mame0289-dev`: already up to date.
- Branch `as3k-mame0289-dev`; initial tracked status clean.
- Commit `eb42b589a18644eeb02e5e1f131e6962f12768c4` is an ancestor of `HEAD`.
- `docs/as3k/STATUS.md` contains the Phase-1 contract; initial `git diff --check` passed.
- Existing `./alphasma3k` was executable and was not rebuilt.

No MAME source/core, driver list, ROM definition, fixture, generated binary, or proprietary artifact changed.

## Script and execution

Local-only script: `../diagnostic/as3kdv_keyboard_phase1.cmd`. It uses explicit `0x...` literals, exact one-byte watchpoints, entry/RTS/return breaks, and safety breaks for `InterruptInstallHandler`, `Keyboard_InterruptHandler`, Phase 2, and the established bus/address/illegal/divide-by-zero handlers. Watch actions record address, data, direction, and PC. The RTS action uses proven `b@0x...` expressions. No debugger syntax correction was required.

```sh
./alphasma3k asma3kdv \
  -debug \
  -debugscript ../diagnostic/as3kdv_keyboard_phase1.cmd \
  -log \
  -seconds_to_run 8
```

Preserved local-only outputs, neither staged nor tracked:

- `../diagnostic/as3kdv_keyboard_phase1_console.log`
- `../diagnostic/as3kdv_keyboard_phase1.log`

## Observed transaction sequence

Entry reported `PC=0x0042E2AA`, `D0=0x00000000`, `A7=0x0003FFD4`, `SR=0x2010`; the PC is the established debugger +2 presentation for entry `0x0042E2A8`.

| # | Location | Direction/data | Watchpoint PC | Semantic result |
| ---: | --- | --- | ---: | --- |
| 1 | PASEL `0xFFFFF403` | read `0x00` | `0x0042E2AE` | old `0x00` |
| 2 | PASEL `0xFFFFF403` | write `0x7F` | `0x0042E2B2` | `0x00 | 0x7F = 0x7F` |
| 3 | PADIR `0xFFFFF400` | read `0x00` | `0x0042E2B8` | old `0x00` |
| 4 | PADIR `0xFFFFF400` | write `0x7F` | `0x0042E2BC` | `0x00 | 0x7F = 0x7F` |
| 5 | PAPUEN `0xFFFFF402` | read `0x00` | `0x0042E2C0` | old `0x00` |
| 6 | PAPUEN `0xFFFFF402` | write `0x00` | `0x0042E2C8` | `0x00 & 0x80 = 0x00` |
| 7 | latch `0x00600000` | write `0xFF` | `0x0042E2D0` | exact expected byte |
| 8 | PADATA `0xFFFFF401` | read `0x00` | `0x0042E2D6` | old `0x00` |
| 9 | PADATA `0xFFFFF401` | write `0x7F` | `0x0042E2DA` | `0x00 | 0x7F = 0x7F` |
| 10 | PDSEL `0xFFFFF41B` | write `0xFF` | `0x0042E2E0` | exact expected byte |
| 11 | PDDIR `0xFFFFF418` | write `0x00` | `0x0042E2E6` | exact expected byte |
| 12 | PDPUEN `0xFFFFF41A` | write `0x00` | `0x0042E2EC` | exact expected byte |

No other Phase-1 hardware transaction was observed.

## Unmapped behavior

The exact latch write was watched and logged as unmapped:

```text
[:maincpu] ':maincpu' (42E2D0): unmapped program memory write to 00600000 = FFFF & FF00
```

It caused no exception, handler, stop, or hard dependency; execution continued normally.

PAPUEN read/write logged as aligned unmapped accesses at `0xFFFFF402` with masks `0xFF00`. The odd-byte PASEL accesses at exact watched address `0xFFFFF403` also produced aligned `0xFFFFF402` unmapped messages with mask `0x00FF`; exact watchpoints still reported PASEL read `0x00` and write `0x7F`.

PDSEL `0xFFFFF41B` was watched with write data `0xFF`, but MAME emitted no unmapped diagnostic. Execution continued. Its direct debugger readback at RTS was `0x00`, so the transaction is demonstrated but retained PDSEL state is not.

Two existing unmapped startup writes to `0xFFFFFD0C` and `0xFFFFFD0E` occurred before the keyboard entry marker and are outside Phase 1.

## RTS, return, and safety

At the requested RTS `0x0042E2EE`, the established +2 presentation was:

```text
AS3KDV_KBD_PHASE1_RTS PC=0042E2F0 D0=00000000 PASEL=00 PADIR=00 PAPUEN=00 PADATA=00 PDSEL=00 PDDIR=00 PDPUEN=00
```

Thus `D0=0`. Direct debugger byte expressions returned `0x00` for all listed registers. These readbacks are separate from the bus observations: watchpoints directly saw `0x7F` writes to PASEL/PADIR/PADATA and `0xFF` to PDSEL.

Execution continued through RTS and reached requested caller return `0x00420168`, again shown +2:

```text
AS3KDV_KBD_CALLER_RETURN PC=0042016A D0=00000000
```

The emulator exited via the debugger before timeout. No CPU exception handler, `InterruptInstallHandler`, `Keyboard_InterruptHandler`, or Phase-2 safety breakpoint fired. Phase 2 was not executed; neither key scanning nor normal keyboard polling ran.

## Publication checks

- Final `git diff --check`: passed.
- Pre-documentation tracked status was clean.
- Only `docs/as3k/CODEX_RESULT.md` is selected for commit.
- No MAME source/core, ROM, fixture, generated binary, diagnostic script/log, or proprietary artifact is staged.
- Documentation commit and push result: successful to `as3k-project/as3k-mame0289-dev` (commit containing this report; exact hash in the final handoff).
