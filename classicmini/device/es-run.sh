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
es_dir=/var/lib/hakchi/emulationstation
source /etc/preinit
script_init
# shellcheck source=es-autofire.sh
source "$es_dir/es-autofire.sh"

export ES_PAD=snes
case "$sftype" in
  nes) originals_year=2016 ;;
  snes) originals_year=2017 ;;
  hvcj) originals_year=2018 ;;
  *) originals_year= ;;
esac
[ "$sftype" = snes ] || ES_PAD=nes

drawSplash(){
  decodepng "$es_dir/resources/$1.png" > /dev/fb0
}

stopPowerWatch(){
  [ -f /run/es-power.pid ] && kill -TERM -- "-$(cat /run/es-power.pid)" 2>/dev/null
  rm -f /run/es-power.pid
  [ -f /run/es-thermal.pid ] && kill -TERM -- "-$(cat /run/es-thermal.pid)" 2>/dev/null
  rm -f /run/es-thermal.pid
}

stockFirmwareName() {
  local sha1
  command -v sunxi-flash >/dev/null 2>&1 || return 0
  sha1=$(sunxi-flash log_read 68 1 | dd status=none bs=32 skip=18 count=1 | hexdump -e '/1 "%02x"' -n20)
  [ -n "$sha1" ] && grep -F " $sha1" /etc/firmwaredb | head -n 1 | cut -d' ' -f1
}

needsCloverconFix() {
  grep -qF "dp-hvc" "${squashfs:-/var/squashfs}/etc/issue" 2>/dev/null || return 1
  [ "$(cat "${squashfs:-/var/squashfs}/etc/clover/boardtype" 2>/dev/null)" = "dp-hvc" ] || return 1
  case "$(stockFirmwareName)" in
    dp-nes-* | dp-shvc-*) return 0 ;;
  esac
  return 1
}

es_notice=
if [ ! -f /run/es-notice-shown ] && needsCloverconFix && [ ! -f /etc/init.d/S77clvcon_detect ]; then
  es_notice="Famicom firmware on NES/SNES hardware: install famicom_clovercon_fix or the pads can freeze or misbehave (github.com/DefKorns/famicom-clovercon-fix)."
  : > /run/es-notice-shown
fi

restore_ui=1
trap '[ -n "$guard_pid" ] && kill "$guard_pid" 2>/dev/null; autofireForGames; [ ! -f /tmp/es-stopping ] && stopPowerWatch; [ "$restore_ui" = 1 ] && [ ! -f /tmp/es-stopping ] && { drawSplash clover; type stockUistart >/dev/null 2>&1 && stockUistart || uistart; }' EXIT
uistop
guardStockUi(){
  local waited=0
  while [ "$waited" -lt 40 ] && [ ! -f /run/es-stock ]; do
    if [ -n "$(pidof clover-mcp)" ]; then
      killall -s KILL clover-mcp 2>/dev/null
      killall -s KILL ReedPlayer-Clover 2>/dev/null
    fi
    sleep 1
    waited=$((waited + 1))
  done
}
guardStockUi &
guard_pid=$!
autofireForUi
stopPowerWatch
setsid sh "$es_dir/es-power-watch.sh" &
echo $! > /run/es-power.pid
setsid sh "$es_dir/es-thermal-watch.sh" &
echo $! > /run/es-thermal.pid
mkdir -p "$es_dir/home/.emulationstation"
[ -d /media/data ] && mkdir -p /media/data/emulationstation/slideshow/audio /media/data/emulationstation/slideshow/media
for cfg in es_settings.cfg es_input.cfg; do
  [ -f "$es_dir/home/.emulationstation/$cfg" ] || cp "$es_dir/defaults/$cfg" "$es_dir/home/.emulationstation/"
