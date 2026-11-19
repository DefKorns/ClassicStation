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

heavy_cores="mupen64*|parallel_n64*|glupen64*|ludicrousn64*|pcsx*|mednafen_psx*|mame*|fbalpha*|fbneo*|desmume*|melonds*|drastic*|ppsspp*|mgba*|gpsp*"

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

scanlineLevel() {
  local level
  level=$(esSetting DisplayScanlineLevel)
  case $level in 1|2|3|4) printf '%s' "$level"; return ;; esac
  [ "$(esSetting DisplayCrtScanlines)" = true ] && printf 1 || printf 0
}

filterName() {
  local name
  name=$(esSetting DisplayFilter)
  case $name in nearest|linear|hlinear|sharp|composite|phosphor) printf '%s' "$name"; return ;; esac
  [ "$(esSetting DisplayMode)" = composite ] && { printf composite; return; }
  case $(esSetting DisplaySmoothness) in 1) printf nearest; return ;; 3) printf linear; return ;; esac
  [ "$(esSetting DisplayCrtSmooth)" = true ] && printf linear
}

scanlinesOn() {
  [ "$(scanlineLevel)" -ge 1 ] && printf true || printf false
}

smoothOn() {
  case $(filterName) in linear|hlinear) printf true ;; *) printf false ;; esac
}

displayArgs() {
  local mode smooth scanlines frame_path
  mode=$(esSetting DisplayMode)
  smooth=$(smoothOn)
  scanlines=$(scanlinesOn)
  case $mode in pixel-perfect|crt-filter) ;; *) mode=keep-aspect-ratio ;; esac
  case $(filterName) in composite|phosphor) smooth=false; scanlines=true ;; esac
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

nesDisplayArgs() {
  case $(esSetting DisplayMode) in
    pixel-perfect) printf ' --graphic-filter ppu' ;;
    crt-filter) printf ' --graphic-filter crt720 --enable-crt-scanlines'; return ;;
    *) printf ' --graphic-filter gpu720' ;;
  esac
  [ "$(scanlinesOn)" = true ] && printf ' --enable-crt-scanlines'
}

overclockShimLine() {
  [ -f "$(dirname "$0")/overclock_auto" ] || return 0
  # shellcheck disable=SC2016 # the expansion belongs to the generated shim
  printf 'case "${1#km_}" in %s) set -- "$@" --overclock ;; esac' "$heavy_cores"
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
  smooth=$(smoothOn)
  scanlines=$(scanlinesOn)
  if [ -n "$(framePath)" ]; then
    printf ' --bezel-mode'
  elif [ "$mode" != crt-filter ] && [ "$(filterName)" != composite ] && [ "$(filterName)" != phosphor ] && { [ "$smooth" = true ] || [ "$scanlines" = true ]; }; then
    printf ' --bezel-mode'
  fi
}

scanlineGains() {
  case $1 in
    2) printf '155,-55,85,10' ;;
    3) printf '195,-95,75,15' ;;
    4) printf '195,-95,65,10' ;;
  esac
}

canoeMagfilter() {
  [ "$(esSetting DisplayMode)" != crt-filter ] || return 0
  case $(filterName) in
    nearest) printf 0 ;;
    linear) printf 1 ;;
    hlinear) printf 2 ;;
    sharp) printf 3 ;;
  esac
}

crtfxJournalEnv() {
  [ -f "$(dirname "$0")/crtfx_journal" ] || return 0
  CRTFX_JOURNAL=/media/data/log/crtfx/journal.log
  export CRTFX_JOURNAL
}

