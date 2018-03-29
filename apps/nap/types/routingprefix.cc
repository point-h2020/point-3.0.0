/*
 * routingprefix.cc
 *
 *  Created on: 17 April 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of Blackadder.
 *
 * Blackadder is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Blackadder is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Blackadder.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "routingprefix.hh"

RoutingPrefix::RoutingPrefix()
{
	_routingPrefixString = string();
	_routingPrefixUint = 0;
}

RoutingPrefix::RoutingPrefix(string networkAddress, string netmask)
{
	_networkAddress = networkAddress;
	_netmask = netmask;
	_routingPrefixUint = _networkAddress.uint() & _netmask.uint();
	ostringstream oss;
	oss << _networkAddress.str() << "/" << (int)_netmask.cidr();
	_routingPrefixString = oss.str();
}

Netmask RoutingPrefix::netmask()
{
	return _netmask;
}
void RoutingPrefix::netmask(string netmask)
{
	_netmask = netmask;
	_routingPrefixUint = _networkAddress.uint() & _netmask.uint();
	ostringstream oss;
	oss << _networkAddress.str() << "/" << (int)_netmask.cidr();
	_routingPrefixString = oss.str();
}

IpAddress RoutingPrefix::networkAddress()
{
	return _networkAddress;
}

void RoutingPrefix::networkAddress(string networkAddress)
{
	_networkAddress = networkAddress;
	_routingPrefixUint = _networkAddress.uint() & _netmask.uint();
	ostringstream oss;
	oss << _networkAddress.str() << "/" << (int)_netmask.cidr();
	_routingPrefixString = oss.str();
}

string RoutingPrefix::str()
{
	return _routingPrefixString;
}

uint32_t RoutingPrefix::uint()
{
	return _routingPrefixUint;
}
