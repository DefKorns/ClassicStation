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

crtfxDevEnv() {
  local es_dir stage
  es_dir=$(dirname "$0")
  stage=$(cat "$es_dir/crtfx.stage" 2>/dev/null)
  [ -f "$es_dir/libcrtfx-$stage.so" ] || return 0
  export LD_PRELOAD="$es_dir/libcrtfx-$stage.so"
  if [ -f "$es_dir/crtfx.gains" ]; then
    CRTFX_GAINS=$(cat "$es_dir/crtfx.gains")
    export CRTFX_GAINS
  fi
  if [ -f "$es_dir/crtfx.filter" ]; then
    CRTFX_FILTER=$(cat "$es_dir/crtfx.filter")
    export CRTFX_FILTER
  fi
  if [ -f "$es_dir/crtfx.order" ]; then
    CRTFX_ORDER=$(cat "$es_dir/crtfx.order")
    export CRTFX_ORDER
  fi
  if [ -f "$es_dir/crtfx.mask" ]; then
    CRTFX_MASK=$(cat "$es_dir/crtfx.mask")
    export CRTFX_MASK
  fi
  if [ -f "$es_dir/crtfx.cap" ]; then
    CRTFX_CAP=$(cat "$es_dir/crtfx.cap")
    export CRTFX_CAP
  fi
  if [ -f "$es_dir/crtfx.slow" ]; then
    CRTFX_SLOW=$(cat "$es_dir/crtfx.slow")
    export CRTFX_SLOW
  fi
  if [ -f "$es_dir/crtfx.sweep" ]; then
    CRTFX_SWEEP=$(cat "$es_dir/crtfx.sweep")
    export CRTFX_SWEEP
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
  local es_dir level gains
  es_dir=$(dirname "$0")
  case "$cmd" in *clover-canoe-shvc*) ;; *) return 0 ;; esac
  crtfxJournalEnv
  if [ -f "$es_dir/crtfx.stage" ]; then
    crtfxDevEnv
    return 0
  fi
  [ "$(esSetting DisplayMode)" != crt-filter ] || return 0
  case $(filterName) in
    composite|phosphor)
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
exec $real "\$@" -magfilter $magfilter
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
printf '#!/bin/sh\nexec retroarch-clover-child "$@"%s\n' "$(retroarchArgs)$(noFrameArg)" > "$shim/retroarch-clover"
chmod +x "$shim/retroarch-clover"

canoeShim
crtfxEnv
PATH="$shim:$PATH" sh -c "$cmd"
