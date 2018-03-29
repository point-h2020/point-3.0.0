/*
 * numberofpublishers.cc
 *
 *  Created on: 21 Sep 2017
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

#include "publishersnamespace.hh"

PublishersNamespace::PublishersNamespace(
		publishers_namespace_t publishersNamespace)
	: _publishersNamespace(publishersNamespace)
{
	_size = sizeof(list_size_t) + _publishersNamespace.size() *
			(sizeof(root_scope_t) + sizeof(publishers_t));
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

PublishersNamespace::PublishersNamespace(uint8_t *pointer, uint32_t size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

PublishersNamespace::~PublishersNamespace()
{
	free(_pointer);
}

uint8_t * PublishersNamespace::pointer()
{
	return _pointer;
}

string PublishersNamespace::primitiveName()
{
	return "NUMBER_OF_PUBLISHERS_PER_NAMESPACE_M";
}

string PublishersNamespace::print()
{
	ostringstream oss;
	publishers_namespace_t::iterator it;
	oss << " | Number of publishers per root scope: " << endl;
	oss << "\tRoot Scope ID\tNumber of publishers" << endl;

	for (it = _publishersNamespace.begin();
			it != _publishersNamespace.end(); it++)
	{
		oss << "\t" << scopeToString(it->first) << "\t\t" << it->second
				<< endl;
	}

	return oss.str();
}

publishers_namespace_t PublishersNamespace::publishersNamespace()
{
	return _publishersNamespace;
}

size_t PublishersNamespace::size()
{
	return _size;
}

void PublishersNamespace::_composePacket()
{
	uint16_t offset = 0;
	list_size_t numOfEntries = _publishersNamespace.size();
	publishers_namespace_t::iterator it;
	// [1] Number of pairs (List size)
	memcpy(_pointer, &numOfEntries, sizeof(numOfEntries));
	offset += sizeof(numOfEntries);

	for (it = _publishersNamespace.begin(); it != _publishersNamespace.end();
			it++)
	{
		// root scopes
		memcpy(_pointer + offset, &it->first, sizeof(root_scope_t));
		offset += sizeof(root_scope_t);
		// number of publishers
		memcpy(_pointer + offset, &it->second, sizeof(publishers_t));
		offset += sizeof(publishers_t);
	}
}

void PublishersNamespace::_decomposePacket()
{
	uint16_t offset = 0;
	list_size_t numOfEntries;
	pair<root_scope_t, publishers_t> listEntry;
	// [1] list size
	memcpy(&numOfEntries, _pointer, sizeof(list_size_t));
	offset += sizeof(list_size_t);

	for (list_size_t i = 0; i < numOfEntries; i++)
	{
		memcpy(&listEntry.first, _pointer + offset, sizeof(root_scope_t));
		offset += sizeof(root_scope_t);
		memcpy(&listEntry.second, _pointer + offset, sizeof(publishers_t));
		offset += sizeof(publishers_t);
		_publishersNamespace.push_back(listEntry);
	}
}
