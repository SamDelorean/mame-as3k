#!/usr/bin/env bash
set -euo pipefail

ROOT="${AS2K_EMU_ROOT:-$(git rev-parse --show-toplevel)}"
STATE="${XDG_STATE_HOME:-$HOME/.local/state}/as2k-emulator-automation"
LOCK="$STATE/cycle.lock"
mkdir -p "$STATE"
cd "$ROOT"

exec 9>"$LOCK"
flock -n 9 || { echo "EMULATOR_CYCLE_ALREADY_RUNNING"; exit 0; }

for f in AGENTS.md docs/as2k/EMULATOR_AUTOMATION_CONTROL.md docs/as2k/EMULATOR_AUTOMATION_INBOX.md docs/as2k/EMULATOR_AUTOMATION_TASK.md docs/as2k/CODEX_NEXT.md docs/as2k/CODEX_RESULT.md docs/as2k/EMULATION_FINDINGS.md; do
  test -f "$f" || { echo "BLOCKED missing $f"; exit 20; }
done

command -v codex >/dev/null || { echo "BLOCKED codex missing"; exit 21; }
command -v git >/dev/null || { echo "BLOCKED git missing"; exit 22; }

PROMPT=$(cat <<'EOF'
You are the unattended productive worker for the AlphaSmart 2000 MAME emulator repository. Read AGENTS.md, docs/as2k/EMULATOR_AUTOMATION_CONTROL.md, docs/as2k/EMULATOR_AUTOMATION_INBOX.md, docs/as2k/EMULATOR_AUTOMATION_TASK.md, docs/as2k/CODEX_NEXT.md, docs/as2k/CODEX_RESULT.md, and docs/as2k/EMULATION_FINDINGS.md before acting. Inspect recent commits and git status. Continue the highest-priority unresolved authorized emulator task. Diagnose first; repair recoverable harness/build/test defects in a bounded way; make the smallest evidence-backed source change; compile only when required with no more than -j3; run focused automated tests; distinguish emulator, harness, firmware, infrastructure and evidence failures; stop on unexplained regression. Never use destructive git recovery, never discard unknown human changes, never commit ROMs/NVRAM/binaries/raw proprietary traces, never touch the separate AS2K-V3.14.x checkout, and never implement DynFS here. Update CODEX_RESULT.md with concise reproducible evidence. Update EMULATION_FINDINGS.md only for material reusable findings. If verified changes are authorized and safe, commit them in English and push only to as2k-mame0289-dev. Do not create documentation churn when there is no material progress.
EOF
)

codex exec --full-auto "$PROMPT"
printf '%s\n' "$(date -Is) cycle_exit=0 head=$(git rev-parse HEAD)" >> "$STATE/history.log"
