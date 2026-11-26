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
  nes|hvcj) kind=nes ;;
  snes) kind=snes ;;
  *) exit 0 ;;
esac
case "$kind" in
  nes) atlas=nes; title=1004x53; sprites_dir=/usr/share/clover-ui/resources/sprites; [ -f "$sprites_dir/hvc.json" ] && atlas=hvc ;;
  snes) atlas=packed; title=1022x53; sprites_dir=; for dir in /usr/share/ui/snes-*/resources/sprites; do [ -d "$dir" ] && { sprites_dir=$dir; break; }; done ;;
esac
[ "$1" = hvcj ] && { sprites_dir=/usr/share/ui/hvc/resources/sprites; atlas=packed; }
[ -x "$es_dir/theme_layers" ] && [ -f "$theme_dir/console.xml" ] || exit 0
[ -f "$sprites_dir/$atlas.json" ] && [ -f "$sprites_dir/$atlas.png" ] || exit 0
layout=$kind
[ "$atlas" = hvc ] && layout=hvc
[ "$1" = hvcj ] && layout=hvcj
wall=
if [ "$kind" = snes ]; then
  region=${sprites_dir#/usr/share/ui/snes-}
  region=${region%%/*}
  case "$region" in
    eur|jpn) ;;
    *) region=usa ;;
  esac
  layout=snes_$region
  [ -f "${sprites_dir%/sprites}/layout/asset/sprite/wallpaper/wall.png" ] && wall=${sprites_dir%/sprites}/layout/asset/sprite/wallpaper/wall.png
fi
logo_align=center
[ "$layout" = nes ] && logo_align=left
stamp="$atlas $layout $title $logo_align $(stat -c '%s-%Y' "$sprites_dir/$atlas.png" "$es_dir/theme_layers" "$theme_dir/layout_$layout.xml" 2>/dev/null | tr '\n' ' ')"
themeIntact() {
  tail -n +2 "$theme_stamp" 2>/dev/null | while read -r _ listed; do
    [ -f "$theme_dir/$listed" ] || exit 1
  done
}
[ -f "$theme_dir/layout.xml" ] && [ "$(head -n 1 "$theme_stamp" 2>/dev/null)" = "$stamp" ] && themeIntact && exit 0
[ -f "$theme_dir/layout_$layout.xml" ] && cp "$theme_dir/layout_$layout.xml" "$theme_dir/layout.xml"
demo_sprites=${sprites_dir%/sprites}/layout/asset/sprite
if [ "$kind" = snes ] && [ -d "$demo_sprites/mario" ] && [ ! -f "$theme_dir/demo/luigi_run01.png" ]; then
  mkdir -p "$theme_dir/demo" && cp "$demo_sprites"/mario/mario_run0[1-3].png "$demo_sprites"/luigi/luigi_run0[1-3].png "$theme_dir/demo/"
fi
case "$1" in
  hvcj)
    rm -f "$theme_dir"/demo/block_*.png "$theme_dir"/demo/mario_*.png "$theme_dir"/demo/luigi_*.png "$theme_dir"/demo/portraits.txt
    if [ ! -f "$theme_dir/demo/.skit_done" ] && [ -f "$sprites_dir/starrysky_1280_534x2_indexcolor.png" ]; then
      mkdir -p "$theme_dir/demo"
      cp "$sprites_dir/starrysky_1280_534x2_indexcolor.png" "$theme_dir/demo/skit_sky.png"
      cp "$sprites_dir/j50_colorlogo_480_280_171129.png" "$theme_dir/demo/skit_logo.png"
      for sample in "$sprites_dir"/sampleasset/[0-9][0-9]_*.png; do
        number=$(basename "$sample" | cut -c1-2)
        "$es_dir/theme_layers" scale "$sample" "$theme_dir/demo/skit_a_$number.png" 3
        "$es_dir/theme_layers" scale "$sample" "$theme_dir/demo/skit_b_$number.png" 2
      done
      : > "$theme_dir/demo/.skit_done"
    fi
    ;;
  snes)
    rm -f "$theme_dir"/demo/block_*.png "$theme_dir"/demo/mario_[0-4].png "$theme_dir"/demo/luigi_[0-4].png "$theme_dir"/demo/skit_*.png "$theme_dir"/demo/portraits.txt
    ;;
  *)
    rm -f "$theme_dir"/demo/skit_*.png
    if [ "$kind" = nes ] && [ -f "$sprites_dir/characters_sbm.png" ]; then
      [ -f "$theme_dir/demo/block_2.png" ] || "$es_dir/theme_layers" demo "$sprites_dir/characters_sbm.png" "$theme_dir/demo" >/dev/null 2>&1
      [ -f "$theme_dir/demo/portraits.txt" ] || [ ! -f "$sprites_dir/wallpaper-hvc.png" ] || "$es_dir/theme_layers" dots "$sprites_dir/wallpaper-hvc.png" "$theme_dir/demo/portraits.txt" >/dev/null 2>&1
    else
      rm -rf "$theme_dir/demo"
    fi
    ;;
esac

background=$(sed -n 's|.*<backgroundColor>\(.*\)</backgroundColor>.*|\1|p' "$theme_dir/console.xml")
wall_back=$(sed -n 's|.*<wallBack>\(.*\)</wallBack>.*|\1|p' "$theme_dir/layout.xml")
[ -n "$wall_back" ] && background=$wall_back
rm -rf "$work"
if "$es_dir/theme_layers" layers "$sprites_dir/$atlas.json" "$sprites_dir/$atlas.png" "$work" --wide-width 772 --title "$title" --logo-align "$logo_align" ${background:+--background "$background"} ${wall:+--wall "$wall"} >/dev/null 2>&1; then
  cp -a "$work/." "$theme_dir/"
  if [ "$1" = hvcj ] && [ -f "$sprites_dir/bg_image_1280x534_v6.png" ]; then
    cp "$sprites_dir/bg_image_1280x534_v6.png" "$theme_dir/wall_bg.png"
  else
    rm -f "$theme_dir/wall_bg.png"
  fi
  if [ "$1" = hvcj ] && [ -f "$sprites_dir/new_clover_j_logo.png" ]; then
    cp "$sprites_dir/new_clover_j_logo.png" "$theme_dir/top_label.png"
  else
    rm -f "$theme_dir/top_label.png"
  fi
  case $layout in
    nes) original_card=nes ;;
    hvc) original_card=hvc ;;
    hvcj) original_card=hvcj ;;
    snes_*) original_card=$layout ;;
    *) original_card= ;;
  esac
  [ -f "$theme_dir/assets/systems/cards/originals_$original_card.png" ] && cp "$theme_dir/assets/systems/cards/originals_$original_card.png" "$theme_dir/assets/systems/cards/auto-originals.png"
  [ "$1" = hvcj ] && [ -f "$sprites_dir/bottom_side_1280_120.png" ] && cp "$sprites_dir/bottom_side_1280_120.png" "$theme_dir/menubar_bottom_j.png"
  if [ ! -f "$theme_dir/.es-themeset" ] && grep -q '<string name="ThemeSet" value="simple" />' "$settings" 2>/dev/null; then
    sed -i 's|<string name="ThemeSet" value="simple" />|<string name="ThemeSet" value="classicstation" />|' "$settings"
    grep -q '<string name="ThemeSet" value="classicstation" />' "$settings" && : > "$theme_dir/.es-themeset"
  fi
  {
    echo "$stamp"
    (cd "$work" && find . -type f -exec md5sum {} +)
    (cd "$theme_dir" && find layout.xml wall_bg.png top_label.png menubar_bottom_j.png assets/systems/cards/auto-originals.png demo -type f -exec md5sum {} + 2>/dev/null)
  } > "$theme_stamp"
fi
rm -rf "$work"
exit 0
