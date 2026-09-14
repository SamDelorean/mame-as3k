# AS2000 Gate 1A trace ordering contract

This note tightens the runtime evidence required by the AS2000 V3.14.x Phase 1A IR-detach gate. It does not change the MAME machine model and it does not add DynFS requirements.

## Why presence-only evidence is insufficient

A trace that contains both retained and patched addresses somewhere in the session is not enough to prove that a real Send or Print action followed the intended control-flow edge. The gate now requires ordered instruction evidence from the CPU trace itself.

## Send ordering

A valid Send capture must use the normal AS2000 keyboard path (`COL.7 / 0x10`, host F12 on `as2k-mame0289-dev`, firmware keycode `$47`) and must contain:

1. `$9716` — the patched editor-side Send redirect;
2. later in the same instruction trace, `$8606` — the retained cable/keyboard Send routine.

`$8606` appearing before `$9716`, or appearing only because of an unrelated main-loop Send invocation, does not prove the patched redirect. Calls synthesized directly at `$8606` are not valid evidence.

The capture must still show no execution of `$D2DC` or `$D099` and no PC in the IR reclaim ranges.

## Print ordering

A valid Print capture must contain one patched detach callsite:

- `$962D`, or
- `$9804`,

followed later in the same instruction trace by `$ABC9`, the retained non-wireless Print path.

Presence of `$ABC9` before the patched callsite is insufficient. `$D437` remains a failure condition.

## Shared hardware invariants

This ordering requirement does not relax any existing emulator constraints. Keep normal keyboard IRQ/MMIO behavior, `$2000/$9000`, PA6 view selection, PA4-PA5 banking, retained wired-host PORTA/PORTD use, and the HC11 timer/capture/compare behavior required by Macintosh/ADB.

## Relationship to the firmware repository

`SamDelorean/AS2K-V3.14.x` now enforces these sequences in `tools/check_mame_gate1a_trace.py`. The full runtime gate therefore needs both positive address hits and the causal order `$9716 -> $8606` and `$962D|$9804 -> $ABC9`, in addition to the existing negative IrDA/reclaim checks.
