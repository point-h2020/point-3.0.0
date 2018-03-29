/*
 * linkstate.cc
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

#include "linkstate.hh"

LinkState::LinkState(uint16_t linkId, uint32_t destinationNodeId,
		uint32_t sourceNodeId, uint16_t sourcePortId, link_type_t linkType,
		link_state_t state)
	: _linkId(linkId),
	  _destinationNodeId(destinationNodeId),
	  _sourceNodeId(sourceNodeId),
	  _sourcePortId(sourcePortId),
	  _linkType(linkType),
	  _state(state)
{
	_size = sizeof(_linkId) + sizeof(_destinationNodeId)
			+ sizeof(_sourceNodeId) + sizeof(_sourcePortId) + sizeof(_linkType)
			+ sizeof(uint16_t);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

LinkState::LinkState(uint8_t * pointer, size_t size)
	: _size(size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

LinkState::~LinkState()
{
	free(_pointer);
}

uint32_t LinkState::destinationNodeId()
{
	return _destinationNodeId;
}

uint16_t LinkState::linkId()
{
	return _linkId;
}

link_type_t LinkState::linkType()
{
	return _linkType;
}

uint8_t * LinkState::pointer()
{
	return _pointer;
}

string LinkState::print()
{
	ostringstream oss;
	oss << " | Link ID: " << linkId();
	oss << " | Link Type: " << linkType();
	oss << " | DST NID: " << destinationNodeId();
	oss << " | State: " << state();
	return oss.str();
}

size_t LinkState::size()
{
	return _size;
}
uint32_t LinkState::sourceNodeId()
{
	return _sourceNodeId;
}

uint16_t LinkState::sourcePortId()
{
	return _sourcePortId;
}

link_state_t LinkState::state()
{
	return _state;
}

void LinkState::_composePacket()
{
	uint8_t * bufferPointer = _pointer;
	// [1] Link ID
	memcpy(bufferPointer, &_linkId, sizeof(_linkId));
	bufferPointer += sizeof(_linkId);
	// [2] DST Node ID
	memcpy(bufferPointer, &_destinationNodeId, sizeof(_destinationNodeId));
	bufferPointer += sizeof(_destinationNodeId);
	// [3] SRC Node ID
	memcpy(bufferPointer, &_sourceNodeId, sizeof(_sourceNodeId));
	bufferPointer += sizeof(_sourceNodeId);
	// [4] SRC Port ID
	memcpy(bufferPointer, &_sourcePortId, sizeof(_sourcePortId));
	bufferPointer += sizeof(_sourcePortId);
	// [5] Link Type
	memcpy(bufferPointer, &_linkType, sizeof(_linkType));
	bufferPointer += sizeof(_linkType);
	// [6] State
	uint16_t linkState = (uint16_t)_state;
	memcpy(bufferPointer, &linkState, sizeof(linkState));
}

void LinkState::_decomposePacket()
{
	uint8_t * bufferPointer = _pointer;
	// [1] Link ID
	memcpy(&_linkId, bufferPointer, sizeof(_linkId));
	bufferPointer += sizeof(_linkId);
	// [2] DST Node ID
	memcpy(&_destinationNodeId, bufferPointer, sizeof(_destinationNodeId));
	bufferPointer += sizeof(_destinationNodeId);
	// [3] SRC Node ID
	memcpy(&_sourceNodeId, bufferPointer, sizeof(_sourceNodeId));
	bufferPointer += sizeof(_sourceNodeId);
	// [4] SRC Port ID
	memcpy(&_sourcePortId, bufferPointer, sizeof(_sourcePortId));
	bufferPointer += sizeof(_sourcePortId);
	// [3] Link Type
	memcpy(&_linkType, bufferPointer, sizeof(_linkType));
	bufferPointer += sizeof(_linkType);
	// [4] State
	uint16_t linkState;
	memcpy(&linkState, bufferPointer, sizeof(linkState));
	_state = (link_state_t)linkState;
}
