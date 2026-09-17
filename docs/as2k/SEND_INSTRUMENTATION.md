# AS2000 stock Send instrumentation map

Status: `STATIC_ROM_RE_EVIDENCE_NOT_WORKER_AUTHORIZATION`

This note records a directed instrumentation model derived from the stock AlphaSmart 2000 v3.1.4 ROM (SHA1 `e0b777dc68c671c31ba808e214fb9d2573b9a853`). It documents useful firmware states after the connected-PC Send path has been demonstrated locally. It does not start, enable, retarget, or schedule any worker.

## Main result

The stock Send engine already exposes enough stable internal state to instrument a transfer without decoding the physical waveform first.

Recommended event sequence:

```text
PC ready ($80D4/$80E5)
  -> Send accepted ($80F2 JSR $8606)
  -> SEND_START ($8606)
  -> SEND_SETUP_DONE ($860C)
  -> source-byte / key translation loop
  -> serializer-byte events ($AA54, RAM $0046)
  -> EOF or abort cleanup ($8662)
  -> SEND_DONE / returned to PC loop ($80F5 -> $80D4)
```

## 1. Transaction start

The connected-PC caller accepts keycode `$47` at `$80EE` and executes:

```asm
80F2  JSR   $8606
80F5  BRA   $80D4
```

Use the first hit on `$8606` as `SEND_START`.

At entry, `$8606` executes:

```asm
8606  JSR   $936E       ; clears keyboard queue indexes $008E/$008F
8609  JSR   $9A83       ; initializes transfer/document traversal
860C  JSR   $A10E       ; fetch/translate next source byte
```

`$9A83` performs two particularly useful writes:

```asm
9A83  LDX   $0128
9A86  STX   $67
9A88  BSET  $6D,#$FF
```

Therefore the first hit on `$860C` after `$8606` is a useful `SEND_SETUP_DONE` marker.

At that point:

- `$0067/$0068` is the current source pointer;
- it has just been initialized from `$0128`;
- `$006D` is set non-zero by the transfer setup;
- `$0124` is the current file EOF used by the Send iterator;
- `$00A4 bit0` selects the release/encoding family used by the transport;
- `$018F` is the configured Send pacing/delay value.

`$006D` is useful as a secondary busy-state sanity check, but it is not Send-specific globally because other firmware paths also call `$9A83`. The authoritative transaction gate should remain entry to `$8606`.

## 2. Source-file progress and normal EOF

`$A10E` calls `$A117`. `$A117` implements the source iterator:

```asm
A117  LDX   $67
A119  CPX   $0124
A11C  BEQ   $A157       ; EOF
...
A14A  LDAA  $00,X       ; read source byte
...
A152  INX
A153  STX   $67
A155  CLV
A156  RTS
A157  SEV
A158  RTS
```

This gives a direct firmware progress model:

```text
send_start = word[$0128]
send_eof   = word[$0124]
send_pos   = word[$0067]
processed  = send_pos - send_start
total      = send_eof - send_start
```

For the current F1-F8 layout these pointers are monotonic within the selected file window.

A particularly useful prediction is available at PC `$860C`: if `$0067 == $0124` before `$A10E` executes, the next fetch will return `V=1`, `$860F BVS $8662` will take the normal EOF cleanup path. This is a clean `SEND_EOF_PENDING` marker that does not require reading the HC11 V flag from the MAME driver.

## 3. Logical translated key state

For the ordinary mapping path, after `$A10E` returns with data, `$8606` stores the translated code:

```asm
8615  STAA  $50
8617  BITA  #$80
```

At PC `$8617`, RAM `$0050` therefore contains the translated key representation for the current source item.

The code then treats bit 7 as a modifier requirement and sends the lower seven bits as the main key code. `$0054 bit 2` tracks the temporary modifier state while the character is emitted.

This marker is useful for diagnostics, but it should not be the only output-capture point: special source characters can take the `$0064 != 0` expansion path at `$8670` and emit multi-event sequences from translation tables.

## 4. Best raw outgoing-byte capture point: `$AA54` + RAM `$0046`

