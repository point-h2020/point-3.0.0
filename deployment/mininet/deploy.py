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

"""ICN mininet deployment tool, constructing multiple distributed clusters of client/NAP pairs for large scale deployment of *-over-ICN.

Usage:
  deploy.py [options]

Options:
  -h --help                         Display this screen.
  -v --version                      Display the version number
  -l=log_level                      set the loglevel <DEBUG|INFO|WARNING|ERROR> [default: INFO]
  -c=clear mininet clusters         access the hosting server and terminate any running mininet clusters
  -m=create mininet cluster         create mininet cluster on a hosting server, take a mn configuration file
  -i=create icn .cfg config         create an ICN, .cfg-based, configuration file for the ICN deployment tool
  -n=create nap .cfg configs        create NAP config files for each NAP in the ICN (i.e. in each cluster)
"""

import sys
import logging
from docopt import docopt
from MnCluster import *
from IPoverICN import *

def main(**kwargs):
    print (kwargs)
    log_level = getattr(logging, kwargs['-l'].upper())
    logging.basicConfig(format='%(levelname)s: %(message)s',level = log_level)
    logger = logging.getLogger(__name__)
    try:
        kwargs['-c'] is not None or raise_ValueError()
        mn_config = kwargs['-c']
        mn = MnClusters(mn_config, log_level)
        mn.clean_clusters()
    except ValueError:
        "Do nothing..."
    try:
        kwargs['-m'] is not None or raise_ValueError()
        mn_config = kwargs['-m']
        mn = MnClusters(mn_config, log_level)
        mn.construct_clusters()
        mn.write_ip_lists()
        logger.info('mininet network is up')
    except ValueError:
        logger.info('not deploying mininet')
    try:
        kwargs['-i'] is not None or raise_ValueError()
        mn_icn_config = kwargs['-i']
        mn_icn = ICNetwork(mn_icn_config, log_level)
        mn_icn.update_clusters()
        mn_icn.merge_clusters_to_network()
        mn_icn.write_network_config()
    except ValueError:
        logger.info('not creating ICN config file')
    try:
        kwargs['-n'] is not None or raise_ValueError()
        mn_ip_icn_config = kwargs['-n']
        mn_ip_icn = IPoverICNCluster(mn_ip_icn_config, log_level)
        mn_ip_icn.write_nap_config()
        mn_ip_icn.write_httpproxy_config()
    except ValueError:
        logger.info('not creating NAP config files')

if __name__ == '__main__':
    arguments = docopt(__doc__, version='ICN mininet deployment tool version: 0.2')
    main(**arguments)
