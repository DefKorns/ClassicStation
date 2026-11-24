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
cpu_freq_dir=/sys/devices/system/cpu/cpu0/cpufreq
es_dir=$(dirname "$0")
settings_file=$es_dir/home/.emulationstation/es_settings.cfg
# shellcheck source=es-autofire.sh
source "$es_dir/es-autofire.sh"

no_suspend_cores="mupen64*|parallel_n64*|glupen64*|ludicrousn64*"
heavy_cores="mupen64*|parallel_n64*|glupen64*|ludicrousn64*|pcsx*|mednafen_psx*|mame*|fbalpha*|fbneo*|desmume*|melonds*|drastic*|ppsspp*|mgba*|gpsp*"

loadSettings() {
  local key value
  # shellcheck disable=SC2034 # value is read by the eval
  while IFS='=' read -r key value; do
    [ -n "$key" ] || continue
    eval "[ -n \"\${es_$key+x}\" ] || es_$key=\$value"
  done <<EOF
$(sed -n 's|^<[a-z]* name="\(Display[A-Za-z]*\)" value="\([^"]*\)" */>.*|\1=\2|p' "$settings_file" 2>/dev/null)
EOF
}

deriveDisplay() {
  local suffix
  case $es_DisplayScanlineLevel in
    1|2|3|4) scanline_level=$es_DisplayScanlineLevel ;;
    *)
      scanline_level=0
      [ "$es_DisplayCrtScanlines" = true ] && scanline_level=1
      ;;
  esac
  scanlines_on=false
  [ "$scanline_level" -ge 1 ] && scanlines_on=true

  filter_name=
  case $es_DisplayFilter in
    nearest|linear|hlinear|sharp|composite|phosphor) filter_name=$es_DisplayFilter ;;
    *)
      if [ "$es_DisplayMode" = composite ]; then
        filter_name=composite
      else
        case $es_DisplaySmoothness in
          1) filter_name=nearest ;;
          3) filter_name=linear ;;
          *) [ "$es_DisplayCrtSmooth" = true ] && filter_name=linear ;;
        esac
      fi
      ;;
  esac
  smooth_on=false
  case $filter_name in linear|hlinear) smooth_on=true ;; esac

  frame_path=
  if [ -n "$es_DisplayFrame" ]; then
    suffix=4_3
    [ "$es_DisplayMode" = pixel-perfect ] && suffix=pixel_perfect
    [ -f "/usr/share/backgrounds/$es_DisplayFrame/${es_DisplayFrame}_$suffix.png" ] && frame_path=/usr/share/backgrounds/$es_DisplayFrame/${es_DisplayFrame}_$suffix
  fi
}

loadSettings
deriveDisplay

displayArgs() {
  local mode="$es_DisplayMode" smooth="$smooth_on" scanlines="$scanlines_on"
  case $mode in pixel-perfect|crt-filter) ;; *) mode=keep-aspect-ratio ;; esac
  case $filter_name in composite|phosphor) smooth=false; scanlines=true ;; esac
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
  [ -z "$frame_path" ] || printf ' --decorative-frame-path %s' "$frame_path"
}

clearOverlayFrame() {
  local file="$1" key="$2" line found=
  [ -r "$file" ] || return 0
  while IFS= read -r line || [ -n "$line" ]; do
    case $line in "$key = \""[!\"]*) found=1; break ;; esac
  done < "$file"
  [ -n "$found" ] || return 0
  sed "s|^$key = .*|$key = \"\"|" "$file" > /tmp/es-overlay.tmp && cat /tmp/es-overlay.tmp > "$file"
  rm -f /tmp/es-overlay.tmp
}

clearRetroarchFrame() {
  [ -z "$frame_path" ] || return 0
  clearOverlayFrame /etc/libretro/overlay/border.cfg overlay0_overlay
  clearOverlayFrame /etc/libretro/overlay/border_with_scanlines.cfg overlay0_desc0_overlay
}

nesDisplayArgs() {
  case $es_DisplayMode in
    pixel-perfect) printf ' --graphic-filter ppu' ;;
    crt-filter) printf ' --graphic-filter crt720 --enable-crt-scanlines'; return ;;
    *) printf ' --graphic-filter gpu720' ;;
  esac
  [ "$scanlines_on" = true ] && printf ' --enable-crt-scanlines'
}

