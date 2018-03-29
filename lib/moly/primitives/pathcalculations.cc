/*
 * pathcalculations.cc
 *
 *  Created on: 19 Sep 2017
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

#include "pathcalculations.hh"

PathCalculationsNamespace::PathCalculationsNamespace(
		path_calculations_namespace_t pathCalculationsNamespace)
	: _pathCalculationsNamespace(pathCalculationsNamespace)
{
	_size = sizeof(list_size_t) + _pathCalculationsNamespace.size() *
			(sizeof(root_scope_t) + sizeof(path_calculations_t));
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

PathCalculationsNamespace::PathCalculationsNamespace(uint8_t *pointer,
		uint32_t size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

PathCalculationsNamespace::~PathCalculationsNamespace()
{
	free(_pointer);
}

path_calculations_namespace_t PathCalculationsNamespace::pathCalculationsNamespace()
{
	return _pathCalculationsNamespace;
}

uint8_t * PathCalculationsNamespace::pointer()
{
	return _pointer;
}

string PathCalculationsNamespace::primitiveName()
{
	return "PATH_CALCULATIONS_NAMESPACE_M";
}

string PathCalculationsNamespace::print()
{
	ostringstream oss;
	oss << " | Number of path calculations per namespace: " << endl;
	oss << "\tNamespace\tNumber of path calculations" << endl;

	for (it = _pathCalculationsNamespace.begin();
			it != _pathCalculationsNamespace.end(); it++)
	{
		oss << "\t" << scopeToString(it->first) << "\t\t" << it->second
				<< endl;
	}

	return oss.str();
}

size_t PathCalculationsNamespace::size()
{
	return _size;
}

void PathCalculationsNamespace::_composePacket()
{
	uint16_t offset = 0;
	list_size_t numOfEntries = _pathCalculationsNamespace.size();
	// [1] Number of pairs (List size)
	memcpy(_pointer, &numOfEntries, sizeof(numOfEntries));
	offset += sizeof(numOfEntries);

	// pair<root scopes, path calculations>
	for (it = _pathCalculationsNamespace.begin();
			it != _pathCalculationsNamespace.end(); it++)
	{
		// root scopes
		memcpy(_pointer + offset, &it->first, sizeof(root_scope_t));
		offset += sizeof(root_scope_t);
		memcpy(_pointer + offset, &it->second,
				sizeof(path_calculations_t));
		offset += sizeof(path_calculations_t);
	}
}

void PathCalculationsNamespace::_decomposePacket()
{
	uint16_t offset = 0;
	list_size_t numOfEntries;
	pair<root_scope_t, path_calculations_t> listEntry;
	// [1] list size
	memcpy(&numOfEntries, _pointer, sizeof(list_size_t));
	offset += sizeof(list_size_t);

	for (list_size_t i = 0; i < numOfEntries; i++)
	{
		memcpy(&listEntry.first, _pointer + offset, sizeof(root_scope_t));
		offset += sizeof(root_scope_t);
		memcpy(&listEntry.second, _pointer + offset, sizeof(path_calculations_t));
		offset += sizeof(path_calculations_t);
		_pathCalculationsNamespace.push_back(listEntry);
	}
}
