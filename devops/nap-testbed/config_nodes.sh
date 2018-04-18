#!/bin/sh

# Configure testbed nodes for ICN deployment

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

nodes=(snap cnap01)

# Distribute SSH keys
for node in "${nodes[@]}"; do
  vagrant ssh $node -- -t <<EOL
./lib/dist_ssh_keys.exp
EOL
done

