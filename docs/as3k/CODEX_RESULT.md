# Codex result — static study of `SystemInstallExceptionVectors`

Date: 2026-08-26

Status: **complete**. Static only: no AS3K run, build, keyboard execution, or MAME source change.

## Repository gate

- Required fast-forward pull: already up to date.
- Branch: `as3k-mame0289-dev`; tracked state was clean.
- Composite-LCD commit `38528ee8ffca9ff718d87e88145cae92d9e0134a` is an ancestor of `HEAD`.

## Evidence and relocation

Original artifacts on `/Users/sperezc/Downloads/AlphaSmart.iso`:

- `AS3000/Software/ModuleSources/SystemModule.c`
- `AS3000/Software/ModuleSources/SystemModule.h`
- `AS3000/Software/BuiltApplet/SystemModule.o.lst`
- `AS3000/Software/BuiltApplet/AWordApplet02.out`

Exact linked fixture: `../diagnostic/AWordApplet02.bin` (240264 bytes). File offset zero maps to `0x00420030`, hence `0x00430504` is file offset `0x000104d4`.

`SystemModule.c` defines `SYSTEM_BUS_ERROR_VECTOR_ADDRESS=0x00000008`, `SYSTEM_ADDRESS_ERROR_VECTOR_ADDRESS=0x0000000c`, `SYSTEM_ILLEGAL_INSTR_VECTOR_ADDRESS=0x00000010`, and `SYSTEM_DIV_BY_ZERO_VECTOR_ADDRESS=0x00000014`. The function uses project types `UInt32_p`/`UInt32` (via `ProjectIncludes.h`).

The applet listing places the function and handlers at code offsets `0x00000b00`, `0x00000b22`, `0x00000bac`, `0x00000c36`, and `0x00000cc0`. The fixture applies the exact code relocation `+0x0042fa04`, yielding `0x00430504`, `0x00430526`, `0x004305b0`, `0x0043063a`, and `0x004306c4`. The listing's four unresolved handler immediates resolve to those addresses; absolute vector destinations remain unchanged.

## Exact linked routine

`SystemInstallExceptionVectors` is 34 bytes, `0x00430504`–`0x00430525`; `RTS` is at `0x00430524`:

```text
0x00430504  MOVE.L #0x00430526,0x00000008
0x0043050c  MOVE.L #0x004305b0,0x0000000c
0x00430514  MOVE.L #0x0043063a,0x00000010
0x0043051c  MOVE.L #0x004306c4,0x00000014
0x00430524  RTS
```

There are no direct calls/branches or helpers. Only these four RAM longwords are written. The routine accesses no `0xffffxxxx` internal register, `0x0060xxxx` external window, or other RAM; it does not explicitly change SR, A7, interrupt masks, VBR-like state, or any CPU control state. It cannot fail/assert/loop.

The target is big-endian. Each architectural `MOVE.L` appears through MAME's 16-bit MC68EZ328 bus as high-word then low-word debugger transactions.

## Complete vector table change

| Vector | RAM address | Handler | Flash address | Size |
|---:|---:|---|---:|---:|
| 2, bus error | `0x00000008` | `SystemBusError` | `0x00430526` | long, 32-bit |
| 3, address error | `0x0000000c` | `SystemAddressError` | `0x004305b0` | long, 32-bit |
| 4, illegal instruction | `0x00000010` | `SystemIllegalInstruction` | `0x0043063a` | long, 32-bit |
| 5, divide by zero | `0x00000014` | `SystemDivideByZeroError` | `0x004306c4` | long, 32-bit |

Expected transactions:

| Store PC | High half | Low half |
|---:|---|---|
| `0x00430504` | `0x00000008`=`0x0043` | `0x0000000a`=`0x0526` |
| `0x0043050c` | `0x0000000c`=`0x0043` | `0x0000000e`=`0x05b0` |
| `0x00430514` | `0x00000010`=`0x0043` | `0x00000012`=`0x063a` |
| `0x0043051c` | `0x00000014`=`0x0043` | `0x00000016`=`0x06c4` |

No CHK, TRAPV, privilege, trace, line-A/F, spurious, autovector, or TRAP vector is installed. Vector 32/TRAP 0 at `0x00000080` and earlier interrupt vectors at `0x00000110`, `0x00000114`, and `0x00000118` are preserved.

