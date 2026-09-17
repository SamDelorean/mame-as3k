#!/usr/bin/env bash
set -euo pipefail
ROOT="${AS2K_EMU_ROOT:-$(git rev-parse --show-toplevel)}"
UNIT="$HOME/.config/systemd/user"
mkdir -p "$UNIT"

cat > "$UNIT/as2k-emulator.service" <<EOF_SERVICE
[Unit]
Description=AS2K emulator unattended productive cycle
After=network-online.target

[Service]
Type=oneshot
WorkingDirectory=$ROOT
Environment=AS2K_EMU_ROOT=$ROOT
ExecStart=/bin/bash $ROOT/tools/automation/run_as2k_emulator_supervised.sh
EOF_SERVICE

cat > "$UNIT/as2k-emulator.timer" <<'EOF_TIMER'
[Unit]
Description=Run AS2K emulator worker 10 minutes after each completed cycle

[Timer]
OnBootSec=2min
OnUnitInactiveSec=10min
AccuracySec=30s
Persistent=true
Unit=as2k-emulator.service

[Install]
WantedBy=timers.target
EOF_TIMER

STATE="${XDG_STATE_HOME:-$HOME/.local/state}/as2k-emulator-automation"
if [ ! -d "$STATE/quota/current" ]; then
  rm -f "$UNIT/as2k-emulator-quota-resume.timer"
fi

systemctl --user daemon-reload
systemctl --user enable --now as2k-emulator.timer
