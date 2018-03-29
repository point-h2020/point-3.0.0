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
from ICN import *
from netaddr import IPNetwork, IPAddress

class IPoverICNCluster(object):
    def __init__(self, filename, log_level):
        logging.basicConfig(format = '%(levelname)s: %(message)s', level = log_level)
        logger = logging.getLogger(__name__)
        try:
            with open(filename) as config_file:
                nap_conf = json.load(config_file)
            try:
                self.user = nap_conf['USER']
            except Exception, e:
                logger.error('failed to parse username: %s' % str(e))
                exit(1)
            try:
                self.root_dir = nap_conf['ROOT_DIR']
            except Exception, e:
                logger.error('failed to parse root directory, default /home/%s: %s' % (self.user, str(e)))
                self.root_dir = '/home/' + self.user
            try:
                self.write_conf = nap_conf['WIRTE_CONFIG']
            except Exception, e:
                logger.error('failed to parse write path, default to /home/%s/napConfig: %s' % (self.user, str(e)))
            try:
                self.management_net = nap_conf['MANAGEMENT_NET']
            except Exception, e:
                logger.error('failed to parse management network, wont ssh configs to nodes: %s' % str(e))
            try:
                self.data_net = nap_conf['DATA_NET']
            except Exception, e:
                logger.error('failed to parse data network, exiting...: %s' % str(e))
                exit(1)
            try:
                self.number_naps = nap_conf['NUMBER_NAPS']
            except Exceptions, e:
                logger.error('failed to parse number of naps, default to 2: %s' % str(e))
                self.number_naps = 2
            try:
                self.interface = nap_conf['napconf']['if']
            except Exception, e:
                logger.error('failed to parse IP interface, exiting... : %s' % str(e))
            try:
                self.clusters = nap_conf['clusters']
            except Exceptions, e:
                logger.error('failed to parse cluster IP, wont ssh...: %s' % str(e))
            try:
                self.routePrefixes = nap_conf['napconf']['routingPrefixes']
            except Exception, e:
                logger.error('failed to parse routing prefixes, exiting...: %s' % str(e))
        except Exception, e:
            logger.error('something went wrong with the NAP config file, exiting...: %s' % str(e))

    def write_nap_config(self):
        """
        wirte the NAP config files and ssh them to the corresponding server hosting the mininet cluster to which the naps belong
        """
        logger = logging.getLogger(__name__)
        management_net = IPNetwork(self.management_net)
        management_ip = management_net.ip
        data_net = IPNetwork(self.data_net)
        data_ip = data_net.ip
        for cluster in self.clusters:
            for i in range(1, self.number_naps+1):
                label = ICNetwork.ip_to_label(ip=str(management_ip))
                filename = "/nap-%s.cfg" % label
                nap_conf = open(filename, "w")
                interface = 'nap%s-%s' % (str(i), self.interface)
                lines = ['napConfig: \n', '{ \n \t',
                     'nodeId = "%s"; \n \t' % label,
                     'interface = "%s"; \n \t' % interface,
                     'networkAddress = "%s"; \n \t' % str(data_ip),
                     'netmask = "255.255.255.255"; \n \t',
                     'routingPrefixes = ( \n \t \t']
                nap_conf.writelines(l for l in lines)
                first_prefix = True
                for prefix in self.routePrefixes:
                    logger.info(prefix)
                    lines = [('{' if first_prefix else ',{'),
                             ' \n \t\t']
                    lines.extend('%s = "%s"; \n \t \t' % (k, prefix[k]) for k in prefix.keys())
                    lines.extend('} \n \t \t')
                    nap_conf.writelines(l for l in lines)
                    first_prefix = False
                lines = [ '); \n \t', 'fqdns = (); \n', '}; \n' ]
                nap_conf.writelines(l for l in lines)
                nap_conf.close()
                try:
                    logger.info('try to SFTP the nap config file to the nap host')
                    cluster_ip = cluster['cluster_ip']
                    logger.debug('cluster IP: %s ' % cluster_ip)
                    ret = self.sftp_conf(server_ip = cluster_ip, dir = self.write_conf['nap'], filename = filename)
                except Exception, e:
                    logger.info('failed to SFTP the nap config: %s' % str(e))
                data_ip = IPAddress(int(data_ip) + 2)
                management_ip = IPAddress(int(management_ip) + 2)

    def write_httpproxy_config( self ):
        """
        wirte the HTTPProxy config files and ssh them to the corresponding
        server hosting the mininet cluster to which the naps belong
        """
        logger = logging.getLogger(__name__)
        logger.info('httproxy')

        management_net = IPNetwork(self.management_net)
        management_ip = management_net.ip
        data_net = IPNetwork(self.data_net)
        data_ip = data_net.ip

        for cluster in self.clusters:
            for i in range(1, self.number_naps+1):
                label = ICNetwork.ip_to_label(ip=str(management_ip))
                filename = '/httpproxy-%s.cfg' % label
                proxy_conf = open(filename, "w")
                interface = 'nap%s-%s' % (str(i), self.interface)
                lines = ['proxyConfig: \n',
                         '{ \n \t',
                         'interface = "%s"; \n' % interface,
                         '}']
                proxy_conf.writelines(l for l in lines)
                proxy_conf.close()
                try:
                    logger.info('try to SFTP the HTTP proxy config file to the proxy host')
                    cluster_ip = cluster['cluster_ip']
                    ret = self.sftp_conf(server_ip = cluster_ip, dir = self.write_conf['httpproxy'], filename=filename)
                except Exception, e:
                    logger.info('failed to SFTP the proxy config: %s' % str(e))
                data_ip = IPAddress(int(data_ip) + 2)
                management_ip = IPAddress(int(management_ip) + 2)


    def sftp_conf( self, server_ip=None, dir=None, filename=None ):
        """
        SFTP config files to nodes

        """

        logger = logging.getLogger(__name__)
        remote_path = self.root_dir + dir
        remote_filename = remote_path + '/' + filename
        logging.info(remote_path)
        successful_connection = False
        attempts = 1
        while not successful_connection and attempts < 6:
            try:
                logger.info( 'Establishing SSH session to %s' % server_ip )
                ssh = paramiko.SSHClient()
                ssh.set_missing_host_key_policy( paramiko.AutoAddPolicy() )
                ssh.connect( str( server_ip ), 22, self.user )
                ssh.exec_command( 'mkdir ' + remote_path )
                ssh.close()
                logger.debug( 'Succeful SSH to %s after %s attempt(s).' % ( server_ip, attempts ) )
                logger.info( 'SSH connection closed.' )
                successful_connection = True
            except:
                logger.error( 'Failed to SSH to %s after %s attmpt(s).' % ( server_ip, attempts ) )
                attempts += 1
        trans = paramiko.Transport(( str( server_ip ) , 22 ))
        trans.start_client()
        self.rsa_auth( transport = trans )
        if not trans.is_authenticated():
            logger.debug('failed to authenticate a RSA private key, will try a password authntication instead')
            trans.auth_password(self.user)
            trans.connect( username=self.user )
        else:
            trans.open_session()
        try:
            sftp = paramiko.SFTPClient.from_transport(trans)
            sftp.put(filename, remote_filename)
            logger.debug('File %s successfuly transfered to %s' % ( filename, str(server_ip) ) )
        except:
            logger.error('failed to establish the SFTP session to transfer %s to %s.' % ( filename, server_ip ) )
            return -1
        sftp.close()
        logger.info('SFTP session closed.')

    def rsa_auth( self, transport ):
        """
        Authenticate rsa_id for passwordless ssh to a machine.
        if passwordless does not work then password should be used

        """
        rsa_private_key = self.root_dir + '.ssh/id_rsa'
        try:
            logging.debug('acquiring private key')
            rsa_id = paramiko.RSAKey.from_private_key_file(rsa_private_key)
        except Exception, e:
            logging.error('failed to load a private key from ~/.ssh/id_rsa, %s' % e)
        agent = paramiko.Agent()
        keys = agent.get_keys() + (rsa_id, )
        if len( keys ) == 0:
            logging.info('No keys available, exiting...')
            return
        for key in keys:
            try:
                transport.auth_publickey(self.user, key)
                logging.debug('Successfully authenticated user %s' % self.user)
                return
            except paramiko.SSHException, e:
                logging.error('Failed to authenticate user %s, %s' % (self.user, e))



