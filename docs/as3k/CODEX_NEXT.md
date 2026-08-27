# Current Codex task — fix Port C atomicity in the AS3K LCD bridge

Date: 2026-08-26

Read `AGENTS.md` first and obey it.

## Established root cause

The previous task proved the LCD busy-loop failure. Read the current `docs/as3k/CODEX_RESULT.md` before editing.

The defect is now evidence-backed:

- `mc68328_base_device::pcdata_w()` stores one byte but invokes `out_port_c` callbacks sequentially from bit 0 through bit 7;
- AlphaWord performs the final cleanup of a 4-bit LCD read with one PCDATA byte write that drops R/W and E together;
- the bridge currently forwards PC4/RW immediately, before PC6/E1 or PC7/E2 receives the falling edge;
- therefore the KS0066 sees E falling after R/W has already become 0;
- `hd44780_base_device::e_w(0)` interprets that edge as a write;
- the bridge DB nibble is `0xf`, so the false write completes command `0x8f`, sets AC=`0x0f`, sets busy, and each later poll refreshes busy forever;
- E2 is exposed to the same ordering.

Temporary instrumentation from the proof task was fully reverted. Do not re-prove the hypothesis.

## Goal

Implement a **driver-local atomic Port C bridge** in:

`src/mame/skeleton/alphasma3k.cpp`

without modifying the MC68EZ328 core or HD44780/KS0066 core.

Then validate:

1. the existing no-LCD diagnostic behavior is unchanged;
2. `asma3kdvl` completes `LCDInitializeModule` with both KS0066 devices present;
3. the repeated false `0x8f` busy loop disappears.

Do not implement display rendering in this task.

---

## A. Pre-edit inspection

From:

`~/Projects/alphasmart-as3k/mame0289`

1. `git pull --ff-only as3k-project as3k-mame0289-dev`.
2. Confirm branch and clean tracked status.
3. Read current:
   - `src/mame/skeleton/alphasma3k.cpp`;
   - `src/devices/machine/mc68328.cpp` Port C implementation;
   - relevant `src/devices/video/hd44780.cpp` `db_w/rw_w/rs_w/e_w` behavior.
4. Reconfirm from the exact core source that `pcdata_w()` invokes selected output callbacks in deterministic bit order 0→7.
5. Reconfirm from the linked AlphaWord LCD initialization that the relevant LCD accesses use `PCSEL=0xff` and PC7 is an output during PCDATA writes, so the PC7 callback is a valid end-of-byte commit point for this bridge.

If either point 4 or 5 is false in the actual source/runtime, stop and report rather than implementing the design below.

---

## B. Implement an atomic bridge in `alphasma3k_state`

### Required semantics

A single MC68EZ328 PCDATA byte write must be presented to the external LCDs as one logical transition, not eight externally visible transitions.

A clean implementation is expected to keep two compact states, for example:

- pending Port C output state assembled from callbacks;
- last applied/committed LCD Port C state.

Exact names are up to you, but keep the implementation minimal and MAME-style.

### Callback collection

For `out_port_c<0..7>()`:

- each callback updates only its bit in the pending Port C state;
- PC0..PC6 must **not** directly call `db_w`, `rw_w`, `rs_w`, or `e_w` anymore;
- after updating PC7, commit the assembled byte to the LCD bridge, because the core has now delivered the complete 0→7 callback burst for that PCDATA write.

Document briefly in source why PC7 is the commit point: the current MC68EZ328 core iterates bit 0→7 and AlphaWord selects all Port C bits for the LCD path.

Do not add a scheduler/timer workaround that might coalesce multiple CPU writes together.

### Atomic commit ordering

Let OLD be the previously applied Port C state and NEW the just-assembled pending state.

Derive:

- DB nibble = bits 0..3;
- RW = bit 4;
- RS = bit 5;
- E1 = bit 6;
- E2 = bit 7.

Apply one transition in this order:

1. **Falling enable edges first, using OLD bus/control state.**
   - If OLD.E1=1 and NEW.E1=0, call `ks0066_0->e_w(0)` before changing DB/RW/RS.
   - If OLD.E2=1 and NEW.E2=0, call `ks0066_1->e_w(0)` before changing DB/RW/RS.

   This is the critical fix: a read cleanup must let E fall while the KS0066 still sees OLD.RW=1, so it remains a read and cannot become a false write.

2. **Apply NEW shared bus/control state.**
   - `db_w(NEW_DB << 4)` to both existing LCD devices;
   - `rw_w(NEW_RW)` to both;
   - `rs_w(NEW_RS)` to both.

3. **Rising enable edges last, using NEW bus/control state.**
   - If OLD.E1=0 and NEW.E1=1, call `ks0066_0->e_w(1)` after NEW DB/RW/RS are present.
   - Same for E2.

4. Record NEW as the applied state.

This ordering models simultaneous GPIO transition semantics needed by the LCD protocol:

- falling E samples the pre-transition read/write state;
- rising E sees the post-transition bus/control state.

Do not change KS0066 semantics.

### Reads

Update `lcd_data_r()` to multiplex using the **applied/committed** E1/E2 state, not a partially assembled pending state.

Preserve the already validated behavior:

- both LCD devices absent -> nibble `0x0`;
- E1 only -> `ks0066_0->db_r() >> 4`;
- E2 only -> `ks0066_1->db_r() >> 4`;
- neither -> `0x0f`;
- both -> deterministic `0x0f` plus existing contention diagnostic.

All handlers must still tolerate missing optional LCD devices.

### State/save/reset

