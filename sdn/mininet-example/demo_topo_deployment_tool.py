#!/usr/bin/env python

import time, re, sys, subprocess, os
from mininet.cli import CLI
from mininet.log import setLogLevel, info, debug, error
from mininet.net import Mininet
from mininet.link import Intf
from mininet.util import quietRun
from mininet.node import Controller, RemoteController, OVSSwitch, Intf, Node, Host
from mininet.topo import Topo

class DemoTopo( Topo ):
	def __init__( self ):
		Topo.__init__( self )

		h1 = self.addHost( 'h1' )
		h2 = self.addHost( 'h2' )

		s1 = self.addSwitch( 's1',  protocols='OpenFlow13' )
		s2 = self.addSwitch( 's2',  protocols='OpenFlow13' )
		s3 = self.addSwitch( 's3',  protocols='OpenFlow13' )
		s4 = self.addSwitch( 's4',  protocols='OpenFlow13' )

		self.addLink(h1, s1)
		self.addLink(s1, s2)
		self.addLink(s1, s3)
		self.addLink(s2, s3)
		self.addLink(s3, h2)
		self.addLink(h1, s4)
		self.addLink(h2, s4)

if __name__ == "__main__":
	topo = DemoTopo()

	ctl = RemoteController( name='c1', ip='192.168.56.1', protocol='tcp', port=6633 )
	net = Mininet( topo=topo, controller=ctl )

	net.hosts[0].setMAC( mac="00:00:00:00:00:01", intf=net.hosts[0].intfList()[0] )
	net.hosts[0].setIP( ip="192.168.2.1/24", intf=net.hosts[0].intfList()[0] )
	net.hosts[0].setMAC( mac="00:00:00:00:11:11", intf=net.hosts[0].intfList()[1] )
	net.hosts[0].setIP( ip="192.168.1.2/24", intf=net.hosts[0].intfList()[1] )
	net.hosts[1].setMAC( mac="00:00:00:00:00:02", intf=net.hosts[1].intfList()[0] )
	net.hosts[1].setIP( ip="192.168.2.2/24", intf=net.hosts[1].intfList()[0] )
	net.hosts[1].setMAC( mac="00:00:00:00:22:22", intf=net.hosts[1].intfList()[1] )
	net.hosts[1].setIP( ip="192.168.1.3/24", intf=net.hosts[1].intfList()[1] )
	net.start()
	net.hosts[0].cmd('route add default gw 192.168.1.1 h1-eth1')
	net.hosts[1].cmd('route add default gw 192.168.1.1 h2-eth1')
	net.hosts[0].cmd('/usr/sbin/sshd &')
	net.hosts[1].cmd('/usr/sbin/sshd &')
	os.system('echo 1 | tee /proc/sys/net/ipv4/ip_forward')
	os.system('ifconfig s4 192.168.1.1/24')
	os.system('ovs-ofctl -O OpenFlow13 add-flow s4 actions=NORMAL')
	CLI(net)