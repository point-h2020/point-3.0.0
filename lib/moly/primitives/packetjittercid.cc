/*
 * packetjitterpercid.cc
 *
 *  Created on: 27 Sep 2017
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

#include "packetjittercid.hh"

PacketJitterCid::PacketJitterCid(node_role_t nodeRole,
		node_identifier_t nodeId, packet_jitter_cid_t packetJitterCid)
	: _nodeRole(nodeRole),
	  _nodeId(nodeId),
	  _packetJitterCid(packetJitterCid)
{
	_size = sizeof(node_role_t) + sizeof(node_identifier_t)
			+ sizeof(list_size_t) + _packetJitterCid.size()
			* (sizeof(content_identifier_t) + sizeof(packet_jitter_t));
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

PacketJitterCid::PacketJitterCid(uint8_t *pointer, uint32_t size)
	: _size(size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

PacketJitterCid::~PacketJitterCid()
{
	free(_pointer);
}

node_identifier_t PacketJitterCid::nodeIdentifier()
{
	return _nodeId;
}

node_role_t PacketJitterCid::nodeRole()
{
	return _nodeRole;
}

uint8_t * PacketJitterCid::pointer()
{
	return _pointer;
}

string PacketJitterCid::primitiveName()
{
	return "PACKET_JITTER_PER_CID_M";
}

string PacketJitterCid::print()
{
	ostringstream oss;
	packet_jitter_cid_t::iterator it;
	oss << " | Packet jitter per CID: " << endl;
	oss << "\tCID\tPacket Jitter [ms]" << endl;

	for (it = _packetJitterCid.begin(); it != _packetJitterCid.end(); it++)
	{
		oss << "\t\t\t" << it->first << "\t" << it->second / 10 << endl;
	}

	return oss.str();
}

packet_jitter_cid_t PacketJitterCid::packetJitterCid()
{
	return _packetJitterCid;
}

size_t PacketJitterCid::size()
{
	return _size;
}

void PacketJitterCid::_composePacket()
{
	uint16_t offset = 0;
	list_size_t numOfEntries = _packetJitterCid.size();
	packet_jitter_cid_t::iterator it;
	// [1] Node role
	memcpy(_pointer, &_nodeRole, sizeof(node_role_t));
	offset += sizeof(node_role_t);
	// [2] Node ID
	memcpy(_pointer, &_nodeId, sizeof(node_identifier_t));
	offset += sizeof(node_identifier_t);
	// [3] Number of pairs (List size)
	memcpy(_pointer, &numOfEntries, sizeof(list_size_t));
	offset += sizeof(list_size_t);

	for (it = _packetJitterCid.begin(); it != _packetJitterCid.end();
			it++)
	{
		// CID
		memcpy(_pointer + offset, &it->first, sizeof(content_identifier_t));
		offset += sizeof(content_identifier_t);
		// Packet jitter
		memcpy(_pointer + offset, &it->second, sizeof(packet_jitter_t));
		offset += sizeof(packet_jitter_t);
	}
}

void PacketJitterCid::_decomposePacket()
{
	uint16_t offset = 0;
	list_size_t numOfEntries;
	pair<content_identifier_t, packet_jitter_t> listEntry;
	// [1] Node role
	memcpy(&_nodeRole, _pointer, sizeof(node_role_t));
	offset += sizeof(node_role_t);
	// [2] Node identifier
	memcpy(&_nodeId, _pointer, sizeof(node_identifier_t));
	offset  += sizeof(node_identifier_t);
	// [3] list size
	memcpy(&numOfEntries, _pointer, sizeof(list_size_t));
	offset += sizeof(list_size_t);

	for (list_size_t i = 0; i < numOfEntries; i++)
	{
		memcpy(&listEntry.first, _pointer + offset,
				sizeof(content_identifier_t));
		offset += sizeof(root_scope_t);
		memcpy(&listEntry.second, _pointer + offset, sizeof(packet_jitter_t));
		offset += sizeof(publishers_t);
		_packetJitterCid.push_back(listEntry);
	}
}
