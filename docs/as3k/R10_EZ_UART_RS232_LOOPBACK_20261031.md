# R10 — MC68EZ328 UART over MAME RS-232 — 2026-10-31

STATUS: UART_8N1_TX_RX_PASS / UART_FIFO_8x_PASS / REGRESSIONS_PASS.
R10 OVERALL remains OPEN: USB D11/D12, Get Utility, native
updater, RS-232 end-to-end with original software, IrDA and printing
have NOT been credited as functional.

## Provenance, original hardware and source
Existing original AlphaSmart development sources on T160:
GoogleDrive/ALPHASMART/Soruce code/Software/ModuleSources/UartModule.c,
DebugModule.c, DebugModule.h, Irda68328EZ.c, UpdaterCommands.h,
RS232Framer.h. UartModule.c binds the EZ UART to PE4 RXD,
PE5 TXD, PE6 RTS, PE7 CTS; documented baud register examples
UBA=0038,0138,0026,0126,0226,0326. DebugModule.c
BBUG path uses UTX status FFFFF906 bit04 and sends
NUL-terminated bytes to FFFFF907 when enabled.
NXP MC68EZ328 User Manual, ch 11, confirms USTCNT FFFFF900,
UBAUD FFFFF902, URX FFFFF904, UTX FFFFF906 and
integer prescaler (65 - prescaler) with 2^divider and
16x clock: https://www.nxp.com/docs/en/reference-manual/MC68EZ328UM.pdf

Previous MAME mc68328.cpp had UTX and RX write handlers marked
'Not Yet Implemented', static UTX status always empty, and no
UART line transmission. This is an actual hardware limitation,
not a problem solved by connecting the host's RS-232 name.

## Single-driver implementation
Changes confined to canonical tree mame-as3k-dev:
src/devices/machine/mc68328.cpp / .h:
- EZ UART TX and RX timed bitstream with start, 8 bits LSB-first,
  one stop bit; timer computed from UBAUD and configured CPU clock.
- 8-byte TX and RX FIFOs with saved contents/indices/count,
  register-data movement, RX-ready/half/full and TX
  available/half/empty indicators, and TX-empty/RX-ready
  IRQ source updates (not yet fully interrupt-tested).
- TXD output and RXD input callbacks for actual bit lines,
  not a host-side character export.
- Mapped 8-bit data access on the UTX register's low byte
  and RX low-byte dequeue, so BBUG's FFFFF907 writes are honored.
- EZ-specific behavior gated by CPU device type; pre-existing
  MC68328 behavior remains on older machines.
src/mame/skeleton/alphasma3k.cpp:
- One standard RS232_PORT at tag 'rs232', physically connected
  to EZ UART TXD/RXD.
- Exposed ONLY loopback/null_modem/pty slots in this AS3K driver
  to avoid intentionally offering extraneous terminal choices.
- One and only original minimal ./alphasma3k binary and
  original ROM-set data; no clones or per-ROM conditions.

Boundaries: UART parity, 7-bit modes, 2 stop bits, CTS/RTS
handshake, IrDA pulse mode, overflow/error timing and external
clock sourcing not implemented or tested, and 16 MHz CPU clock
is already described as unverified in source. The generated
nominal baud therefore requires calibration against real AS3K.
The RS-232 generic framework pulls some MAME secondary deps
despite limited options, but driver list remains six ROM-system
entries from the single AS3K source, not additional emulators.

## Dynamic CPU-space and RS-232 loopback proofs
Built same reduced MAME with SUBTARGET=alphasma3k,
SOURCES=src/mame/skeleton/alphasma3k.cpp,
PRECOMPILE=0 REGENIE=1, exit 0. Driver list 1 source file,
6 registered ROM entries. First regeneration exposed too many
default RS-232 device options; the selected slot list was then
restricted before final relink. No additional executable created.

Original WRITER ROM with '-rs232 loopback' and private NVRAM,
Lua wrote UBAUD=0226, USTCNT=E000 via real EZ peripheral
memory map, then wrote 0xA5 to UTX FFFFF907.
Read back URX FFFFF904 = 20A5 (DATA_READY and A5);
TX status during send=60A5 and after send=E0A5.
Second 0x5A similarly produced URX=205A and
TX after send=E05A; MAME exited 0.

