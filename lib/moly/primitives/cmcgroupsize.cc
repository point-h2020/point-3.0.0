/*
 * cmcgroupsize.cc
 *
 *  Created on: 22 Feb 2016
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

#include <iostream>
#include <sstream>
#include "cmcgroupsize.hh"

CmcGroupSize::CmcGroupSize(uint32_t groupSize)
	: _groupSize(groupSize)
{
	_size = sizeof(_groupSize);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

CmcGroupSize::CmcGroupSize(uint8_t * pointer,
		size_t size)
	: _size(size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

CmcGroupSize::~CmcGroupSize()
{
	free(_pointer);
}

uint32_t CmcGroupSize::groupSize()
{
	return _groupSize;
}

uint8_t * CmcGroupSize::pointer()
{
	return _pointer;
}

string CmcGroupSize::print()
{
	ostringstream oss;
	oss << " | CMC Group Size: " << groupSize() / 10.0;
	oss << " |";
	return oss.str();
}

size_t CmcGroupSize::size()
{
	return _size;
}

void CmcGroupSize::_composePacket()
{
	// [1] Coincidental multicast group size
	memcpy(_pointer, &_groupSize, sizeof(_groupSize));
}

void CmcGroupSize::_decomposePacket()
{
	// [1] Coincidental multicast group size
	memcpy(&_groupSize, _pointer, sizeof(_groupSize));
}
