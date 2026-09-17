#!/usr/bin/env bash
# Shared quota-pause control for the unattended AS2K emulator worker.
# This file is sourced by the supervisor/cycle scripts.

ROOT="${ROOT:-${AS2K_EMU_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}}"
STATE="${STATE:-${XDG_STATE_HOME:-$HOME/.local/state}/as2k-emulator-automation}"
AS2K_QUOTA_DIR="${STATE}/quota"
AS2K_QUOTA_CURRENT="$AS2K_QUOTA_DIR/current"
AS2K_QUOTA_UNIT_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user"
AS2K_QUOTA_TIMER="$AS2K_QUOTA_UNIT_DIR/as2k-emulator-quota-resume.timer"

as2k_quota_fingerprint() {
  {
    git rev-parse HEAD
    git status --porcelain=v1 -z
    git diff --binary
    git diff --cached --binary
    while IFS= read -r -d '' f; do
      printf 'UNTRACKED\0%s\0' "$f"
      sha256sum -- "$f"
    done < <(git ls-files --others --exclude-standard -z)
  } | sha256sum | awk '{print $1}'
}

as2k_quota_archive_current() {
  [ -d "$AS2K_QUOTA_CURRENT" ] || return 0
  local stamp dest
  stamp=$(date '+%Y%m%dT%H%M%S%z')
  dest="$AS2K_QUOTA_DIR/archive/current-$stamp"
  mkdir -p "$AS2K_QUOTA_DIR/archive"
  mv "$AS2K_QUOTA_CURRENT" "$dest"
}

as2k_quota_checkpoint() {
  local resume_epoch="$1" reason="$2" codex_log="${3:-}" attempts="${4:-0}"
  local stamp checkpoint resume_calendar
  mkdir -p "$AS2K_QUOTA_DIR/checkpoints" "$AS2K_QUOTA_UNIT_DIR"
  as2k_quota_archive_current

  stamp=$(date '+%Y%m%dT%H%M%S%z')
  checkpoint="$AS2K_QUOTA_DIR/checkpoints/$stamp"
  mkdir -p "$checkpoint" "$AS2K_QUOTA_CURRENT"

  git status --porcelain=v1 > "$checkpoint/status.txt"
  git diff --binary > "$checkpoint/worktree.patch"
  git diff --cached --binary > "$checkpoint/index.patch"
  git ls-files --others --exclude-standard -z > "$checkpoint/untracked.zlist"
  {
    git diff --name-only -z
    git diff --cached --name-only -z
    git ls-files --others --exclude-standard -z
  } | sort -zu > "$checkpoint/files.zlist"
  if [ -s "$checkpoint/files.zlist" ]; then
    tar --null --ignore-failed-read -czf "$checkpoint/files.tar.gz" -T "$checkpoint/files.zlist" 2>/dev/null || true
  fi
  if [ -n "$codex_log" ] && [ -f "$codex_log" ]; then
    cp "$codex_log" "$checkpoint/codex.log"
  fi

  printf '%s\n' "$resume_epoch" > "$AS2K_QUOTA_CURRENT/resume_epoch"
  printf '%s\n' "$reason" > "$AS2K_QUOTA_CURRENT/reason"
  printf '%s\n' "$attempts" > "$AS2K_QUOTA_CURRENT/attempts"
  git rev-parse HEAD > "$AS2K_QUOTA_CURRENT/head"
  as2k_quota_fingerprint > "$AS2K_QUOTA_CURRENT/fingerprint"
  printf '%s\n' "$checkpoint" > "$AS2K_QUOTA_CURRENT/checkpoint"
  date -Is > "$AS2K_QUOTA_CURRENT/paused_at"

  resume_calendar=$(date -d "@$resume_epoch" '+%Y-%m-%d %H:%M:%S')
  cat > "$AS2K_QUOTA_TIMER" <<EOF_TIMER
[Unit]
Description=Resume AS2K emulator worker after Codex quota reset

[Timer]
OnCalendar=$resume_calendar
AccuracySec=30s
Persistent=true
Unit=as2k-emulator.service

[Install]
WantedBy=timers.target
EOF_TIMER

  systemctl --user disable --now as2k-emulator.timer >/dev/null 2>&1 || true
  systemctl --user disable --now as2k-emulator-quota-resume.timer >/dev/null 2>&1 || true
  systemctl --user daemon-reload
  systemctl --user enable --now as2k-emulator-quota-resume.timer >/dev/null

  printf 'QUOTA_PAUSED reason=%s resume=%s checkpoint=%s\n' \
    "$reason" "$(date -d "@$resume_epoch" -Is)" "$checkpoint" | tee -a "$STATE/supervisor.log"
}

