/*
 * addnode.cc
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

#include "addnode.hh"
#include <sstream>

AddNode::AddNode(string name, uint32_t nodeId, node_role_t nodeRole)
	: _name(name),
	  _nodeId(nodeId),
	  _nodeRole(nodeRole)
{
	_offset = 0;
	_size = sizeof(uint32_t) + name.size() + sizeof(_nodeId)
			+ sizeof(uint16_t);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

AddNode::AddNode(uint8_t * pointer, size_t size)
	: _size(size)
{
	_offset = 0;
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

AddNode::~AddNode()
{
	free(_pointer);
}

string AddNode::name()
{
	return _name;
}

uint32_t AddNode::nodeId()
{
	return _nodeId;
}

uint8_t * AddNode::pointer()
{
	return _pointer;
}

string AddNode::print()
{
	ostringstream oss;
	oss << " | Node Name: " << name();
	oss << " | Node ID: " << (int)nodeId();
	oss << " | Node Role: ";

	switch (nodeRole())
	{
	case NODE_ROLE_GW:
		oss << "GW";
		break;
	case NODE_ROLE_FN:
		oss << "FN";
		break;
	case NODE_ROLE_NAP:
		oss << "NAP";
		break;
	case NODE_ROLE_RV:
		oss << "RV";
		break;
	case NODE_ROLE_SERVER:
		oss << "SERVER";
		break;
	case NODE_ROLE_TM:
		oss << "TM";
		break;
	case NODE_ROLE_UE:
		oss << "UE";
		break;
	case NODE_ROLE_MOOSE:
		oss << "MOOSE";
		break;
	default:
		oss << "- (" << nodeRole() << ")";
	}

	oss << " |";
	return oss.str();
}

size_t AddNode::size()
{
	return _size;
}

node_role_t AddNode::nodeRole()
{
	return _nodeRole;
}

void AddNode::_composePacket()
{
	uint32_t nameLength = _name.length();
	// [1] Length of name
	memcpy(_pointer, &nameLength, sizeof(uint32_t));
	_offset += sizeof(uint32_t);
	// [2] Name
	memcpy(_pointer + _offset, _name.c_str(), nameLength);
	_offset += nameLength;
	// [3] Node ID
	memcpy(_pointer + _offset, &_nodeId, sizeof(_nodeId));
	_offset += sizeof(_nodeId);
	// [4] Node Role
	uint16_t nodeRole = (uint16_t)_nodeRole;
	memcpy(_pointer + _offset, &nodeRole, sizeof(nodeRole));
	_offset = 0;
}

void AddNode::_decomposePacket()
{
	uint32_t nameLength;
	// [1] Length of name
	memcpy(&nameLength, _pointer, sizeof(uint32_t));
	_offset += sizeof(uint32_t);
	// [2] Name
	char name[nameLength + 1];
	memcpy(name, _pointer + _offset, nameLength);
	name[nameLength] = '\0';
	_name.assign(name);
	_offset += nameLength;
	// [3] Node ID
	memcpy(&_nodeId, _pointer + _offset, sizeof(_nodeId));
	_offset += sizeof(_nodeId);
	// [4] Node Role
	uint16_t nodeRole;
	memcpy(&nodeRole, _pointer + _offset, sizeof(nodeRole));
	_nodeRole = (node_role_t)nodeRole;
	_offset = 0;
}
