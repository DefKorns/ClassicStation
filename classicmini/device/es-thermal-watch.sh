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
cpu_freq_dir=/sys/devices/system/cpu/cpu0/cpufreq
thermal_limit=80

while sleep 5; do
  read -r scaling_max < "$cpu_freq_dir/scaling_max_freq" || continue
  read -r cpu_max < "$cpu_freq_dir/cpuinfo_max_freq" || continue
  read -r boot_freq < "$cpu_freq_dir/cpuinfo_boot_freq" || continue
  case $boot_freq in "" | *[!0-9]*) continue ;; esac
  [ "$scaling_max" = "$cpu_max" ] || continue
  [ "$boot_freq" != "$cpu_max" ] || continue
  read -r temp < /sys/class/thermal/thermal_zone0/temp || continue
  case $temp in "" | *[!0-9]*) continue ;; esac
  [ "$temp" -lt 1000 ] || temp=$((temp / 1000))
  if [ "$temp" -ge "$thermal_limit" ]; then
    echo "$boot_freq" > "$cpu_freq_dir/scaling_max_freq"
    echo "$temp" > /tmp/es-oc-cutoff
  fi
done