A separate 8-byte burst [10,32,54,76,98,BA,DC,FE]
was written as eight UTX bytes, transmitted as timed
bitstreams through the MAME RS232 loopback device and
read sequentially from the real URX hardware register.
All EIGHT matched in order, each with DATA_READY.
After eight low-byte reads, URX was 0000.
Test logged R10_FIFO_RX8_PASS=1 and exit 0.

## Independent host process via Linux PTY (dynamic proof)
A MAME instance running the SAME original WRITER ROM with
`-rs232 pty` exposed /dev/pts/1 on the T160 host.
A separate Python host process opened the slave in raw mode;
MAME's native pseudo-terminal handled byte/serial conversion
and EZ_UART sent/received timed 8N1 TXD/RXD pulses.
When Lua wrote 0xA5 to CPU UTX at FFFFF907, the independent
host read byte A5 from /dev/pts/1. After enabling UART,
the host sent raw byte 5A; the MAME CPU-space URX read
returned 205A. Python test logged:
R10_PTY_MAME_EXIT=0, HOST_RX_BYTES=a5,
R10_PTY_HOST_TX_5A, R10_PTY_CPU_RX=205A,
R10_PTY_BIDIRECTIONAL_PASS=True.
An initial attempt sent the host byte before UART was enabled
and got URX=0000; corrected only TEST TIMING,
not the hardware code.
This proves *external host process* exchange with the
hardware UART model, not native System3 file-transfer protocol.
Private scripts: r10_uart_pty.lua, r10_pty_host_test.py
and r10_uart_pty_mame.log.

## Regressions
- MAME -validate exit 0.
- ROM audits asma3kdi/as3kdv/asma3ksb/asma3ksw:
  4/4 OK, exit 0. Historical KS0066 F05
  NEEDS REDUMP notice persists.
- Real R3 KeyboardInitialize diagnostic Phase1:
  SCR 1D, PAPUEN FF then 80, latch FF, return
  00420168 with D0=0, exit 0.
- The original R8 WRITER SRAM document 'r8proof'
  recovered from an independent private NVRAM copy,
  240x36 LCD screenshot PIXEL-IDENTICAL to R8.
- The original R9 BOOT-installed AlphaWord applet
  persisted via private Flash and SRAM copies,
  document 'flash' screenshot PIXEL-IDENTICAL to R9.
- Scoped git diff --check for changed source PASS.
- Pre-existing user-modified AGENTS.md and untracked
  R1 original notes left untouched.

## Scope and next work without inventing increments
UART hardware basic 8N1 TX/RX, 8-byte FIFO and independent
host PTY bidirectional exchange are credited PASS. Native
firmware file-transfer protocol frames, IrDA, USB, official
updater and Send/Print are NOT credited. Continue R10 in the
same executable from original UsbAsmModule.s PE1/PE2
electrical transfer and UsbChip.c D11/D12 protocols,
and/or complete real UART RTS/CTS, IRQ and RS232Framer.
Do NOT credit full R10 or advertise complete communications.

## Private evidence on T160
../private/roms/as3k/mame_sets/r10_uart_patch.py
../private/roms/as3k/mame_sets/r10_uart_minbuild.log
../private/roms/as3k/mame_sets/r10_uart_loopback.lua
../private/roms/as3k/mame_sets/r10_uart_loopback.log
../private/roms/as3k/mame_sets/r10_uart_fifo.lua
../private/roms/as3k/mame_sets/r10_uart_fifo.log
../private/roms/as3k/mame_sets/r10_uart_validate.log
../private/roms/as3k/mame_sets/r10_uart_rom_audit.log
../private/roms/as3k/mame_sets/r10_uart_r3_regression.log
../private/roms/as3k/mame_sets/r10_uart_r8_regression.log
../private/roms/as3k/mame_sets/r10_uart_r9_regression.log
../private/roms/as3k/mame_sets/r10_uart_r8_regression.png
../private/roms/as3k/mame_sets/r10_uart_r9_regression.png
No ROMs, NVRAM data, binary or generated captures to Git.
