# AS2000 $0001 investigation — observed in both BIOSes

2026-09-12, parent b1f08e607ec. REVIEW accepted the bounded diagnostic
investigation. No observable defect attributable to this read was demonstrated.

## Source evidence

`alphasma.cpp` configures MC68HC11D0 at 8 MHz, with no INIT override.
The CPU constructor specifies 192 bytes internal RAM, a 64-byte register block,
no EEPROM and reset INIT=$00. `internal_map()` overlays the register view;
`init_w(0)` selects registers $0000-$003F and RAM $0040-$00FF.
The driver's banked external RAM therefore does not supply $0001 in this state.
The driver also selects high unmapped read values, but that is not the source
of the mapped register's value.

`mc68hc11d0_device::mc68hc11_reg_map()` explicitly maps offset $01 to
`reg01_r()`, which returns $FF without side effects. Its existing comment calls
the register reserved and attributes the AS2000 read/write-back to TFLG1 to a
programming error. This is a source assertion to test, not verified hardware
behavior. The driver TODO's word “nonexistent” does not mean currently unmapped.
No production change is justified by the TODO alone.

TFLG1 at register offset $23 is write-one-to-clear: `tflg1_w(data)` clears
matching flag bits and corresponding pending interrupt state. A write of $FF
would clear all modeled TFLG1 flags. Whether a particular $0001 read supplies
that write, and whether this matters to execution, needs instruction context.
Register relocation can change the mapping; capture records INIT on each access.

## Available firmware

Private `asma2k.zip` contains both declared BIOS files, each 33,253 bytes:

| MAME BIOS | Firmware | ZIP CRC32 | Declared SHA1 |
| --- | --- | --- | --- |
| v314 | v3.1.4 | 49487f6d | e0b777dc68c671c31ba808e214fb9d2573b9a853 |
| v308 | v3.0.8 | 0b3b1a0c | 97878819188a1ec40052fbce9d5a5059728d5aec |

ZIP metadata was inspected in PREPARE; the validator performs MAME ROM audits.
No ROM content or disassembly is included here.

## Reproduce locally

Run `/home/spc/Projects/alphasmart/tools/automation/state/as2k-local-validator.sh`.
Optional `AS2K_ROMPATH` overrides the canonical private ROM directory.
The executable snapshots tracked source plus the new debugger script outside
Git, builds production and existing CI instrumentation, and validates each build.
It runs both BIOSes from fresh NVRAM for 15 emulated seconds using:

```
-debug -debugger none -debugscript scripts/as2k_register_0001.cmd
```

The validator supplies the absolute snapshot script path and private output
paths. Each capture has a 180-second wall limit and 512 MiB per-file limit.
The corrected launcher attaches before an explicitly requested soft reset. No keys are injected in these
captures, so accesses belong to boot/idle rather than an explicit user action.
The script records actual read-tap data as `wpdata` (see `debug/points.cpp` and
`debugcpu.h`; the watchpoint manual only documents this for writes), PC, INIT,
SP and A/B, plus TFLG1 writes. PC is the CPU state at the access and may already
have advanced through instruction operands; use the private instruction trace
to identify the accessing instruction and caller. The full trace also records
register state before instructions, permitting review of downstream data flow.

Artifacts stay under external `state/as2k-validation.*`. Instruction traces are
private and must never be committed or copied into public documentation.
Capture checks require a real read returning $FF at INIT register page zero for
each BIOS, TFLG1 activity and a nonempty context trace. A mismatch fails for review;
absence of a read is incomplete evidence. Merely seeing both accesses is not
proof that the read supplied the write. Review must report derived PCs, call
context and consequence before declaring this investigation complete.

The existing v314 idle-gated keyboard/LCD/F1/F8/NVRAM/restart-recall test is
preserved for the instrumented baseline. No synthetic wake build is needed for
this register task. Successful runtime and regression results are recorded below.

## REVIEW: failed capture and correction

