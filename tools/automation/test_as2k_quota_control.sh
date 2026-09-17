#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
CONTROL="$ROOT/tools/automation/as2k_quota_control.sh"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

mkdir -p "$TMP/bin" "$TMP/home/.config/systemd/user" "$TMP/home/.local/state"
cat > "$TMP/bin/systemctl" <<'EOF_SYSTEMCTL'
#!/usr/bin/env bash
printf 'systemctl %s\n' "$*" >> "$MOCK_LOG"
exit 0
EOF_SYSTEMCTL
chmod +x "$TMP/bin/systemctl"

REPO="$TMP/repo"
mkdir -p "$REPO"
cd "$REPO"
git init -q
git config user.email test@example.invalid
git config user.name 'AS2K quota test'
printf 'base\n' > tracked.txt
git add tracked.txt
git commit -qm base
printf 'changed\n' >> tracked.txt
printf 'new\n' > untracked.txt

export HOME="$TMP/home"
export XDG_CONFIG_HOME="$TMP/home/.config"
export XDG_STATE_HOME="$TMP/home/.local/state"
export MOCK_LOG="$TMP/systemctl.log"
export PATH="$TMP/bin:$PATH"
export AS2K_EMU_ROOT="$REPO"

bash "$CONTROL" import-pause 'Sep 19th, 2026 7:00 AM'
out=$(bash "$CONTROL" status)
printf '%s\n' "$out" | grep -q '^QUOTA_PAUSED '
test -f "$TMP/home/.config/systemd/user/as2k-emulator-quota-resume.timer"
grep -q '^OnCalendar=2026-09-19 07:05:00$' "$TMP/home/.config/systemd/user/as2k-emulator-quota-resume.timer"
test -s "$TMP/home/.local/state/as2k-emulator-automation/quota/current/fingerprint"
test -s "$TMP/home/.local/state/as2k-emulator-automation/quota/current/checkpoint"
checkpoint=$(cat "$TMP/home/.local/state/as2k-emulator-automation/quota/current/checkpoint")
test -f "$checkpoint/worktree.patch"
test -f "$checkpoint/files.tar.gz"
grep -q 'disable --now as2k-emulator.timer' "$TMP/systemctl.log"
grep -q 'enable --now as2k-emulator-quota-resume.timer' "$TMP/systemctl.log"

echo 'VALIDATED AS2K quota pause checkpoint and resume timer'
