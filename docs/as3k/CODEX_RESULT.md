# Codex result — dynamic validation of `SystemInstallExceptionVectors`

Date: 2026-08-26

Status: **complete; all pass criteria met**. No MAME source, core, ROM definition, fixture, generated binary, or proprietary artifact was changed.

## Repository gate

- `git pull --ff-only as3k-project as3k-mame0289-dev`: already up to date.
- Branch: `as3k-mame0289-dev`.
- Initial tracked `git status --short`: clean.
- Static-study commit `4b3f5e065fdf8e694dbe86d1cafb819a67ec4cb0` is an ancestor of `HEAD`.
- Initial `git diff --check`: passed.
- Existing `./alphasma3k` executable was present; no rebuild was performed.

## Command and local-only script

Created and retained outside the repository:

- `../diagnostic/as3kdv_exception_vectors.cmd`
- `../diagnostic/as3kdv_exception_vectors_console.log`
- `../diagnostic/as3kdv_exception_vectors.log`

The script was the exact plan from `CODEX_NEXT.md`; no debugger-syntax correction was required. All numeric literals in it use explicit `0x...` notation.

Executed only:

```sh
./alphasma3k asma3kdv \
  -debug \
  -debugscript ../diagnostic/as3kdv_exception_vectors.cmd \
  -log \
  -seconds_to_run 8
```

Console termination was `Exited via the debugger`.

## Observations

Installer entry was reached. The debugger logged `PC=0x00430506`, `A7=0x0003ffd4`, and `SR=0x2014`; the reported PC is +2 from the breakpoint address, matching the documented debugger behavior.

Exactly these eight watchpoint transactions occurred after entry:

| Vector | Transaction | Address | Data | Reported PC |
|---|---:|---:|---:|---:|
| 2 | high | `0x00000008` | `0x0043` | `0x0043050c` |
| 2 | low | `0x0000000a` | `0x0526` | `0x0043050c` |
| 3 | high | `0x0000000c` | `0x0043` | `0x00430514` |
| 3 | low | `0x0000000e` | `0x05b0` | `0x00430514` |
| 4 | high | `0x00000010` | `0x0043` | `0x0043051c` |
| 4 | low | `0x00000012` | `0x063a` | `0x0043051c` |
| 5 | high | `0x00000014` | `0x0043` | `0x00430524` |
| 5 | low | `0x00000016` | `0x06c4` | `0x00430524` |

Thus every architectural `MOVE.L` appeared as the expected big-endian high-word transaction followed by its low-word transaction. The reconstructed values are:

- `0x00000008 = 0x00430526` (`SystemBusError`)
- `0x0000000c = 0x004305b0` (`SystemAddressError`)
- `0x00000010 = 0x0043063a` (`SystemIllegalInstruction`)
- `0x00000014 = 0x004306c4` (`SystemDivideByZeroError`)

The RTS breakpoint at `0x00430524` was reached. Its memory expressions returned exactly those four final longwords. At RTS the preserved vectors were:

- TRAP 0, `0x00000080 = 0x00400178`
- level 4, `0x00000110 = 0x00431a04`
- level 5, `0x00000114 = 0x00431ab0`
- level 6, `0x00000118 = 0x00431ae6`

The level 4/5/6 values exactly match the previously validated values. The same four installed and four preserved vector values were still present at the keyboard-entry stop.

No breakpoint for `SystemBusError`, `SystemAddressError`, `SystemIllegalInstruction`, or `SystemDivideByZeroError` fired.

`KeyboardInitializeModule` entry was reached before timeout. The breakpoint at `0x0042e2a8` logged `PC=0x0042e2aa`, the documented +2 presentation, and immediately executed `quit`. No breakpoint was placed inside the routine and no keyboard instruction was executed or reverse engineered.

## Publication

- Final `git diff --check`: passed.
- Only `docs/as3k/CODEX_RESULT.md` was selected for commit.
- No diagnostic script/log, MAME source, ROM, fixture, generated binary, or proprietary artifact was staged.
- Documentation commit: this result commit; its SHA and push outcome are reported in the final task handoff because a commit cannot contain its own SHA.
- Push target: `as3k-project/as3k-mame0289-dev`.
