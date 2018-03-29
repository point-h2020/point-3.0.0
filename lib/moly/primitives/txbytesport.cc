/*
 * txbytesport.cc
 *
 *  Created on: 19 Apr 2017
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

#include "txbytesport.hh"

TxBytesPort::TxBytesPort(uint32_t nodeId, uint16_t portId, uint32_t bytes)
	: _nodeId(nodeId),
	  _portId(portId),
	  _bytes(bytes)
{
	_size = sizeof(_nodeId) + sizeof(_portId) + sizeof(_bytes);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

TxBytesPort::TxBytesPort(uint8_t* pointer, size_t size)
	: _size(size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

TxBytesPort::~TxBytesPort()
{
	free(_pointer);
}

uint32_t TxBytesPort::bytes()
{
	return _bytes;
}

uint8_t *TxBytesPort::pointer()
{
	return _pointer;
}

uint32_t TxBytesPort::nodeId()
{
	return _nodeId;
}

string TxBytesPort::print()
{
	ostringstream oss;
	oss << " | Node ID: " << _nodeId;
	oss << " | Port ID: " << _portId;
	oss << " | TxBytesPort: " << _bytes << "bytes";
	oss << " |";
	return oss.str();
}

uint16_t TxBytesPort::portId()
{
	return _portId;
}

size_t TxBytesPort::size()
{
	return _size;
}

void TxBytesPort::_composePacket()
{
	uint8_t * bufferPointer = _pointer;
	// [1] Port ID
	memcpy(bufferPointer, &_nodeId, sizeof(_nodeId));
	bufferPointer += sizeof(_nodeId);
	// [2] NID
	memcpy(bufferPointer, &_portId, sizeof(_portId));
	bufferPointer += sizeof(_portId);
	// [3] TX Bytes
	memcpy(bufferPointer, &_bytes, sizeof(_bytes));
}

void TxBytesPort::_decomposePacket()
{
	uint8_t * bufferPointer = _pointer;
	// [1] Node ID
	memcpy(&_nodeId, bufferPointer, sizeof(_nodeId));
	bufferPointer += sizeof(_nodeId);
	// [2] Port ID
	memcpy(&_portId, bufferPointer, sizeof(_portId));
	bufferPointer += sizeof(_portId);
	// [5] TX Bytes
	memcpy(&_bytes, bufferPointer, sizeof(_bytes));
}
