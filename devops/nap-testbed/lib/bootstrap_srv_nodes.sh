#!/bin/sh

# Bootstrap the IP nodes of the ICN testbed

# Copyright (c) 2016--2018 [Alexander Phinikarides](alexandrosp@prime-tel.com)

# Install other software
echo "deb http://ftp.uk.debian.org/debian jessie-backports main" > /etc/apt/sources.list.d/jessie-backports.list
DEBIAN_FRONTEND=noninteractive apt update
DEBIAN_FRONTEND=noninteractive apt install -y \
  iperf3 \
  nginx \
  ffmpeg

# Copy web root
mkdir -p /var/www/srv01.point
cp -r /vagrant/lib/html /var/www/srv01.point/
cat <<EOF | sudo tee /etc/nginx/sites-available/srv01.point
server {
  listen 80;
  listen [::]:80;
  server_name srv01.point;

  root /var/www/srv01.point/html;
  index index.html index.htm;

  location / { }
}
EOF
ln -s /etc/nginx/sites-available/srv01.point /etc/nginx/sites-enabled/

# DNS
echo "127.0.0.1 srv01.point" >> /etc/hosts

# Services
systemctl enable nginx.service
systemctl restart nginx.service

