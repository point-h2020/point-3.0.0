/*
 * networklatencyperfqdn.cc
 *
 *  Created on: 9 Nov 2016
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

#include "networklatencyfqdn.hh"

NetworkLatencyFqdn::NetworkLatencyFqdn(uint32_t fqdn, uint16_t latency)
	: _fqdn(fqdn),
	  _latency(latency)
{
	_size = sizeof(_fqdn) + sizeof(_latency);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

NetworkLatencyFqdn::NetworkLatencyFqdn(uint8_t * pointer, size_t size)
	: _size(size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

NetworkLatencyFqdn::~NetworkLatencyFqdn()
{
	free(_pointer);
}

uint32_t NetworkLatencyFqdn::fqdn()
{
	return _fqdn;
}

uint16_t NetworkLatencyFqdn::latency()
{
	return _latency;
}

uint8_t * NetworkLatencyFqdn::pointer()
{
	return _pointer;
}

string NetworkLatencyFqdn::print()
{
	ostringstream oss;
	oss << " | FQDN: " << fqdn();
	oss << " | Network Latency: " << latency() / 10.0;
	oss << " |";
	return oss.str();
}

size_t NetworkLatencyFqdn::size()
{
	return _size;
}

void NetworkLatencyFqdn::_composePacket()
{
	// [1] FQDN
	memcpy(_pointer, &_fqdn, sizeof(_fqdn));
	//[2] Network latency
	memcpy(_pointer + sizeof(_fqdn), &_latency, sizeof(_latency));
}

void NetworkLatencyFqdn::_decomposePacket()
{
	// [1] FQDN
	memcpy(&_fqdn, _pointer, sizeof(_fqdn));
	// [2] Network latency
	memcpy(&_latency, _pointer + sizeof(_fqdn), sizeof(_latency));
}
