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

theme=$(cd "$(dirname "$0")/../device/themes/classicstation" && pwd)
generated="barLogoPos barLogoMax"

# shellcheck disable=SC2016
used=$(cat "$theme/theme.xml" "$theme/nes/theme.xml" "$theme/snes/theme.xml" | grep -o '\${[A-Za-z0-9_.]*}' | sed 's/^\${//;s/}$//' | grep -v '^system\.' | sort -u)
failed=0

for layout in "$theme"/layout*.xml; do
  defined=$( { grep -o '<[A-Za-z][A-Za-z0-9_]*>' "$theme/console.xml" | sed 's/[<>]//g'; grep -o '<[A-Za-z][A-Za-z0-9_]*>' "$layout" | sed 's/[<>]//g'; for name in $generated; do echo "$name"; done; } | sort -u)
  missing=$(printf '%s\n' "$used" | grep -v -x -F "$defined")
  if [ -n "$missing" ]; then
    echo "$(basename "$layout"): undefined variables: $(echo "$missing" | tr '
' ' ')"
    failed=1
  fi
done

[ "$failed" -eq 0 ] && echo "theme variables test passed"
exit "$failed"
