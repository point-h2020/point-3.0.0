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

import logging, json, os, sys, re
import paramiko

from time import sleep
from subprocess import  Popen, PIPE, STDOUT
from icn_mn import *
from ICN import *
from netaddr import IPNetwork, IPAddress

class MnCluster(object):
    def __init__(self, **kwargs):
        logger = logging.getLogger(__name__)
        try:
            self.cluster_ip = kwargs['cluster_ip']
        except:
            logger.error('no valid host IP %s' % str(e))
            exit(1)
        try:
            self.topology = kwargs['topology']
        except:
            logger.info('topology is not specified, default to DAG')
            self.topology = 'dag'
        try:
            self.number_pairs = int(kwargs['number_pairs'])
        except:
            logger.info('number of pairs is not specified, default to 1')
            self.number_pairs = 1
        try:
            self.switch_type = kwargs['switch_type']
        except:
            logger.info('switch type is not specified, default to OVS')
        try:
            self.controller_ip = kwargs['controller_ip']
        except:
            self.controller_ip = '127.0.0.1'
            logger.info('controller IP is not specified, use default local controller')
        try:
            self.management_if = kwargs['management_if']
        except:
            logger.info('management interface is not specified, will check ICN interface before giving up')
            try:
                self.management_if = kwargs['data_if']
            except:
                logger.error('no valid management or ICN interface')
                exit(1)
        try:
            self.data_if = kwargs['data_if']
        except:
            logger.info('ICN interface is not specified, will use the management interface for ICN')
            self.data_if = kwargs['management_if']

