#!/usr/bin/env bash
set -u
ROOT="${AS2K_EMU_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
STATE="${XDG_STATE_HOME:-$HOME/.local/state}/as2k-emulator-automation"
mkdir -p "$STATE"
cd "$ROOT" || exit 30

retry() { local n=0; until "$@"; do n=$((n+1)); [ "$n" -ge 3 ] && return 1; sleep $((n*10)); done; }

# Never overwrite unknown local work. Only synchronize a clean checkout automatically.
if [ -n "$(git status --porcelain)" ]; then
  echo "BLOCKED_UNSAFE_GIT_STATE: local changes present" | tee -a "$STATE/supervisor.log"
  exit 31
fi

retry git fetch origin as2k-mame0289-dev || { echo "CONTROL_PLANE: fetch failed" | tee -a "$STATE/supervisor.log"; exit 32; }
LOCAL=$(git rev-parse HEAD)
REMOTE=$(git rev-parse origin/as2k-mame0289-dev)
BASE=$(git merge-base HEAD origin/as2k-mame0289-dev)
if [ "$LOCAL" = "$BASE" ]; then
  git merge --ff-only origin/as2k-mame0289-dev || exit 33
elif [ "$REMOTE" != "$BASE" ] && [ "$LOCAL" != "$REMOTE" ]; then
  echo "BLOCKED_UNSAFE_GIT_STATE: branch diverged" | tee -a "$STATE/supervisor.log"
  exit 34
fi

bash tools/automation/run_as2k_emulator_cycle.sh
rc=$?
printf '%s supervisor_exit=%s head=%s\n' "$(date -Is)" "$rc" "$(git rev-parse HEAD)" >> "$STATE/supervisor.log"
exit "$rc"
