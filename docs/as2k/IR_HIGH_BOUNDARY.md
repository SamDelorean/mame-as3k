# AS2000 high-IrDA reclaim boundary handoff

This note mirrors the closed firmware reverse-engineering result from `SamDelorean/AS2K-V3.14.x` into the MAME work. It changes only the documented trace/reclaim boundary; it does not modify emulator behavior.

## Stock v3.1.4 bytes

The version string `AlphaSmart 2000 v3.1.4` starts at `$E0EB` and terminates at `$E101`.

Observed bytes immediately afterward:

```text
E100: 34 00 1E D1 3C 34 30 6F 01 B6 05 68 81 02 27 07
```

Therefore:

- `$E102 = $1E`
- `$E103 = $D1`
- `$E104 = $3C`
- direct firmware callers are known at `$D305 -> $E104` and `$D466 -> $E104`

The adjacent bytes `$E102-$E103` form the 16-bit value `$1ED1`.

## Historical boundary closure

The older reference firmware contains the same structural field immediately after its version string:

- older field address/value: `$DD20-$DD21 = $22B3`
- older relation: `$DD20 + $22B3 = $FFD3`
- v3.1.4 relation: `$E102 + $1ED1 = $FFD3`

The field moved by `$03E2` between firmware generations and its value decreased by exactly `$03E2`, preserving the same endpoint `$FFD3`.

Therefore `$E102-$E103` is a self-relative ROM-tail span/length metadata word, not IrDA payload. It must remain byte-identical in the current V3.14.x family. Executable high-IrDA begins at `$E104`.

## MAME consequence

For **Gate 1A**, the current runtime safety contract is:

- no execution in `$E104-$FFBF` after the IR detach patch;
- no execution in `$D098-$D487` or `$D499-$D517`;
- `$D488-$D498` remains the protected low-range exception;
- Send keycode `$47` must follow the normal matrix/IRQ path and demonstrate causal `$9716 -> $8606`;
- Print must demonstrate causal `$962D|$9804 -> $ABC9`;
- `$D2DC`, `$D099`, and `$D437` remain forbidden;
- do not simplify HC11 timer/MMIO/banking behavior to make the test easier.

The corrected IR candidate reclaim is:

```text
D098-D487  1008 bytes
D499-D517   127 bytes
E104-FFBF  7868 bytes
-------------------
TOTAL      9003 bytes
```

For **Gate 1B**, `$E102-$E103` must be preserved byte-for-byte. There is no longer an unresolved ownership question at this boundary, but Gate 1B remains blocked until the derived-ROM Gate 1A runtime trace and required smoke tests pass.

## Reproducible evidence

The firmware repository provides:

- `recon/high_ir_boundary_v1.md`
- `tools/audit_high_ir_boundary.py`
- `tests/test_high_ir_boundary_audit.py`
- structural protection in `tests/test_ir_detach.py`

The audit validates the exact stock v3.1.4 SHA-1 locally. No proprietary ROM bytes are published.

## Scope discipline

This result does not authorize new DynFS code, host-transport reclaim, or changes to Send/Print. It only corrects the high-IrDA ownership boundary used by firmware and emulator validation.