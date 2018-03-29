/*
 * packeterrorrate.cc
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

#include "packeterrorrate.hh"

PacketErrorRate::PacketErrorRate(uint32_t nodeId, uint16_t portId,
		uint32_t rate)
	: _nodeId(nodeId),
	  _portId(portId),
	  _rate(rate)
{
	_size = sizeof(_nodeId) + sizeof(_portId) + sizeof(_rate);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

PacketErrorRate::PacketErrorRate(uint8_t* pointer, size_t size)
	: _size(size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

PacketErrorRate::~PacketErrorRate()
{
	free(_pointer);
}

uint8_t *PacketErrorRate::pointer()
{
	return _pointer;
}

uint32_t PacketErrorRate::nodeId()
{
	return _nodeId;
}

string PacketErrorRate::print()
{
	ostringstream oss;
	oss << " | Node ID: " << _nodeId;
	oss << " | Port ID: " << _portId;
	oss << " | PacketErrorRate: " << _rate;
	oss << " |";
	return oss.str();
}

uint32_t PacketErrorRate::rate()
{
	return _rate;
}

uint16_t PacketErrorRate::portId()
{
	return _portId;
}

size_t PacketErrorRate::size()
{
	return _size;
}

void PacketErrorRate::_composePacket()
{
	uint8_t * bufferPointer = _pointer;
	// [1] Port ID
	memcpy(bufferPointer, &_nodeId, sizeof(_nodeId));
	bufferPointer += sizeof(_nodeId);
	// [2] NID
	memcpy(bufferPointer, &_portId, sizeof(_portId));
	bufferPointer += sizeof(_portId);
	// [3] Rate
	memcpy(bufferPointer, &_rate, sizeof(_rate));
}

void PacketErrorRate::_decomposePacket()
{
	uint8_t * bufferPointer = _pointer;
	// [1] Node ID
	memcpy(&_nodeId, bufferPointer, sizeof(_nodeId));
	bufferPointer += sizeof(_nodeId);
	// [2] Port ID
	memcpy(&_portId, bufferPointer, sizeof(_portId));
	bufferPointer += sizeof(_portId);
	// [3] Rate
	memcpy(&_rate, bufferPointer, sizeof(_rate));
}
