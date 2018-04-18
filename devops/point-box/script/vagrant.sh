#!/bin/bash -eux

echo '==> Configuring settings for vagrant'

SSH_USER=${SSH_USERNAME}
SSH_PASS=${SSH_PASSWORD}
SSH_USER_HOME=${SSH_USER_HOME:-/home/${SSH_USER}}
POINT_INSECURE_KEY="ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDOUdjCUFW5nFmfhkxr1SR7WvHZS9cgifuOrTX6H6lAtaH+utAwkNn/jX5TWL+W9ta7NR2OigreVOPmUi/d2wPNpvwAsG3RPUYPVLDmOZTqVCm/isdYD/Fgu+Snzh5pG6y03AnHAc19CLVnA7q6icEtOU1FMabKlTSYz2hCWtBQsexvPk/j68ph8lLupgQKj44+NpVmAsFISp/1TKAlnMh/HKBsZRvwdCHklTeqA5dYyOCt8UoVd4nL8vdllwa3GBYH1bdqvA/WjbN2VNFaZKEXNjjpM/wjDtiYgOc/xAUKA8yZgoxOijRWoSKgt7sWcVACDFbgAFh5PWtbujC2lweJ point@pointcycle2"

# Add point user (if it doesn't already exist)
if ! id -u $SSH_USER >/dev/null 2>&1; then
  echo "==> Creating ${SSH_USER} user"
  /usr/sbin/groupadd $SSH_USER
  /usr/sbin/useradd $SSH_USER -g $SSH_USER -G sudo -d $SSH_USER_HOME --create-home
  echo "${SSH_USER}:${SSH_PASS}" | chpasswd
fi

# Set up sudo
( cat <<EOP
%$SSH_USER ALL=(ALL) NOPASSWD:ALL
EOP
) > /tmp/${SSH_USER}
chmod 0440 /tmp/${SSH_USER}
mv /tmp/${SSH_USER} /etc/sudoers.d/

if [ "$INSTALL_VAGRANT_KEY" = "true" ] || [ "$INSTALL_VAGRANT_KEY" = "1" ]; then
  echo '==> Installing point insecure SSH key'
  mkdir -pm 700 $SSH_USER_HOME/.ssh
  echo "${VAGRANT_INSECURE_KEY}" > $SSH_USER_HOME/.ssh/authorized_keys
  chmod 600 $SSH_USER_HOME/.ssh/authorized_keys
  chown -R $SSH_USER:$SSH_USER $SSH_USER_HOME/.ssh
fi

