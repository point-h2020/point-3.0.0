#!/bin/bash

SSH_USER=${SSH_USERNAME}

if [[ ! "$DESKTOP" =~ ^(true|yes|on|1|TRUE|YES|ON])$ ]]; then
  exit
fi

apt-get install -y task-gnome-desktop

# just in case the previous command failed to download all packages do it again
apt-get install -y task-gnome-desktop

GDM_CONFIG=/etc/gdm3/daemon.conf

# Configure gdm autologin

if [ -f $GDM_CONFIG ]; then
  sed -i s/"daemon]$"/"daemon]\nAutomaticLoginEnable=true\nAutomaticLogin=${SSH_USER}"/ $GDM_CONFIG
fi

# Need to disable NetworkManager because it overwrites vagrant's settings in /etc/resolv.conf with empty content on the first boot
systemctl disable NetworkManager.service

echo "==> Removing desktop components"
apt-get -y purge gnome-getting-started-docs
apt-get -y purge $(dpkg --get-selections | grep -v deinstall | grep libreoffice | cut -f 1)

