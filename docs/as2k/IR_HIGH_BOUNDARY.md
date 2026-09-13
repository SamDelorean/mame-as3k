# AS2000 high-IrDA reclaim boundary handoff

This note mirrors a firmware reverse-engineering checkpoint from `SamDelorean/AS2K-V3.14.x` into the MAME work. It does not change the current Gate 1A runtime trace range.

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

The adjacent bytes `$E102-$E103` form the 16-bit value `$1ED1`. The aligned firmware reachability pass previously showed a one-byte gap at `$E103`; that gap alone is not proof that `$E103` can be physically overwritten independently of `$E102`.

## MAME consequence

For **Gate 1A**, keep the existing runtime safety contract unchanged:

- no execution in `$E103-$FFBF` after the IR detach patch;
- no execution in the low IR candidate ranges;
- `$D488-$D498` remains the protected low-range exception;
- Send keycode `$47` must still follow the normal matrix/IRQ path and reach retained cable Send `$8606` in causal order from `$9716`;
- do not simplify HC11 timer/MMIO/banking behavior to make this test easier.

For **Gate 1B**, firmware work now adds a separate prerequisite: do not physically fill/overwrite the `$E103` boundary byte until the data ownership of `$E102-$E103` is closed. Runtime non-execution and physical overwrite safety are separate proofs.

## Reproducible local audit

The firmware repository now provides:

- `recon/high_ir_boundary_v1.md`
- `tools/audit_high_ir_boundary.py`
- `tests/test_high_ir_boundary_audit.py`

The audit validates the exact stock v3.1.4 SHA-1 locally and inventories raw big-endian occurrences of `$E102`, `$E103`, `$E104`, and `$1ED1`, plus direct extended `JSR/JMP` references to the boundary. No proprietary ROM bytes are published.

Until that audit is reviewed against the local ROM, the public candidate accounting remains `$E103-$FFBF` and 9,004 total IR-candidate bytes, but physical reclaim of the first high-range byte is blocked.
