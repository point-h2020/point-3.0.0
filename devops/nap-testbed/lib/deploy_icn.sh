#!/usr/bin/env bash

# Runs the deployment script on the RV/TM node
# This script should be run under the `point` account

# Copyright (c) 2016--2018 [Alexander Phinikarides](alexandrosp@prime-tel.com)

~/blackadder/deployment/deploy -c ~/topology.cfg -l -n --rvinfo

