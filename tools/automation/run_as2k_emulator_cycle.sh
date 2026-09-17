#!/usr/bin/env bash
set -euo pipefail

ROOT="${AS2K_EMU_ROOT:-$(git rev-parse --show-toplevel)}"
STATE="${XDG_STATE_HOME:-$HOME/.local/state}/as2k-emulator-automation"
LOCK="$STATE/cycle.lock"
mkdir -p "$STATE"
cd "$ROOT"

source tools/automation/as2k_quota_control.sh

exec 9>"$LOCK"
flock -n 9 || { echo "EMULATOR_CYCLE_ALREADY_RUNNING"; exit 0; }

for f in AGENTS.md docs/as2k/EMULATOR_AUTOMATION_CONTROL.md docs/as2k/EMULATOR_AUTOMATION_INBOX.md docs/as2k/EMULATOR_AUTOMATION_TASK.md docs/as2k/CODEX_NEXT.md docs/as2k/CODEX_RESULT.md docs/as2k/EMULATION_FINDINGS.md; do
  test -f "$f" || { echo "BLOCKED missing $f"; exit 20; }
done

command -v codex >/dev/null || { echo "BLOCKED codex missing"; exit 21; }
command -v git >/dev/null || { echo "BLOCKED git missing"; exit 22; }

RECOVERY_NOTE=""
if [ "${AS2K_EMU_QUOTA_RECOVERY:-0}" = "1" ]; then
  RECOVERY_NOTE=$(cat <<'EOF_NOTE'
A previous productive cycle hit the Codex usage limit after modifying the worktree. The supervisor has verified that the current dirty tree exactly matches the automation-owned quota checkpoint. Do not start a new task until that checkpoint is resolved. Inspect the existing changes, finish only that interrupted work, rerun its focused validation, and commit/push it only if verified and authorized. Never discard or overwrite the checkpoint. If the checkpoint is already clean/no-op, continue the previously authorized task from the control documents.
EOF_NOTE
)
fi

PROMPT=$(cat <<EOF_PROMPT
You are the unattended productive worker for the AlphaSmart 2000 MAME emulator repository. Read AGENTS.md, docs/as2k/EMULATOR_AUTOMATION_CONTROL.md, docs/as2k/EMULATOR_AUTOMATION_INBOX.md, docs/as2k/EMULATOR_AUTOMATION_TASK.md, docs/as2k/CODEX_NEXT.md, docs/as2k/CODEX_RESULT.md, and docs/as2k/EMULATION_FINDINGS.md before acting. Inspect recent commits and git status. ${RECOVERY_NOTE} Continue the highest-priority unresolved authorized emulator task. Diagnose first; repair recoverable harness/build/test defects in a bounded way; make the smallest evidence-backed source change; compile only when required with no more than -j3; run focused automated tests; distinguish emulator, harness, firmware, infrastructure and evidence failures; stop on unexplained regression. Never use destructive git recovery, never discard unknown human changes, never commit ROMs/NVRAM/binaries/raw proprietary traces, never touch the separate AS2K-V3.14.x checkout, and never implement DynFS here. Update CODEX_RESULT.md with concise reproducible evidence. Update EMULATION_FINDINGS.md only for material reusable findings. If verified changes are authorized and safe, commit them in English and push only to as2k-mame0289-dev. Do not create documentation churn when there is no material progress.
EOF_PROMPT
)

CODEX_LOG=$(mktemp "$STATE/codex-cycle.XXXXXX.log")
set +e
codex exec \
  --sandbox danger-full-access \
  -c 'approval_policy="never"' \
  -C "$ROOT" \
  "$PROMPT" 2>&1 | tee "$CODEX_LOG"
rc=${PIPESTATUS[0]}
set -e

if [ "$rc" -ne 0 ] && grep -Eqi "You've hit your usage limit|usage limit.*try again at|try again at .*usage" "$CODEX_LOG"; then
  as2k_quota_pause_from_log "$CODEX_LOG"
  printf '%s cycle_exit=75 class=QUOTA_PAUSED head=%s\n' "$(date -Is)" "$(git rev-parse HEAD)" >> "$STATE/history.log"
  rm -f "$CODEX_LOG"
  exit 75
fi

if [ "$rc" -ne 0 ]; then
  printf '%s cycle_exit=%s class=ANALYSIS_PUBLICATION head=%s\n' "$(date -Is)" "$rc" "$(git rev-parse HEAD)" >> "$STATE/history.log"
  rm -f "$CODEX_LOG"
  exit "$rc"
fi

if [ "${AS2K_EMU_QUOTA_RECOVERY:-0}" = "1" ]; then
  if [ -n "$(git status --porcelain)" ]; then
    if ! as2k_quota_reschedule_incomplete "$CODEX_LOG"; then
      printf '%s cycle_exit=76 class=QUOTA_RECOVERY_BLOCKED head=%s\n' "$(date -Is)" "$(git rev-parse HEAD)" >> "$STATE/history.log"
      rm -f "$CODEX_LOG"
      exit 76
    fi
    printf '%s cycle_exit=75 class=QUOTA_RECOVERY_INCOMPLETE head=%s\n' "$(date -Is)" "$(git rev-parse HEAD)" >> "$STATE/history.log"
    rm -f "$CODEX_LOG"
    exit 75
  fi
  as2k_quota_complete
fi

printf '%s cycle_exit=0 head=%s\n' "$(date -Is)" "$(git rev-parse HEAD)" >> "$STATE/history.log"
rm -f "$CODEX_LOG"
