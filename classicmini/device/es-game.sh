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
export ES_GAME=1
shim=/tmp/es-shim
profile=/var/lib/clover/profiles/0
volatile=/var/cache/clover/volatile
settings_file=$(dirname "$0")/home/.emulationstation/es_settings.cfg

esSetting() {
  sed -n "s|^<[a-z]* name=\"$1\" value=\"\([^\"]*\)\" */>.*|\1|p" "$settings_file" 2>/dev/null | head -n 1
}

framePath() {
  local mode frame suffix
  mode=$(esSetting DisplayMode)
  frame=$(esSetting DisplayFrame)
  [ -n "$frame" ] || return 0
  suffix=4_3
  [ "$mode" = pixel-perfect ] && suffix=pixel_perfect
  [ -f "/usr/share/backgrounds/$frame/${frame}_$suffix.png" ] && printf '/usr/share/backgrounds/%s/%s_%s' "$frame" "$frame" "$suffix"
}

displayArgs() {
  local mode smooth scanlines frame_path
  mode=$(esSetting DisplayMode)
  smooth=$(esSetting DisplayCrtSmooth)
  scanlines=$(esSetting DisplayCrtScanlines)
  case $mode in pixel-perfect|crt-filter) ;; *) mode=keep-aspect-ratio ;; esac
  printf ' --video-mode %s' "$mode"
  if [ "$mode" = crt-filter ]; then
    :
  elif [ "$smooth" = true ] && [ "$scanlines" = true ]; then
    printf ' --crt-mode'
  elif [ "$scanlines" = true ]; then
    printf ' --enable-scanlines'
  elif [ "$smooth" = true ]; then
    printf ' --enable-smooth'
  fi
  frame_path=$(framePath)
  [ -n "$frame_path" ] && printf ' --decorative-frame-path %s' "$frame_path"
}

clearOverlayFrame() {
  local file="$1" key="$2"
  grep -q "^$key = \"[^\"]" "$file" 2>/dev/null || return 0
  sed "s|^$key = .*|$key = \"\"|" "$file" > /tmp/es-overlay.tmp && cat /tmp/es-overlay.tmp > "$file"
  rm -f /tmp/es-overlay.tmp
}

clearRetroarchFrame() {
  [ -z "$(framePath)" ] || return 0
  clearOverlayFrame /etc/libretro/overlay/border.cfg overlay0_overlay
  clearOverlayFrame /etc/libretro/overlay/border_with_scanlines.cfg overlay0_desc0_overlay
}

retroarchArgs() {
  local mode smooth scanlines
  mode=$(esSetting DisplayMode)
  smooth=$(esSetting DisplayCrtSmooth)
  scanlines=$(esSetting DisplayCrtScanlines)
  if [ -n "$(framePath)" ]; then
    printf ' --bezel-mode'
  elif [ "$mode" != crt-filter ] && { [ "$smooth" = true ] || [ "$scanlines" = true ]; }; then
    printf ' --bezel-mode'
  fi
}

desktop=$1
cmd=$(tr -d '\r' < "$desktop" | sed -n 's/^Exec=//p' | head -n 1)
path=$(tr -d '\r' < "$desktop" | sed -n 's/^Path=//p' | head -n 1)
code=$(tr -d '\r' < "$desktop" | sed -n 's/^Code=//p' | head -n 1)
resume=$2
[ -n "$cmd" ] || exit 1
if [ -d "$path" ]; then cd "$path" || exit 1; fi

if [ -n "$code" ]; then
  out=$volatile/$code/suspendpoint0
  for stale in "$volatile"/*/suspendpoint0; do
    [ "$stale" = "$out" ] || rm -rf "$stale"
  done
  rm -rf "$out"
  mkdir -p "$profile/$code" "$out/rollback"
  suspend_dir=$profile/$code/suspendpoint$resume
  native=
  case "$cmd" in
    *kachikachi*)
      case " $cmd " in
        *" --retroarch "*|*" -retroarch "*|*" --core "*|*" -core "*) ;;
        *) native=1 ;;
      esac
      ;;
  esac
  if [ -n "$native" ]; then
    cmd="$cmd --save-data-backing-file $profile/$code/cartridge.sram --save-screenshot-on-quit $out/state.png --save-on-quit $out/state"
    [ -n "$resume" ] && cmd="$cmd --load-state-file $suspend_dir/state"
  else
    cmd="$cmd --save-data-backing-file $profile/$code/cartridge.sram --title-code $code$(displayArgs)"
    cmd="$cmd --save-time-path $out/state.time --save-screenshot-on-quit $out/state.png"
    case "$cmd" in
      *kachikachi*)
        cmd="$cmd --save-on-quit $out/state"
        [ -n "$resume" ] && cmd="$cmd --load-state-file $suspend_dir/state --load-time-path $suspend_dir/state.time"
        ;;
      *)
        cmd="$cmd --rollback-mode record --rollback-output-dir $out/rollback/"
        [ -n "$resume" ] && cmd="$cmd --load-time-path $suspend_dir//state.time --rollback-input-dir $suspend_dir/rollback/"
        ;;
    esac
  fi
fi

clearRetroarchFrame
mkdir -p "$shim"
printf '#!/bin/sh\nexec retroarch-clover-child "$@"%s\n' "$(retroarchArgs)" > "$shim/retroarch-clover"
chmod +x "$shim/retroarch-clover"

PATH="$shim:$PATH" sh -c "$cmd"