overclockShimLine() {
  [ -f "$es_dir/overclock_auto" ] || return 0
  # shellcheck disable=SC2016 # the expansion belongs to the generated shim
  printf 'case "${1#km_}" in %s) set -- "$@" --overclock ;; esac' "$heavy_cores"
}

noSuspendShimLine() {
  # shellcheck disable=SC2016 # the expansion belongs to the generated shim
  printf 'case "${1#km_}" in %s) set -- "$@" --ra-nosaves ;; esac' "$no_suspend_cores"
}

noFrameArg() {
  [ -z "$frame_path" ] || return 0
  # shellcheck disable=SC2016 # the pattern is the literal text of the child script
  grep -q 'n "\$frame"' /bin/retroarch-clover-child 2>/dev/null && return 0
  printf ' --decorative-frame-path %s' "$es_dir/resources/frame_none"
}

retroarchArgs() {
  if [ -n "$frame_path" ]; then
    printf ' --bezel-mode'
  elif [ "$es_DisplayMode" != crt-filter ] && [ "$filter_name" != composite ] && [ "$filter_name" != phosphor ] && { [ "$smooth_on" = true ] || [ "$scanlines_on" = true ]; }; then
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
  [ "$es_DisplayMode" != crt-filter ] || return 0
  case $filter_name in
    nearest) printf 0 ;;
    linear) printf 1 ;;
    hlinear) printf 2 ;;
    sharp) printf 3 ;;
  esac
}

crtfxJournalEnv() {
  [ -f "$es_dir/crtfx_journal" ] || return 0
  CRTFX_JOURNAL=/media/data/log/crtfx/journal.log
  export CRTFX_JOURNAL
}

crtfxEnv() {
  local level gains canoe
  case "$cmd" in
    *clover-canoe-shvc*) canoe=1 ;;
    *clover-kachikachi*)
      case " $cmd " in *" --retroarch "*|*" -retroarch "*|*" --core "*|*" -core "*) return 0 ;; esac
      ;;
    *) return 0 ;;
  esac
  crtfxJournalEnv
  if [ "$es_DisplayMode" = crt-filter ]; then
    local sharp square
    [ -z "$canoe" ] && [ -f "$es_dir/libcrtfx.so" ] || return 0
    sharp=$es_DisplayCrtSharp
    case $sharp in 1|2|3) ;; *) sharp= ;; esac
    level=$scanline_level
    [ "$level" -ge 2 ] && gains=$(scanlineGains "$level")
    [ "$es_DisplayCrtSquare" = 1 ] && square=1
    [ -n "$sharp" ] || [ -n "$gains" ] || [ -n "$square" ] || return 0
    export LD_PRELOAD="$es_dir/libcrtfx.so"
    [ -z "$gains" ] || export CRTFX_GAINS="$gains"
    [ -z "$sharp" ] || export CRTFX_SHARP="$sharp"
    [ -z "$square" ] || export CRTFX_SQUARE=1
    return 0
  fi
  case "$canoe$filter_name" in
    1composite|1phosphor)
      [ -f "$es_dir/libcrtfx.so" ] || return 0
      export LD_PRELOAD="$es_dir/libcrtfx.so"
      gains=$(scanlineGains 3)
      export CRTFX_GAINS="$gains"
      export CRTFX_FILTER=1
      [ "$filter_name" = phosphor ] && export CRTFX_MASK=60
      return 0
      ;;
  esac
  level=$scanline_level
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

openborOverclock() {
  local current max
  [ -f "$es_dir/overclock_auto" ] || return 0
  read -r current < "$cpu_freq_dir/scaling_max_freq"
  read -r max < "$cpu_freq_dir/cpuinfo_max_freq"
  [ "$current" = "$max" ] && return 0
  printf '%s\n' "$max" > "$cpu_freq_dir/scaling_max_freq" && openbor_oc=1
}

