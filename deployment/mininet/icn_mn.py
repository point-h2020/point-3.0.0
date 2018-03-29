#!/usr/bin/env python
#
# Copyright (C) 2016-2018  Mays AL-Naday
# All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version
# 3 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# See LICENSE and COPYING for more details.
#

"""ICN mininet network example, constructing pairs of clients and NAPs for large scale deployment of *-over-ICN.

Usage:
  icn_mn.py [options]

Options:
  -h --help                 Display this screen.
  -v --version              Display the version number
  -l=log_level              set the loglevel <DEBUG|INFO|WARNING|ERROR> [default: info]
  -t=topolgoy               DAG [default: dag]
  -n=num_pairs              number of clinet - NAP pairs [default: 1]
  -s=switch_type            switch type [default: OVS]
  -c=controller             ip address of SDN controller [default: 127.0.0.1]
  --of=OpenFlow versio       OpenFlow version number [default: OpenFlow13]
  --mi=management_if        add management interface to the topology for remote access
  --di=data_if              add data interface for ICN traffic, can be the same as management_if
  --mn=management_net       management network for remote accessability
  --dn=data_net             *-over-ICN data network
  --en=external_net         external network for remote connectivity
"""

import re, sys, subprocess
from netaddr import IPNetwork, IPAddress
from docopt import docopt
from mininet.cli import CLI
from mininet.log import setLogLevel, info, debug, error
from mininet.net import Mininet
from mininet.link import Intf
from mininet.util import quietRun
from mininet.node import Controller, RemoteController, OVSSwitch, Intf, Node, Host
from mininet.nodelib import LinuxBridge
from mininet.topo import Topo


def raise_KeyError(): raise KeyError()
def raise_ValueError(): raise ValueError()

def checkIntf( intf ):
    info('Info: Make sure intf ',intf, ' exists and is not configured.\n')
    if ( ' %s:' % intf ) not in quietRun( 'ip link show' ):
        if ( ' %s@' %intf ) not in quietRun( 'ip link show' ):
            error( 'Error:', intf, 'does not exist!\n' )
            exit( 1 )
    ips = re.findall( r'\d+\.\d+\.\d+\.\d+', quietRun( 'ifconfig ' + intf ) )
    if ips:
        debug( 'DEBUG:', intf, 'has an IP address,'
                    'and is probably in use! I will Remove the IP address and reallocate it to the bridge.\n' )
        return ips[0]
"""This method is not used at the moment but will be used to bridge the physical interface as well as the virtual"""
def checkVirtIntf( intf ):
    physIntf = re.findall(ur'([\a-\z]+\d+)[\.]', intf)
    if physIntf:
        info('Interface %s is a virtual interface, will bridge the physical interface %s as well\n', (intf, physIntf[0]))
        return physIntf[0]

class DAGTopo(Topo):
    def __init__( self, **kwargs ):
        """
        create a mininet topology, comprising sets of end_point <-> nap pairs
        :param number_end_points: number of IP end points in the topology
        :param icn_switch: the name of the ICN switch, if it is the same as the management switch or none,
        one switch will be created for both ICN and management
        :param management_switch: the name of the management switch, if it is the same as the ICN switch,
        one switch will be created for both ICN and management
        :param management_network: the network used for host accessability and remote management
        :return: mininet topology
        """
        super(DAGTopo, self).__init__( self, **kwargs )
        """create and add switches: s1 (Mgmt), s2(bridge)"""
        if kwargs['-s'] == 'ovs':
            debug('DEBUG: add switches of type OVSSwitch')
            management_sw = self.addSwitch('s1', cls=OVSSwitch, protocols=kwargs['--of'])
            if kwargs['--di'] != kwargs['--mi']:
                data_sw = self.addSwitch('s2', cls=OVSSwitch, protocols=kwargs['--of'])
        else:
            debug('DEBUG: add switches of type LinuxBridge')
            management_sw = self.addSwitch('s1', cls=LinuxBridge)
            if kwargs['--di'] != kwargs['--mi']:
                data_sw = self.addSwitch('s2', cls=LinuxBridge)

        """add clients-NAPs, the first IP addrss is taken from the input args"""
        number_pairs = int(kwargs['-n'])
        management_net = IPNetwork(kwargs['--mn'])
        prefixlen = '/%s' % str(management_net.prefixlen)
        management_ip = management_net.ip
        for i in range(1, number_pairs+1):
            debug('DEBUG: add pair: %s \n' % i)
            """add client"""
            ip = str(management_ip) + prefixlen
            client = self.addHost('client%s' % i, ip = ip)

            """increment the IP address"""
            management_ip = IPAddress(int(management_ip) + 1)
            ip = str(management_ip) + prefixlen

            """add NAP"""
            nap = self.addHost('nap%s' % i, ip = ip)
            management_ip = IPAddress(int(management_ip) + 1)

            """add client <->/NAP <-> management switch link"""
            self.addLink(client, management_sw,)
            self.addLink(nap, management_sw,)
            """add client<->NAP link"""
            self.addLink(client, nap,)

            """add NAP separate ICN link, if data_if is different from management_if"""
            if kwargs['--di'] != kwargs['--mi']:
                debug('DEBUG: add separate ICN link to nap%s\n' % i)
                self.addLink(nap, data_sw,)

