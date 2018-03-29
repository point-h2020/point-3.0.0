/*
 * buffersizes.cc
 *
 *  Created on: 17 Oct 2017
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

#include "buffersizes.hh"

BufferSizes::BufferSizes(node_role_t nodeRole, buffer_sizes_t bufferSizes)
	: _nodeRole(nodeRole),
	  _bufferSizes(bufferSizes)
{
	_pointerSize = sizeof(node_role_t) + sizeof(list_size_t)
			+ _bufferSizes.size() * (sizeof(buffer_name_t) +
					sizeof(buffer_size_t));
	_pointer = (uint8_t *)malloc(_pointerSize);
	_composePacket();
}

BufferSizes::BufferSizes(uint8_t *pointer, uint32_t pointerSize)
{
	_pointer = (uint8_t *)malloc(pointerSize);
	memcpy(_pointer, pointer, pointerSize);
	_decomposePacket();
}

BufferSizes::~BufferSizes()
{
	free(_pointer);
}

buffer_sizes_t BufferSizes::bufferSizes()
{
	return _bufferSizes;
}

node_role_t BufferSizes::nodeRole()
{
	return _nodeRole;
}

uint8_t *BufferSizes::pointer()
{
	return _pointer;
}

string BufferSizes::primitiveName()
{
	return "BUFFER_SIZES_M";
}

string BufferSizes::print()
{
	ostringstream oss;
	oss << " | Buffer sizes: " << endl;
	oss << "\tBuffer Name\t\t\t\tSize" << endl;

	for (_it = _bufferSizes.begin(); _it != _bufferSizes.end(); _it++)
	{
		oss << "\t" << bufferNameToString(_it->first) << "\t\t" << _it->second
				<< endl;
	}

	return oss.str();
}

uint32_t BufferSizes::size()
{
	return _pointerSize;
}

void BufferSizes::_composePacket()
{
	uint16_t offset = 0;
	list_size_t numOfEntries = _bufferSizes.size();
	// [1] Node role
	memcpy(_pointer, &_nodeRole, sizeof(node_role_t));
	offset += sizeof(node_role_t);
	// [2] Number of pairs (List size)
	memcpy(_pointer, &numOfEntries, sizeof(numOfEntries));
	offset += sizeof(numOfEntries);

	// pair<buffer_name_t, buffer_size_t>
	for (_it = _bufferSizes.begin(); _it != _bufferSizes.end(); _it++)
	{
		// root scopes
		memcpy(_pointer + offset, &_it->first, sizeof(buffer_name_t));
		offset += sizeof(buffer_name_t);
		memcpy(_pointer + offset, &_it->second, sizeof(buffer_size_t));
		offset += sizeof(buffer_size_t);
	}
}

void BufferSizes::_decomposePacket()
{
	uint16_t offset = 0;
	list_size_t numOfEntries;
	pair<buffer_name_t, buffer_size_t> listEntry;
	// [1] node role
	memcpy(&_nodeRole, _pointer, sizeof(node_role_t));
	offset += sizeof(node_role_t);
	// [2] list size
	memcpy(&numOfEntries, _pointer, sizeof(list_size_t));
	offset += sizeof(list_size_t);

	for (list_size_t i = 0; i < numOfEntries; i++)
	{
		memcpy(&listEntry.first, _pointer + offset, sizeof(buffer_name_t));
		offset += sizeof(buffer_name_t);
		memcpy(&listEntry.second, _pointer + offset, sizeof(buffer_size_t));
		offset += sizeof(buffer_size_t);
		_bufferSizes.push_back(listEntry);
	}
}
