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
from netaddr import IPNetwork, IPAddress

class ICNNode(object):
    def __init__(self, *args, **kwargs):
        logger = logging.getLogger(__name__)
        self.connections = []
        try:
            self.platform = kwargs['platform']
        except:
            logger.info('no valid type, default to access node')
            self.platform = ''
        try:
            self.label = kwargs['label']
        except:
            logger.info('no valid label')
            exit(1)
        try:
            self.testbed_ip = kwargs['testbed_ip']
        except:
            logger.info('no valid testbed_ip')
            exit(1)
        try:
            self.running_mode = kwargs['running_mode']
        except:
            self.running_mode = 'user'
        try:
            self.role = kwargs['role']
        except:
            self.role = []
        try:
            self.name = kwargs['name']
        except:
            logger.debug('no valid name, will default to nap')
            self.name = 'nap'
        try:
            self.operating_system = kwargs['operating_system']
        except:
            logger.info('default operating system is Linux')
            self.operating_system = 'Linux'
        try:
            self.sdn_implementation = kwargs['sdn_implementation']
        except:
            if self.operating_system == 'ovs':
                logger.info('default sdn_implementation is ports')
                self.sdn_implementation = 'ports'
            else:
                self.sdn_implementation = None
        try:
            self.bridge = kwargs['bridge']
        except:
            if self.sdn_implementation == 'ovs':
                logger.info('default bridge name is: br')
                self.bridge = 'br'
            else:
                self.bridge = None
        self.promisc = False
        try:
            for c in kwargs['connections']:
                logger.debug(c)
                connection = ICNConnection(**c)
                connection.from_node = self.label
                if not connection:
                    logger.error('failed to parse node connection')
                    exit(1)
                else:
                    self.connections.append(connection)
        except Exception, e:
            self.connections = []
            logger.warning('node %s without connections due to : %s' % (self.label, str(e)))

class ICNConnection(object):
    def __init__(self, *args, **kwargs):
        logger = logging.getLogger(__name__)
        self.from_node = None
        try:
            self.to_node = kwargs['to']
        except:
            self.to_node = None
        try:
            self.src_if = kwargs['src_if']
        except:
            logger.debug('no src_if, expect src_pt instead')
        try:
            self.src_pt = kwargs['src_pt']
        except:
            logger.debug('no src_pt, expect src_if instead')
        try:
            self.dst_if = kwargs['dst_if']
        except:
            logger.debug('no dst_if, expect dst_pt instead')
        try:
            self.dst_pt = kwargs['dst_pt']
        except:
            logger.debug('no dst_pt, expect dst_if instead')
        try:
            self.name = kwargs['name']
        except:
            logger.debug('name must be provided for mininet nodes, otherwise default is nap')
            self.name = 'nap'
        try:
            self.bridge = kwargs['bridge']
        except:
            logger.debug('bridge must be provided for mininet nodes, otherwise default is br')
            self.bridge = 'br'

class ICNCluster(object):
    def __init__(self, *args, **kwargs):
        log_level = kwargs['log']
        logging.basicConfig(format = '%(levelname)s: %(message)s', level = log_level)
        logger = logging.getLogger(__name__)
        self.nodes = []
        self.cluster_ip = None
        self.hosts = 0
        self.start_ip = None



