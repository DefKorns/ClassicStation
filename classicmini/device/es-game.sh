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
# shellcheck source=es-autofire.sh
source "$(dirname "$0")/es-autofire.sh"

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

noFrameArg() {
  [ -z "$(framePath)" ] || return 0
  # shellcheck disable=SC2016 # the pattern is the literal text of the child script
  grep -q 'n "\$frame"' /bin/retroarch-clover-child 2>/dev/null && return 0
  printf ' --decorative-frame-path %s' "$(dirname "$0")/resources/frame_none"
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

native=
saved=
if [ -n "$code" ]; then
  case "$cmd" in
    *kachikachi*)
      case " $cmd " in
        *" --retroarch "*|*" -retroarch "*|*" --core "*|*" -core "*) ;;
        *) native=1 ;;
      esac
      ;;
  esac
  out=$volatile/$code/suspendpoint0
  floating=$volatile/$code/0
  rm -rf "$volatile"/*/suspendpoint0
  rm -f "$volatile"/*/0.state "$volatile"/*/0.state.png "$volatile"/*/0.state_time.txt
  mkdir -p "$profile/$code" "$volatile/$code"
  suspend_dir=$profile/$code/suspendpoint$resume
  saved=$profile/$code/$resume
  if [ -n "$native" ]; then
    if [ ! -f "$profile/$code/save.sram" ] && [ -f "$profile/$code/cartridge.sram" ]; then
      cp "$profile/$code/cartridge.sram" "$profile/$code/save.sram"
    fi
    cmd="$cmd --save-data-backing-file $profile/$code/save.sram --save-screenshot-on-quit $floating.state.png --save-on-quit $floating.state"
    if [ -n "$resume" ]; then
      if [ -f "$saved.state" ]; then
        cmd="$cmd --load-state-file $saved.state"
      else
        cmd="$cmd --load-state-file $suspend_dir/state"
      fi
    fi
  else
    mkdir -p "$out/rollback"
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
printf '#!/bin/sh\nexec retroarch-clover-child "$@"%s\n' "$(retroarchArgs)$(noFrameArg)" > "$shim/retroarch-clover"
chmod +x "$shim/retroarch-clover"

autofireForGames
launched=$(cut -d. -f1 /proc/uptime)
PATH="$shim:$PATH" sh -c "$cmd"
status=$?
autofireForUi
if [ -n "$native" ] && [ -f "$floating.state" ]; then
  played=$(($(cut -d. -f1 /proc/uptime) - launched))
  if [ -n "$resume" ]; then
    if [ -f "$saved.state" ]; then
      previous=$(cat "$saved.state_time.txt" 2>/dev/null)
    else
      previous=$(cat "$suspend_dir/state.time" 2>/dev/null)
    fi
    case $previous in ''|*[!0-9]*) previous=0 ;; esac
    played=$((played + previous))
  fi
  printf '%s' "$played" > "$floating.state_time.txt"
fi
exit $status
