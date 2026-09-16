#!/usr/bin/env bash
set -u

# Deterministic BOOT_EDITOR harness for the AS2000 evidence matrix.
# Private ROM/NVRAM paths stay local. This script never invokes Codex.
# Contract: 0 validated, 10 behavioral mismatch, 20 infrastructure/private input,
# 30 observation harness unavailable.

ROOT="${AS2K_EMU_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null)}"
MAME="${AS2K_DIAG_BIN:-$ROOT/as2kdiag}"
ROM="${AS2K_ROM:-$HOME/Projects/alphasmart/private/roms/as2k/AS2000_v3.1.4.bin}"
CGROM_ZIP_DIR="${AS2K_CGROM_ZIP_DIR:-$HOME/Projects/alphasmart/private/cgrom}"
CGROM_RAW_DIR="${AS2K_CGROM_RAW_DIR:-$HOME/Projects/alphasmart/private/roms/ks0066}"
MACHINE="${AS2K_MACHINE:-asma2k}"
TIMEOUT_SEC="${AS2K_BOOT_TIMEOUT:-20}"
EXPECTED_SHA1="e0b777dc68c671c31ba808e214fb9d2573b9a853"

if [[ -z "$ROOT" || ! -d "$ROOT" ]]; then
  echo "BLOCKED repo_root"
  exit 20
fi
if [[ ! -x "$MAME" ]]; then
  echo "BLOCKED as2kdiag_missing path=$MAME"
  exit 20
fi
if [[ ! -f "$ROM" ]]; then
  echo "BLOCKED private_rom_missing path=$ROM"
  exit 20
fi
if [[ ! -f "$CGROM_ZIP_DIR/ks0066.zip" && ! -f "$CGROM_RAW_DIR/ks0066_f05.bin" ]]; then
  echo "BLOCKED ks0066_missing checked=$CGROM_ZIP_DIR/ks0066.zip,$CGROM_RAW_DIR/ks0066_f05.bin"
  exit 20
fi
command -v sha1sum >/dev/null 2>&1 || { echo "BLOCKED sha1sum_missing"; exit 20; }
command -v timeout >/dev/null 2>&1 || { echo "BLOCKED timeout_missing"; exit 20; }

actual_sha1="$(sha1sum "$ROM" | awk '{print $1}')"
if [[ "$actual_sha1" != "$EXPECTED_SHA1" ]]; then
  echo "BLOCKED wrong_rom_sha1 actual=$actual_sha1 expected=$EXPECTED_SHA1"
  exit 20
fi

# Known t640 private layout. Keep all three roots because MAME may resolve the
# machine archive and the KS0066 device ROM through different directory forms.
ROMPATH="$(dirname "$ROM");$CGROM_ZIP_DIR;$CGROM_RAW_DIR"

state_base="${XDG_STATE_HOME:-$HOME/.local/state}/as2k-emulator-automation/boot-editor"
mkdir -p "$state_base"
run_dir="$(mktemp -d "$state_base/run.XXXXXX")" || { echo "BLOCKED mktemp_failed"; exit 20; }
log="$run_dir/error.log"

# Editor readiness is established by the measured AS2000 idle criterion used
# successfully by the automated editor exercise: 60 consecutive frames at
# firmware idle/STOP PC $87D7 with the natural-keyboard queue empty.
probe="$run_dir/editor_ready.lua"
cp /tmp/as2k_editor_ready.lua "$probe"

set +e
(
  cd "$run_dir"
  timeout "$TIMEOUT_SEC" "$MAME" "$MACHINE" \
    -rompath "$ROMPATH" \
    -autoboot_script "$probe" \
    -video none -sound none -skip_gameinfo -nothrottle -log \
    -seconds_to_run "$TIMEOUT_SEC" >/dev/null 2>&1
)
rc=$?
set -e
log="$run_dir/error.log"

if grep -q 'AS2K_GATE1A EDITOR_READY' "$log"; then
  echo "VALIDATED BOOT_EDITOR marker=$(grep 'AS2K_GATE1A EDITOR_READY' "$log" | head -n 1)"
  exit 0
fi

if grep -q 'ks0066_f05.bin NOT FOUND' "$log"; then
  echo "BLOCKED BOOT_EDITOR ks0066_not_resolved rompath=$ROMPATH log=$log"
  exit 20
fi

# A timeout is normal for an emulated machine that keeps running; without a
# readiness marker it means this harness cannot yet prove editor readiness.
if [[ "$rc" -eq 124 || "$rc" -eq 0 ]]; then
  echo "HARNESS_REQUIRED BOOT_EDITOR no_editor_readiness_marker log=$log"
  exit 30
fi

echo "BLOCKED BOOT_EDITOR mame_rc=$rc log=$log"
exit 20
