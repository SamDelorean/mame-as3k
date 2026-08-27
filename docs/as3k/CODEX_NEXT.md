# Current Codex task — prove or reject the LCD Port C atomicity hypothesis

Date: 2026-08-26

Read `AGENTS.md` first and obey it.

## Context already established

The previous `asma3kdvl` experiment is published in `docs/as3k/CODEX_RESULT.md` and must **not** be repeated blindly.

Key observed result:

- both `LCD_Reset` calls completed;
- 9 busy polls exited normally with reconstructed byte `0x00`;
- a later E1/TOP operation became stuck reading `0x8f`;
- high nibble `0x8`, low nibble `0xf`;
- busy remained set and the loop returned to `0x00430e28` hundreds of thousands of times;
- no E1/E2 contention occurred.

The current bridge in `src/mame/skeleton/alphasma3k.cpp` publishes Port C callbacks immediately, one line at a time.

The MC68EZ328 core calls `out_port_c` callbacks from bit 0 through bit 7 inside `pcdata_w()`.

For Port C bits configured as input, `pcdata_w()` publishes logical 1 on the output callback.

Therefore, during `LCD_ReadByte`, PC0-PC3 can make the bridge's `m_lcd_data` become `0x0f` while the firmware is reading.

The firmware's final read-strobe cleanup writes a single new PCDATA value that drops both R/W and E. Since callbacks are delivered in bit order, PC4/RW is processed before PC6/E1 or PC7/E2.

Relevant MAME 0.289 `hd44780_base_device::e_w()` behavior is already known:

```cpp
if (!state && m_enabled && m_rw_input == 0)
{
    switch (m_rs_input)
    {
        case 0: control_write(m_db_input); break;
        case 1: data_write(m_db_input);    break;
    }
}
```

In 4-bit control read, `control_read()` returns busy/address high nibble first and AC low nibble second. A repeated read value `0x8f` is therefore consistent with busy=1 and AC=0x0f.

## Hypothesis to test

The bridge is violating the atomic nature of the original PCDATA byte write:

1. the firmware finishes the second LCD read with E=1 and R/W=1;
2. it writes PCDATA=0;
3. callbacks PC0-PC3 arrive first and leave `m_lcd_data=0x0f`;
4. callback PC4 changes R/W from 1 to 0 while E is still logically high in the bridge/device;
5. callback PC6 or PC7 then drops E;
6. `hd44780_base_device::e_w(0)` sees R/W=0 and treats this falling edge as a **write**, using DB=`0xf0`;
7. in 4-bit mode this spurious nibble completes/reissues a command such as `0x8f`, setting AC=0x0f and busy again;
8. each subsequent busy poll repeats the same false write, so busy can never expire.

This is a hypothesis. **Do not fix the bridge until this stage proves or rejects it.**

## A. Inspect existing evidence first

From `~/Projects/alphasmart-as3k/mame0289`:

1. Verify branch/status and pull the current handoff safely.
2. Read the existing local logs if present:
   - `../diagnostic/as3kdvl_lcd_controller.log`
   - `../diagnostic/as3kdvl_lcd_controller_console.log`
3. Locate the **first** transition from normal `LCD_ReadByte=0x00` to stuck `LCD_ReadByte=0x8f`.
4. Extract enough preceding markers to identify:
   - which `LCD_WriteByte` invocation caused the busy period;
   - whether it is inside `LCD_Reset`, `LCDMoveCursor`, or another known caller;
   - E1 vs E2;
   - the immediately preceding PCDATA write/read sequence.
5. If necessary, correlate statically with `LCDModule.c` and the linked disassembly. Do not modify source for this part.

Report the first stuck operation precisely before instrumenting anything.

## B. Temporary diagnostic instrumentation only

Temporarily instrument **only** `src/mame/skeleton/alphasma3k.cpp`.

Do not modify:

