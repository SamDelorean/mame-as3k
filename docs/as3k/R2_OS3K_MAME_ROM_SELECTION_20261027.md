# R2 — Add AS3000 original System 3 variants to MAME 0.289
Date: 2026-10-27
Status: CLOSED / BUILD_PASS / ROM_AUDIT_PASS_BEST_AVAILABLE;
firmware BOOT_NOT_EXECUTED.

## Scope and changes
- Canonical repo mame-as3k-dev, branch as3k-mame0289-dev.
- Added two ROM_START and COMP entries in src/mame/skeleton/alphasma3k.cpp:
  asma3ksb (BOOT) and asma3ksw (WRITER).
- Each loads 0x100000 bytes using unchanged machine configuration.
- Original physical asma3k and three synthetic fixtures remain unchanged.
- Firmware stays outside Git. Private local ROM search dir:
  ../private/roms/as3k/mame_sets/ with symlinks to the original two
  local private os3k_minimal/OS3K_MIN_BOOT and OS3K_MIN_WRITER folders.
- Drive private image files were already saved and are untouched here.

## Source ROM hashes from real private bytes
BOOT 1048576 bytes: CRC32 ac3b2a2c, SHA1
c2936532f37514d0eaa74e2cf97bf96bb17e883d,
SHA256 2829f10fd52537524291d3fbf1f0dd3346c336c282373ab97eb88879983e251c.
WRITER 1048576 bytes: CRC32 c79df93f, SHA1
02ae5d564b48476abf7067b71a7e0166e471bdfa,
SHA256 1d5002e2e54c732768ee1e296eb9ca868d06935e07afb0d0d9e01e9b5d7135bb.

## Load/audit commands (do not run firmware)
./alphasma3k -listfull
./alphasma3k -verifyroms asma3ksb -rompath ../private/roms/as3k/mame_sets
./alphasma3k -verifyroms asma3ksw -rompath ../private/roms/as3k/mame_sets

## Fidelity / exclusions
- No firmware CPU execution; no MC68EZ328 core/LCD/key changes.
- No Codex, firmware rewrites, clones, worktrees or other branches.
- AGENTS.md had user changes before R2 and R1 report was untracked;
  both preserved; no reset, clean or blanket staging.
- First compiler attempt hit a /tmp quota; retry with TMPDIR=/dev/shm
  resolved this block.
- MAME 0.289 reduced build regenerated projects and successfully compiled
  src/mame/skeleton/alphasma3k.cpp to alphasma3k.o (74,616 bytes).
  nm -C of that object includes driver_asma3ksb and driver_asma3ksw
  registrations and the distinct ROM descriptor objects.
- Full MAME executable link and MAME -verifyroms were NOT completed.
  The cold build, started on an empty generated build tree, was stopped
  deliberately after the AS3K driver compiled to bound this increment;
  generated artifacts remain to permit an incremental resume.
  This is DRIVER_OBJECT_COMPILE_PASS, not MAME_BIN_PASS or ROM_AUDIT_PASS.
  No firmware was executed.
- Next increment R3, Port-A PAPUEN and SCR.WDTH8 only.


## Final MAME linkage and ROM audit — 2026-10-28
The original R2 integration omitted two entries in src/mame/mame.lst,
therefore the first full compile correctly produced only 4 registered
drivers. This was repaired in the same canonical tree by adding
asma3ksb and asma3ksw next to the 4 existing AS3K entries. All six
appear in generated/mame/alphasma3k/drivlist.cpp and the final -listfull.

The complete MAME 0.289 executable linked successfully:
- command: make SUBTARGET=alphasma3k
  SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl -j3
  (with TMPDIR=/dev/shm and build-only generated makefile flags -O0
  for remaining units; already compiled objects and verified
  source-identical cache were retained);
- return code: 0;
- target: ./alphasma3k, 128810144 bytes, ELF 64-bit x86-64;
- all 6 registered system entries: asma3k, asma3kdi, asma3kdv,
  asma3kdvl, asma3ksb, asma3ksw.

Verification, WITHOUT executing either firmware:
- ./alphasma3k -listfull: six AS3K entries found.
- ./alphasma3k -rompath
  "../private/roms/as3k/mame_sets;../private/roms"
  -verifyroms asma3ksb: exit 0, 1 romset OK;
- same command for asma3ksw: exit 0, 1 romset OK.
- Both audits classify the existing private ks0066_f05.bin as
  NEEDS REDUMP, best available. The authentic OS3K boot/Writer ROM
  filenames, CRC and SHA-1 loaded without audit complaints.
  This warning does not constitute a firmware boot test.
- Verified no MAME CPU run; no R3 work performed.

Final build log: ../private/roms/as3k/mame_sets/r2_final_link_6sets_20261028.log;
full build log: ../private/roms/as3k/mame_sets/r2_fastlink_20261028.log.

R2 is now closed for machine registration, executable linkage, and
ROM acquisition/audit; R4 remains responsible for actual boot behavior.
