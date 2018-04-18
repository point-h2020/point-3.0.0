#!/bin/sh

# Zero the free space of a VM

# Copyright (c) 2016--2018 [Alexander Phinikarides](alexandrosp@prime-tel.com)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

DEBIAN_FRONTEND=noninteractive apt remove -y --purge debconf-utils
rm -rf /var/cache/apt

count=$(df --sync -kP / | tail -n1  | awk -F ' ' '{print $4}')
let count--
dd if=/dev/zero of=/whitespace bs=1024 count=$count
rm /whitespace

touch /root/disk_zeroed
sync