done
sh "$es_dir/es-theme.sh" "$sftype"
librarySignature() {
  find "$games_dir" /media/data/emulationstation/covers "$es_dir/es-library" "$es_dir/es_romaji.txt" \( -name '*.desktop' -o -name '*.png' -o -name 'aliases.txt' -o -name 'es-library' -o -name 'es_romaji.txt' \) ! -path '*/CLV-Z-ESRET/*' -exec ls -ln --full-time {} + 2>/dev/null | md5sum
}
buildLibrary() {
library_stamp=$es_dir/library.stamp
games_dir=$(findGameStorage)
export ES_STORE="$games_dir"
games_count=0
[ -n "$games_dir" ] && games_count=$(find "$games_dir" -name '*.desktop' ! -path '*/CLV-Z-ESRET/*' | wc -l)
library_sig=$(librarySignature)
if [ -d "$es_dir/library" ] && [ "$(cat "$library_stamp.count" 2>/dev/null)" = "$games_count" ] && [ "$(cat "$library_stamp.sig" 2>/dev/null)" = "$library_sig" ]; then
  rm -rf /tmp/es-library
  cp -a "$es_dir/library" /tmp/es-library
else
  "$es_dir/es-library" /tmp/es-library
  rm -rf "$es_dir/library"
  cp -a /tmp/es-library "$es_dir/library"
  echo "$games_count" > "$library_stamp.count"
  echo "$library_sig" > "$library_stamp.sig"
fi
sh "$es_dir/es-favorites.sh" /tmp/es-library "$es_dir/favorites.txt"
settings_file=$es_dir/home/.emulationstation/es_settings.cfg
if [ -s /tmp/es-library/originals.txt ] && [ ! -f "$es_dir/originals_seeded" ] && [ -f "$settings_file" ]; then
  grep -q 'name="CollectionSystemsAuto"' "$settings_file" || echo '<string name="CollectionSystemsAuto" value="" />' >> "$settings_file"
  grep -q 'name="CollectionSystemsAuto" value="[^"]*originals' "$settings_file" || sed -i 's|\(name="CollectionSystemsAuto" value="[^"]*\)"|\1,originals"|; s|value=",originals"|value="originals"|' "$settings_file"
  grep -q 'name="CollectionSystemsAuto" value="[^"]*originals' "$settings_file" && : > "$es_dir/originals_seeded"
fi
sh "$es_dir/es-systems.sh" /tmp/es-library "$es_dir/es_systems.cfg" "$es_dir/home/.emulationstation/es_systems.cfg"
}
buildLibrary
[ -f /media/data/emulationstation/audio/bgm_home.wav ] || sh "$es_dir/es-tools.sh" audio import >/dev/null 2>&1
sh "$es_dir/es-tools.sh" swap boot >/dev/null 2>&1
while true; do
  rm -f /tmp/es-quit /tmp/es-restart /tmp/es-sysrestart /tmp/es-shutdown
  [ -f /tmp/es-rebuild ] && { rm -f /tmp/es-rebuild; drawSplash restart; buildLibrary; }
  es_started=$(date +%s)
  HOME=$es_dir/home LD_LIBRARY_PATH=$es_dir ES_FAVORITES_FILE=$es_dir/favorites.txt ES_ORIGINALS_FILE=/tmp/es-library/originals.txt ES_ORIGINALS_YEAR=$originals_year ES_NOTICE=$es_notice "$es_dir/emulationstation" "$@"
  es_notice=
  es_status=$?
  case $es_status in 129|143) restore_ui=0; break ;; esac
  if [ "$es_status" -ne 0 ] && [ $(($(date +%s) - es_started)) -lt 15 ]; then
    es_failures=$(($(cat "$es_dir/boot_failures" 2>/dev/null || echo 0) + 1))
    echo "$es_failures" > "$es_dir/boot_failures"
    if [ "$es_failures" -ge 2 ]; then
      echo stock > "$es_dir/boot_default"
      break
    fi
    continue
  fi
  rm -f "$es_dir/boot_failures"
  [ -f /tmp/es-restart ] && continue
  if [ -f /tmp/es-sysrestart ] || [ -f /tmp/es-shutdown ] || { [ "$es_status" -eq 0 ] && [ ! -f /tmp/es-quit ]; }; then
    restore_ui=0
  fi
  if [ -f /tmp/es-sysrestart ]; then
    touch /tmp/es-restarting
    drawSplash restart
    reboot
  elif [ -f /tmp/es-shutdown ]; then
    drawSplash shutdown
    poweroff
  fi
  break
done
