# AS2000 PB4/EXT MAME test plan

Status: DEFERRED_2026_10
Prepared: 2026-09-28

This file intentionally defines tests without implementing or running them. Project direction is to wait until October 2026 before MAME validation of PB4/EXT.

## Static prerequisites already closed

- PB4 is stock CS4.
- CS4 slots 26/27 are both disabled/impossible.
- CPBF raw 0x81E3 is 0xFF stock.
- PB4 MCU-I/O target is CPBF=0x10 / raw 0xEF.
- Port B runtime addresses are Pin=0x3003, Direction=0x3005, Data=0x3007.
- PB4 mask is 0x10.
- Direction and Data reset to 0.

## Runtime model to add later

Do not implement in September 2026.

The future diagnostic/profile implementation should model:
1. PB4 function selected as MCU-I/O only for the extended-hardware profile.
2. PB4 direction and data registers.
3. EXT pin level, including reset bias LOW while DIR4=0.
4. eight RAM banks selected by EXT:PA5:PA4.
5. sixteen AppROM pages selected by EXT:PA5:PA4:CTRL7.

## Frozen test sequence for October 2026

1. Reset: EXT observed 0.
2. Verify DATA4=0 and DIR4=0.
3. Write DATA4=0 while still input.
4. Change DIR4 0->1; verify EXT remains 0 with no high transition.
5. Set DATA4=1; verify EXT=1.
6. Clear DATA4; verify EXT=0.
7. EXT=0: verify legacy RAM banks 0-3 unchanged.
8. EXT=1: verify new RAM banks 4-7 independently selectable.
9. EXT=0: verify legacy DictROM/AppROM pages 0-7 unchanged.
10. EXT=1: verify pages 8-15 independently selectable.
11. Exercise PA6 RAM-vs-I/O/Dict switching under both EXT states.
12. Exercise sleep/wake and power-off sequencing.
13. Re-run keyboard, LCD, file-memory, Send/Print/host regressions relevant to the active profile.
14. Verify ordinary stock profile never observes PB4/EXT behavior.

## Fail conditions

The gate fails if:
- EXT is not deterministically 0 at reset;
- DIR enable creates a high glitch;
- any legacy bank/page changes when EXT=0;
- PA6 view selection breaks;
- sleep/wake or power-off loses deterministic EXT state;
- stock-profile behavior changes.

No pass/fail state may be recorded until these tests are actually executed in October 2026 or later.