class ICNetwork(object):
    def __init__(self, filename, log_level):
        logging.basicConfig(format = '%(levelname)s: %(message)s', level = log_level)
        logger = logging.getLogger(__name__)
        self.clusters = []
        cluster = ICNCluster(log=log_level)
        try:
            with open(filename) as config_file:
                icn_conf = json.load(config_file)
            try:
                self.root_dir = icn_conf['ROOT_DIR']
            except Exception, e:
                logger.error('failed to parse root directory: %s' % str(e))
            try:
                self.write_topo = icn_conf['WRITE_TOPO']
            except Exception, e:
                logger.error('failed to parse write conf: %s' % str(e))
            try:
                self.management_net = icn_conf['MANAGEMENT_NET']
            except Exception, e:
                logger.error('failed to parse management network: %s' % str(e))
            try:
                self.click_global_conf = icn_conf['CLICK_GLOBAL_CONF']
            except Exception, e:
                logger.error('failed to parse click global config params: %s' % str(e))
            try:

                start_ip = IPNetwork(self.management_net).ip
                for c in icn_conf['clusters']:
                    try:
                        cluster = ICNCluster(log=log_level)
                        for n in icn_conf['cluster_topology']:
                            icn_node = ICNNode(**n)
                            if not icn_node:
                                logger.error('something went wrong with node parsing')
                                exit(1)
                            else:
                                cluster.nodes.append(icn_node)
                                if icn_node.platform == 'mininet' and icn_node.operating_system in ['Linux', 'Darwin']:
                                    cluster.hosts += 1
                            logger.debug('number of mininet hosts: %s' % str(cluster.hosts))
                        cluster.start_ip = start_ip
                        cluster.cluster_ip = c['cluster_ip']
                        self.clusters.append(cluster)
                        start_ip = IPAddress(int(start_ip) + (cluster.hosts * 2))
                    except Exception, e:
                        logger.error('failed to parse cluster IP, exiting...')
                        exit(1)
            except Exception, e:
                logger.error('failed to parse the ICN cluster topology: %s' % str(e))
        except Exception, e:
            logger.error('smothing wrong with the configuration file: %s' % str(e))

    def update_clusters(self):
        logger = logging.getLogger(__name__)

        """update the testbed_ip of each cluster nodes using the cluster start IP"""
        number_clusters = len(self.clusters)
        logger.debug('number of cluster: %s' % number_clusters)
        for i in range(1, number_clusters):
            ip = self.clusters[i].start_ip
            logger.debug('cluster %s: start ip: %s' % (str(i),str(ip)))
            for node in self.clusters[i].nodes:
                logger.debug('before: label: %s, testbed_ip: %s, platform: %s' % (node.label, node.testbed_ip, node.platform))
                label = node.label
                to_nodes = []
                if node.platform == 'mininet' and node.operating_system in ['Linux', 'Darwin']:
                    node.testbed_ip = ip
                    node.label = self.ip_to_label(ip = node.testbed_ip)
                elif node.platform == 'mininet' and node.operating_system == 'ovs':
                    node.testbed_ip = self.clusters[i].cluster_ip
                    node.label = self.ip_to_label(ip=node.testbed_ip, words = [3])
                elif node.platform != 'mininet' and node.operating_system == 'ovs':
                    src_pt = int(node.connections[0].src_pt) + i
                    bridge = node.connections[0].bridge
                ip = ip + 2
                logger.debug('after: label: %s, testbed_ip: %s, platform: %s' % (node.label, node.testbed_ip, node.platform))
                """update node connections and src_pt"""
                for connection in node.connections:
                    connection.from_node = node.label
                    if node.platform != 'mininet' and node.operating_system == 'ovs':
                        connection.src_pt = str(src_pt)
                        connection.bridge = bridge
                    """ collect to_nodes"""
                    to_nodes.append(connection.to_node)
                """update adjacent nodes to the node"""
                for adj_node in self.clusters[i].nodes:
                    if adj_node.label in to_nodes:
                        for adj_connection in adj_node.connections:
                            if adj_connection.to_node == label:
                                adj_connection.to_node = node.label
                                if node.platform != 'mininet' and node.operating_system == 'ovs':
                                    adj_connection.dst_pt = src_pt
                logger.debug('after conn: label: %s, testbed_ip: %s, platform: %s' % (node.label, node.testbed_ip, node.platform))

    @staticmethod
    def ip_to_label(label_len = 8, ip = None, words=[2,3]):
        logger = logging.getLogger(__name__)
        label_suffix = ""
        ip = IPAddress(ip)
        for i in words:
            label_suffix = label_suffix + str(ip.words[i])
        logger.info('label suffix %s ' % label_suffix)
        label_suffix_len = len(label_suffix)
        label_prefix_len = label_len - label_suffix_len
        label = '0' * label_prefix_len + label_suffix
        return label

    def merge_clusters_to_network(self):
        logger = logging.getLogger(__name__)
        number_clusters = len(self.clusters)
        for i in range(0, number_clusters):
            for node in self.clusters[i].nodes:
                for j in range(0, number_clusters):
                    for other_node in self.clusters[j].nodes:
                        if i != j and node.label == other_node.label:
                            node.connections.extend(other_node.connections)
                            index = self.clusters[j].nodes.index(other_node)
                            del self.clusters[j].nodes[index]

    def write_network_config(self):
        """
        write_network_config:
        wirte the NAP connections to a file - careful, this assumes a strict sequence of creation where:
        NAP ips always start after client IPs, therefore index is always +num_clients
        NAP management interface is the one to be used for blackadder connections
        """
        logger = logging.getLogger(__name__)
        logger.info('---------------------- write ICN network configuration ----------------------')
        if not os.path.exists(self.root_dir + self.write_topo):
            os.makedirs(self.root_dir + self.write_topo)
        click_conf = open(self.root_dir + self.write_topo + '/mn_icn.cfg', "w")
        global_conf = self.click_global_conf
        lines = (key + '= ' + str(global_conf[key]) + ';\n' if isinstance(global_conf[key], int)
                 else key + '= "%s";'% str(global_conf[key]) + '\n'
                 for key in global_conf)
        click_conf.writelines(lines)
        lines = ['network = { \n \t', 'nodes = ( \n \t \t']
        click_conf.writelines(l for l in lines)
        written_nodes = []
        node_counter = 0
        for cluster in self.clusters:
            for node in cluster.nodes:
                logger.debug('Node: label: %s, testbed_ip: %s' % (node.label, node.testbed_ip))
                lines = [',{' if node_counter > 0 else '{',
                         '\n \t \t \t',
                         'testbed_ip = "%s"; \n \t \t \t' % node.testbed_ip,
                         'label = "%s"; \n \t \t \t' % node.label,
                         'running_mode = "%s"; \n \t \t \t' % node.running_mode,
                         'role = "%s"; \n \t \t \t' % node.role,
                         'name = "%s"; \n \t \t \t' % node.name,
                         'promisc = ' + str(node.promisc).lower() + '; \n \t \t \t']
                if node.operating_system == 'ovs':
                    lines.extend(['operating_system = "%s"; \n \t \t \t' % node.operating_system,
                                  'sdn_implementation ="%s"; \n \t \t \t' % node.sdn_implementation,
                                  'bridge = "%s"; \n \t \t \t' % node.bridge])
                lines.extend(['connections = (\n \t \t \t',
                              '{\n \t \t \t \t'])
                click_conf.writelines(l for l in lines)
                connection_counter = 0
                connections = node.connections
                number_connections = len(connections)
                for connection in connections:
                    logger.debug('Connections:')
                    logger.debug(connection.__dict__.keys())
                    logger.debug(connection.__dict__.values())
                    lines = ['to = "%s"; \n \t \t \t \t' % connection.to_node,
                             'bridge = "%s"; \n \t \t \t \t' % connection.bridge,
                             ('src_if = "%s"; \n \t \t \t \t' % connection.src_if
                              if hasattr(connection, 'src_if')
                              else 'src_pt = "%s"; \n \t \t \t \t' % connection.src_pt),
                             ('dst_if = "%s"; \n \t \t \t' % connection.dst_if
                              if hasattr(connection, 'dst_if')
                              else 'dst_pt = "%s"; \n \t \t \t' % connection.dst_pt)]
                    click_conf.writelines(l for l in lines)
                    lines = (['}, \n \t \t \t',
                             '{ \n \t \t \t \t'] if connection_counter < number_connections -1
                             else ['} \n \t \t \t', '); \n \t \t', '}\n \t \t'])
                    click_conf.writelines(l for l in lines)
                    connection_counter += 1
                node_counter += 1
        lines = ['); \n', '};\n']
        click_conf.writelines(l for l in lines)
        click_conf.close()
