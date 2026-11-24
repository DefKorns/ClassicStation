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

cloverconParam() {
  [ -w "/sys/module/clovercon/parameters/$1" ] && printf '%s' "$2" > "/sys/module/clovercon/parameters/$1"
}

autofireForUi() {
  cloverconParam autofire 0
  cloverconParam autofire_xy 0
}

autofireForGames() {
  [ -r /etc/preinit.d/p0000_config ] && source /etc/preinit.d/p0000_config
  [ -n "${cfg_clovercon_autofire:-}" ] && cloverconParam autofire "$cfg_clovercon_autofire"
  [ -n "${cfg_clovercon_autofire_xy:-}" ] && cloverconParam autofire_xy "$cfg_clovercon_autofire_xy"
  return 0
}
