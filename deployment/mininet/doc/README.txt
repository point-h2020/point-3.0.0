Deployment Extension for constructing ICN Mininet Topology Distributed over multiple hosts

Author: Mays AL-Naday
Date: 13-Jan-2015

Objective:
    Construct an ICN network of mininet hosts, distributed in the form of clusters over multiple machines
    and can access and be accessed from external networks.
    The created topology is the form of a DAG with one or multiple Root nodes that bridges mininet to external networks,
    such as the POINT overlay testbed.
Assumption:
    The Mininet network will be bridged (breakout) to other ICN networks (e.g. POINT overlay testbed), thereby creating a
    large scale Domain network consisting of multiple segments. Since the network is still a single domain (or zone), only one
    RV and one TM can exist in the network. Therefore, it has been assumed that the mininet network does not have a RV/TM node
    as both functions will be provided by the main network (not mininet).
Description:
    create clusters of mininet on multiple machines, which can access the local
     as well as remote machines and mininet clusters, on the same network. To enable the breakout a linux bridge is created
     inside mininet and used to bridge the interface of the machine hosting the cluster while transferring its IP address
     to the linux bridge created inside mininet. Transferring the IP address of the machine interface to the LinuxBridge
     effectivily means (from mininet perspective) changing the management IP address of the Linux switch. This has created no
     issues as the linux switch is not controlled by the Controller of mininet, however carefull if you try that with OVSwitch as
     it can create unexpected behaviour between the switch and the controller
     It should be noted that this mininet setup depends entirely on LinuxBridges, so all the switches created are LinuxBridges
     NOT OVSwitches, to minimize any issues with mininet Controller, arising from changing the management IP address of the switch
     as well as any issues that can potentially arise from mixing SDN and L2 switching techs.
     The network is constructed by sshing to each machine, clear/terminate/and exit any existing mininet sessions,
     and running the ICNminient setup with the approperiate arguments in a background screen, which can be called later on for viewing.
     screen is particularly used because otherwise running mininet without start(net) provides no known way (at least up to my knowledge)
     to access running mininet and control its run, the issue of this is that if for e.g. I want to exit mininet and clear all the bridges,
     doing sudo mn -c wont clean and terminate all the bridges, hence the screen is created for start/stop mininet

Deps:
python-netaddr: parsing/manipulating IPv4 addresses
bridge-utils: create linux bridges
python-paramiko: sshing and initiating background processes over remote hosts from python

Configs:
mininet_config.json: input, json-based, configuration template that describes:
 * the deployment domain and requirements to create the ICN configuration file
 * the set of machines to be used for constructing the mininet ICN network
 * config params include:
    NUM_CLIENTS: number of client<->NAP pairs you want to create in each mininet cluster
    TOPOLOGY: the topology type, currently only DAG is supported, but will be upgraded in the future
    BRIDGE_INTF: the machine interface used to bridge mininet cluster to the machines and the external network
    WRITE_PATH: path to write config files, such as .cfg ICN network configuration
    READ_PATH: path for reading the .json template file
    MANAGEMENT_NET: IPv4 network in the form of network-address/prefixlen used to manage mininet hosts
    DATA_NET: IPv4 network in the form of network-address/prefixlen used between the clients and NAPs for IP services
    bridge_nodes: Roots of the DAG network, they exist in both mininet network and any other network that mininet will bridge to.
                  for e.g. to bridge to the POINT testbed, bridge_nodes are nodes in POINT testbed that will be used to bridge mininet to the testbed
    servers: the machines used for mininet clusters, a cluster per machine

mininet_conf.cfg: output, libconfig-based, configuration template that describes the ICN topology of the mininet network,
 including its briding node(s) to other ICN networks

How To:
* Make sure all machines are preped, including:
    - install:
         - mininet 2.2.1
         - the deps listed above
         - click & blackadder
    - enable IP forwarding:
        sudo su
        echo 1 > /proc/sys/net/ipv4/ip_forward (add this line to /etc/rc.local for perminant setup)
    - Make sure the machines are accissable from each other
* clone blackadder from POINT Github repository (the monitoring branch)
    Notice: no ICN installations of click and Blackadder are required at this stage, though feel free to install them ;)
* cd ~/blackadder/deployment/mininet
* make sure the .json template reflects the machine setup you have and the network you want to create, review the config params above.
* Make sure passwordless ssh is enabled on all machines with passwordless sudo
* Make sure to chmod +x deploy.py and ICNmininet.py
* run: ./deploy mininet_config.json
* look at the created mininet_conf.cfg and try to run that from ~/blackadder/deployment/deploy ;), you will get a SigFault
because the topology does not have a RV/TM, recall that we assume the RV/TM exist in another Topology such as the POINT testbed.
* The outcomes of this phase are:
    - create mininet clusters that can be accessed from anyhost within the management network
    - create an ICN configuraiton file of the mininet network, to be used for ICN deployment
* So now you need to move to the next phase to deploy the ICN network using the created mininet_conf.cfg
as well as the main network (containing the RV/TM) .cfg




