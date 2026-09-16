#!/usr/bin/env bash
set -euo pipefail
ROOT="${AS2K_EMU_ROOT:-$(git rev-parse --show-toplevel)}"
cd "$ROOT"

[ "$(git branch --show-current)" = "as2k-mame0289-dev" ] || { echo "BLOCKED: switch to as2k-mame0289-dev"; exit 40; }
[ -z "$(git status --porcelain)" ] || { echo "BLOCKED: checkout has local changes"; exit 41; }
for c in git codex systemctl flock; do command -v "$c" >/dev/null || { echo "BLOCKED: missing $c"; exit 42; }; done

# Keep the user service alive after logout when the host permits it.
loginctl enable-linger "$USER" 2>/dev/null || true
bash tools/automation/install_as2k_emulator_timer.sh
systemctl --user start as2k-emulator.service

echo "STARTED_OK"
echo "status: systemctl --user status as2k-emulator.timer as2k-emulator.service"
echo "logs:   journalctl --user -u as2k-emulator.service -f"
