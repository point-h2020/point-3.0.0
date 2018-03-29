/*
 * channelaquisitiontime.cc
 *
 *  Created on: 15 Sep 2017
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

#include "channelaquisitiontime.hh"

ChannelAquisitionTime::ChannelAquisitionTime(uint32_t channelAquisitionTime)
	: _channelAquisitionTime(channelAquisitionTime)
{
	_size = sizeof(_channelAquisitionTime);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

ChannelAquisitionTime::ChannelAquisitionTime(uint8_t *pointer, uint32_t size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

ChannelAquisitionTime::~ChannelAquisitionTime()
{
	free(_pointer);
}

uint32_t ChannelAquisitionTime::channelAquisitionTime()
{
	return _channelAquisitionTime;
}

uint8_t * ChannelAquisitionTime::pointer()
{
	return _pointer;
}

string ChannelAquisitionTime::print()
{
	ostringstream oss;
	oss << " | Aquisition Time: " << channelAquisitionTime() / 10.0;
	oss << " |";
	return oss.str();
}

size_t ChannelAquisitionTime::size()
{
	return _size;
}

void ChannelAquisitionTime::_composePacket()
{
	// [1] Channel aquisition time
	memcpy(_pointer, &_channelAquisitionTime, sizeof(_channelAquisitionTime));
}

void ChannelAquisitionTime::_decomposePacket()
{
	// [1] Channel aquisition time
	memcpy(&_channelAquisitionTime, _pointer, sizeof(_channelAquisitionTime));
}