def start_network(**kwargs):
    if kwargs['-t'] == 'dag':
        topo = DAGTopo(**kwargs)
    else:
        error('ERROR: unkown topology, default to DAG\n')
        topo = DAGTopo(**kwargs)

    """add controller"""
    controller_ip = kwargs['-c']
    if controller_ip != '127.0.0.1':
        debug('DEBUG: add remote controller on tcp:%s:6633' % controller_ip)
        ctl = RemoteController(name='c1',
                                ip = controller_ip,
                                protocol='tcp',
                                port=6633)
    else:
        ctl = Controller(name = 'c1')
    net = Mininet(topo=topo, controller=ctl)
    "bridge the management interface to the management switch"
    management_sw = net.switches[0]
    management_if = kwargs['--mi']
    management_if_ip = checkIntf(management_if)
    try:
        management_if_ip is not None or raise_ValueError()
        info('INFO: remove the IP address from %s \n' % management_if)
        subprocess.check_output(['ifconfig', management_if, '0.0.0.0'])
    except ValueError:
        error('ERROR: no valid IP address for remote access, mininet wont be accissable externally\n')
    info('INFO: add: %s to switch %s \n' % (management_if, management_sw.name))
    Intf(management_if, node = management_sw)

    """bridge the ICN data interface to data switch, if different from management switch"""
    try:
        data_sw = net.switches[1]
        data_if = kwargs['--di']
        data_if_ip = checkIntf(data_if)
        try:
            data_if_ip is not None or raise_ValueError()
            info('INFO: remove the IP address from %s \n' % data_if)
            subprocess.check_output(['ifconfig', data_if, '0.0.0.0'])
        except ValueError:
            info('INFO: no IP address for ICN data interface')
        info('add: %s to switch %s \n' % (data_if, data_sw.name))
        Intf(data_if, node=data_sw)
    except:
        info('INFO: no separate data switch, management switch is used for both purposes\n')

    """configure ovs switch ports"""
    if kwargs['-s'] == 'ovs':
        of_protocol = kwargs['--of']
        management_ports = management_sw.intfs
        for port in management_ports:
            management_sw.cmd('ovs-ofctl -O %s mod-port %s %s up' % (of_protocol, management_sw.name, port))
            management_sw.cmd('ovs-ofctl -O %s mod-port %s %s no-flood' % (of_protocol, management_sw.name, port))
            management_sw.cmd('ovs-vsctl -- set interface %s options:key=flow' % port)
        try:
            data_ports = data_sw.intfs
            for port in data_ports:
                data_sw.cmd('ovs-ofctl -O %s mod-port %s %s up' % (of_protocol, data_sw.name, port))
                data_sw.cmd('ovs-ofctl -O %s mod-port %s %s no-flood' % (of_protocol, data_sw.name, port))
                data_sw.cmd('ovs-vsctl -- set interface %s options:key=flow' % port)
        except:
            debug('DEBUG: no separate data switch')

    """add data network and configure remote accessability"""
    number_pairs = int(kwargs['-n'])
    data_net = IPNetwork(kwargs['--dn'])
    prefixlen = '/%s' % str(data_net.prefixlen)
    data_ip = data_net.ip
    ssh_cmd = '/usr/sbin/sshd &'
    forward_cmd = 'echo 1 > /proc/sys/net/ipv4/ip_forward'
    icmp_cmd = 'sudo iptables -I OUTPUT -p icmp --icmp-type destination-unreachable -j DROP'
    for i in range(0, number_pairs):
        ip = str(data_ip) + prefixlen
        """configure the client"""
        net.hosts[i].setIP(ip, intf=net.hosts[i].intfs[1])
        debug('DEBUG: client configure ssh')
        net.hosts[i].cmd(ssh_cmd)
        data_ip = IPAddress(int(data_ip) + 1)
        ip = str(data_ip) + prefixlen
        """configure default gw of client to be the NAP"""
        debug('DEBUG: client configure default route')
        net.hosts[i].cmd('route add default gw %s %s' % (str(data_ip), net.hosts[i].intfs[1]))

        """configure the NAP"""
        net.hosts[i + number_pairs].setIP(ip, intf=net.hosts[i + number_pairs].intfs[1])
        debug('DEBUG: NAP configure ssh\n')
        net.hosts[i + number_pairs].cmd(ssh_cmd)
        debug('DEBUG: NAP configure forward\n')
        net.hosts[i + number_pairs].cmd(forward_cmd)
        debug('DEBUG: NAP configure ICMP\n')
        net.hosts[i + number_pairs].cmd(icmp_cmd)

        """configure default route to the NAP, to push all default traffic to the cluster host"""
        try:
            management_if_ip is not None or raise_ValueError()
            debug('DEBUG: NAP configure default route\n')
            net.hosts[i + number_pairs].cmd('route add default gw %s %s' %
                                        (management_if_ip, net.hosts[i + number_pairs].intfs[0]))
        except ValueError:
            debug('DEBUG: NAP no valid management IP\n')
        data_ip = IPAddress(int(data_ip) + 1)

    """ add route to external network"""
    try:
        kwargs['--en'] is not None or raise_KeyError()
        external_net = kwargs['--en']
        route_cmd = 'route add -net %s gw %s' % (external_net, management_if_ip)
        for i in range(0, number_pairs):
            debug('DEBUG: NAP configure route to the external network\n')
            net.hosts[i].cmd(route_cmd, net.hosts[i].intfs[0])
            debug('DEBUG: NAP configure route to the external network\n')
            net.hosts[i + number_pairs].cmd(route_cmd, net.hosts[i + number_pairs].intfs[0])
    except KeyError:
        debug('DEBUG: no valid external network, mininet will not be accessable from external networks\n')

    info('INFO: starting\n')
    net.start()
    debug('DEBUG: assign the management IP to the management switch. Careful the controller may not like it\n')
    if management_if_ip is not None:
        subprocess.check_output(['ifconfig', management_sw.name, management_if_ip])

    """ add management_if and data_if to switches after net.start so the host system recongize mn bridging"""
    if kwargs['-s'] == 'ovs':
        management_sw.cmd('ovs-vsctl add port %s %s -- set interface %s options:key=flow' %
                          (management_sw.name, management_if, management_if))
        management_sw.cmd('ovs-ofctl -O %s add-flow %s "priority=0, action=NORMAL" ' %
                          (of_protocol, management_sw.name))
        try:
            data_sw.cmd('ovs-vsct add port %s %s -- set interface %s options:key=flow' %
                        (data_sw.name, data_if, data_if))
        except:
            debug('DEBUG: no separate data switch\n')
    else:
        management_sw.cmd('brctl addif %s %s' %(management_sw.name, management_if))
        try:
            data_sw.cmd('brctl addif %s %s' %(data_sw, data_if))
        except:
            debug('DEBUG: no separate data switch\n')
    """ start minient cli"""
    debug('DEBUG: start mininet cli\n')
    CLI(net)
    debug('DEBUG: stop mininet\n')
    net.stop()
    debug('DEBUG: re-assign management and data IPs to respective host interfaces\n')
    if management_if_ip is not None:
        subprocess.check_output(['ifconfig' ,management_if, management_if_ip])
    try:
        subprocess.check_output(['ifconfig', data_if, data_if_ip])
    except:
        debug('DEBUG: no separate data switch\n')

