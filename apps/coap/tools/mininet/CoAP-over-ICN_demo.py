#!/usr/bin/python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.link import Intf
from subprocess import call
import time

'''
(Real Interface)--s1-- |--snap1--|               |--cnap1--c1
                                 |               |
(Real Interface)--s2-- |--snap2--|--|  FW  RVTM  |--cnap2--c2
                                 |  |  |      |  |
(Real Interface)--s2-- |--snap3--|  |  |      |  |--cnap3--c3 
                                    |-----i1-----|  

'''

emulateDemo          = True           #change to True to create virtual coap servers
#emulateDemo			= False
configuration_folder = "Demo" 
#configuration_folder = "Aalto" #change to Aalto to test Aalto devices https://wiki.point-h2020.eu/pointwiki/index.php?title=ICN_Demo#URLs_and_IP_addresses
class DemoTopology(Topo):
    def build(self):
        #switches
        i1    = self.addSwitch('i1')
        s1    = self.addSwitch('s1')
        s2    = self.addSwitch('s2')
        s3    = self.addSwitch('s3')
        c1    = self.addSwitch('c1')
        c2    = self.addSwitch('c2')
        c3    = self.addSwitch('c3')
        #hosts
        snap1 = self.addHost('snap1')
        snap2 = self.addHost('snap2')
        snap3 = self.addHost('snap3')
        cnap1 = self.addHost('cnap1')
        cnap2 = self.addHost('cnap2')
        cnap3 = self.addHost('cnap3')
        fw    = self.addHost('fw')
        rvtm  = self.addHost('rvtm')
        #Connections
        self.addLink(snap1,i1)
        self.addLink(snap2,i1)
        self.addLink(snap3,i1)
        self.addLink(snap1,s1)
        self.addLink(snap2,s2)
        self.addLink(snap3,s3)
        self.addLink(fw,i1)
        self.addLink(rvtm,i1)
        self.addLink(cnap1,i1)
        self.addLink(cnap2,i1)
        self.addLink(cnap3,i1)
        self.addLink(cnap1,c1)
        self.addLink(cnap2,c2)
        self.addLink(cnap3,c3)
        '''
        The following hosts are coap server used only for testing purposes
        '''
        if emulateDemo == True:
            west1 = self.addHost('west1')
            east1 = self.addHost('east1')
            west2 = self.addHost('west2')
            east2 = self.addHost('east2')
            west3 = self.addHost('west3')
            east3 = self.addHost('east3')
            self.addLink(west1,s1)
            self.addLink(west2,s2)
            self.addLink(west3,s3)
            self.addLink(east1,s1)
            self.addLink(east2,s2)
            self.addLink(east3,s3)

