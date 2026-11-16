#!/bin/sh
#  Copyright (C) 2026 DefKorns (https://defkorns.github.io/LICENSE)
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
set -e

branch=${1:-stable}
cd "$(dirname "$0")/../.."

git remote get-url upstream >/dev/null
git fetch upstream "$branch"
base=$(git merge-base HEAD "upstream/$branch")
behind=$(git rev-list --count "HEAD..upstream/$branch")
echo "upstream/$branch: $(git rev-parse --short "upstream/$branch"), $behind commits not merged"
[ "$behind" -gt 0 ] || exit 0

echo
echo "Upstream changes in files I still have:"
git diff --name-status --diff-filter=MRC "$base" "upstream/$branch" | while read -r _ path _; do
  [ -e "$path" ] && echo "  $path"
done

echo
echo "Upstream changes in files I deleted (keep them deleted):"
git diff --name-only --diff-filter=MRC "$base" "upstream/$branch" | while read -r path; do
  [ -e "$path" ] || echo "  $path"
done

echo
echo "To merge: git merge upstream/$branch"
