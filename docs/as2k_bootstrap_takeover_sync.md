# AS2000 bootstrap diagnostic — synchronized status

Canonical takeover state is tracked in:

```
SamDelorean/AS2K-DictROM-Takeover
docs/SYNC_CHECKPOINT_2026-09-29.md
```

Current branch:

```
as2k-mame0289-dev
```

Completed preparation:

- increment 1: HC11D0 HPRIO + post-bootstrap injection — STATIC PASS;
- increment 2: `asma2kbt` activation + exact stage0 guard + zero-Z guard — STATIC PASS;
- increment 3: runnable diagnostic ROM set — STATIC PASS.

Frozen runtime inputs:

```
stage0: 27 bytes
SHA1   ab76eafa386311b2ab70ea644345fb15767e908f

BT8 DictROM: 131072 bytes
SHA1   6e84689da9e0705920bca24137c88a387dedfac1
```

Next action is native compile/runtime only. Do not add DebugTool or enlarge the
HC11 bootstrap model before this path runs.

Required runtime result:

```
INJECT $0040
HPRIO C0 -> E0 -> 60 -> 20
FIRST_DICT_FETCH $4000 bank0
zero stock-Z execution
PC $4010
internal RAM $00A0-$00A3 = "BT8!"
```

Native run is currently pending because the remote Commander execution quota is
unavailable.
