/*
 * matchesnamespace.cc
 *
 *  Created on: 28 Sep 2017
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

#include "matchesnamespace.hh"

MatchesNamespace::MatchesNamespace(
		matches_namespace_t matchesNamespace)
	: _matchesNamespace(matchesNamespace)
{
	_size = sizeof(uint32_t) + _matchesNamespace.size() *
			(sizeof(root_scope_t) + sizeof(matches_t));
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

MatchesNamespace::MatchesNamespace(uint8_t *pointer, uint32_t size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

MatchesNamespace::~MatchesNamespace()
{
	free(_pointer);
}

matches_namespace_t MatchesNamespace::matchesNamespace()
{
	return _matchesNamespace;
}

uint8_t * MatchesNamespace::pointer()
{
	return _pointer;
}

string MatchesNamespace::primitiveName()
{
	return "MATCHES_PER_NAMESPACE_M";
}

string MatchesNamespace::print()
{
	ostringstream oss;
	oss << " | Number of pub-sub matches per namespace: " << endl;
	oss << "\tRoot Scope ID\tNumber of pub-sub matches" << endl;

	for (it = _matchesNamespace.begin();
			it != _matchesNamespace.end(); it++)
	{
		oss << "\t" << scopeToString(it->first) << "\t\t" << it->second
				<< endl;
	}

	return oss.str();
}

size_t MatchesNamespace::size()
{
	return _size;
}

void MatchesNamespace::_composePacket()
{
	uint16_t offset = 0;
	uint32_t numOfEntries = _matchesNamespace.size();
	// [1] Number of pairs (List size)
	memcpy(_pointer, &numOfEntries, sizeof(numOfEntries));
	offset += sizeof(numOfEntries);

	// pair<root scopes, pub/sub matches>
	for (it = _matchesNamespace.begin();
			it != _matchesNamespace.end(); it++)
	{
		// root scopes
		memcpy(_pointer + offset, &it->first, sizeof(root_scope_t));
		offset += sizeof(root_scope_t);
		memcpy(_pointer + offset, &it->second,
				sizeof(matches_t));
		offset += sizeof(matches_t);
	}
}

void MatchesNamespace::_decomposePacket()
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
		memcpy(&listEntry.second, _pointer + offset, sizeof(matches_t));
		offset += sizeof(matches_t);
		_matchesNamespace.push_back(listEntry);
	}
}
