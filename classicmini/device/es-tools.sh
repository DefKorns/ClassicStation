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
case "$1" in
  overclock | swap | temperature | developer) ;;
  *)
    source /etc/preinit
    script_init
    ;;
esac

es_dir=/var/lib/hakchi/emulationstation
usb_resources=/media/data/emulationstation/resources
audio_dir=/media/data/emulationstation/audio
audio_files="bgm_home.wav se_sys_cursor.wav se_sys_click.wav se_sys_click_short.wav se_sys_cancel.wav se_sys_click_game.wav se_sys_push.wav se_sys_attention.wav se_sys_lock.wav se_sys_unlock.wav se_sys_delete.wav"
nand_keep="menu_display.svg menu_system.svg menu_network.svg menu_library.svg menu_sound.svg menu_sound.png menu_help.svg menu_selected.png menu_disk.svg folder.svg opensans_hebrew_condensed_light.ttf opensans_hebrew_condensed_regular.ttf frame.png scroll_gradient.png splash.svg arrow.svg option_arrow.svg button.png button_filled.png checkbox_checked.svg checkbox_unchecked.svg on.svg off.svg slider_knob.svg textinput_ninepatch.png textinput_ninepatch_active.png star_filled.svg star_unfilled.svg busy_0.svg busy_1.svg busy_2.svg busy_3.svg window_icon_256.png help"
home_disabled="0x7FFF"
home_default="0x0024"
home_prev_file=/var/lib/hakchi/emulationstation/home_combination.prev
cpu_freq_dir=/sys/devices/system/cpu/cpu0/cpufreq
swap_file=/media/data/emulationstation/classicstation.swap
swap_conf=/media/data/emulationstation/swap.conf
swap_sizes="256 512"
swap_default_size=512
swap_margin_mb=128

isValidName() {
  case "$1" in
  "" | *[!A-Za-z0-9_]*) return 1 ;;
  esac
}

isValidValue() {
  case "$1" in
  "" | *[!A-Za-z0-9_.x-]*) return 1 ;;
  esac
}

getCfg() {
  eval "echo \"\$cfg_clovercon_$1\""
}

applyCfg() {
  isValidName "$1" && isValidValue "$2" || return 1
  eval "cfg_clovercon_$1='$2'"
  save_config
  case $1 in autofire|autofire_xy) return 0 ;; esac
  param_file="/sys/module/clovercon/parameters/$1"
  [ -w "$param_file" ] && echo "$2" > "$param_file"
}

printState() {
  if "$@"; then echo "on"; else echo "off"; fi
}

cfgEquals() {
  [ "$(getCfg "$1")" = "$2" ]
}

findCloverSounds() {
  for dir in /usr/share/ui/shvc/resources/sounds/hvc /usr/share/clover-ui/shvc/resources/sounds/hvc; do
    [ -f "$dir/bgm_home.wav" ] && { echo "$dir"; return 0; }
  done
  found=$(find /usr/share -name bgm_home.wav 2>/dev/null | head -n 1)
  [ -n "$found" ] && dirname "$found"
}

overclockAvailable() {
  [ -w "$cpu_freq_dir/scaling_max_freq" ] && [ -r "$cpu_freq_dir/cpuinfo_boot_freq" ] && [ -r "$cpu_freq_dir/cpuinfo_max_freq" ] \
    && [ "$(cat "$cpu_freq_dir/cpuinfo_boot_freq")" != "$(cat "$cpu_freq_dir/cpuinfo_max_freq")" ]
}

overclockActive() {
  [ "$(cat "$cpu_freq_dir/scaling_max_freq")" = "$(cat "$cpu_freq_dir/cpuinfo_max_freq")" ]
}

swapConfGet() {
  [ -f "$swap_conf" ] && sed -n "s/^$1=//p" "$swap_conf" | head -n 1
}

swapConfSet() {
  mkdir -p "${swap_conf%/*}" || return 1
  { [ -f "$swap_conf" ] && grep -v "^$1=" "$swap_conf"; echo "$1=$2"; } > "$swap_conf.tmp" && mv "$swap_conf.tmp" "$swap_conf"
}

