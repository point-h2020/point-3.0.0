/*
 * openfiledescriptors.cc
 *
 *  Created on: 18 Oct 2017
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

#include "filedescriptorstype.hh"

FileDescriptorsType::FileDescriptorsType(node_role_t nodeRole,
		file_descriptors_type_t fileDescriptorsType)
	: _nodeRole(nodeRole),
	  _fileDescriptorsType(fileDescriptorsType)
{
	_pointerSize = sizeof(node_role_t) + sizeof(list_size_t)
			+ _fileDescriptorsType.size() * (sizeof(file_descriptors_t) +
					sizeof(file_descriptor_type_t));
	_pointer = (uint8_t *)malloc(_pointerSize);
	_composePacket();
}

FileDescriptorsType::FileDescriptorsType(uint8_t *pointer,
		uint32_t pointerSize)
{
	_pointer = (uint8_t *)malloc(pointerSize);
	memcpy(_pointer, pointer, pointerSize);
	_decomposePacket();
}

FileDescriptorsType::~FileDescriptorsType()
{
	free(_pointer);
}

file_descriptors_type_t FileDescriptorsType::fileDescriptorsType()
{
	return _fileDescriptorsType;
}

node_role_t FileDescriptorsType::nodeRole()
{
	return _nodeRole;
}

uint8_t *FileDescriptorsType::pointer()
{
	return _pointer;
}

string FileDescriptorsType::primitiveName()
{
	return "OPEN_FILE_DESCRIPTORS_M";
}

string FileDescriptorsType::print()
{
	ostringstream oss;
	oss << " | open file descriptors: " << endl;
	oss << "\tDescriptor Type\t\t\tNumber of descriptors" << endl;

	for (_it = _fileDescriptorsType.begin();
			_it != _fileDescriptorsType.end(); _it++)
	{
		oss << "\t" << descriptorTypeToString(_it->first) << "\t"
				<< _it->second << endl;
	}

	return oss.str();
}

uint32_t FileDescriptorsType::size()
{
	return _pointerSize;
}

void FileDescriptorsType::_composePacket()
{
	uint16_t offset = 0;
	list_size_t numOfEntries = _fileDescriptorsType.size();
	// [1] Node role
	memcpy(_pointer, &_nodeRole, sizeof(node_role_t));
	offset += sizeof(node_role_t);
	// [2] Number of pairs (List size)
	memcpy(_pointer, &numOfEntries, sizeof(numOfEntries));
	offset += sizeof(numOfEntries);

	for (_it = _fileDescriptorsType.begin();
			_it != _fileDescriptorsType.end(); _it++)
	{
		memcpy(_pointer + offset, &_it->first, sizeof(file_descriptors_t));
		offset += sizeof(file_descriptors_t);
		memcpy(_pointer + offset, &_it->second, sizeof(file_descriptor_type_t));
		offset += sizeof(file_descriptor_type_t);
	}
}

void FileDescriptorsType::_decomposePacket()
{
	uint16_t offset = 0;
	list_size_t numOfEntries;
	pair<file_descriptor_type_t, file_descriptors_t> listEntry;
	// [1] node role
	memcpy(&_nodeRole, _pointer, sizeof(node_role_t));
	offset += sizeof(node_role_t);
	// [2] list size
	memcpy(&numOfEntries, _pointer, sizeof(list_size_t));
	offset += sizeof(list_size_t);

	for (list_size_t i = 0; i < numOfEntries; i++)
	{
		memcpy(&listEntry.first, _pointer + offset, sizeof(file_descriptors_t));
		offset += sizeof(file_descriptors_t);
		memcpy(&listEntry.second, _pointer + offset,
				sizeof(file_descriptor_type_t));
		offset += sizeof(file_descriptor_type_t);
		_fileDescriptorsType.push_back(listEntry);
	}
}