- `src/devices/machine/mc68328.cpp/.h`;
- `src/devices/video/hd44780.cpp/.h`;
- ROM definitions or hashes;
- the semantic behavior of the bridge.

Add minimal `logerror` diagnostics sufficient to establish ordering. Do not add a permanent fix.

At minimum capture:

1. In `lcd_data_w`:
   - when DB4-DB7 change while E1 or E2 is currently asserted;
   - old/new nibble;
   - which bit callback caused it;
   - current RW/RS/E1/E2.

2. In `lcd_rw_w`:
   - old RW -> new RW;
   - current nibble;
   - RS/E1/E2;
   - especially a marker if RW drops 1->0 while E1 or E2 is still high.

3. In `lcd_e1_w` / `lcd_e2_w`, immediately **before** calling `ks0066->e_w(state)`:
   - old E -> new E;
   - current bridge nibble;
   - current RW and RS.

Use a distinctive prefix such as:

`AS3KBRIDGE_`

Keep logging narrow. We do not need another multi-hundred-thousand-line busy-loop log.

## C. Narrow execution

Rebuild incrementally, no clean.

Create a new debugger script, for example:

`../diagnostic/as3kdvl_bridge_order.cmd`

The run should stop as soon as the first stuck busy read (`byte & 0x80 != 0`) has been captured with the surrounding bridge log events.

Use locally verified debugger condition syntax; do not guess it. If a reliable conditional breakpoint cannot be expressed, stop after the first occurrence by another verified debugger mechanism or use a very short controlled run that captures only the first transition.

Run only `asma3kdvl`.

Preserve:

- console output;
- `error.log`/bridge diagnostics;
- the relevant AS3KDVL debugger markers.

## D. Required proof

The hypothesis is considered **confirmed** only if the trace shows this order for the final read cleanup on the affected controller:

1. E is still high and RW was 1;
2. DB nibble becomes `0x0f` (or otherwise the exact value later passed to KS0066);
3. RW callback changes the device/bridge to 0 **before** the E falling callback;
4. the E falling callback is then invoked with RW=0, RS as observed, DB nibble as observed;
5. static `hd44780_base_device::e_w()` semantics imply that this exact falling edge calls `control_write()` or `data_write()`;
6. the next busy read becomes/repeats `0x8f` with AC low nibble `0xf`.

If any step differs, reject or refine the hypothesis from evidence. Do not force the conclusion.

Also determine whether the same ordering exists on E2 even though E2 did not become stuck in the previous run.

## E. Clean up temporary instrumentation

After capturing the evidence:

1. Revert **all temporary AS3KBRIDGE logging changes** from `alphasma3k.cpp`.
2. Do not revert the already-validated permanent AS3K source/bridge/diagnostic systems.
3. `git diff --check` must pass.
4. The tracked source should return to exactly the published semantic state before this diagnostic.

Do not compile a fix in this task.

## F. Result and publication

Replace `docs/as3k/CODEX_RESULT.md` with a factual report containing:

- first operation that becomes stuck;
- exact PCDATA/DB/RW/RS/E callback sequence around the failure;
- whether DB became `0x0f` while E was high;
- whether RW changed 1->0 before E fell;
- exact state seen by `e_w(0)`;
- whether `hd44780_base_device::e_w()` therefore performs an unintended write;
- whether that write can be reconstructed as the command/data that yields AC=0x0f / readback `0x8f`;
- whether the hypothesis is confirmed, rejected, or only partially confirmed;
- whether E2 shows the same ordering;
- paths of the temporary diagnostic logs/scripts;
- confirmation that temporary source instrumentation was reverted;
- `git diff --check`;
- final `git status --short`;
- commit SHA and push result.

Commit/push only safe tracked handoff/result files if source has returned to the validated state. Never add `roms/`, generated ROMs, proprietary binaries, or diagnostic logs.

Push only to `as3k-mame0289-dev`.

## Stop condition

Stop after proving or rejecting the atomicity hypothesis.

**Do not implement the bridge fix in this task.**
