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
assets=${1:?Console Language assets/desktop folder}

awk '
  /^Name *=/ && !(FILENAME in done) {
    done[FILENAME] = 1
    name = $0
    sub(/^Name *= */, "", name)
    sub(/\r$/, "", name)
    gsub(/®|™/, " ", name)
    gsub(/  +/, " ", name)
    sub(/^ +/, "", name)
    sub(/ +$/, "", name)
    code = FILENAME
    sub(/.*\//, "", code)
    sub(/\.desktop$/, "", code)
    if (!(code in titles)) titles[code] = name
  }
  END { for (code in titles) print code "\t" titles[code] }
' "$assets"/*/CLV-*/CLV-*.desktop | LC_ALL=C sort
