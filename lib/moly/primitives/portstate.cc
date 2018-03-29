/*
 * portstate.cc
 *
 *  Created on: 13 Jan 2016
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

#include "portstate.hh"

PortState::PortState(uint32_t nodeId, uint16_t portId, state_t state)
	: _nodeId(nodeId),
	  _portId(portId),
	  _state(state)
{
	_offset = 0;
	_size = sizeof(_nodeId) + sizeof(_portId) + sizeof(state_t);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
#ifdef MOLY_DEBUG
	cout << "((MOLY)) Composed PortState payload is " << _size
			<< " bytes large" << endl;
#endif
}

PortState::PortState(uint8_t * pointer, size_t size)
	: _size(size)
{
	_offset = 0;
	_pointer = (uint8_t *)malloc(_size);
	memcpy(_pointer, pointer, _size);
	_decomposePacket();
#ifdef MOLY_DEBUG
	cout << "((MOLY)) Decomposed PortState payload is " << _size
			<< " bytes large\n";
#endif
}

PortState::~PortState()
{
	free(_pointer);
}

uint32_t PortState::nodeId()
{
	return _nodeId;
}

uint8_t * PortState::pointer()
{
	return _pointer;
}

string PortState::primitiveName()
{
	return "PORT_STATE_M";
}

string PortState::print()
{
	ostringstream oss;
	oss << " | Node ID: " << nodeId();
	oss << " | Port ID: " << portId();
	oss << " | State: " << state();
	return oss.str();
}

uint16_t PortState::portId()
{
	return _portId;
}

size_t PortState::size()
{
	return _size;
}

state_t PortState::state()
{
	return (state_t)_state;
}

void PortState::_composePacket()
{
	// [1] Node ID
	memcpy(_pointer, &_nodeId, sizeof(_nodeId));
	_offset += sizeof(_nodeId);
	// [2] SRC Port ID
	memcpy(_pointer + _offset, &_portId, sizeof(_portId));
	_offset += sizeof(_portId);
	// [3] State
	memcpy(_pointer + _offset, &_state, sizeof(state_t));
	_offset = 0;
}

void PortState::_decomposePacket()
{
	// [1] Node ID
	memcpy(&_nodeId, _pointer, sizeof(_nodeId));
	_offset += sizeof(_nodeId);
	// [2] Port ID
	memcpy(&_portId, _pointer + _offset, sizeof(_portId));
	_offset += sizeof(_portId);
	// [3] State
	memcpy(&_state, _pointer + _offset, sizeof(state_t));
	_offset = 0;
}
