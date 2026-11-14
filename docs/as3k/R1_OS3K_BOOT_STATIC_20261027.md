# R1 — OS3K 2005 / AS3000 static boot contract
Date: 2026-10-27
Status: STATIC_PASS. No MAME/CPU execution; no driver edits.

## Canonical inputs
Drive ALPHASMART/SmartApplets3000/SMALLOS3KROM.OS3KOS (14,998 bytes)
Drive ALPHASMART/SmartApplets3000/OS3KROM.OS3KOS (277,488 bytes)
Private Flash images (1,048,576 bytes each): OS3K_MIN_BOOT.bin, OS3K_MIN_WRITER.bin.

## Manufacturer installation layout (file fields at +0x50)
Small ROM at 0x00400000 (vectors SP=0x0003FFF0, PC=0x0040042A).
System segment 1 at 0x004C0000 for 0x40000 bytes.
System segment 2 at 0x00404000 for 0x3BEC bytes.
The two images include these positions byte-for-byte. The WRITER additionally
contains the original 101,596-byte AlphaWordPlus.OS3KApp at 0x00408000,
the System3 sequential applet scan entry.

## Static control-flow proof
Small ROM 0x0040042A → DragonBall EZ GPIO/chip-select/IRQ initialization;
0x0040054C JMP 0x00400A90.
0x00400A90 performs a four-key chord test and optional password/recovery path.
0x00400AD2 validates installed System3:
  - reads trailer at 0x004FFFC0: word at +4 is 0x0301 (>0x0300);
  - compares installed name at 0x004FFFC6 to Small ROM string 0x004037C8, both 'System 3';
  - invokes 0x004FFFA2, a JMP to System3 0x004C332E;
  - that helper compares the installed name to secondary image name at 0x00407BD8.
The matching names and both trampoline opcodes have been confirmed in both
generated Flash images and in the original package. The normal success
path calls 0x00400BD8 → JSR 0x004FFF98 →
  ANDI.W #0xF0FF,SR; JMP 0x004C007A →
  SP=0x0003FFF0; A6=0; JMP 0x004C7390.
System3 at 0x004C7390 runs initialization. Its path later invokes
0x004C0068 → 0x004C7B82, which references the runtime applet
pointer table at 0x00000D82 and dispatches installed SmartApplets.

## Static acceptance / limitations
PASS (static): reset vector, code targets, installed revision 0x0301,
primary and secondary matching identifiers, exact flash segment addresses,
and executable System3 entry all exist consistently in BOOT and WRITER.
NO CPU_EXEC_PASS or MAME_BOOT_PASS is claimed; GPIO/USB/cpu/chip-select
side effects not exercised here, and the branch could still stall at runtime.
This increment did not run MAME, modify the ROMs, rebuild, change source,
start Codex, commit, push or touch the physical device.

## Follow-on
R2: introduce two distinct private OS3K variant ROM sets in the *existing*
mame-as3k-dev driver with faithful data and hashes, retaining the historical
asma3k and its diagnostic sets; do not replace the original physical dump.
Then R3: PAPUEN 0xFFFFF402 and SCR.WDTH8 stage already scoped in
docs/as3k/CODEX_NEXT.md. Avoid rerunning previously completed fixtures
without a specific regression reason.
