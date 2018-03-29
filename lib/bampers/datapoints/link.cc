/*
 * link.cc
 *
 *  Created on: 19 Dec 2015
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

#include "link.hh"

Link::Link(Namespace &namespaceHelper)
	: _namespaceHelper(namespaceHelper)
{}

Link::~Link(){}

void Link::add(uint16_t linkId, uint32_t destinationNodeId,
		uint32_t sourceNodeId, uint16_t sourcePort, link_type_t linkType,
		string linkName)
{
	string scopePath;
	uint8_t offset = 0;
	uint32_t epoch = time(0);
	uint32_t linkNameLength = linkName.length();
	scopePath = _namespaceHelper.getScopePath(linkId, destinationNodeId,
			sourceNodeId, sourcePort, linkType, II_NAME);
	uint32_t dataLength = sizeof(epoch) /* EPOCH */
			+ sizeof(uint32_t) /* Name length */ +
			linkName.length() /* Name itself */;
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] link name length
	memcpy(data + offset, &linkNameLength, sizeof(linkNameLength));
	offset += sizeof(linkNameLength);
	// [3] link name itself
	memcpy(data + offset, linkName.c_str(), linkName.length());
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Link::state(uint16_t linkId, uint32_t destinationNodeId,
		uint32_t sourceNodeId, uint16_t sourcePortId, link_type_t linkType,
		link_state_t state)
{
	string scopePath;
	uint8_t offset = 0;
	uint16_t linkState;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(linkId, destinationNodeId,
			sourceNodeId, sourcePortId, linkType, II_STATE);
	size_t dataLength = sizeof(epoch) + sizeof(linkState);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] State
	linkState = (uint16_t)state;
	memcpy(data + offset, &state, sizeof(linkState));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}
