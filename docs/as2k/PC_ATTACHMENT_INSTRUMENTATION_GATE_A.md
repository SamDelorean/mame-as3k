# AS2000 PC attachment instrumentation — Gate A

Status: implementation authorized; Gate A remains fail-closed.

Canonical Drive contract (2026-09-17): stock ROM must demonstrate `ATTACHMENT_DETECTED -> PC_KEYBOARD_READY`. PA2 high is sampled by `$8067` as wired-host presence. `$85A1-$85C5` then classifies PC/Mac using IC3/PA0 and OC2 timeout. The PC return (`V=1`) reaches `$80C8`, which derives PC state and calls `$A3AC`. The first `$80D4` after `$A3AC` is the ROM-level `PC_KEYBOARD_READY` signature; the stricter idle signature is repeated `$80E5` with keyboard queue `$008E == $008F`.

The next implementation is diagnostic infrastructure in the AS2000 MAME driver, followed by an `as2kdiag` rebuild. It must expose an external attachment input separately from the firmware PORTA latch and log the causal checkpoints `$8067`, `$85A1/$85C5`, `$80C8`, `$A3AC`, `$80D4` and `$80E5`.

Hard constraints:

- stock ROM only for Gate A/B attachment proof;
- no RAM writes by the harness, including `$008A`;
- no forced PC/program counter;
- no direct `$8606` call;
- no Send injection before `PC_KEYBOARD_READY`;
- Gate 1A IRLESS instrumentation/contracts remain separate and unchanged;
- ROM/NVRAM artifacts remain private.

PASS is impossible from PA0/PA2 state or `$008A` alone. Evidence must establish the ordered transition into the ROM's stable PC keyboard loop.
