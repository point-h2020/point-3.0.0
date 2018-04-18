#!/usr/bin/env bash

# Setup a named tmux session with pre-defined windows
# This script should be run under the `point` account

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

tmux new-session -A -d -s blackadder -n "deploy"
tmux new-window -t blackadder:1 -n "nap"
tmux new-window -t blackadder:2 -n "mona"
tmux new-window -t blackadder:3 -n "moose"
tmux new-window -t blackadder:4 -n "bash"

tmux send-key -t blackadder:0 "echo -e '\n ~/blackadder/deployment/deploy -c ~/topology.cfg -l -n --rvinfo \n'" C-m
tmux select-window -t blackadder:0