swapSize() {
  local size
  size=$(swapConfGet size)
  case " $swap_sizes " in
  *" $size "*) echo "$size" ;;
  *) echo "$swap_default_size" ;;
  esac
}

swapAvailable() {
  command -v swapon >/dev/null && command -v mkswap >/dev/null && [ -d /media/data ] && [ -w /media/data ] \
    && [ "$(awk '$2 == "/media" { print $3 }' /proc/mounts)" = ext4 ]
}

swapFileReady() {
  [ -f "$swap_file" ] && [ "$(stat -c %s "$swap_file")" = "$(($(swapSize) * 1048576))" ]
}

swapActive() {
  grep -q "^$swap_file " /proc/swaps
}

swapCreate() {
  local size_mb="$1" free_kb need_mb old_kb
  need_mb=$((size_mb + swap_margin_mb))
  free_kb=$(df -k /media | tail -n 1 | awk '{ print $4 }')
  old_kb=$(du -k "$swap_file" 2>/dev/null | awk '{ print $1 }')
  old_kb=${old_kb:-0}
  rm -f "$swap_file.tmp"
  if [ "$free_kb" -lt $((need_mb * 1024)) ]; then
    if [ $((free_kb + old_kb)) -lt $((need_mb * 1024)) ]; then
      echo "Not enough free space on the SD card: $need_mb MB needed."
      return 1
    fi
    swapConfSet enabled 0
    rm -f "$swap_file"
  fi
  if dd if=/dev/zero of="$swap_file.tmp" bs=1M count="$size_mb" 2>/dev/null && chmod 600 "$swap_file.tmp" \
    && mkswap "$swap_file.tmp" >/dev/null 2>&1 && sync && mv "$swap_file.tmp" "$swap_file"; then
    return 0
  fi
  rm -f "$swap_file.tmp"
  echo "Could not create the swap file on the SD card."
  return 1
}

isUsbWritable() {
  [ -d /media/data ] && [ -w /media ]
}

hasUsbResources() {
  [ -f "$usb_resources/fallback_ja.ttf" ] && [ -f "$usb_resources/menu_system.svg" ]
}

isJapaneseConsole() {
  [ "$sfregion" = jpn ]
}

isJapaneseFamicom() {
  [ "$sftype" = "nes" ] && [ "$sfregion" = "jpn" ]
}

homeEnabled() {
  current="$(getCfg home_combination)"
  [ -n "$current" ] && [ "$(printf '%d' "$current")" != "$(printf '%d' "$home_disabled")" ]
}

esHasInternet() {
  ip route 2>/dev/null | grep -q '^default' || return 1
  for host in connectivitycheck.gstatic.com cloudflare.com; do
    nc -z -w 3 "$host" 443 >/dev/null 2>&1 && return 0
  done
  return 1
}

diskRow() {
  df -h "$2" 2>/dev/null | tail -1 | awk -v label="$1" '{ gsub("%", "", $5); printf "%s\t%s / %s\t%s\n", label, $3, $2, $5 }'
}

