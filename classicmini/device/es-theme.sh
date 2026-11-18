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
sprites_dir=/usr/share/clover-ui/resources/sprites
es_dir=/var/lib/hakchi/emulationstation
theme_dir=/media/data/emulationstation/themes/classicstation
theme_stamp=$theme_dir/.es-generated.md5
settings=$es_dir/home/.emulationstation/es_settings.cfg
work=/tmp/es-theme

[ "$1" = nes ] || exit 0
[ -x "$es_dir/theme_layers" ] && [ -f "$theme_dir/console.xml" ] || exit 0
[ -f "$sprites_dir/nes.json" ] && [ -f "$sprites_dir/nes.png" ] || exit 0
[ -f "$theme_stamp" ] && exit 0

background=$(sed -n 's|.*<backgroundColor>\(.*\)</backgroundColor>.*|\1|p' "$theme_dir/console.xml")
rm -rf "$work"
if "$es_dir/theme_layers" layers "$sprites_dir/nes.json" "$sprites_dir/nes.png" "$work" --wide-width 772 ${background:+--background "$background"} >/dev/null 2>&1; then
  cp -a "$work/." "$theme_dir/"
  (cd "$work" && find . -type f -exec md5sum {} +) > "$theme_stamp"
  sed -i 's|<string name="ThemeSet" value="simple" />|<string name="ThemeSet" value="classicstation" />|' "$settings"
fi
rm -rf "$work"
exit 0