as2k_quota_pause_from_log() {
  local codex_log="$1" retry_line retry_text resume_epoch
  retry_line=$(grep -Eai "try again at|usage limit" "$codex_log" | tail -n 1 || true)
  retry_text=$(printf '%s\n' "$retry_line" | sed -nE 's/.*try again at ([^.]+\.?)/\1/p' | sed -E 's/([0-9]+)(st|nd|rd|th)/\1/g; s/[.]$//')
  if [ -n "$retry_text" ]; then
    resume_epoch=$(LC_ALL=C date -d "$retry_text" +%s 2>/dev/null || true)
  else
    resume_epoch=""
  fi
  if [ -z "$resume_epoch" ]; then
    resume_epoch=$(date -d '+12 hours' +%s)
  fi
  resume_epoch=$((resume_epoch + 300))
  as2k_quota_checkpoint "$resume_epoch" "CODEX_USAGE_LIMIT" "$codex_log" 0
}

as2k_quota_is_paused() {
  [ -d "$AS2K_QUOTA_CURRENT" ] && [ -f "$AS2K_QUOTA_CURRENT/resume_epoch" ]
}

as2k_quota_due() {
  local resume_epoch
  resume_epoch=$(cat "$AS2K_QUOTA_CURRENT/resume_epoch")
  [ "$(date +%s)" -ge "$resume_epoch" ]
}

as2k_quota_checkpoint_matches() {
  local expected_head expected_fp
  expected_head=$(cat "$AS2K_QUOTA_CURRENT/head")
  expected_fp=$(cat "$AS2K_QUOTA_CURRENT/fingerprint")
  [ "$(git rev-parse HEAD)" = "$expected_head" ] || return 1
  [ "$(as2k_quota_fingerprint)" = "$expected_fp" ]
}

as2k_quota_reschedule_incomplete() {
  local codex_log="${1:-}" attempts resume_epoch
  attempts=$(cat "$AS2K_QUOTA_CURRENT/attempts" 2>/dev/null || printf '0')
  attempts=$((attempts + 1))
  if [ "$attempts" -ge 3 ]; then
    printf 'QUOTA_RECOVERY_BLOCKED attempts=%s dirty checkpoint preserved\n' "$attempts" | tee -a "$STATE/supervisor.log"
    return 1
  fi
  resume_epoch=$(date -d '+10 minutes' +%s)
  as2k_quota_checkpoint "$resume_epoch" "QUOTA_RECOVERY_INCOMPLETE" "$codex_log" "$attempts"
}

as2k_quota_complete() {
  local stamp dest
  if [ -d "$AS2K_QUOTA_CURRENT" ]; then
    stamp=$(date '+%Y%m%dT%H%M%S%z')
    dest="$AS2K_QUOTA_DIR/archive/recovered-$stamp"
    mkdir -p "$AS2K_QUOTA_DIR/archive"
    mv "$AS2K_QUOTA_CURRENT" "$dest"
  fi
  systemctl --user disable --now as2k-emulator-quota-resume.timer >/dev/null 2>&1 || true
  rm -f "$AS2K_QUOTA_TIMER"
  systemctl --user daemon-reload
  systemctl --user enable --now as2k-emulator.timer >/dev/null
  printf 'QUOTA_RECOVERED periodic_timer_resumed\n' | tee -a "$STATE/supervisor.log"
}

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  cd "$ROOT" || exit 30
  case "${1:-}" in
    import-pause)
      retry_text="${2:-}"
      [ -n "$retry_text" ] || { echo "usage: $0 import-pause 'Sep 19th, 2026 7:00 AM'" >&2; exit 2; }
      retry_text=$(printf '%s\n' "$retry_text" | sed -E 's/([0-9]+)(st|nd|rd|th)/\1/g; s/[.]$//')
      resume_epoch=$(LC_ALL=C date -d "$retry_text" +%s 2>/dev/null || true)
      [ -n "$resume_epoch" ] || { echo "BLOCKED: cannot parse quota reset time: $retry_text" >&2; exit 3; }
      resume_epoch=$((resume_epoch + 300))
      as2k_quota_checkpoint "$resume_epoch" "IMPORTED_CODEX_USAGE_LIMIT" "" 0
      ;;
    status)
      if as2k_quota_is_paused; then
        resume_epoch=$(cat "$AS2K_QUOTA_CURRENT/resume_epoch")
        printf 'QUOTA_PAUSED resume=%s reason=%s checkpoint=%s\n' \
          "$(date -d "@$resume_epoch" -Is)" \
          "$(cat "$AS2K_QUOTA_CURRENT/reason")" \
          "$(cat "$AS2K_QUOTA_CURRENT/checkpoint")"
      else
        echo "QUOTA_NOT_PAUSED"
      fi
      ;;
    *)
      echo "usage: $0 {import-pause <reset-time>|status}" >&2
      exit 2
      ;;
  esac
fi
