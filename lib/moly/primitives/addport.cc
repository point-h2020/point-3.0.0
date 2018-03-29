/*
 * addport.cc
 *
 *  Created on: 3 May 2017
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

#include "addport.hh"

AddPort::AddPort(node_identifier_t nodeId, port_identifier_t portId,
		string portName)
	: _nodeId(nodeId),
	  _portId(portId),
	  _portName(portName)
{
	_offset = 0;
	_pointerSize = sizeof(node_identifier_t) + sizeof(port_identifier_t) +
			+sizeof(uint32_t) /*Length name*/ + portName.length();
	_pointer = (uint8_t *)malloc(_pointerSize);
	_composePacket();
}

AddPort::AddPort(uint8_t *pointer, uint32_t pointerSize)
	: _pointerSize(pointerSize)
{
	_offset = 0;
	_pointer = (uint8_t *)malloc(pointerSize);
	memcpy(_pointer, pointer, pointerSize);
	_decomposePacket();
}

AddPort::~AddPort()
{
	free(_pointer);
}

node_identifier_t AddPort::nodeId()
{
	return _nodeId;
}

uint8_t * AddPort::pointer()
{
	return _pointer;
}

port_identifier_t AddPort::portId()
{
	return _portId;
}

string AddPort::portName()
{
	return _portName;
}

string AddPort::print()
{
	ostringstream oss;
	oss << " | Node ID: " << nodeId();
	oss << " | Port ID: " << portId();
	oss << " | Port Name: " << portName();
	oss << " |";
	return oss.str();
}

size_t AddPort::size()
{
	return _pointerSize;
}

void AddPort::_composePacket()
{
	_offset = 0;
	// [1] NodeId
	memcpy(_pointer, &_nodeId, sizeof(node_identifier_t));
	_offset += sizeof(node_identifier_t);
	// [2] PortId
	memcpy(_pointer + _offset, &_portId, sizeof(port_identifier_t));
	_offset += sizeof(port_identifier_t);
	// [3] Length of name
	uint32_t nameLength = _portName.length();
	memcpy(_pointer + _offset, &nameLength, sizeof(nameLength));
	_offset += sizeof(nameLength);
	// [4] Name
	memcpy(_pointer + _offset, _portName.c_str(), nameLength);
	_offset = 0;
}

void AddPort::_decomposePacket()
{
	uint32_t nameLength;
	_offset = 0;
	// [1] NodeId
	memcpy(&_nodeId, _pointer, sizeof(node_identifier_t));
	_offset += sizeof(node_identifier_t);
	// [2] PortId
	memcpy(&_portId, _pointer + _offset, sizeof(port_identifier_t));
	_offset += sizeof(port_identifier_t);
	// [3] Length of name
	memcpy(&nameLength, _pointer + _offset, sizeof(uint32_t));
	_offset += sizeof(uint32_t);
	// [4] Name
	char name[nameLength + 1];
	memcpy(name, _pointer + _offset, nameLength);
	name[nameLength] = '\0';
	_portName.assign(name);
	_offset = 0;
}
