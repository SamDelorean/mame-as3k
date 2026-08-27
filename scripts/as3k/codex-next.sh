#!/bin/sh

set -eu

repo_root=$(git rev-parse --show-toplevel)
cd "$repo_root"

branch=$(git branch --show-current)
if [ "$branch" != "as3k-mame0289-dev" ]; then
	printf 'error: expected branch as3k-mame0289-dev, found %s\n' "$branch" >&2
	exit 1
fi

git pull --ff-only as3k-project as3k-mame0289-dev

{
	printf '%s\n\n' 'Follow the project instructions and execute the current task completely.'
	printf '%s\n' '--- AGENTS.md ---'
	cat AGENTS.md
	printf '%s\n' '--- docs/as3k/CODEX_NEXT.md ---'
	cat docs/as3k/CODEX_NEXT.md
} | codex exec --approve-for-me -C "$repo_root" -

printf '%s\n' '--- docs/as3k/CODEX_RESULT.md ---'
cat docs/as3k/CODEX_RESULT.md
printf '%s\n' '--- git status --short ---'
git status --short
