/*
 * netmask.cc
 *
 *  Created on: 17 Apr 2016
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

#include <boost/algorithm/string.hpp>
#include <vector>
#include "netmask.hh"

Netmask::Netmask()
{
	_netmaskUint = 0;
	_netmaskString = string();
	_cidrUint = 0;
}
Netmask::Netmask(string netmask)
{
	struct in_addr address;
	inet_aton(netmask.c_str(), &address);
	_netmaskUint = address.s_addr;
	_netmaskString = netmask;
	_cidr();
}

uint8_t Netmask::cidr()
{
	return _cidrUint;
}

void Netmask::operator=(string &netmask)
{
	struct in_addr address;
	inet_aton(netmask.c_str(), &address);
	_netmaskUint = address.s_addr;
	_netmaskString = netmask;
	_cidr();
}

void Netmask::operator=(uint32_t &netmask)
{
	char netmaskChar[256];
	ostringstream oss;
	struct in_addr netmaskStruct;
	netmaskStruct.s_addr = netmask;
	strcpy(netmaskChar, inet_ntoa(netmaskStruct));
	oss << netmaskChar;
	_netmaskString = oss.str();
	_netmaskUint = netmask;
}

uint32_t Netmask::uint()
{
	return _netmaskUint;
}

string Netmask::str()
{
	return _netmaskString;
}

void Netmask::_cidr()
{
	_cidrUint = 0;
	vector<string> strs;
	vector<string>::iterator it;
	boost::split(strs, _netmaskString, boost::is_any_of("."));
	for (it = strs.begin(); it != strs.end(); it++)
	{
		uint8_t octet = atoi((*it).c_str());
		switch(octet)
		{
		case 0x80:
			_cidrUint += 1;
			break;
		case 0xC0:
			_cidrUint += 2;
			break;
		case 0xE0:
			_cidrUint += 3;
			break;
		case 0xF0:
			_cidrUint += 4;
			break;
		case 0xF8:
			_cidrUint += 5;
			break;
		case 0xFC:
			_cidrUint += 6;
			break;
		case 0xFE:
			_cidrUint += 7;
			break;
		case 0xFF:
			_cidrUint += 8;
			break;
		}
	}
}
