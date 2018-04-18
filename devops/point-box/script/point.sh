#!/bin/bash -eux

# Set up and provision a POINT box

SSH_USER=${SSH_USERNAME}
SSH_USER_HOME=${SSH_USER_HOME:-/home/${SSH_USER}}

# Set up kernel variables for NAP operational deployment
cat <<EOF | tee -a /etc/sysctl.d/99-sysctl.conf
# POINT
net.ipv4.conf.all.force_igmp_version=2
net.ipv4.conf.default.force_igmp_version=2
net.ipv4.ip_forward=0
net.ipv4.conf.all.rp_filter=2
net.ipv4.conf.default.rp_filter=2
net.ipv4.ip_local_port_range=15000 60999
net.ipv4.tcp_fin_timeout=30
net.ipv4.tcp_tw_recycle=1
net.ipv4.tcp_tw_reuse=1
vm.max_map_count=600000
kernel.pid_max=200000
kernel.threads-max=120000
EOF

# Set up supporting software
echo "==> Installing supporting software packages"
apt-get -y update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  git \
  build-essential \
  htop \
  expect \
  python2.7 \
  vim \
  httping \
  tcptraceroute \
  fping \
  mtr \
  curl \
  tcpdump \
  echoping

# Set up blackadder and click
echo "==> Cloning POINT software and Click router"
cd ${SSH_USER_HOME}
# Fix SSH keys
ssh-keyscan -t rsa -H github.com >> ${SSH_USER_HOME}/.ssh/known_hosts
mkdir /root/.ssh
ssh-keyscan -t rsa -H github.com >> /root/.ssh/known_hosts
chown -R point:point .ssh
ssh -T git@github.com
# Clone the repos
git clone https://github.com/point-h2020/point-3.0.0.git blackadder
git clone https://github.com/kohler/click.git click
git clone https://github.com/Tencent/rapidjson.git
cp -r rapidjson/include blackadder/apps/rv_monitoring/

# Install dependencies
echo "==> Installing POINT dependencies"
DEBIAN_FRONTEND=noninteractive apt-get install -y $(cat ~/blackadder/apt-get.txt)

# Compile click
echo "==> Compiling Click"
cd ${SSH_USER_HOME}/click
make clean
./configure --disable-linuxmodule
make -j$(nproc)
make install
touch ${SSH_USER_HOME}/click_ready
chown -R ${SSH_USER}:${SSH_USER} ${SSH_USER_HOME}/click

# Compile blackadder and all components
echo "==> Compiling POINT software"
cd ${SSH_USER_HOME}/blackadder
# XXX: workaround for autoreconf error
touch lib/ChangeLog
./make-all-clean.sh
# RV-MOLY interface
cd apps/rv_monitoring
make -j$(nproc)
# NAP doc (requires texlive)
# cd ~/blackadder/apps/nap/doc/tex
# make clean
# make
# Bootstrapping
# XXX: segfaults on Ubuntu 15.10+
cd ${SSH_USER_HOME}/blackadder/apps/bootstrapping
make clean
make all
cd
touch ${SSH_USER_HOME}/blackadder_ready
chown -R ${SSH_USER}:${SSH_USER} ${SSH_USER_HOME}/blackadder

# Fix invocation of VLC as root
echo "==> Fix VLC permissions"
if [ -f /usr/bin/vlc ]; then
  sed -i 's/geteuid/getppid/' /usr/bin/vlc
fi

# Turn off ICMP destination unreachable messages on the NAP nodes
echo "==> Turn off ICMP type 3 on the NAPs"
cat <<EOF | tee /etc/rc.local
#!/bin/sh -e
iptables -I OUTPUT -p icmp --icmp-type destination-unreachable -j DROP
exit 0
EOF
chmod +x /etc/rc.local

# Cleanup
echo "==> Removing APT files"
find /var/lib/apt -type f | xargs rm -f
echo "==> Removing caches"
find /var/cache -type f -exec rm -rf {} \;