The physical serializer starts at `$AA52`:

```asm
AA52  STAA  $46
AA54  LDAB  #$01
...
```

The crucial property is that **when the instruction callback reports PC `$AA54`, the previous instruction has already latched the exact outgoing transport byte into RAM `$0046`**.

Therefore the recommended first instrumentation boundary is:

```text
if send_active && PC == $AA54:
    wire_byte = RAM[$0046]
```

Advantages:

- no need to expose or depend on the HC11 A-register state ID from the CPU core;
- captures the exact byte that the stock ROM handed to the serializer;
- preserves all stock translation, modifier, make/break, special-key and Send sequencing;
- can be gated by `send_active`, so normal connected-keyboard traffic using the same serializer is excluded.

`$AA26` remains the higher-level pacing wrapper; `$AA52/$AA54` is the more convenient instrumentation boundary because the byte is already materialized in RAM.

## 5. Encoding-family marker

The Send engine repeatedly branches on `$00A4 bit0`.

Observed release forms are:

```text
A4 bit0 = 0 path:  $F0, code
A4 bit0 = 1 path:  code | $80
```

The same bit also changes modifier/special-key encodings. Log `$00A4 & 1` once at `SEND_SETUP_DONE`; it provides the decoder mode for the entire Send transaction unless runtime evidence later shows it can change mid-transfer.

Do not assign external protocol names solely from these byte forms until the host-classification/transport mapping is fully closed.

## 6. Cleanup, successful completion, and abort classification

All observed Send termination paths converge on:

```asm
8662  JSR   $8CB8
8665  CLV
8666  RTS
```

`$8CB8` clears transfer-related state and restores the attached-PC UI:

```asm
8CB8  CLR   $006D
8CBB  CLR   $008B
8CBE  JSR   $A3AC
8CC1  JMP   $936E       ; clear keyboard queue
```

So `$8662` is `SEND_CLEANUP` and `$8666` is the internal function return. `$80F5` is an even stronger external `SEND_DONE` marker because it proves `$8606` returned to the connected-PC caller.

However `$8662` is shared by both normal EOF and early termination. Classify the cleanup as:

```text
normal completion: SEND_EOF_PENDING was seen before $8662
abort/error:        $8662 reached without SEND_EOF_PENDING
```

Early cleanup can be caused by a transport error or by another queued local key. The loop checks the keyboard queue at `$8659`; a non-empty key other than `$C7` falls through to `$8662`.

## 7. Recommended emulator-side diagnostic state machine

A minimal instrumentation state can therefore be:

```text
send_active = false
send_eof_pending = false

PC $8606:
    send_active = true
    send_eof_pending = false
    log SEND_START

PC $860C while send_active:
    read $0067, $0128, $0124, $00A4, $018F, $006D
    if $0067 == $0124:
        send_eof_pending = true
        log SEND_EOF_PENDING

PC $8617 while send_active:
    log translated_code = RAM[$0050]

PC $AA54 while send_active:
    log wire_byte = RAM[$0046]

PC $8662 while send_active:
    log SEND_CLEANUP(normal = send_eof_pending)

PC $80F5 while send_active:
    log SEND_DONE
    send_active = false
```

For the eventual `salida.txt` sink, open/truncate on `$8606`, capture/decode only `$AA54` bytes while `send_active`, and flush/close on `$80F5`. Keep the raw-byte trace available alongside decoded text during development.

## 8. Useful validation predictions

For a short known document, a successful stock transfer should show:

1. `$80F2 -> $8606`;
2. first `$860C` with `$0067 == $0128` and `$006D != 0`;
3. monotonically increasing `$0067` as source bytes are consumed;
4. one or more `$AA54` events per logical source character;
5. `$0067 == $0124` at a later `$860C`;
6. `$8662 -> $8666`;
7. return to `$80F5 -> $80D4`;
8. `$006D == 0` after cleanup.

These markers should make it possible to distinguish file traversal, logical key mapping, physical-byte serialization, normal EOF, early abort, and return to ready state without patching the ROM or bypassing its control flow.
