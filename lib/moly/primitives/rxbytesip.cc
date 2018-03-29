/*
 * rxbytesip.cc
 *
 *  Created on: 2 Oct 2017
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

#include "rxbytesip.hh"

RxBytesIp::RxBytesIp(node_identifier_t nodeId, bytes_t rxBytes)
	: _nodeId(nodeId),
	  _rxBytes(rxBytes)
{
	_size = sizeof(node_identifier_t) + sizeof(bytes_t);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

RxBytesIp::RxBytesIp(uint8_t *pointer, uint32_t size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

RxBytesIp::~RxBytesIp()
{
	free(_pointer);
}

node_identifier_t RxBytesIp::nodeId()
{
	return _nodeId;
}

uint8_t * RxBytesIp::pointer()
{
	return _pointer;
}

string RxBytesIp::primitiveName()
{
	return "RX_BYTES_IP_M";
}

string RxBytesIp::print()
{
	ostringstream oss;
	oss << " | Node ID: " << nodeId();
	oss << " | RX Bytes: " << rxBytes();
	return oss.str();
}

bytes_t RxBytesIp::rxBytes()
{
	return _rxBytes;
}

size_t RxBytesIp::size()
{
	return _size;
}

void RxBytesIp::_composePacket()
{
	uint8_t * bufferPointer = _pointer;
	// [1] Node ID
	memcpy(bufferPointer, &_nodeId, sizeof(node_identifier_t));
	bufferPointer += sizeof(node_identifier_t);
	// [2] RX Bytes
	memcpy(bufferPointer, &_rxBytes, sizeof(bytes_t));
}

void RxBytesIp::_decomposePacket()
{
	uint8_t * bufferPointer = _pointer;
	// [1] Node ID
	memcpy(&_nodeId, bufferPointer, sizeof(node_identifier_t));
	bufferPointer += sizeof(node_identifier_t);
	// [2] RX Bytes
	memcpy(&_rxBytes, bufferPointer, sizeof(bytes_t));
}
