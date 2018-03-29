/*
 * cputilisation.cc
 *
 *  Created on: 16 Feb 2017
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
#include "cpuutilisation.hh"

CpuUtilisation::CpuUtilisation(node_role_t nodeRole, uint16_t cpuUtilisation)
	: _nodeRole(nodeRole),
	  _cpuUtilisation(cpuUtilisation)
{
	_size = sizeof(_nodeRole) + sizeof(_endpointId) + sizeof(_cpuUtilisation);
	_pointer = (uint8_t *)malloc(_size);
	_endpointId = 0;
	_composePacket();
}

CpuUtilisation::CpuUtilisation(node_role_t nodeRole, uint32_t endpointId,
		uint16_t cpuUtilisation)
	: _nodeRole(nodeRole),
	  _endpointId(endpointId),
	  _cpuUtilisation(cpuUtilisation)
{
	_size = sizeof(_nodeRole) + sizeof(_endpointId) + sizeof(_cpuUtilisation);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

CpuUtilisation::CpuUtilisation(uint8_t * pointer, size_t size)
	: _size(size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

CpuUtilisation::~CpuUtilisation()
{
	free(_pointer);
}

uint16_t CpuUtilisation::cpuUtilisation()
{
	return _cpuUtilisation;
}

uint32_t CpuUtilisation::endpointId()
{
	return _endpointId;
}

node_role_t CpuUtilisation::nodeRole()
{
	return _nodeRole;
}

uint8_t * CpuUtilisation::pointer()
{
	return _pointer;
}

string CpuUtilisation::print()
{
	ostringstream oss;

	oss << " | Node Role: " << nodeRole();
	oss << " | Endpoint ID: " << endpointId();
	oss << " | CPU Utilisation " << cpuUtilisation() / 100.0;
	oss << " |";
	return oss.str();
}

size_t CpuUtilisation::size()
{
	return _size;
}

void CpuUtilisation::_composePacket()
{
	uint8_t offset = 0;
	// [1] Node role
	memcpy(_pointer, &_nodeRole, sizeof(_nodeRole));
	offset += sizeof(_nodeRole);
	// [2] Endpoint ID
	memcpy(_pointer + offset, &_endpointId, sizeof(_endpointId));
	offset += sizeof(_endpointId);
	// [2] CPU Utilisation
	memcpy(_pointer + offset, &_cpuUtilisation,	sizeof(_cpuUtilisation));
}

void CpuUtilisation::_decomposePacket()
{
	uint8_t offset = 0;
	// [1] Node role
	memcpy(&_nodeRole, _pointer, sizeof(_nodeRole));
	offset += sizeof(_nodeRole);
	// [2] Endpoint ID
	memcpy(&_endpointId, _pointer + offset, sizeof(_endpointId));
	offset += sizeof(_endpointId);
	// [3] CPU utilisation
	memcpy(&_cpuUtilisation, _pointer + offset, sizeof(_cpuUtilisation));
}