The 2026-09-12T17:15:02-06:00 validation exited 1 with no v314 read.
Both BIOS artifact directories lacked context-private.trace; their console
logs showed normal bounded completion, not a successful diagnostic attachment.
Source explains the failure: debug_none::wait_for_debugger() calls go() before
process_source_file(), whose loop requires the CPU to remain stopped. Thus the
queued -debugscript commands did not execute. This does not refute the TODO.

The reproduction above describes the failed initial method. The corrected
external validator instead generates a Lua launcher from the same command file,
executes each diagnostic command directly using debugger:command(), emits a
setup marker, and requests one soft reset. A guard prevents repeated setup/reset
when autoboot fires again. Both BIOS runs retain fresh NVRAM directories, 15 s
emulated duration, 180 s wall and 512 MiB file limits. Capture assertions now
require the setup marker and trace file before checking firmware reads.
This correction passed LOCAL execution at 2026-09-12T18:04:09-06:00. It captures reset/boot/idle after
an explicit soft reset; it does not prove observation of the initial cold reset
or that pre-attachment execution has no effect on RAM. No user input is injected.
The keyboard/LCD/recall gates were not reached in the failed run; they passed
in the corrected run. No production source correction is justified.

## Evidence boundaries

No CPU, video, driver, mapping or workflow changes. Diagnostics are opt-in.
Physical reserved-register values remain unverified; there is no new hardware
source. PREPARE read the mandatory private knowledge base. Its blank-LCD and
candidate-core notes are older than the successful STOP/XIRQ REVIEW in the prior
CODEX_RESULT (available at parent b1f08e607ec); preserve that provenance conflict.
The prior regression passed `az09=+`, F1/F8 and restart recall. Synthetic 1.25 s
XIRQ timing remains unverified, and physical wake modeling is outside scope.

## Accepted runtime evidence

Summary: 2026-09-12T18:04:09-06:00, exit 0. Private artifacts:
`/home/spc/Projects/alphasmart/tools/automation/state/as2k-validation.4Rf2NAKT`.
REVIEW inspected only targeted context around the reads and capture markers;
no firmware bytes or disassembly are reproduced here.

| BIOS | Caller PC / routine entry | Read instruction PC / access PC | Value / INIT | Following TFLG1 instruction PC / access PC | Idle STOP |
| --- | --- | --- | --- | --- | --- |
| v314 | $808A / $87BB | $87C6 / $87C8 | $FF / $00 | $87C8 / $87CA | $87D7 |
| v308 | $8082 / $87C6 | $87D1 / $87D3 | $FF / $00 | $87D3 / $87D5 | $87E2 |

Each BIOS produced exactly one observed read, after soft reset during entry to
idle from boot, with no user input. SP at the read was $00FD. The private trace
shows A changing from $0C to $FF and the next instruction storing that same
value to TFLG1 ($0023); the write watchpoint independently records $FF.
Execution then reaches STOP with CCR=$40 in both BIOSes. Thus the returned value
is used as a write-one-to-clear mask for all modeled TFLG1 flags, not as a branch
condition in this observed sequence. The trace does not measure which timer
flags were set immediately before the write, hardware reserved-register values,
or alternate wake/user-action paths. Calling the firmware a programming error
remains an unverified interpretation of the existing core comment.

The v314 PC/CCR and idle routine agree with canonical private STOP evidence.
No new contradiction was found. The private blank-LCD/candidate-core notes
conflict in checkpoint provenance with the prior published successful REVIEW;
the new baseline again passes decoded `az09=+`, F1/F8, saved file texts and
restart recall of `memory8`. This is ASCII bus reconstruction, not CGROM pixel
validation or exhaustive RAM coverage. Both BIOS ROM audits and production /
instrumented build validation passed, as did all nine isolated STOP/XIRQ checks.
The baseline smoke gate is v314 only. Existing CGROM NEEDS REDUMP persists.

Disposition: diagnostic question answered; no speculative register change.
Hardware fidelity remains OPEN/DEFERRED pending measured reserved-register
read values or an authoritative hardware specification, or a reproducible
observable defect linked to the read. Initial cold-reset capture and exhaustive
firmware paths are not established. Physical XIRQ source/period and firmware-
backed unmasked XIRQ remain open; synthetic 1.25 s timing is not hardware proof.