case "$1" in
  ip)
    ifconfig | grep "inet " | grep -v 127.0.0.1 | awk 'match($0, /([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)/) { print "Local IP: " substr($0, RSTART, RLENGTH) }'
    public_ip=$(wget -q -T 4 -t 1 -O - http://ipecho.net/plain 2>/dev/null)
    if [ -n "$public_ip" ]; then echo "Public IP: $public_ip"; else echo "Public IP: offline"; fi
    ;;
  reconnect)
    hasAddress() {
      ip -4 addr show wlan0 2>/dev/null | grep -q 'inet '
    }
    if esHasInternet; then
      echo "Already connected to the internet."
    else
      if ! hasAddress || ! wpa_cli status 2>/dev/null | grep -q '^wpa_state=COMPLETED'; then
        wpa_cli enable_network all >/dev/null 2>&1
        wpa_cli reconnect >/dev/null 2>&1
        for _ in $(seq 1 10); do
          hasAddress && break
          sleep 1
        done
        hasAddress || /etc/init.d/S92networking restart >/dev/null 2>&1
        sleep 3
      fi
      if esHasInternet; then echo "Connected."; elif hasAddress; then echo "Connected, but there is no internet."; else echo "Connection failed. Check the Wi-Fi adapter."; fi
    fi
    ;;
  networks)
    listNetworks() {
      wpa_cli scan_results 2>/dev/null | awk -F'	' 'NR > 2 && $5 != "" && !seen[$5]++ { print $5 "	" ($4 ~ /WPA|WEP/ ? "1" : "0") }' | head -n 20
    }
    found=$(listNetworks)
    if [ "$(echo "$found" | grep -c .)" -lt 2 ]; then
      wpa_cli scan >/dev/null 2>&1
      sleep 3
      found=$(listNetworks)
    fi
    echo "$found"
    ;;
  wificonnect)
    network_id=$(wpa_cli list_networks | awk -F'	' -v ssid="$ES_SSID" '$2 == ssid { print $1; exit }')
    added=0
    if [ -z "$network_id" ]; then
      network_id=$(wpa_cli add_network | tail -n 1)
      wpa_cli set_network "$network_id" ssid "\"$ES_SSID\"" >/dev/null
      wpa_cli set_network "$network_id" scan_ssid 1 >/dev/null
      added=1
    fi
    if [ -n "$ES_PSK" ]; then
      wpa_cli set_network "$network_id" psk "\"$ES_PSK\"" >/dev/null
    else
      wpa_cli set_network "$network_id" key_mgmt NONE >/dev/null
    fi
    wpa_cli select_network "$network_id" >/dev/null
    connected=0
    for _ in $(seq 1 20); do
      if wpa_cli status 2>/dev/null | grep -q '^wpa_state=COMPLETED'; then connected=1; break; fi
      sleep 1
    done
    if [ "$connected" = 1 ]; then
      for _ in $(seq 1 10); do
        ip -4 addr show wlan0 2>/dev/null | grep -q 'inet ' && break
        sleep 1
      done
      ip -4 addr show wlan0 2>/dev/null | grep -q 'inet ' || /etc/init.d/S92networking restart >/dev/null 2>&1
      wpa_cli enable_network all >/dev/null
      wpa_cli save_config >/dev/null
      sleep 2
      if esHasInternet; then echo "Connected to $ES_SSID."; else echo "Connected to $ES_SSID, but there is no internet."; fi
    else
      [ "$added" = 1 ] && wpa_cli remove_network "$network_id" >/dev/null
      wpa_cli enable_network all >/dev/null
      wpa_cli save_config >/dev/null
      echo "Could not connect to $ES_SSID. Check the password."
    fi
    ;;
  savednetworks)
    wpa_cli list_networks 2>/dev/null | awk -F'	' 'NR > 2 && $2 != "" { print $2 "	" ($4 ~ /CURRENT/ ? "CONNECTED" : "") }'
    ;;
  wififorget)
    network_id=$(wpa_cli list_networks | awk -F'	' -v ssid="$ES_SSID" '$2 == ssid { print $1; exit }')
    if [ -z "$network_id" ]; then
      echo "$ES_SSID is not saved."
    else
      wpa_cli remove_network "$network_id" >/dev/null
      wpa_cli save_config >/dev/null
      echo "Forgot $ES_SSID."
    fi
    ;;
  wifistatus)
    wpa_cli list_networks 2>/dev/null | awk -F'	' 'NR > 2 && $4 ~ /CURRENT/ { print $2; exit }'
    ;;
  wifiselect)
    network_id=$(wpa_cli list_networks | awk -F'	' -v ssid="$ES_SSID" '$2 == ssid { print $1; exit }')
    if [ -z "$network_id" ]; then
      echo "$ES_SSID is not saved."
    else
      wpa_cli enable_network all >/dev/null 2>&1
      wpa_cli select_network "$network_id" >/dev/null
      connected=0
      for _ in $(seq 1 20); do
        if wpa_cli status 2>/dev/null | grep -q '^wpa_state=COMPLETED'; then connected=1; break; fi
        sleep 1
      done
      if [ "$connected" = 1 ]; then echo "Connected to $ES_SSID."; else echo "Could not connect to $ES_SSID."; fi
    fi
    ;;
  wifidisconnect)
    wpa_cli disable_network all >/dev/null 2>&1
    wpa_cli disconnect >/dev/null 2>&1
    echo "Disconnected."
    ;;
  wifibackup)
    wifi_dir=/media/data/wifi_backup/$(echo "$ES_SSID" | tr '[:upper:]' '[:lower:]' | tr -cd 'a-z0-9_.-')
    wpa_cli save_config >/dev/null 2>&1
    if [ "$wifi_dir" = /media/data/wifi_backup/ ] || ! grep -q "ssid=\"$ES_SSID\"" /etc/wpa_supplicant/wpa_supplicant.conf; then
      echo "$ES_SSID is not saved."
    elif mkdir -p "$wifi_dir" && awk -v ssid="ssid=\"$ES_SSID\"" '
        /^[ 	]*network[ 	]*=[ 	]*\{/ { inside = 1; block = $0 "\n"; keep = 0; next }
        inside { block = block $0 "\n"; sub(/^[ 	]+/, ""); if ($0 == ssid) keep = 1; if ($0 ~ /^\}/) { inside = 0; if (keep) kept = kept block } next }
        { print }
        END { printf "%s", kept }
      ' /etc/wpa_supplicant/wpa_supplicant.conf > "$wifi_dir/wpa_supplicant.conf.tmp" && mv -f "$wifi_dir/wpa_supplicant.conf.tmp" "$wifi_dir/wpa_supplicant.conf"; then
      echo "Backed up $ES_SSID to the SD card."
    else
      rm -f "$wifi_dir/wpa_supplicant.conf.tmp"
      echo "Could not write the backup. Check the SD card."
    fi
    ;;
  wifibackups)
    for wifi_dir in /media/data/wifi_backup/*/; do
      [ -f "${wifi_dir}wpa_supplicant.conf" ] || continue
      wifi_dir=${wifi_dir%/}
      echo "${wifi_dir##*/}"
    done
    ;;
  wifirestore)
    wifi_backup=/media/data/wifi_backup/$ES_SSID/wpa_supplicant.conf
    if [ -z "$ES_SSID" ] || [ "${ES_SSID#*/}" != "$ES_SSID" ] || [ ! -f "$wifi_backup" ]; then
      echo "No backup found on the SD card."
    else
      blockField() {
        awk -v block="$1" -v field="$2" '
          /^[ 	]*network[ 	]*=[ 	]*\{/ { n++; inside = 1; next }
          inside && /^[ 	]*\}/ { inside = 0; next }
          inside && n == block { sub(/^[ 	]+/, ""); i = index($0, "="); if (substr($0, 1, i - 1) == field) { print substr($0, i + 1); exit } }
        ' "$wifi_backup"
      }
      blocks=$(grep -c '^[[:space:]]*network[[:space:]]*=' "$wifi_backup")
      restored=0
      block=1
      while [ "$block" -le "$blocks" ]; do
        ssid=$(blockField "$block" ssid | sed 's/^"\(.*\)"$/\1/')
        if [ -n "$ssid" ] && ! wpa_cli list_networks | awk -F'	' -v ssid="$ssid" '$2 == ssid { found = 1 } END { exit !found }'; then
          network_id=$(wpa_cli add_network | tail -n 1)
          awk -v block="$block" '
            /^[ 	]*network[ 	]*=[ 	]*\{/ { n++; inside = 1; next }
            inside && /^[ 	]*\}/ { inside = 0; next }
            inside && n == block { sub(/^[ 	]+/, ""); i = index($0, "="); if (i > 1) print substr($0, 1, i - 1) "	" substr($0, i + 1) }
          ' "$wifi_backup" | while IFS='	' read -r key value; do
            wpa_cli set_network "$network_id" "$key" "$value" >/dev/null
          done
          restored=$((restored + 1))
        fi
        block=$((block + 1))
      done
      wpa_cli enable_network all >/dev/null
      wpa_cli save_config >/dev/null
      if [ "$restored" -gt 0 ]; then echo "Restored $restored network(s)."; else echo "Every network in the backup is already saved."; fi
    fi
    ;;
  sysinfo)
    [ -f /var/version ] && source /var/version
    printf "Console\t%s-%s\n" "$sftype" "$sfregion"
    printf "hakchi\t%s\n" "$hakchiVersion"
    printf "Kernel\t%s\n" "$kernelVersion"
    printf "Boot\t%s\n" "$bootVersion"
    diskRow "NAND storage" "$mountpoint/var/lib"
    diskRow "SD card" /media
    saves="$(readlink -f /var/saves)"
    [ -d "$saves" ] && printf "Saves\t%s\n" "$(du -sh "$saves" 2>/dev/null | cut -f1)"
    printf "Temperature\t%s C\n" "$(cat /sys/class/thermal/thermal_zone0/temp)"
    ;;
  savelog)
    log_dir=/media/data/log
    mkdir -p "$log_dir" && dmesg > "$log_dir/dmesg.log" && echo "Saved to $log_dir/dmesg.log" || echo "Could not save the log."
    ;;
  cache)
    sync
    echo 1 > /proc/sys/vm/drop_caches
    echo "Cache cleared."
    free -m | awk 'NR == 2 { print "Free memory: " $4 " MB" }'
    ;;
  epilepsy)
    case "$2" in
      state) if [ "$cfg_disable_armet" = "y" ]; then echo "off"; else echo "on"; fi ;;
      toggle)
        if [ "$cfg_disable_armet" = "y" ]; then cfg_disable_armet='n'; else cfg_disable_armet='y'; fi
        save_config
        ;;
    esac
    ;;
  romaji)
    case "$2" in
      available) printState test -f "$es_dir/es_romaji.txt" && isJapaneseConsole ;;
      state) printState test -f "$es_dir/romaji_on" ;;
      toggle)
        if [ -f "$es_dir/romaji_on" ]; then rm -f "$es_dir/romaji_on"; else touch "$es_dir/romaji_on"; fi
        rm -f "$es_dir/library.stamp.count"
        touch /tmp/es-rebuild
        ;;
    esac
    ;;
  overclock)
    case "$2" in
      available) printState overclockAvailable ;;
      state) printState overclockActive ;;
      auto)
        case "$3" in
          state) printState test -f "$es_dir/overclock_auto" ;;
          toggle) if [ -f "$es_dir/overclock_auto" ]; then rm -f "$es_dir/overclock_auto"; else touch "$es_dir/overclock_auto"; fi ;;
        esac
        ;;
      cutoff)
        [ -f /tmp/es-oc-cutoff ] && { cat /tmp/es-oc-cutoff; rm -f /tmp/es-oc-cutoff; }
        ;;
      toggle)
        overclockAvailable || exit 1
        if overclockActive; then
          cat "$cpu_freq_dir/cpuinfo_boot_freq" > "$cpu_freq_dir/scaling_max_freq"
        else
          cat "$cpu_freq_dir/cpuinfo_max_freq" > "$cpu_freq_dir/scaling_max_freq"
        fi
        ;;
    esac
    ;;
  swap)
    case "$2" in
      available) printState swapAvailable ;;
      state) printState swapActive ;;
      size) swapSize ;;
      setsize)
        case " $swap_sizes " in
        *" $3 "*) swapConfSet size "$3" ;;
        *) exit 1 ;;
        esac
        ;;
      enable)
        swapAvailable || { echo "Swap needs an ext4 SD card or USB stick."; exit 1; }
        if swapActive && ! swapFileReady; then
          swapoff "$swap_file" || { echo "Not enough free memory to resize the swap. Close games and try again."; exit 1; }
        fi
        if swapActive; then swapConfSet enabled 1; echo "Swap is already enabled."; exit 0; fi
        swapFileReady || swapCreate "$(swapSize)" || exit 1
        swapon -p 0 "$swap_file" || { echo "Could not enable the swap file."; exit 1; }
        swapConfSet enabled 1
        echo "Swap file enabled ($(swapSize) MB). No restart needed, and it stays on after restarts."
        ;;
      disable)
        if swapActive && ! swapoff "$swap_file"; then
          echo "Not enough free memory to turn swap off. Close games and try again."
          exit 1
        fi
        swapConfSet enabled 0
        echo "Swap file disabled. No restart needed. The file stays on the SD card."
        ;;
      boot)
        [ "$(swapConfGet enabled)" = 1 ] && swapAvailable && [ -f "$swap_file" ] && ! swapActive && swapon -p 0 "$swap_file" >/dev/null 2>&1
        ;;
      info)
        free -m | awk '/buffers\/cache/ { free = $4 } /^Swap/ { used = $3; total = $2 } END { printf "%s MB free, swap %s / %s MB\n", free, used, total }'
        ;;
    esac
    ;;
  developer)
    printf 'overclock_available=%s
