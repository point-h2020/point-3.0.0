/*
 * rxbytesipmulticast.cc
 *
 *  Created on: 29 Sep 2017
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the MOnitoring LibrarY (MOLY).
 *
 * MOLY is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOLY is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOLY. If not, see <http://www.gnu.org/licenses/>.
 */

#include "rxbytesipmulticast.hh"

RxBytesIpMulticast::RxBytesIpMulticast(node_identifier_t nodeId,
		ip_version_t ipVersion, bytes_t rxBytes)
	: _nodeId(nodeId),
	  _ipVersion(ipVersion),
	  _rxBytes(rxBytes)
{
	_size = sizeof(node_identifier_t) + sizeof(ip_version_t) + sizeof(bytes_t);
	_pointer = (uint8_t *)malloc(_size);

	if (ipVersion < 4)
	{
		_ipVersion = 4;
	}

	_composePacket();
}

RxBytesIpMulticast::RxBytesIpMulticast(uint8_t *pointer, uint32_t size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

RxBytesIpMulticast::~RxBytesIpMulticast()
{
	free(_pointer);
}

ip_version_t RxBytesIpMulticast::ipVersion()
{
	return _ipVersion;
}

node_identifier_t RxBytesIpMulticast::nodeId()
{
	return _nodeId;
}

uint8_t * RxBytesIpMulticast::pointer()
{
	return _pointer;
}

string RxBytesIpMulticast::primitiveName()
{
	return "RX_BYTES_IP_MULTICAST_M";
}

string RxBytesIpMulticast::print()
{
	ostringstream oss;
	oss << " | Node ID: " << nodeId();
	oss << " | IP Version: " << ipVersion();
	oss << " | RX Bytes: " << rxBytes();
	return oss.str();
}

bytes_t RxBytesIpMulticast::rxBytes()
{
	return _rxBytes;
}

size_t RxBytesIpMulticast::size()
{
	return _size;
}

void RxBytesIpMulticast::_composePacket()
{
	uint8_t * bufferPointer = _pointer;
	// [1] Node ID
	memcpy(bufferPointer, &_nodeId, sizeof(node_identifier_t));
	bufferPointer += sizeof(node_identifier_t);
	// [2] IP Version
	memcpy(bufferPointer, &_ipVersion, sizeof(ip_version_t));
	bufferPointer += sizeof(ip_version_t);
	// [3] RX Bytes
	memcpy(bufferPointer, &_rxBytes, sizeof(bytes_t));
}

void RxBytesIpMulticast::_decomposePacket()
{
	uint8_t * bufferPointer = _pointer;
	// [1] Node ID
	memcpy(&_nodeId, bufferPointer, sizeof(node_identifier_t));
	bufferPointer += sizeof(node_identifier_t);
	// [2] IP Version
	memcpy(&_ipVersion, bufferPointer, sizeof(ip_version_t));
	bufferPointer += sizeof(ip_version_t);
	// [3] RX Bytes
	memcpy(&_rxBytes, bufferPointer, sizeof(bytes_t));
}
