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
es_dir=/var/lib/hakchi/emulationstation
theme_dir=/media/data/emulationstation/themes/classicstation
theme_stamp=$theme_dir/.es-generated.md5
settings=$es_dir/home/.emulationstation/es_settings.cfg
work=/tmp/es-theme

case "$1" in
  nes) atlas=nes; title=1004x53; sprites_dir=/usr/share/clover-ui/resources/sprites ;;
  snes) atlas=packed; title=1022x53; sprites_dir=; for dir in /usr/share/ui/snes-*/resources/sprites; do [ -d "$dir" ] && { sprites_dir=$dir; break; }; done ;;
  *) exit 0 ;;
esac
[ -x "$es_dir/theme_layers" ] && [ -f "$theme_dir/console.xml" ] || exit 0
[ -f "$sprites_dir/$atlas.json" ] && [ -f "$sprites_dir/$atlas.png" ] || exit 0
layout=$1
wall=
if [ "$1" = snes ]; then
  region=${sprites_dir#/usr/share/ui/snes-}
  region=${region%%/*}
  case "$region" in
    eur|jpn) ;;
    *) region=usa ;;
  esac
  layout=snes_$region
  [ -f "${sprites_dir%/sprites}/layout/asset/sprite/wallpaper/wall.png" ] && wall=${sprites_dir%/sprites}/layout/asset/sprite/wallpaper/wall.png
fi
[ -f "$theme_dir/layout_$layout.xml" ] && cp "$theme_dir/layout_$layout.xml" "$theme_dir/layout.xml"
[ -f "$theme_stamp" ] && exit 0

background=$(sed -n 's|.*<backgroundColor>\(.*\)</backgroundColor>.*|\1|p' "$theme_dir/console.xml")
wall_back=$(sed -n 's|.*<wallBack>\(.*\)</wallBack>.*|\1|p' "$theme_dir/layout.xml")
[ -n "$wall" ] && [ -n "$wall_back" ] && background=$wall_back
rm -rf "$work"
if "$es_dir/theme_layers" layers "$sprites_dir/$atlas.json" "$sprites_dir/$atlas.png" "$work" --wide-width 772 --title "$title" ${background:+--background "$background"} ${wall:+--wall "$wall"} >/dev/null 2>&1; then
  cp -a "$work/." "$theme_dir/"
  (cd "$work" && find . -type f -exec md5sum {} +) > "$theme_stamp"
  sed -i 's|<string name="ThemeSet" value="simple" />|<string name="ThemeSet" value="classicstation" />|' "$settings"
fi
rm -rf "$work"
exit 0