- Save the new pending/applied Port C state with `save_item()`.
- Reset both to zero.
- Remove obsolete per-line bridge state if it is no longer needed; do not leave two competing representations.
- Keep source changes limited to `alphasma3k.cpp` unless `mame.lst` needs no semantic change.

Do not touch:

- `src/devices/machine/mc68328.cpp/.h`;
- `src/devices/video/hd44780.cpp/.h`;
- ROM hashes;
- synthetic fixture contents;
- display rendering.

---

## C. Static checks and build

1. `git diff --check`.
2. Inspect and explain the complete `alphasma3k.cpp` diff before build.
3. Incremental build only, no clean:

```sh
make SUBTARGET=alphasma3k \
  SOURCES=src/mame/skeleton/alphasma3k.cpp \
  OSD=sdl \
  REGENIE=1 \
  -j2
```

Save output to:

`../diagnostic/rebuild_lcd_atomic_bridge.log`

Expect 4 drivers and a successful link.

4. Confirm `asma3kdi` and `asma3kdv` still audit good:

```sh
./alphasma3k asma3kdi -verifyroms
./alphasma3k asma3kdv -verifyroms
```

Do not use `asma3kdvl -verifyroms` as a success gate because its synthetic F05 checksum is deliberately wrong.

---

## D. Exact no-LCD regression

Run the already validated no-LCD diagnostic unchanged:

```sh
./alphasma3k asma3kdv \
  -debug \
  -debugscript ../diagnostic/as3kdv_lcd_nolcd.cmd \
  -log \
  -seconds_to_run 8
```

Save the new result separately, e.g.:

`../diagnostic/as3kdv_lcd_atomic_bridge_nolcd_regression.log`

Required regression behavior remains:

- LCD_RESET_ENTRY = 2;
- WRITEBYTE_ENTRY = 11;
- READBYTE_ENTRY = 11;
- PCDATA_READ = 22;
- BUSY_BRANCH = 11;
- E1 PCDATA = `0x50`, reconstructed byte `0x00`;
- E2 PCDATA = `0x90`, reconstructed byte `0x00`;
- busy always 0;
- branch exits to `0x00430e40`;
- final breakpoint on RTS `0x0043079e` reached.

If this regression changes, stop. Do not continue to the KS0066 test and do not improvise a second fix.

---

## E. KS0066 controller test after the fix

1. Confirm the local-only synthetic file still exists:

`roms/asma3kdvl/ks0066_f05.bin`

It must still be exactly 4096 zero bytes with:

- CRC32 `c71c0011`;
- SHA1 `1ceaf73df40e531df3bfb26b4fb7cd95fb7bff1d`;
- SHA256 `ad7facb2586fc6e966c004d7d1d16b024f5805ff7cb47c7a85dabd8b48892ca7`.

If missing, regenerate only this synthetic file locally. Never commit it.

2. Run the existing controller script:

```sh
./alphasma3k asma3kdvl \
  -debug \
  -debugscript ../diagnostic/as3kdvl_lcd_controller.cmd \
  -log \
  -seconds_to_run 8
```

Preserve console and `error.log` under new filenames, for example:

- `../diagnostic/as3kdvl_lcd_atomic_bridge_console.log`
- `../diagnostic/as3kdvl_lcd_atomic_bridge.log`

3. Confirm loader behavior remains only the expected `WRONG CHECKSUMS` warnings for synthetic F05, with no required-file failure.

4. Report:

- LCD_RESET_ENTRY count;
- WRITEBYTE_ENTRY count;
- READBYTE_ENTRY count;
- PCDATA_READ count;
- BUSY_BRANCH count;
- distinct E1/TOP PCDATA values;
- distinct E2/BOTTOM PCDATA values;
- every distinct reconstructed `LCD_ReadByte` value;
- whether any read has busy bit 1;
- how many BUSY_BRANCH events loop to `0x00430e28` and how many exit to `0x00430e40`;
- whether `0x8f` appears at all;
- whether any busy condition eventually clears rather than being refreshed indefinitely;
- whether there is E1/E2 contention;
- whether both LCD_Reset calls finish;
- whether `LCDMoveCursor` finishes;
- whether final breakpoint `0x0043079e` is reached before timeout.

### Success criterion

The fix is successful if:

- no-LCD regression is unchanged;
- the repeated false `0x8f` loop is gone;
- no contention occurs;
- `asma3kdvl` reaches the RTS at `0x0043079e` before timeout.

Normal finite busy polling is acceptable and desirable evidence that the real KS0066 busy timer is being observed.

If `asma3kdvl` still hangs, stop at the first new failure and report it without speculative additional changes.

---

## F. Commit and publish

If both regression and KS0066 test pass:

1. `git diff --check`.
2. `git status --short`.
3. Ensure nothing under `roms/`, diagnostics, generated binaries, or proprietary historical artifacts is staged.
4. Replace `docs/as3k/CODEX_RESULT.md` with a factual report.
5. Commit only safe tracked changes.
6. Push only to `as3k-project/as3k-mame0289-dev`.

The result report must include:

- exact state representation chosen;
- exact commit ordering for falling edges / controls / rising edges;
- why PC7 is a safe commit point in this firmware/core path;
- build result;
- no-LCD regression result;
- KS0066 test counts and distinct read values;
- whether busy=1 was observed and whether it cleared;
- whether `0x8f` disappeared;
- final RTS result;
- contention result;
- `git diff --check`;
- final status;
- commit SHA;
- push result.

## Stop condition

Stop after validating the atomic bridge fix through `LCDInitializeModule`.

Do **not** implement 40×4 rendering, opening screen, keyboard, or later modules in this task.
