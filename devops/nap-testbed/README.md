# IP-over-ICN virtual testbed for POINT H2020 platform

Copyright (c) 2016--2018 [Alexander Phinikarides](mailto:alexandrosp@prime-tel.com)

---------------------------------------

## Introduction

This repository contains code and instructions for deploying a IP-over-ICN virtual testbed on VirtualBox and creating an OVA template.
The testbed is using [public release 3.0.0](https://github.com/point-h2020/point-3.0.0) of the [POINT H2020](https://www.point-h2020.eu/) project.

The testbed nodes and networks are described in `Vagrantfile`. `vagrant up` is used to create and provision the nodes and `config_nodes.sh` is used to configure the POINT software stack.
The POINT network can be started with `lib/deploy_icn.sh` on the `snap` node.

A virtual appliance (OVA) for the whole testbed can be created with `create_package.sh`, for deployment on VirtualBox.

---------------------------------------

## Using the OVA template

Requirements:

- [VirtualBox](https://www.virtualbox.org/wiki/Downloads)
- SSH client
- (If on Windows) [Xming](https://sourceforge.net/projects/xming/)

A user can simply import the generated OVA template into VirtualBox.
The OVA contains the four _pre-configured_ VMs which provide native ICN and IP-over-ICN functionality.
No additional configuration is necessary.

The size of the whole testbed template is around 5 GB.

---------------------------------------

## Automated provisioning

_Information in this section applies only to developers.
End users should consult the [Using the OVA template](#using-the-ova-template) section._

### Base box

The testbed is created by Vagrant using the [official POINT 3.0.0 boxes](../point-box/) which are based on Debian Jessie x64.
All nodes are configured with two CPU cores, 2 GB RAM and the virtio driver as well as promiscuous mode enabled for all network interfaces. In addition, a sound device is emulated, to be able to play back audio during video/audio streaming.

### Configuration management

The Vagrantfile contains the description of the entire testbed environment.
A developer can try new functionality without worrying about manual redeployment and configuration in case of failure.
More importantly, the base box can be exchanged by modifying **one** variable, making the testing of the environment on different versions of POINT very easy.
Instead of Debian, the developer can also use another supported flavour of Linux (e.g. Arch, CentOS etc.) by exchanging the Debian specific commands (i.e. `apt` for `yum` or `pacman`) and the corresponding package names.

### Create the testbed environment from scratch

Requirements:

- [Vagrant](https://www.vagrantup.com/)
- _(Optional)_ Vagrant [sahara plugin](https://github.com/jedi4ever/sahara)
- [VirtualBox](https://www.virtualbox.org/wiki/Downloads)
- VirtualBox [Oracle VM Extension Pack](https://www.virtualbox.org/wiki/Downloads)
- Bash/Zsh
- SSH client
- _(If on Windows)_ [Xming](https://sourceforge.net/projects/xming/)

Step-by-step procedure:

**Step 1:** Provision the testbed

```sh
vagrant up
```

**Step 2:** Prepare the testbed for ICN deployment

```sh
./config_nodes.sh
```

_(Optional)_ **Step 3:** Take a snapshot of the provisioned VMs

```sh
vagrant plugin install sahara
./save_state.sh
```

### Package the environment into an OVA for distribution

_(Optional)_ **Step 1:** Return the testbed to its previous state

```sh
vagrant sandbox rollback
```

**Step 2:** Package the whole testbed as a virtual appliance (OVA)

```sh
./create_package.sh
```

### Detailed procedure

#### Provisioning

When `vagrant up` is invoked, the following base configuration is applied:

1. Create a management network in VirtualBox (10.0.19.0/24 NAT)
2. Create a new VM with 2 CPU, 2048 MB RAM and four network interfaces (eth0 NAT, eth1 internal net, eth2 internal net, eth3 internal net)
3. Set the vanilla [POINT 3.0.0 disk image](../point-box/) as master
4. Take a snapshot of the master disk image and attach it to the VM (this keeps overall testbed disk usage and provisioning time low)
5. Boot the VM
6. Configure networks, interfaces, IPs, routes
7. Copy the necessary folders and files from the host to the VM

On the **NAP nodes**, the additional configuration below is applied:

1. Create new, secure SSH keys for the `point` account
2. Create the `nap_nodes` file
3. Create an ICN topology and NAP config files

On the **server nodes**, the additional configuration below is applied:

1. Install iperf3, ffmpeg
2. Install and configure NGINX
3. Copy web site folder to the web server root
4. Set DNS entries

On the **UE (client) nodes**, the additional configuration below is applied:

1. Install iperf3, VLC
2. Set DNS entries

#### IP-over-ICN configuration

Following the successful execution of `vagrant up`, the `config_node.sh` script should be executed.

When `config_node.sh` is invoked, the following configuration is applied:

1. SSH into the first node
2. Run an `expect` script which connects to each node of the testbed through SSH, distributes the public SSH keys and answers automatically all user prompts
3. Repeat this procedure for all nap nodes in the testbed

---------------------------------------

## Usage

### Connect to the nodes

To connect to the nodes, use the following:

```sh
vagrant ssh snap
```

or,

```sh
ssh point@$SNAP
```

Default (insecure) credentials are `point`:`pointh2020`

### Deploy and start POINT

The testbed can be deployed in two ways: 1) a **developer** can use Vagrant to provision and deploy the testbed from scratch and, 2) a **user** can simply import the OVA file and start the packaged VMs.

When the nodes have been successfully configured, the POINT overlay can be started on the sNAP node with:

```sh
~/blackadder/deployment/deploy -c ~/topology.cfg -l -n --rvinfo
# or, ~/lib/deploy_icn.sh
```

### Run IP-over-ICN applications

Information sent using IP can be routed through an ICN network by using nodes configured as Network Attachment Points (NAPs).
A server NAP is required to serve the IP server node and a client NAP is required to serve the IP client node.
In this testbed, node `snap` is configured as a server NAP (sNAP), serving IP server node `srv01`.
Node `cnap01` is configured as a client NAP (cNAP), serving the IP client node `ue01`.

`srv01` is automatically configured to serve a simple HTML website which contains a 1080p video file.
To test HTTP-over-ICN, open Firefox on `ue01` and navigate to http://srv01.point
The video should start playing automatically.

