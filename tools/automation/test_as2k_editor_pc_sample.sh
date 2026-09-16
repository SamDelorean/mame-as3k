#!/usr/bin/env bash
set -u

# Temporary deterministic diagnostic used to identify a stable AS2000 editor PC.
# It does not modify firmware and does not invoke Codex.
# The diagnostic build must emit lines of the form:
#   AS2K_EDITOR_SAMPLE PC=XXXX
# This harness ranks the observed PCs so a stable editor marker can be selected
# from measured runtime evidence rather than guessed from disassembly.

ROOT="${AS2K_EMU_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null)}"
MAME="${AS2K_DIAG_BIN:-$ROOT/as2kdiag}"
ROM="${AS2K_ROM:-$HOME/Projects/alphasmart/private/roms/as2k/AS2000_v3.1.4.bin}"
CGROM_ZIP_DIR="${AS2K_CGROM_ZIP_DIR:-$HOME/Projects/alphasmart/private/cgrom}"
CGROM_RAW_DIR="${AS2K_CGROM_RAW_DIR:-$HOME/Projects/alphasmart/private/roms/ks0066}"
MACHINE="${AS2K_MACHINE:-asma2k}"
TIMEOUT_SEC="${AS2K_EDITOR_SAMPLE_TIMEOUT:-12}"
EXPECTED_SHA1="e0b777dc68c671c31ba808e214fb9d2573b9a853"

[[ -x "$MAME" ]] || { echo "BLOCKED as2kdiag_missing path=$MAME"; exit 20; }
[[ -f "$ROM" ]] || { echo "BLOCKED private_rom_missing path=$ROM"; exit 20; }
if [[ ! -f "$CGROM_ZIP_DIR/ks0066.zip" && ! -f "$CGROM_RAW_DIR/ks0066_f05.bin" ]]; then
  echo "BLOCKED ks0066_missing"
  exit 20
fi
actual_sha1="$(sha1sum "$ROM" | awk '{print $1}')"
[[ "$actual_sha1" == "$EXPECTED_SHA1" ]] || { echo "BLOCKED wrong_rom_sha1 actual=$actual_sha1"; exit 20; }

ROMPATH="$(dirname "$ROM");$CGROM_ZIP_DIR;$CGROM_RAW_DIR"
state_base="${XDG_STATE_HOME:-$HOME/.local/state}/as2k-emulator-automation/editor-pc-sample"
mkdir -p "$state_base"
run_dir="$(mktemp -d "$state_base/run.XXXXXX")" || { echo "BLOCKED mktemp_failed"; exit 20; }
log="$run_dir/error.log"
ranked="$run_dir/ranked-pcs.txt"

set +e
timeout "$TIMEOUT_SEC" "$MAME" "$MACHINE" \
  -rompath "$ROMPATH" -window -skip_gameinfo -nothrottle \
  -seconds_to_run "$TIMEOUT_SEC" >"$log" 2>&1
rc=$?
set -e

samples="$(grep -c 'AS2K_EDITOR_SAMPLE PC=' "$log" 2>/dev/null || true)"
if [[ "$samples" -eq 0 ]]; then
  echo "HARNESS_REQUIRED editor_pc_sampler_not_built log=$log"
  exit 30
fi

grep -oE 'AS2K_EDITOR_SAMPLE PC=[0-9A-Fa-f]{4}' "$log" \
  | sed 's/.*PC=//' \
  | tr '[:lower:]' '[:upper:]' \
  | sort | uniq -c | sort -nr >"$ranked"

echo "EDITOR_PC_SAMPLE samples=$samples ranked=$ranked"
head -n 20 "$ranked"

if [[ "$rc" -ne 0 && "$rc" -ne 124 ]]; then
  echo "BLOCKED mame_rc=$rc log=$log"
  exit 20
fi
exit 0
