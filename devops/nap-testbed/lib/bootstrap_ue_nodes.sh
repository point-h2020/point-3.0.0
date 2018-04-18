#!/bin/sh

# Bootstrap the UE nodes of the ICN testbed

# Copyright (c) 2016--2018 [Alexander Phinikarides](alexandrosp@prime-tel.com)

# Install other software
DEBIAN_FRONTEND=noninteractive apt update
DEBIAN_FRONTEND=noninteractive apt install -y \
  iperf3 \
  vlc

# DNS
echo "172.19.1.2 srv01.point" >> /etc/hosts

