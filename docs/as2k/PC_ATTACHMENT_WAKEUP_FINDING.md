# AS2000 PC attachment: editor wake-up and firmware detection

## Scope and evidence (2026-09-23)

A focused, private AS2000 v3.1.4 diagnostic run established that selecting the emulator's PC-keyboard accessory while the firmware is waiting in the editor does not, by itself, cause another firmware connection probe. The selector updates the emulator's accessory state, but does not assert the normal keyboard IRQ. An ordinary key press through the existing keyboard matrix wakes the firmware, which then samples the connection state and enters the PC path. This is a **test-sequencing finding**, not a demonstrated missing hardware signal or a request to synthesize an IRQ on accessory selection.

The passive DIAG instruction-PC observer recorded the following sequence after editor readiness, accessory mode 2 selection and a normal `L` key press:

```text
8067 -> 806B -> 85A1 -> 85B4 / 85BE (timer wait)
     -> 85C2 -> 85C3 -> 806E -> 8070 -> 80D4
```

The wait loop visited `$85B4` 1,667 times before `$85C2`. At `$85C3`, `CCR=$53` (V set); the firmware followed the PC branch through `$8070` and reached `$80D4`. TFLG1 changed from `$B8` during the wait to `$F8` at exit. The previous absence of `$80E5` in a periodically sampled Lua PC trace was not sufficient evidence of failed attachment; the per-instruction observer subsequently confirmed `$80D4`.

## Reproduction and interpretation

Use a private firmware image and isolated NVRAM. Boot to editor-ready; select PC-keyboard accessory mode 2; press a normal keyboard key using the matrix/IRQ path; then inspect a bounded per-instruction trace of the firmware connection probe. Do not force the CPU PC, PA0/PA2, timer flags or ROM control flow. An emulator selector log (`selected=2`) alone does not prove firmware recognition. A run that selects the accessory but never wakes the editor cannot evaluate the outcome of `$85A1`.

This result establishes firmware entry into the PC attachment path, **not** successful Send text transfer. Gate 1A still requires a separate normal-matrix Send validation, including the expected `$9716 -> $8606` route and forbidden IrDA-region checks. No proprietary ROM image, private NVRAM, binary, or execution log is included in this repository. The additional per-instruction host-probe logging used for this observation was local DIAG instrumentation, not a proposed stable-emulator change.
