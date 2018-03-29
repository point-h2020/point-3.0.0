/*
 * subscriberspernamespace.cc
 *
 *  Created on: 25 Sep 2017
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

#include "subscribersnamespace.hh"

SubscribersNamespace::SubscribersNamespace(
		subscribers_namespace_t subcribersNamespace)
	: _subcribersNamespace(subcribersNamespace)
{
	_size = sizeof(list_size_t) + _subcribersNamespace.size() *
			(sizeof(root_scope_t) + sizeof(subscribers_t));
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

SubscribersNamespace::SubscribersNamespace(
		uint8_t *pointer, uint32_t size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

SubscribersNamespace::~SubscribersNamespace()
{
	free(_pointer);
}

subscribers_namespace_t SubscribersNamespace::subcribersNamespace()
{
	return _subcribersNamespace;
}

uint8_t * SubscribersNamespace::pointer()
{
	return _pointer;
}

string SubscribersNamespace::print()
{
	ostringstream oss;
	subscribers_namespace_t::iterator it;
	oss << " | Number of subscribers per root scope: " << endl;
	oss << "\tNamespace\tNumber of subscribers" << endl;

	for (it = _subcribersNamespace.begin();
			it != _subcribersNamespace.end(); it++)
	{
		oss << "\t" << scopeToString(it->first) << "\t\t" << it->second
				<< endl;
	}

	return oss.str();
}

size_t SubscribersNamespace::size()
{
	return _size;
}

void SubscribersNamespace::_composePacket()
{
	uint16_t offset = 0;
	uint32_t numOfEntries = _subcribersNamespace.size();
	subscribers_namespace_t::iterator it;
	// [1] Number of pairs (List size)
	memcpy(_pointer, &numOfEntries, sizeof(numOfEntries));
	offset += sizeof(numOfEntries);

	for (it = _subcribersNamespace.begin();
			it != _subcribersNamespace.end(); it++)
	{
		// root scopes
		memcpy(_pointer + offset, &it->first, sizeof(root_scope_t));
		offset += sizeof(root_scope_t);
		memcpy(_pointer + offset, &it->second,
				sizeof(subscribers_t));
		offset += sizeof(subscribers_t);
	}
}

void SubscribersNamespace::_decomposePacket()
{
	uint16_t offset = 0;
	list_size_t numOfEntries;
	pair<root_scope_t, subscribers_t> listEntry;
	// [1] list size
	memcpy(&numOfEntries, _pointer, sizeof(numOfEntries));
	offset += sizeof(numOfEntries);

	for (list_size_t i = 0; i < numOfEntries; i++)
	{
		memcpy(&listEntry.first, _pointer + offset, sizeof(root_scope_t));
		offset += sizeof(root_scope_t);
		memcpy(&listEntry.second, _pointer + offset, sizeof(subscribers_t));
		offset += sizeof(subscribers_t);
		_subcribersNamespace.push_back(listEntry);
	}
}
