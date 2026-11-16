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
cd "$(dirname "$0")/CLV-Z-ESRET" || exit 1
out=$(pwd)/../CLV-Z-ESRET.clvg
stage=$(mktemp -d) || exit 1
trap 'rm -rf "$stage"' EXIT

# hakchi skips the description header only when it is exactly "Text = ", and the commit hooks strip that space
cp CLV-Z-ESRET.png CLV-Z-ESRET_small.png "$stage"
sed 's/^Text =$/Text = /' CLV-Z-ESRET.desktop > "$stage/CLV-Z-ESRET.desktop"

cd "$stage" || exit 1
tar --owner=0 --group=0 --numeric-owner --mode=755 -cf - CLV-Z-ESRET.desktop CLV-Z-ESRET.png CLV-Z-ESRET_small.png | gzip -9 -n > "$out"
