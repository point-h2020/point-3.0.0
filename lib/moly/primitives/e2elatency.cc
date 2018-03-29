/*
 * e2elatency.cc
 *
 *  Created on: 3 Feb 2017
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

#include "e2elatency.hh"

E2eLatency::E2eLatency(node_role_t nodeRole, uint32_t endpointId,
		uint16_t latency)
	: _nodeRole(nodeRole),
	  _endpointId(endpointId),
	  _latency(latency)
{
	_size = sizeof(_nodeRole) + sizeof(_endpointId) + sizeof(_latency);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

E2eLatency::E2eLatency(uint8_t * pointer, size_t size)
	: _size(size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

E2eLatency::~E2eLatency()
{
	free(_pointer);
}

uint32_t E2eLatency::endpointId()
{
	return _endpointId;
}

uint16_t E2eLatency::latency()
{
	return _latency;
}

node_role_t E2eLatency::nodeRole()
{
	return _nodeRole;
}

uint8_t * E2eLatency::pointer()
{
	return _pointer;
}

string E2eLatency::print()
{
	ostringstream oss;
	oss << " | Latency: " << latency() / 10.0 << "ms";
	oss << " |";
	return oss.str();
}

size_t E2eLatency::size()
{
	return _size;
}

void E2eLatency::_composePacket()
{
	uint16_t offset = 0;
	// [1] Node Role
	memcpy(_pointer, &_nodeRole, sizeof(_nodeRole));
	offset += sizeof(_nodeRole);
	// [2] Endpoint ID
	memcpy(_pointer + offset, &_endpointId, sizeof(_endpointId));
	offset += sizeof(_endpointId);
	// [3] E2E latency
	memcpy(_pointer + offset, &_latency, sizeof(_latency));
}

void E2eLatency::_decomposePacket()
{
	uint16_t offset = 0;
	// [1] Node Role
	memcpy(&_nodeRole, _pointer, sizeof(_nodeRole));
	offset += sizeof(_nodeRole);
	// [2] Endpoint ID
	memcpy(&_endpointId, _pointer + offset, sizeof(_endpointId));
	offset += sizeof(_endpointId);
	// [3] E2E latency
	memcpy(&_latency, _pointer + offset, sizeof(_latency));
}