def parse_required(**arguments):
    try:
        arguments['--mi'] is not None or raise_KeyError()
    except KeyError:
        error('ERROR: management interface not specified, try the data interface before giving up\n')
        try:
            arguments['--di'] is not None or raise_KeyError()
            arguments['--mi'] = arguments['--di']
        except KeyError:
            error('ERROR: no valid interface for external access, giving up\n')
            exit(1)
    try:
        arguments['--di'] is not None or raise_KeyError()
    except KeyError:
        debug('DEBUG: data interface not specified, will use management interface as data interface\n')
        arguments['--di'] = arguments['--mi']
    try:
        arguments['--mn'] is not None or raise_KeyError()
    except KeyError:
        error('ERROR: management network not speicified\n')
        exit(1)
    try:
        arguments['--dn'] is not None or raise_KeyError()
    except KeyError:
        error('ERROR: data network not speicified\n')
        exit(1)
    try:
        arguments['--en']
    except:
        debug('DEBUG: external network not specified, it will not possible to have remote connectivity\n')
    return arguments


def main(**kwargs):
    try:
        log_level = arguments['-l']
    except:
        log_level = 'info'
        info('log level not specified, default to INFO')
    setLogLevel( log_level )
    kwargs = parse_required(**kwargs)
    mn = start_network(**kwargs)
    # info(mn)


if __name__ == '__main__':
    arguments = docopt(__doc__, version='ICN mininet version: 0.2')
    main(**arguments)