openborRestoreClock() {
  local boot
  read -r boot < "$cpu_freq_dir/cpuinfo_boot_freq"
  printf '%s\n' "$boot" > "$cpu_freq_dir/scaling_max_freq"
}

openborShim() {
  local first name
  first=${cmd%% *}
  case "$first" in /bin/openbor|/bin/openbor-legacy|/bin/openbor-debug) ;; *) return 0 ;; esac
  name=${first##*/}
  sed '/startmcp.flag/d' /bin/openbor > "$shim/$name" || return 0
  chmod +x "$shim/$name"
  cmd="$shim/$name --rexec \"$PWD\"${cmd#"$first"}"
  openborOverclock
}

isRetroarchState() {
  [ -f "$1" ] && [ "$(head -c 9 "$1")" != HANASTATE ]
}

desktop=$1
cr=$(printf '\r')
cmd=
path=
code=
seen_keys=
while IFS= read -r line || [ -n "$line" ]; do
  line=${line%"$cr"}
  case $line in
    Exec=*) case $seen_keys in *E*) ;; *) cmd=${line#Exec=}; seen_keys=${seen_keys}E ;; esac ;;
    Path=*) case $seen_keys in *P*) ;; *) path=${line#Path=}; seen_keys=${seen_keys}P ;; esac ;;
    Code=*) case $seen_keys in *C*) ;; *) code=${line#Code=}; seen_keys=${seen_keys}C ;; esac ;;
  esac
done < "$desktop"
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
  resumed_time=0
  if [ "$resume" = floating ]; then
    resume=
    if [ -n "$native" ] && [ -f "$floating.state" ]; then
      resumed=$volatile/$code.resume.state
      cp "$floating.state" "$resumed"
      [ -f "$floating.state.png" ] && cp "$floating.state.png" "$resumed.png"
      resumed_time=
      { read -r resumed_time < "$floating.state_time.txt"; } 2>/dev/null
      case $resumed_time in ''|*[!0-9]*) resumed_time=0 ;; esac
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
    if { [ -n "$resumed" ] && isRetroarchState "$resumed"; } || { [ -n "$resume" ] && isRetroarchState "$saved.state"; }; then
      cmd="$cmd --retroarch"
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
printf '#!/bin/sh\n%s\n%s\nexec retroarch-clover-child "$@"%s\n' "$(overclockShimLine)" "$(noSuspendShimLine)" "$(retroarchArgs)$(noFrameArg)" > "$shim/retroarch-clover"
chmod +x "$shim/retroarch-clover"

canoeShim
openborShim
crtfxEnv
autofireForGames
read -r launched _ < /proc/uptime
launched=${launched%.*}
PATH="$shim:$PATH" sh -c "$cmd"
status=$?
[ -z "${openbor_oc:-}" ] || openborRestoreClock
if [ -n "${resumed:-}" ]; then
  if [ ! -f "$floating.state" ]; then
    cp "$resumed" "$floating.state"
    [ -f "$resumed.png" ] && cp "$resumed.png" "$floating.state.png"
    printf '%s' "$resumed_time" > "$floating.state_time.txt"
    restored=1
  fi
  rm -f "$resumed" "$resumed.png"
fi
if [ -n "${resume_dir:-}" ]; then
  if [ ! -f "$out/state.png" ]; then
    rm -rf "$out"
    mv "$resume_dir" "$out"
  else
    rm -rf "$resume_dir"
  fi
fi
autofireForUi
if [ -n "$native" ] && [ -f "$floating.state" ] && [ -z "${restored:-}" ]; then
  read -r now _ < /proc/uptime
  played=$((${now%.*} - launched))
  if [ -n "$resume" ]; then
    previous=
    if [ -f "$saved.state" ]; then
      { read -r previous < "$saved.state_time.txt"; } 2>/dev/null
    else
      { read -r previous < "$suspend_dir/state.time"; } 2>/dev/null
    fi
    case $previous in ''|*[!0-9]*) previous=0 ;; esac
    played=$((played + previous))
  fi
  played=$((played + ${resumed_time:-0}))
  printf '%s' "$played" > "$floating.state_time.txt"
fi
exit $status