crtfxEnv() {
  local es_dir level gains canoe
  es_dir=$(dirname "$0")
  case "$cmd" in
    *clover-canoe-shvc*) canoe=1 ;;
    *clover-kachikachi*)
      case " $cmd " in *" --retroarch "*|*" -retroarch "*|*" --core "*|*" -core "*) return 0 ;; esac
      ;;
    *) return 0 ;;
  esac
  crtfxJournalEnv
  if [ "$(esSetting DisplayMode)" = crt-filter ]; then
    local sharp square
    [ -z "$canoe" ] && [ -f "$es_dir/libcrtfx.so" ] || return 0
    sharp=$(esSetting DisplayCrtSharp)
    case $sharp in 1|2|3) ;; *) sharp= ;; esac
    level=$(scanlineLevel)
    [ "$level" -ge 2 ] && gains=$(scanlineGains "$level")
    [ "$(esSetting DisplayCrtSquare)" = 1 ] && square=1
    [ -n "$sharp" ] || [ -n "$gains" ] || [ -n "$square" ] || return 0
    export LD_PRELOAD="$es_dir/libcrtfx.so"
    [ -z "$gains" ] || export CRTFX_GAINS="$gains"
    [ -z "$sharp" ] || export CRTFX_SHARP="$sharp"
    [ -z "$square" ] || export CRTFX_SQUARE=1
    return 0
  fi
  case "$canoe$(filterName)" in
    1composite|1phosphor)
      [ -f "$es_dir/libcrtfx.so" ] || return 0
      export LD_PRELOAD="$es_dir/libcrtfx.so"
      gains=$(scanlineGains 3)
      export CRTFX_GAINS="$gains"
      export CRTFX_FILTER=1
      [ "$(filterName)" = phosphor ] && export CRTFX_MASK=60
      return 0
      ;;
  esac
  level=$(scanlineLevel)
  [ "$level" -ge 2 ] || return 0
  gains=$(scanlineGains "$level")
  [ -f "$es_dir/libcrtfx.so" ] && [ -n "$gains" ] || return 0
  export LD_PRELOAD="$es_dir/libcrtfx.so"
  export CRTFX_GAINS="$gains"
}

canoeShim() {
  local magfilter real
  rm -f "$shim/canoe-shvc"
  case "$cmd" in *clover-canoe-shvc*) ;; *) return 0 ;; esac
  real=$(command -v canoe-shvc) || return 0
  magfilter=$(canoeMagfilter)
  [ -n "$magfilter" ] || return 0
  cat > "$shim/canoe-shvc" <<SHIM
#!/bin/sh
n=\$#
while [ \$n -gt 0 ]; do
  a=\$1
  shift
  n=\$((n - 1))
  if [ "\$a" = -magfilter ] && [ \$n -gt 0 ]; then
    shift
    n=\$((n - 1))
    continue
  fi
  set -- "\$@" "\$a"
done
exec "$real" "\$@" -magfilter $magfilter
SHIM
  chmod +x "$shim/canoe-shvc"
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
  resumed=
  resume_dir=
  if [ "$resume" = floating ]; then
    resume=
    if [ -n "$native" ] && [ -f "$floating.state" ]; then
      resumed=$volatile/$code.resume.state
      cp "$floating.state" "$resumed"
    elif [ -z "$native" ] && [ -d "$out" ]; then
      resume_dir=$volatile/$code.resume.dir
      rm -rf "$resume_dir"
      mv "$out" "$resume_dir"
    fi
  fi
  rm -rf "$volatile"/*/suspendpoint0
  rm -f "$volatile"/*/0.state "$volatile"/*/0.state.png "$volatile"/*/0.state_time.txt
  mkdir -p "$profile/$code" "$volatile/$code"
  suspend_dir=$profile/$code/suspendpoint$resume
  [ -n "$resume_dir" ] && suspend_dir=$resume_dir
  saved=$profile/$code/$resume
  if [ -n "$native" ]; then
    if [ ! -f "$profile/$code/save.sram" ] && [ -f "$profile/$code/cartridge.sram" ]; then
      cp "$profile/$code/cartridge.sram" "$profile/$code/save.sram"
    fi
    cmd="$cmd --save-data-backing-file $profile/$code/save.sram --save-screenshot-on-quit $floating.state.png --save-on-quit $floating.state"
    cmd="$cmd$(nesDisplayArgs)"
    [ -n "$resumed" ] && cmd="$cmd --load-state-file $resumed"
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
        if [ -n "$resume" ] || [ -n "$resume_dir" ]; then cmd="$cmd --load-state-file $suspend_dir/state --load-time-path $suspend_dir/state.time"; fi
        ;;
      *)
        cmd="$cmd --rollback-mode record --rollback-output-dir $out/rollback/"
        if [ -n "$resume" ] || [ -n "$resume_dir" ]; then cmd="$cmd --load-time-path $suspend_dir//state.time --rollback-input-dir $suspend_dir/rollback/"; fi
        ;;
    esac
  fi
fi

clearRetroarchFrame
mkdir -p "$shim"
printf '#!/bin/sh\n%s\nexec retroarch-clover-child "$@"%s\n' "$(overclockShimLine)" "$(retroarchArgs)$(noFrameArg)" > "$shim/retroarch-clover"
chmod +x "$shim/retroarch-clover"

canoeShim
crtfxEnv
autofireForGames
launched=$(cut -d. -f1 /proc/uptime)
PATH="$shim:$PATH" sh -c "$cmd"
status=$?
[ -n "${resumed:-}" ] && rm -f "$resumed"
[ -n "${resume_dir:-}" ] && rm -rf "$resume_dir"
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
