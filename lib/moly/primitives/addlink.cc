/*
 * addlink.cc
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

#include "addlink.hh"
#include <iomanip>
#include <iostream>
#include <sstream>
AddLink::AddLink(uint16_t linkId, uint32_t destinationNodeId,
		uint32_t sourceNodeId, uint16_t sourcePortId, link_type_t linkType,
		string name)
	: _linkId(linkId),
	  _destinationNodeId(destinationNodeId),
	  _sourceNodeId(sourceNodeId),
	  _sourcePortId(sourcePortId),
	  _linkType(linkType),
	  _name(name)
{
	_offset = 0;
	_size = sizeof(uint32_t) + name.length() + sizeof(_linkId)
			+ sizeof(_destinationNodeId) + sizeof(_sourceNodeId)
			+ sizeof(_sourcePortId)	+ sizeof(uint16_t);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

AddLink::AddLink(uint8_t * pointer, size_t size)
	: _size(size)
{
	_offset = 0;
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

AddLink::~AddLink()
{
	free(_pointer);
}

uint32_t AddLink::destinationNodeId()
{
	return _destinationNodeId;
}

uint16_t AddLink::linkId()
{
	return _linkId;
}

link_type_t AddLink::linkType()
{
	return _linkType;
}

string AddLink::name()
{
	return _name;
}

uint8_t * AddLink::pointer()
{
	return _pointer;
}

string AddLink::print()
{
	ostringstream oss;
	oss << "| Link name: " << name();
	oss << " | Link ID: " << linkId();
	oss << " | SRC Node ID: " << sourceNodeId();
	oss << " | DST Node ID: " << destinationNodeId();
	oss << " | Source Port: " << sourcePortId();
	oss << " | Link type: ";

	switch (linkType())
	{
	case LINK_TYPE_802_3:
		oss << "802.3";
		break;
	case LINK_TYPE_802_11:
		oss << "802.11";
		break;
	case LINK_TYPE_802_11_A:
		oss << "802.11a";
		break;
	case LINK_TYPE_802_11_B:
		oss << "802.11b";
		break;
	case LINK_TYPE_802_11_G:
		oss << "802.11g";
		break;
	case LINK_TYPE_802_11_N:
		oss << "802.11n";
		break;
	case LINK_TYPE_802_11_AA:
		oss << "802.11aa";
		break;
	case LINK_TYPE_802_11_AC:
		oss << "802.11ac";
		break;
	case LINK_TYPE_SDN_802_3_Z:
		oss << "802.3z";
		break;
	case LINK_TYPE_SDN_802_3_AE:
		oss << "802.3.ae";
		break;
	case LINK_TYPE_GPRS:
		oss << "GPRS";
		break;
	case LINK_TYPE_UMTS:
		oss << "UMTS";
		break;
	case LINK_TYPE_LTE:
		oss << "LTE";
		break;
	case LINK_TYPE_LTE_A:
		oss << "LTE-A";
		break;
	case LINK_TYPE_OPTICAL:
		oss << "Optical";
		break;
	case LINK_TYPE_UNKNOWN:
		oss << "Unknown";
		break;
	default:
		oss << "-";
	}

	oss << " |";
	return oss.str();
}

size_t AddLink::size()
{
	return _size;
}

uint32_t AddLink::sourceNodeId()
{
	return _sourceNodeId;
}

uint16_t AddLink::sourcePortId()
{
	return _sourcePortId;
}

void AddLink::_composePacket()
{
	uint32_t nameLength = _name.length();
	// [1] Length
	memcpy(_pointer, &nameLength, sizeof(uint32_t));
	_offset += sizeof(nameLength);
	// [2] Name
	memcpy(_pointer + _offset, _name.c_str(), nameLength);
	_offset += nameLength;
	// [3] Link ID
	memcpy(_pointer + _offset, &_linkId, sizeof(_linkId));
	_offset += sizeof(_linkId);
	// [4] SRC Node ID
	memcpy(_pointer + _offset, &_sourceNodeId, sizeof(_sourceNodeId));
	_offset += sizeof(_sourceNodeId);
	// [5] DST Node ID
	memcpy(_pointer + _offset, &_destinationNodeId, sizeof(_destinationNodeId));
	_offset += sizeof(_destinationNodeId);
	// [6] Type
	uint16_t linkType = _linkType;
	memcpy(_pointer + _offset, &linkType, sizeof(linkType));
	_offset = 0;
}

void AddLink::_decomposePacket()
{
	uint32_t nameLength;
	uint16_t linkType;
	// [1] Length
	memcpy(&nameLength, _pointer, sizeof(uint32_t));
	_offset += sizeof(uint32_t);
	// [2] Name
	char name[nameLength + 1];
	memcpy(name, _pointer + _offset, nameLength);
	name[nameLength] = '\0';
	_name.assign(name);
	_offset += nameLength;
	// [3] Link ID
	memcpy(&_linkId, _pointer + _offset, sizeof(_linkId));
	_offset += sizeof(_linkId);
	// [4] SRC Node ID
	memcpy(&_sourceNodeId, _pointer + _offset, sizeof(_sourceNodeId));
	_offset += sizeof(_sourceNodeId);
	// [5] DST Node ID
	memcpy(&_destinationNodeId, _pointer + _offset, sizeof(_destinationNodeId));
	_offset += sizeof(_destinationNodeId);
	// [6] Link Type
	memcpy(&linkType, _pointer + _offset, sizeof(linkType));
	_linkType = (link_type_t)linkType;
	_offset = 0;
}