' "$(printState overclockAvailable)"
    printf 'overclock_state=%s
' "$(printState overclockActive)"
    printf 'overclock_auto=%s
' "$(printState test -f "$es_dir/overclock_auto")"
    printf 'swap_available=%s
' "$(printState swapAvailable)"
    printf 'swap_state=%s
' "$(printState swapActive)"
    printf 'swap_size=%s
' "$(swapSize)"
    printf 'swap_info=%s
' "$(sh "$0" swap info)"
    printf 'temperature=%s
' "$(sh "$0" temperature)"
    printf 'cutoff=%s
' "$(sh "$0" overclock cutoff)"
    ;;
  temperature)
    degrees=$(cat /sys/class/thermal/thermal_zone0/temp)
    [ "$degrees" -lt 1000 ] 2>/dev/null || degrees=$((degrees / 1000))
    printf "%s C, %s MHz\n" "$degrees" "$(($(cat "$cpu_freq_dir/scaling_cur_freq") / 1000))"
    ;;
  audio)
    case "$2" in
      state) printState test -f "$audio_dir/bgm_home.wav" ;;
      import)
        source_dir=$(findCloverSounds)
        if [ -z "$source_dir" ]; then echo "The Clover sounds were not found."; exit 1; fi
        isUsbWritable || { echo "No USB available."; exit 1; }
        mkdir -p "$audio_dir" || exit 1
        copied=0
        for file in $audio_files; do
          [ -f "$audio_dir/$file" ] && continue
          if [ -f "$source_dir/$file" ] && cp "$source_dir/$file" "$audio_dir/$file"; then copied=$((copied + 1)); fi
        done
        echo "Imported $copied sound file(s) from $source_dir."
        ;;
    esac
    ;;
  usb)
    case "$2" in
      available) printState isUsbWritable ;;
      state) printState hasUsbResources ;;
      copy)
        mkdir -p "$usb_resources" /media/data/emulationstation/themes || { echo "Could not create the folders on the USB."; exit 1; }
        cp -a "$es_dir/resources/." "$usb_resources/" || { echo "Could not copy the resources to the USB."; exit 1; }
        if ! diff -rq "$es_dir/resources" "$usb_resources" 2>/dev/null | grep -v '^Only in '"$usb_resources" | grep -q .; then
          for file in "$es_dir"/resources/*; do
            name=${file##*/}
            case " $nand_keep " in *" $name "*) continue ;; esac
            rm -rf "$file"
          done
          set -- "$es_dir"/resources/*
          echo "Copied the resources to the USB and left $# in the NAND. EmulationStation will restart."
        else
          echo "The copy on the USB does not match, nothing was removed from the NAND."
        fi
        ;;
    esac
    ;;
  clovercon)
    case "$2" in
      available)
        case "$3" in
          fc_start) printState isJapaneseFamicom ;;
          *) echo "off" ;;
        esac
        ;;
      home)
        case "$3" in
          state) printState homeEnabled ;;
          toggle)
            if homeEnabled; then
              getCfg home_combination > "$home_prev_file"
              applyCfg home_combination "$home_disabled"
            else
              applyCfg home_combination "$(cat "$home_prev_file" 2>/dev/null || echo "$home_default")"
            fi
            ;;
        esac
        ;;
      state)
        isValidName "$3" && printState cfgEquals "$3" 1
        ;;
      toggle)
        isValidName "$3" && { if [ "$(getCfg "$3")" = "1" ]; then applyCfg "$3" 0; else applyCfg "$3" 1; fi; }
        ;;
      is)
        isValidName "$3" && printState cfgEquals "$3" "$4"
        ;;
      set)
        applyCfg "$3" "$4"
        ;;
    esac
    ;;
esac