def createNetwork():
    topo  = DemoTopology()
    net   = Mininet(topo)
    snap1 = net.get('snap1')
    snap2 = net.get('snap2')
    snap3 = net.get('snap3')
    cnap1 = net.get('cnap1')
    cnap2 = net.get('cnap2')
    cnap3 = net.get('cnap3')
    fw    = net.get('fw')
    rvtm  = net.get('rvtm')
    # mac addresses for ICN part
    cnap1.setMAC( "00:00:00:00:00:01" )
    cnap2.setMAC( "00:00:00:00:00:02" )
    cnap3.setMAC( "00:00:00:00:00:03" )
    snap1.setMAC( "00:00:00:00:00:09" )
    snap2.setMAC( "00:00:00:00:00:10" )
    snap3.setMAC( "00:00:00:00:00:11" )
    fw.setMAC   ( "00:00:00:00:00:07" )
    rvtm.setMAC ( "00:00:00:00:00:08" )
    '''
    The following is the netowrk configuration for snaps
    addapt accordingly
    '''
    info     ( '*** Configuring snap1 \n' )
    snap1.cmd( "ip addr flush dev snap1-eth0" )
    snap1.cmd( "ip addr flush dev snap1-eth1" )
    snap1.cmd( "ifconfig snap1-eth1 inet6 add fe80::da80:39ff:fe02:1/64" )
    snap1.cmd( "ifconfig snap1-eth1 up" )
    
    info     ( '*** Configuring snap2 \n')
    snap2.cmd( "ip addr flush dev snap2-eth0" )
    snap2.cmd( "ip addr flush dev snap2-eth1" )
    snap2.cmd( "ifconfig snap2-eth1 inet6 add fe80::da80:39ff:fe02:2/64" )
    snap2.cmd( "ifconfig snap2-eth1 up" )
    
    info     ( '*** Configuring snap3 \n' )
    snap3.cmd( "ip addr flush dev snap3-eth0" )
    snap3.cmd( "ip addr flush dev snap3-eth1" )
    snap3.cmd( "ifconfig snap3-eth1 inet6 add fe80::da80:39ff:fe02:3/64" )
    snap3.cmd( "ifconfig snap3-eth1 up" ) 
    '''
    end of configuration
    '''
    '''
    The following is the netowrk configuration for cnaps
    addapt accordingly
    '''
    info     ( '*** Configuring cnap1 \n' )
    cnap1.cmd( "ip addr flush dev cnap1-eth1" )
    cnap1.cmd( "ifconfig cnap1-eth1 172.16.191.2 netmask 255.255.255.0" )
    cnap1.cmd( "ifconfig cnap1-eth1 up" )
    
    info     ( '*** Configuring cnap2 \n' )
    cnap2.cmd( "ip addr flush dev cnap2-eth1" )
    cnap2.cmd( "ifconfig cnap2-eth1 172.16.192.2 netmask 255.255.255.0" )
    cnap2.cmd( "ifconfig cnap2-eth1 up" )

    info     ( '*** Configuring cnap3 \n' )
    cnap3.cmd( "ip addr flush dev cnap3-eth1" )
    cnap3.cmd( "ifconfig cnap3-eth1 172.16.193.2 netmask 255.255.255.0" )
    cnap3.cmd( "ifconfig cnap3-eth1 up" )
    '''
    end of configuration
    '''
    '''
    The following configuration is for coap server and it is used only 
    for debugging purposes. 
    '''
    if emulateDemo == True:
        west1 = net.get('west1')
        west2 = net.get('west2')
        west3 = net.get('west3')
        east1 = net.get('east1')
        east2 = net.get('east2')
        east3 = net.get('east3')
        
        info     ( '*** Configuring west1 \n' )
        west1.cmd( "ip addr flush dev west1-eth0" )
        west1.cmd( "ifconfig west1-eth0 inet6 add fe80::da80:39ff:fe02:c0c7/64" )
        west1.cmd( "ifconfig west1-eth0 up" )
        
        info     ( '*** Configuring west2 \n' )
        west2.cmd( "ip addr flush dev west2-eth0" )
        west2.cmd( "ifconfig west2-eth0 inet6 add fe80::da80:39ff:fe02:d16e/64" )
        west2.cmd( "ifconfig west2-eth0 up" )
        
        info     ( '*** Configuring west3 \n' )
        west3.cmd( "ip addr flush dev west3-eth0" )
        west3.cmd( "ifconfig west3-eth0 inet6 add  fe80::da80:39ff:fe02:c118/64" )
        west3.cmd( "ifconfig west3-eth0 up" )
        
        info     ( '*** Configuring east1 \n' )
        east1.cmd( "ip addr flush dev east1-eth0" )
        east1.cmd( "ifconfig east1-eth0 inet6 add fe80::da80:39ff:fe02:c0e7/64" )
        east1.cmd( "ifconfig east1-eth0 up" )
        
        info     ( '*** Configuring east2 \n' )
        east2.cmd( "ip addr flush dev east2-eth0" )
        east2.cmd( "ifconfig east2-eth0 inet6 add fe80::da80:39ff:fe02:d164/64" )
        east2.cmd( "ifconfig east2-eth0 up" )
        
        info     ( '*** Configuring east3 \n' )
        east3.cmd( "ip addr flush dev east3-eth0" )
        east3.cmd( "ifconfig east3-eth0 inet6 add fe80::da80:39ff:fe02:d176/64" )
        east3.cmd( "ifconfig east3-eth0 up" )
    '''
    end of configuration
    '''
    net.start()
    info      ( '*** Starting blackadder \n' )
    cnap1.cmd ( 'click click_conf/Demo/cnap0001.conf >output/cnap1CLICK.out 2>&1&' )
    cnap2.cmd ( 'click click_conf/Demo/cnap0002.conf >output/cnap2CLICK.out 2>&1&' )  
    cnap3.cmd ( 'click click_conf/Demo/cnap0003.conf >output/cnap3CLICK.out 2>&1&' )  
    fw.cmd    ( 'click click_conf/Demo/fw000000.conf >output/FW.out 2>&1&' ) 
    rvtm.cmd  ( 'click click_conf/Demo/rvtm0000.conf >output/RV.out 2>&1&' ) 
    snap1.cmd ( 'click click_conf/Demo/snap0001.conf >output/snap1CLICK.out 2>&1&' )
    snap2.cmd ( 'click click_conf/Demo/snap0002.conf >output/snap2CLICK.out 2>&1&' )
    snap3.cmd ( 'click click_conf/Demo/snap0003.conf >output/snap3CLICK.out 2>&1&' ) 
    info( '*** I am going to sleep for one sec. \n' )
    time.sleep(1)
    info( '*** Starting topology manager on host rvtm \n' )
    rvtm.cmd( ' ../../../../TopologyManager/tm  click_conf/Demo/topology.graphml >output/TM.out 2>&1&' )
    info( '*** Configuring host network interfaces\n' )

    if emulateDemo == False:
        call(["ovs-vsctl", "add-port", "s1", "eth0.100"])
        call(["ovs-vsctl", "add-port", "s2", "eth0.200"])
        call(["ovs-vsctl", "add-port", "s3", "eth0.300"])

    call(["ifconfig", "c1", "172.16.191.1", "netmask", "255.255.255.0"])
    call(["ifconfig", "c2", "172.16.192.1", "netmask", "255.255.255.0"])
    call(["ifconfig", "c3", "172.16.193.1", "netmask", "255.255.255.0"])
    info( '*** I am going to sleep for one sec. \n' )
    time.sleep(1)
    info( '*** Starting snap1 \n' )
    snap1.cmd( ' ../../core/pointcoap  config/' + configuration_folder + '/snap1.conf >output/snap1NAP.out 2>&1&' )
    info( '*** I am going to sleep for one sec. \n' )
    time.sleep(1)
    info( '*** Starting snap2 \n' )
    snap2.cmd( ' ../../core/pointcoap  config/' + configuration_folder + '/snap2.conf >output/snap2NAP.out 2>&1&' )
    info( '*** I am going to sleep for one sec. \n' )
    time.sleep(1)
    info( '*** Starting snap3 \n' )
    snap3.cmd( ' ../../core/pointcoap  config/' + configuration_folder + '/snap3.conf >output/snap3NAP.out 2>&1&' )
    info( '*** I am going to sleep for one sec. \n' )
    time.sleep(1)
    info( '*** Starting cnap1 \n' )
    cnap1.cmd( ' ../../core/pointcoap >output/cnap1NAP.out 2>&1&' )
    info( '*** Starting cnap2 \n' )
    cnap2.cmd( ' ../../core/pointcoap >output/cnap2NAP.out 2>&1&' )
    info( '*** Starting cnap3 \n' )
    cnap3.cmd( ' ../../core/pointcoap >output/cnap3NAP.out 2>&1&' )
    if emulateDemo == True:
        info( '*** Starting CoAP Servers \n' )
        west1.cmd( '../coap-apps/demo_server >output/west1SERVER.out 2>&1&' )
        west2.cmd( '../coap-apps/demo_server >output/west2SERVER.out 2>&1&' )
        west3.cmd( '../coap-apps/demo_server >output/west3SERVER.out 2>&1&' )
        east1.cmd( '../coap-apps/demo_server >output/east1SERVER.out 2>&1&' )
        east2.cmd( '../coap-apps/demo_server >output/east2SERVER.out 2>&1&' )
        east3.cmd( '../coap-apps/demo_server >output/east3SERVER.out 2>&1&' )
        
    info( '*** Starting CLI \n' )
    CLI( net )
    net.stop()
            
if __name__ == '__main__':
    setLogLevel('info')
    createNetwork()

