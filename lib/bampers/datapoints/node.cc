/*
 * node.cc
 *
 *  Created on: 17 Feb 2016
 *      Author: point
 */

#include "node.hh"

Node::Node(Namespace &namespaceHelper)
	: _namespaceHelper(namespaceHelper)
{}

Node::~Node(){}

void Node::add(node_role_t nodeRole, string nodeName, uint32_t nodeId)
{
	string scopePath;
	uint32_t epoch = time(0);
	uint32_t dataLength = sizeof(epoch) /*EPOCH*/ +
			sizeof(uint32_t) /*Name length*/ +
			nodeName.length() /*node name*/;
	uint8_t *data = (uint8_t *)malloc(dataLength);
	uint8_t offset = 0;
	scopePath = _namespaceHelper.getScopePath(nodeRole, nodeId, II_NAME);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] Name length
	uint32_t nodeNameLength = nodeName.length();
	memcpy(data + offset, &nodeNameLength, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	// [3] Node name
	memcpy(data + offset, nodeName.c_str(), nodeNameLength);
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}
