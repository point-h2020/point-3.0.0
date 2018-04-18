#!/bin/sh

# Bootstrap the ICN nodes of the ICN testbed

# Copyright (c) 2016--2018 [Alexander Phinikarides](alexandrosp@prime-tel.com)

# Generate new SSH keys
sudo -u point ssh-keygen -t rsa -q -N "" -f /home/point/.ssh/id_rsa

# Set up testbed nodes management
cat <<EOF | sudo -u point tee /home/point/nap_nodes
10.10.20.102
10.10.20.103
EOF

# ICN topology file
sudo -u point ln -s /home/point/lib/topology.cfg /home/point/

# NAP configuration files
nap_instance=$( grep 10.10.20 /etc/network/interfaces | awk -F '.' '{print $4}' )

case $nap_instance in
  102)
    ln -sf /home/point/lib/cnap01-nap.cfg /etc/nap/nap.cfg
    ln -sf /home/point/lib/cnap01-nap.cfg /etc/nap/nap-00000102.cfg
    ;;
  103)
    ln -sf /home/point/lib/snap-nap.cfg /etc/nap/nap.cfg
    ln -sf /home/point/lib/snap-nap.cfg /etc/nap/nap-00000103.cfg
    ;;
esac

