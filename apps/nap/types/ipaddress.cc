/*
 * ipaddress.cc
 *
 *  Created on: 15 Feb 2016
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

#include "ipaddress.hh"

IpAddress::IpAddress()
{
	_ipAddressUint = 0;
	_ipAddressString = string();
}

IpAddress::IpAddress(struct in_addr *address)
{
	char srcIp[256];
	_ipAddressUint = address->s_addr;
	strcpy(srcIp, inet_ntoa(*address));
	_ipAddressString = string(srcIp);
}

IpAddress::IpAddress(string ipAddress)
	: _ipAddressString(ipAddress)
{
	struct in_addr address;
	inet_aton(ipAddress.c_str(), &address);
	_ipAddressUint = address.s_addr;
}

IpAddress::IpAddress(uint32_t ipAddress)
	: _ipAddressUint(ipAddress)
{
	char srcIp[256];
	ostringstream oss;
	struct in_addr address;
	address.s_addr = ipAddress;
	strcpy(srcIp, inet_ntoa(address));
	oss << srcIp;
	_ipAddressString = oss.str();
}

struct in_addr IpAddress::inAddress()
{
	struct in_addr addr;
	addr.s_addr = _ipAddressUint;
	return addr;
}

void IpAddress::operator =(string &ipAddress)
{
	struct in_addr address;
	inet_aton(ipAddress.c_str(), &address);
	_ipAddressUint = address.s_addr;
	_ipAddressString = ipAddress;
}

void IpAddress::operator =(uint32_t &ipAddress)
{
	char srcIp[256];
	ostringstream oss;
	struct in_addr address;
	_ipAddressUint = ipAddress;
	address.s_addr = ipAddress;
	strcpy(srcIp, inet_ntoa(address));
	oss << srcIp;
	_ipAddressString = oss.str();
}

struct sockaddr_in IpAddress::socketAddress()
{
	struct sockaddr_in sockAddress;
	sockAddress.sin_addr = inAddress();
	sockAddress.sin_family = AF_INET;
	return sockAddress;
}

string IpAddress::str()
{
	return _ipAddressString;
}

uint32_t IpAddress::uint()
{
	return _ipAddressUint;
}

uint32_t *IpAddress::uintPointer()
{
	return &_ipAddressUint;
}
