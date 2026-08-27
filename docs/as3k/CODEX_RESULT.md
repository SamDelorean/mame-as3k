# Codex result — LCD Port C atomicity hypothesis

Date: 2026-08-26

Status: **confirmed with one timing refinement**. The per-bit Port C callback ordering converts the final falling edge of each 4-bit busy read into an unintended control write. No bridge fix was implemented.

## First stuck operation

- Both `LCD_Reset` calls completed: E1/TOP first and E2/BOTTOM second, each with four normal write/busy-poll sequences (`0x06`, `0x0c`, `0x01`, `0x80`).
- After reset, an E1/TOP `LCD_WriteByte(data=0x0c, RS=1 argument meaning command/control, E1=1, E2=1)` at entry stack `A7=0x0003ff8c` completed its initial busy poll and performed the intended write.
- The next E1/TOP `LCD_WriteByte(data=0x80, RS=1, E1=1, E2=1)` at entry stack `A7=0x0003ff98` became stuck in its **pre-write busy poll**. Its intended `0x80` write was never reached.
- Static linkage places these post-reset calls in `LCDMoveCursor`, called by LCD initialization at `0x0043078c` with line 1, column 1. The failing operation is therefore the cursor-position command in `LCDMoveCursor`, not either `LCD_Reset`.
- The first stuck read reconstructed high nibble `0x8`, low nibble `0xf`, byte `0x8f` at `PC=0x00430e34`, with `A7=0x0003ff88`.

## Exact bridge sequence and controller effect

For the final cleanup of the successful busy poll preceding the E1 `0x0c` write, the trace shows:

```text
AS3KBRIDGE_RW old=0 new=1 DB=f RS=0 E1=0 E2=0
AS3KBRIDGE_E1 old=0 new=1 DB=f RW=1 RS=0
AS3KBRIDGE_E1 old=1 new=0 DB=f RW=1 RS=0
AS3KBRIDGE_E1 old=0 new=1 DB=f RW=1 RS=0
AS3KDVL_BRIDGE_READ_COMBINED ... D6=00000000 D0=00000000
AS3KBRIDGE_RW old=1 new=0 DB=f RS=0 E1=1 E2=0 DROP_WHILE_E_HIGH
AS3KBRIDGE_E1 old=1 new=0 DB=f RW=0 RS=0
```

The same sequence immediately before the first stuck result is:

```text
AS3KDVL_BRIDGE_WRITEBYTE ... A7=0003FF98 ARG_DATA=00000080 ARG_RS=00000001 ARG_E1=00000001 ARG_E2=00000001
AS3KBRIDGE_RW old=0 new=1 DB=f RS=0 E1=0 E2=0
AS3KBRIDGE_E1 old=0 new=1 DB=f RW=1 RS=0
AS3KBRIDGE_E1 old=1 new=0 DB=f RW=1 RS=0
AS3KBRIDGE_E1 old=0 new=1 DB=f RW=1 RS=0
AS3KDVL_BRIDGE_READ_COMBINED ... D6=00000080 D0=0000000F
AS3KBRIDGE_RW old=1 new=0 DB=f RS=0 E1=1 E2=0 DROP_WHILE_E_HIGH
AS3KBRIDGE_E1 old=1 new=0 DB=f RW=0 RS=0
AS3KDVL_BRIDGE_FIRST_STUCK ... D0=0000008F D7=0000008F
```

Thus the state passed to `ks0066->e_w(0)` is exactly E falling, RW=0, RS=0, bridge nibble `0xf` / device DB input `0xf0`. In MAME 0.289, `hd44780_base_device::e_w()` therefore calls `control_write(0xf0)` on that edge.

The command reconstruction is exact:

- A two-strobe control read leaves the 4-bit phase on the low nibble.
- Before the post-reset E1 `0x0c` write, the prior instruction register is `0x80` from the last reset command.
- The false cleanup write supplies low nibble `0xf`, completing `0x8f`.
- `0x8f` is Set DDRAM Address and sets AC to `0x0f` plus busy.
- The intended `0x0c` command subsequently executes, leaves AC at `0x0f`, and sets busy.
- The next `LCD_WriteByte(0x80)` first polls busy and consequently reads high `0x8`, low `0xf`, or `0x8f`. Its cleanup again performs a false control write and refreshes busy, so the loop cannot expire.

## Refinement and E2 result

- The trace did **not** show DB changing to `0x0f` while E was high. Because PC0-PC3 are configured as inputs, their callbacks publish ones before PC4/RW and before the enable callback; DB is already `0x0f` while E is low. This refines step 3 of the proposed timeline but does not change the false-write mechanism.
- RW always changed 1->0 before the relevant E falling edge, on both E1 and E2.
- E2 shows the same ordering on every captured reset busy poll: `DB=f`, RW 1->0 while E2=1, then E2 falls with RW=0 and RS=0. E2 did not become stuck in this run because execution stopped at the first E1 `0x8f`, but it is exposed to the same bridge defect.

## Commands, artifacts, and cleanup

- Safe update: `git pull --ff-only as3k-project as3k-mame0289-dev` (already up to date).
- Incremental build, no clean: `make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 -j2` (passed; 4 drivers found and linked `alphasma3k`).
- Narrow run: `./alphasma3k asma3kdvl -debug -debugscript ../diagnostic/as3kdvl_bridge_order.cmd -log -seconds_to_run 8` (exited via debugger at the first `d0==0x8f`).
- Debugger condition syntax was verified against the local MAME debugger documentation before use.
- Preserved local-only artifacts:
  - `../diagnostic/as3kdvl_bridge_order.cmd`
  - `../diagnostic/as3kdvl_bridge_order.log` (144 lines)
  - `../diagnostic/as3kdvl_bridge_order_console.log` (10 lines)
- All temporary `AS3KBRIDGE_` source instrumentation was reverted. `src/mame/skeleton/alphasma3k.cpp` is byte-for-byte identical to tracked `HEAD`; no core, ROM definition, hash, or bridge semantics remain changed.
- `git diff --check`: passed.
- No file under `roms/`, diagnostic log, generated binary, or proprietary artifact was staged.

## Publication

- Result commit: `3ed400e1` (`as3k: prove LCD bridge atomicity failure`).
- Push target: `as3k-project/as3k-mame0289-dev`.
- Push result: succeeded (`468cb8c2..3ed400e1`).
- Final status after recording publication is expected to be clean.
