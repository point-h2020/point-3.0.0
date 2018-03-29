/*
 * port.cc
 *
 *  Created on: 26 Apr 2017
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of BlAckadder Monitoring wraPpER clasS (BAMPERS).
 *
 * BAMPERS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * BAMPERS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * BAMPERS. If not, see <http://www.gnu.org/licenses/>.
 */

#include "port.hh"

Port::Port(Namespace &namespaceHelper)
	: _namespaceHelper(namespaceHelper)
{}

Port::~Port() {}

void Port::add(uint32_t nodeId, uint16_t portId, string portName)
{
	string scopePath;
	uint32_t epoch = time(0);
	uint32_t nodeNameLength;
	uint32_t dataLength = sizeof(epoch) /*EPOCH*/ +
			sizeof(nodeNameLength) /*Name length*/ +
			portName.length() /*node name*/;
	uint8_t *data = (uint8_t *)malloc(dataLength);
	uint8_t offset = 0;
	scopePath = _namespaceHelper.getScopePath(nodeId, portId, II_NAME);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] Port length
	nodeNameLength = portName.length();
	memcpy(data + offset, &nodeNameLength, sizeof(nodeNameLength));
	offset += sizeof(nodeNameLength);
	// [3] Port name
	memcpy(data + offset, portName.c_str(), nodeNameLength);
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Port::packetDropRate(uint32_t nodeId, uint16_t portId, uint32_t rate)
{
	string scopePath;
	uint32_t epoch = time(0);
	uint32_t dataLength = sizeof(epoch) + sizeof(rate);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	uint8_t offset = 0;
	scopePath = _namespaceHelper.getScopePath(nodeId, portId,
			II_PACKET_DROP_RATE);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] Rate
	memcpy(data + offset, &rate, sizeof(rate));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Port::packetErrorRate(uint32_t nodeId, uint16_t portId, uint32_t rate)
{
	string scopePath;
	uint32_t epoch = time(0);
	uint32_t dataLength = sizeof(epoch) + sizeof(rate);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	uint8_t offset = 0;
	scopePath = _namespaceHelper.getScopePath(nodeId, portId,
			II_PACKET_ERROR_RATE);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] Rate
	memcpy(data + offset, &rate, sizeof(rate));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Port::state(uint32_t nodeId, uint16_t portId, state_t state)
{
	string scopePath;
	uint32_t epoch = time(0);
	uint32_t dataLength = sizeof(epoch) + sizeof(state_t);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	uint8_t offset = 0;
	scopePath = _namespaceHelper.getScopePath(nodeId, portId, II_STATE);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] State
	memcpy(data + offset, &state, sizeof(state_t));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}
