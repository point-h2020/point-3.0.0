# VM template for POINT H2020 platform

Copyright (c) 2016--2018 [Alexander Phinikarides](mailto:alexandrosp@prime-tel.com)

Create a pre-configured Debian Jessie VM with ready-to-run POINT v3.0.0 software stack.

_Based on [boxcutter/debian](https://github.com/boxcutter/debian/)_

## Requirements

- Linux/OSX/Windows OS
- [Packer](https://www.packer.io/)
- [VirtualBox](https://www.virtualbox.org/wiki/Downloads)

## Set up SSH key-based auth

Use the local (insecure) SSH key for managing the VM:

```sh
eval $(ssh-agent)
```

or

```sh
eval $(gnome-keyring-daemon --start)
export SSH_AUTH_SOCK
```

and then

```sh
chmod 600 keys/point_insecure_key
ssh-add keys/point_insecure_key
ssh-add -L
```

## Build the box

Build POINT on Debian 8 for VirtualBox:

```sh
packer-io build -var-file=debian8.json debian.json
```

Build POINT on Debian 8 with Gnome desktop for VirtualBox:

```sh
packer-io build -var-file=debian8-desktop.json debian.json
```

Build artifacts will be in `box/virtualbox`.

## Usage

The `.ova` templates in `box/virtualbox/ova` can be imported into VirtualBox.
The `.box` files can be used directly with Vagrant via `vagrant box add --name pointvm8-3.0.0 box/virtualbox/pointvm8-3.0.0.box`
Adding the boxes to Vagrant allows the [definition and provisioning of POINT testbeds](../nap-testbed/).

