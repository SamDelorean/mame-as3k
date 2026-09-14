#!/bin/sh

set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$repo_root"

branch=$(git branch --show-current)
if [ "$branch" != "as2k-mame0289-dev" ]; then
	printf 'error: expected branch as2k-mame0289-dev, found %s\n' "$branch" >&2
	exit 1
fi
if [ -n "$(git status --porcelain)" ]; then
	printf 'error: AS2000 worktree is not clean before Codex\n' >&2
	git status --short
	exit 1
fi


git pull --ff-only origin as2k-mame0289-dev

{
	printf '%s\n\n' 'Follow the project instructions and execute the current task completely.'
	printf '%s\n' '--- AGENTS.md ---'
	cat AGENTS.md
	printf '%s\n' '--- docs/as2k/CODEX_NEXT.md ---'
	cat docs/as2k/CODEX_NEXT.md
} | script -qefc "codex exec --dangerously-bypass-approvals-and-sandbox -C \"$repo_root\" -" /dev/null

printf '%s\n' '--- docs/as2k/CODEX_RESULT.md ---'
cat docs/as2k/CODEX_RESULT.md
printf '%s\n' '--- git status --short ---'
git status --short
