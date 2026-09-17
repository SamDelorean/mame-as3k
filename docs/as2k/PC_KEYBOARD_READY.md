# AS2000 PC keyboard-ready signature

Status: `ROM_RE_EVIDENCE_ONLY_NOT_WORKER_AUTHORIZATION`

This note records a static reverse-engineering result from the stock AlphaSmart 2000 v3.1.4 ROM. It is documentation for a directed local test. It does **not** start, enable, retarget, or schedule any worker.

## Source

- stock ROM SHA1: `e0b777dc68c671c31ba808e214fb9d2573b9a853`
- CPU ROM base: `$8000`
- decoder: project HC11 reachability/disassembly tooling aligned with MAME's HC11 decoder

## Result: there is no separate `keyboard_ready` flag in the observed entry path

The firmware transition into PC keyboard emulation is a control-flow transition. RAM `$008A` is state set during that transition, but it is **not** sufficient evidence that the CPU has reached the loop that accepts `Send`.

The confirmed PC-entry sequence is:

```text
$8067  sample PA2
  -> $806B JSR $85A1
  -> V=1
  -> $8070 JSR $80C8
  -> $80C8 set $008A=$FF
  -> $80CB JSR $A3AC   ; synchronous Attached-to-PC UI
  -> $80CE/$80D1 clear $0052/$0053
  -> $80D4             ; PC service loop begins
```

`$008A` is written before `$A3AC` completes, so `$008A!=0` alone is too early as a `PC_KEYBOARD_READY` test.

## Primary ready indicator

The first execution of **PC `$80D4` after the `$80C8 -> $A3AC` entry sequence** is the first unambiguous ROM-level indication that the PC transition has completed and the firmware is in the service loop that can accept the local `Send` key.

Relevant loop:

```asm
80D4  BCLR  $08,#$02
80D7  BSET  $08,#$01
80DA  BRCLR $00,#$01,$814D
80DE  BRCLR $00,#$04,$80C5
80E2  JSR   $89B6
80E5  JSR   $938C
80E8  BVS   $80D4
80EA  CMPA  #$C7
80EC  BEQ   $80E5
80EE  CMPA  #$47
80F0  BNE   $80F7
80F2  JSR   $8606
80F5  BRA   $80D4
```

Therefore a local diagnostic may use the first hit on `$80D4` after confirmed PC entry as `PC_KEYBOARD_READY`.

## Stronger ready indicator: firmware is explicitly waiting for a local key

For a stricter directed test, observe **PC `$80E5`** while PA2 remains high. `$80E5` calls `$938C`, the keyboard-queue consumer.

```asm
938C  LDAB  $8E
938E  CMPB  $8F
9390  BEQ   $93A1
9392  INCB
9393  ANDB  #$0F
9395  STAB  $8E
9397  LDX   #$0000
939A  ABX
939B  LDAA  $90,X
939D  STAA  $70
939F  CLV
93A0  RTS
93A1  SEV
93A2  RTS
```

Interpretation:

- `$008E` = queue read index;
- `$008F` = queue write index;
- `$0090-$009F` = 16-byte circular queue;
- `$008E == $008F` -> queue empty -> `$938C` returns `V=1`;
- `$80E8 BVS $80D4` then loops and continues waiting;
- a queued key returns in `A` with `V=0`.

Thus the strongest simple signature is:

```text
PC mode entered
AND PA2 still high
AND PC repeatedly reaches $80E5
AND $008E == $008F
```

That means, at ROM level, **PC keyboard mode is active and waiting for a local key**.

## Directed Send test

Once the ready signature above is observed, inject `Send` only through the normal MAME keyboard matrix/IRQ path (`Send` keycode `$47`).

The expected causal proof is:

```text
$938C returns A=$47, V=0
  -> $80EE CMPA #$47
  -> $80F2 JSR $8606
```

A hit on `$80F2` followed by entry to `$8606` is the correct proof that `Send` has been accepted by the stock connected-PC loop. Do not call `$8606` directly and do not force the HC11 PC.

## Disconnect path useful for the current local test

The PC loop checks PA2 at `$80DE`:

```asm
80DE  BRCLR $00,#$04,$80C5
80C5  JMP   $816D

816D  BSR   $817D
816F  BVC   $8162
8171  BRA   $8156

817D  LDX   #$4650
8180  BRSET $00,#$04,$8189
8184  DEX
8185  BNE   $8180
8187  CLV
8188  RTS
8189  SEV
818A  RTS

8162  LDD   $52
...
8169  CLR   $008A
816C  RTS
```

If PA2 remains low through the `$817D` confirmation window, the routine returns `V=0`, reaches `$8169`, clears `$008A`, and exits the PC loop. If PA2 reappears during that window, `V=1` and control returns toward `$80D4`.

This gives a ROM-level disconnect signature independent of the LCD message.

## Recommended diagnostic markers

For the next local test, the useful addresses are:

- `$80C8` — PC entry selected;
- `$A3AC` — Attached-to-PC UI routine;
- `$80D4` — **primary PC_KEYBOARD_READY**;
- `$80E5` — stronger waiting-for-key signature;
- `$80EE` — Send key comparison;
- `$80F2` — wired Send accepted;
- `$8606` — Send/document-stream engine entered;
- `$80DE/$816D/$817D/$8169` — disconnect confirmation and return path.

The test should preserve the stock ROM and normal matrix/IRQ path. It should not write `$008A`, patch the ROM, force PC, or replace `$85A1`/`$8606` with shortcuts.