## Handler inspection

Each handler is 138 bytes and intentionally loops after displaying an LCD diagnostic:

- `SystemBusError`: `0x00430526`–`0x004305af`; self-loop `0x004305a6`.
- `SystemAddressError`: `0x004305b0`–`0x00430639`; self-loop `0x00430630`.
- `SystemIllegalInstruction`: `0x0043063a`–`0x004306c3`; self-loop `0x004306ba`.
- `SystemDivideByZeroError`: `0x004306c4`–`0x0043074d`; self-loop `0x00430744`.

All save D0/D1/A0/A1 and call `LCDClearDisplay` (`0x004308f8`), `LocalGetMessage` (`0x0042f252`), `LCDSetString` (`0x004307f4`), `LCDMoveCursor` (`0x00430c76`), and `LCDSetCString` (`0x0043088a`). Their literals identify bus error, address error, illegal instruction, or divide-by-zero. They write no RAM signature. LCD hardware is referenced only through helpers; those can busy-poll. Each then executes `BRA` to itself forever; following restore/`RTE` code is unreachable. Source calls these unfinished power-cycle diagnostic paths. They are safety vectors and must not execute during normal initialization.

## MAME compatibility

`mc68ez328_device` derives through `mc68328_base_device` from `m68000_device`. The core resets `m_vbr` to zero, labels VBR “m68010+”, and fetches exceptions from `(vector << 2) + m_vbr`; this MC68000-generation target therefore uses the table at address zero. The routine never changes VBR.

Only aligned RAM longword writes and `RTS` are required. Earlier vector testing already demonstrated mapped low RAM and high/low 16-bit transactions. No credible emulator limitation specific to this routine was found.

## Exact next dynamic test (not run)

Run only `asma3kdv` with logging. Proposed debugger script:

```text
temp0 = 0
bp 0x00430504,1,{ temp0 = 1 ; logerror "EXCEPTION_INSTALL_ENTRY PC=%08X A7=%08X SR=%04X\n",pc,sp,sr ; g }
wp 0x00000008,0x4,w,temp0==1,{ logerror "VECTOR2 ADDR=%08X DATA=%08X PC=%08X\n",wpaddr,wpdata,pc ; g }
wp 0x0000000c,0x4,w,temp0==1,{ logerror "VECTOR3 ADDR=%08X DATA=%08X PC=%08X\n",wpaddr,wpdata,pc ; g }
wp 0x00000010,0x4,w,temp0==1,{ logerror "VECTOR4 ADDR=%08X DATA=%08X PC=%08X\n",wpaddr,wpdata,pc ; g }
wp 0x00000014,0x4,w,temp0==1,{ logerror "VECTOR5 ADDR=%08X DATA=%08X PC=%08X\n",wpaddr,wpdata,pc ; g }
bp 0x00430526,1,{ logerror "UNEXPECTED_BUS_ERROR\n" ; quit }
bp 0x004305b0,1,{ logerror "UNEXPECTED_ADDRESS_ERROR\n" ; quit }
bp 0x0043063a,1,{ logerror "UNEXPECTED_ILLEGAL_INSTRUCTION\n" ; quit }
bp 0x004306c4,1,{ logerror "UNEXPECTED_DIVIDE_BY_ZERO\n" ; quit }
bp 0x00430524,1,{ logerror "INSTALL_RTS V2=%08X V3=%08X V4=%08X V5=%08X\n",d@0x00000008,d@0x0000000c,d@0x00000010,d@0x00000014 ; g }
bp 0x0042e2a8,1,{ logerror "KEYBOARD_ENTRY_STOP PC=%08X V2=%08X V3=%08X V4=%08X V5=%08X\n",pc,d@0x00000008,d@0x0000000c,d@0x00000010,d@0x00000014 ; quit }
g
```

Pass criteria: exactly the eight high/low transactions above; final values `0x00430526`, `0x004305b0`, `0x0043063a`, `0x004306c4`; no handler breakpoint; arrival at `0x0042e2a8` before keyboard executes. No helper breakpoint is needed because the installer calls none.

## Publication

- `git diff --check`: passed.
- Pre-commit `git status --short`: only `M docs/as3k/CODEX_RESULT.md`; after committing this sole documentation change, expected final status is clean.
- MAME source changed: no.
