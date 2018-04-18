#!/bin/sh

# Create a boxed template of the 4-node IP-over-ICN testbed

# Copyright (c) 2016--2018 [Alexander Phinikarides](alexandrosp@prime-tel.com)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

_product_icn="POINT NAP node"
_product_ip="POINT IP node"
_producturl="https://www.point-h2020.eu/"
_version="3.0.0"
_vendor="PrimeTel PLC"
_vendorurl="http://primetel.com.cy/research/en/"
_eula="LICENSE.blackadder"

# Stop any running VMs
vagrant halt

# Get the list of prepared VMs
_vms=(`vboxmanage list vms | /usr/bin/grep nap-testbed | cut -d'"' -f2`)

# Export all machines into a single OVA template
vboxmanage export ${_vms[0]} ${_vms[1]} ${_vms[2]} ${_vms[3]} \
  --vsys 0 \
  --product "${_product_ip}" \
  --producturl "${_producturl}" \
  --vendor "${_vendor}" \
  --vendorurl "${_vendorurl}" \
  --version "${_version}" \
  --eulafile "${_eula}" \
  --description "ue01 is an IP client" \
  --vsys 1 \
  --product "${_product_icn}" \
  --producturl "${_producturl}" \
  --vendor "${_vendor}" \
  --vendorurl "${_vendorurl}" \
  --version "${_version}" \
  --description "cnap01 is a cNAP node" \
  --vsys 2 \
  --product "${_product_icn}" \
  --producturl "${_producturl}" \
  --vendor "${_vendor}" \
  --vendorurl "${_vendorurl}" \
  --version "${_version}" \
  --description "snap is a sNAP and RV/TM node" \
  --vsys 3 \
  --product "${_product_ip}" \
  --producturl "${_producturl}" \
  --vendor "${_vendor}" \
  --vendorurl "${_vendorurl}" \
  --version "${_version}" \
  --description "srv01 is an IP server" \
  --manifest \
  --ovf20 \
  -o point_3.0.0_nap_testbed_`date "+%Y%m%dT%H%M%S"`.ova

echo -e "\n The VMs have been exported."
echo -e "\n To continue using the testbed, start the environment back up with 'vagrant up'\n"