class MnClusters(object):
    """
      all attributes will be defined inside __init__ to be instance (NOT class!!) attributes
    """
    def __init__(self, filename, log_level):
        logging.basicConfig(format = '%(levelname)s: %(message)s', level = log_level)
        logger = logging.getLogger(__name__)
        self.clusters = []
        try:
            with open(filename) as config_file:
                mn_conf = json.load(config_file)
            try:
                self.user = mn_conf['USER']
            except Exception, e:
                logger.error('Faild to parse the user name: %s' % str(e))
                exit( 1 )
            try:
                self.root_dir = mn_conf['ROOT_DIR']
            except Exception, e:
                logger.error('Failed to parse the root directory: %s' % str(e))
                exit( 1 )
            try:
                self.read = mn_conf['READ']
            except Exception, e:
                logger.error('Failed to parse the read files: %s' % str(e))
                exit(1)
            try:
                self.management_net = mn_conf['MANAGEMENT_NET']
            except Exception, e:
                logger.error('Failed to parse the management network: %s' % str(e))
                exit( 1 )
            try:
                self.data_net = mn_conf['DATA_NET']
            except Exception, e:
                logger.error('Failed to parse the data network: %s' % str(e))
                exit( 1 )
            try:
                self.external_net = mn_conf['EXTERNAL_NET']
            except Exception, e:
                logger.warning('Failed to parse the external network: %s, will assume no external net is set' % str(e))
            try:
                for c in mn_conf['clusters']:
                    cluster = MnCluster(**c)
                    if not cluster:
                        logger.error('something went wrong with the cluster, continue to next')
                    else:
                        self.clusters.append( cluster )
            except Exception, e:
                logger.error('Failed to parse the cluster nodes: %s' % str(e))
        except Exception, e:
            logger.error('smothing wrong with the configuration file: %s' % str(e))

    def construct_clusters( self ):
        logger = logging.getLogger(__name__)
        logger.info('clear any existing clusters')
        self.clean_clusters()
        read_conf = self.root_dir + self.read['icn_mn']
        cluster_management_net = IPNetwork(self.management_net)
        cluster_data_net = IPNetwork(self.data_net)
        for cluster in self.clusters:
            successful_connection = False
            attempts = 1
            logger.info('Establishing ssh session to %s' % cluster.cluster_ip)
            while not successful_connection and attempts < 6:
                try:
                    remote_host = paramiko.SSHClient()
                    remote_host.set_missing_host_key_policy(paramiko.AutoAddPolicy())
                    remote_host.connect(cluster.cluster_ip, username = self.user)
                    logger.debug( 'succeful ssh to %s after %s attempt(s).' % ( cluster.cluster_ip, attempts ) )
                    """ create mininet in a screen """
                    command = 'screen -d -m -S mnet sudo ' + read_conf + \
                      ' -t %s -n %d -s %s --mi %s --di %s --mn %s --dn %s --en %s'%( cluster.topology,
                                                                                cluster.number_pairs,
                                                                                cluster.switch_type,
                                                                                cluster.management_if,
                                                                                cluster.data_if,
                                                                                cluster_management_net,
                                                                                cluster_data_net,
                                                                                self.external_net)
                    """add the contoller IP"""
                    if cluster.switch_type == 'ovs':
                        command = command + ' -c %s' % cluster.controller_ip
                    logger.debug(command)
                    stdin, stdout, stderr = remote_host.exec_command(command)
                    logger.error(stderr.read())
                    remote_host.close()
                    logger.debug('ssh connection to %s closed.' % str(cluster.cluster_ip))
                    """ increase the management and data network addresses"""
                    next_management_ip = IPAddress(int(cluster_management_net.ip) + (cluster.number_pairs * 2))
                    cluster_management_net = IPNetwork(str(next_management_ip) +
                                                        '/' +
                                                        str(cluster_management_net.prefixlen))
                    next_data_ip = IPAddress(int(cluster_data_net.ip) + (cluster.number_pairs * 2))
                    cluster_data_net = IPNetwork(str(next_data_ip) +
                                                  '/' +
                                                  str(cluster_data_net.prefixlen ))
                    successful_connection = True
                except:
                    logger.debug( 'failed ssh to %s after %s attempt(s).' % (cluster.cluster_ip, attempts))
                    attempts += 1

    def clean_clusters( self ):
        logger = logging.getLogger(__name__)
        """ the original command is commented becasue the keystroke is not taken via ssh,
            instead the command is encapsulated in a bash script and placed in the homedir
             of each hosting cluster then ssh only run the bash script"""
        # cmd_exit = 'screen -S mnet -p 0 -X stuff "exit$(printf \\r)"'
        cmd_exit = self.root_dir + self.read['exit_mn']
        logger.debug('clear mininet cluster with: %s' % cmd_exit)
        for cluster in self.clusters:
            cluster_ip = IPAddress(cluster.cluster_ip)
            try:
                logger.debug('sshing to host: %s' % str(cluster_ip))
                remote_host = paramiko.SSHClient()
                remote_host.set_missing_host_key_policy(paramiko.AutoAddPolicy())
                remote_host.connect(str(cluster_ip), username=self.user)
                remote_host.exec_command(cmd_exit)
                remote_host.close()
            except Exception, e:
                logger.debug("sshing to host: %s has failed due to: %s" % (str(cluster_ip), str(e)))

    def write_ip_lists( self ):
        """
        write the list of IPs for remote management of both naps and endpoints in mininet.
        writing is done into a file structure:
        ~/blackadder/mn_testbed/naps for naps IPs
        ~/blackadder/mn_testbed/end_points for end_points IPs
        """
        management_net = IPNetwork( str( self.management_net ) )
        management_ip = management_net.ip
        nap_file_path = self.root_dir + self.read['naps']
        ep_file_path = self.root_dir + self.read['end_points']
        for cluster in self.clusters:
            cluster_ip = IPAddress(cluster.cluster_ip)
            label = ICNetwork.ip_to_label(ip=cluster_ip, words=[3])
            nap_filename = 'nap-%s' % label
            nap_nodes = open( nap_file_path + nap_filename, "w" )
            ep_filename = 'endpoint-%s' % label
            ep_nodes = open( ep_file_path + ep_filename, "w" )
            for c in range(0, cluster.number_pairs):
                ep_nodes.write('%s\n' % management_ip)
                management_ip = IPAddress(int( management_ip ) + 1)
                nap_nodes.write('%s\n' % management_ip)
                management_ip = IPAddress( int( management_ip ) + 1 )
            ep_nodes.close()
            nap_nodes.close()
