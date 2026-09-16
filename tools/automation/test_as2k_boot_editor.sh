#!/usr/bin/env bash
set -u

# Deterministic BOOT_EDITOR harness for the AS2000 evidence matrix.
# Private ROM/NVRAM paths stay local. This script never invokes Codex.
# Contract: 0 validated, 10 behavioral mismatch, 20 infrastructure/private input,
# 30 observation harness unavailable.

ROOT="${AS2K_EMU_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null)}"
MAME="${AS2K_DIAG_BIN:-$ROOT/as2kdiag}"
ROM="${AS2K_ROM:-$HOME/Projects/alphasmart/private/roms/as2k/AS2000_v3.1.4.bin}"
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
command -v sha1sum >/dev/null 2>&1 || { echo "BLOCKED sha1sum_missing"; exit 20; }
command -v timeout >/dev/null 2>&1 || { echo "BLOCKED timeout_missing"; exit 20; }

actual_sha1="$(sha1sum "$ROM" | awk '{print $1}')"
if [[ "$actual_sha1" != "$EXPECTED_SHA1" ]]; then
  echo "BLOCKED wrong_rom_sha1 actual=$actual_sha1 expected=$EXPECTED_SHA1"
  exit 20
fi

state_base="${XDG_STATE_HOME:-$HOME/.local/state}/as2k-emulator-automation/boot-editor"
mkdir -p "$state_base"
run_dir="$(mktemp -d "$state_base/run.XXXXXX")" || { echo "BLOCKED mktemp_failed"; exit 20; }
log="$run_dir/error.log"

# This first harness deliberately uses existing diagnostic PC instrumentation only.
# A usable editor is accepted only if the run demonstrates the known editor/input
# observation marker. Mere process survival or ROM boot is not enough.
set +e
timeout "$TIMEOUT_SEC" "$MAME" "$MACHINE" \
  -rompath "$(dirname "$ROM")" \
  -window -skip_gameinfo -nothrottle -seconds_to_run "$TIMEOUT_SEC" \
  -verbose >"$log" 2>&1
rc=$?
set -e

if grep -Eq 'AS2K_AUTOMATION.*(EDITOR_READY|INPUT_START)|AS2K_GATE1A.*(EDITOR_READY|INPUT_START)' "$log"; then
  echo "VALIDATED BOOT_EDITOR marker=$(grep -E 'AS2K_AUTOMATION.*(EDITOR_READY|INPUT_START)|AS2K_GATE1A.*(EDITOR_READY|INPUT_START)' "$log" | head -n 1)"
  exit 0
fi

# A timeout is normal for an emulated machine that keeps running; without a
# readiness marker it means this harness cannot yet prove editor readiness.
if [[ "$rc" -eq 124 || "$rc" -eq 0 ]]; then
  echo "HARNESS_REQUIRED BOOT_EDITOR no_editor_readiness_marker log=$log"
  exit 30
fi

echo "BLOCKED BOOT_EDITOR mame_rc=$rc log=$log"
exit 20
