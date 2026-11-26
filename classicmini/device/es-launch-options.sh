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
here=$(dirname "$0")

core_dir=${ES_CORE_DIR:-/etc/libretro/core}
info_dir=${ES_CORE_INFO_DIR:-/etc/libretro/info}
launch_dir=${ES_LAUNCH_DIR:-/media/data/emulationstation/launch}

isValidName() {
  case "$1" in
    "" | *[!A-Za-z0-9_]*) return 1 ;;
  esac
}

isValidCode() {
  case "$1" in
    "" | *[!A-Za-z0-9_-]*) return 1 ;;
  esac
}

coreLabel() {
  local name="$1" label
  label=$(sed -n 's/^corename = "\(.*\)".*/\1/p' "$info_dir/${name}_libretro.info" 2>/dev/null | head -n 1)
  [ -n "$label" ] || label=$(printf '%s' "${name#km_}" | tr 'a-z_' 'A-Z ')
  [ "$name" = "${name#km_}" ] || label="$label (KMFD)"
  printf '%s' "$label"
}

coreServes() {
  local wanted="$1" base="${2#km_}"
  case $base in
    genesis*)
      case $wanted in megadrive | segacd | mastersystem | gamegear) return 0 ;; esac
      return 1
      ;;
    picodrive*)
      case $wanted in megadrive | segacd | sega32x | mastersystem) return 0 ;; esac
      return 1
      ;;
    smsplus* | gearsystem)
      case $wanted in mastersystem | gamegear) return 0 ;; esac
      return 1
      ;;
    fceumm* | nestopia)
      case $wanted in nes | fds) return 0 ;; esac
      return 1
      ;;
  esac
  [ "$("$here/es-library" --system-of "$base")" = "$wanted" ]
}

listCores() {
  local wanted="$1" file name
  for file in "$core_dir"/*_libretro.so; do
    [ -f "$file" ] || continue
    name=${file##*/}
    name=${name%_libretro.so}
    coreServes "$wanted" "$name" && printf 'core	%s	%s
' "$name" "$(coreLabel "$name")"
  done
}

defaultCore() {
  local desktop="${ES_LIBRARY_DIR:-/tmp/es-library}/$1/$2.desktop" program name
  program=$(sed -n 's/^Exec=\([^ ]*\).*/\1/p' "$desktop" 2>/dev/null | head -n 1)
  name=${program##*/}
  case $name in
    "" | clover-* | kachikachi*) printf 'default\tnative\tNATIVE\n' ;;
    *)
      if [ -f "$program" ] && grep -q retroarch-clover "$program" 2>/dev/null; then
        printf 'default\t%s\t%s\n' "$name" "$(coreLabel "$name")"
      else
        printf 'default\tnative\tNATIVE\n'
      fi
      ;;
  esac
}

currentCore() {
  [ -f "$launch_dir/$1.cfg" ] && sed -n 's/^core=//p' "$launch_dir/$1.cfg" | head -n 1
}

case "$1" in
  list)
    isValidCode "$3" || exit 1
    printf 'current\t%s\n' "$(currentCore "$3")"
    defaultCore "$2" "$3"
    listCores "$2"
    ;;
  set)
    game_code=$2
    isValidCode "$game_code" || exit 1
    if [ "$3" = native ]; then
      rm -f "${launch_dir:?}/$game_code.cfg"
    else
      isValidName "$3" && [ -f "$core_dir/$3_libretro.so" ] || exit 1
      mkdir -p "$launch_dir" || exit 1
      printf 'core=%s\n' "$3" > "$launch_dir/$game_code.cfg"
    fi
    ;;
  *) exit 1 ;;
esac
