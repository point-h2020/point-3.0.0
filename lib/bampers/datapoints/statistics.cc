/*
 * statistics.cc
 *
 *  Created on: Dec 17, 2015
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

#include "statistics.hh"

Statistics::Statistics(Namespace &namespaceHelper)
	: _namespaceHelper(namespaceHelper)
{}

Statistics::~Statistics(){}

void Statistics::bufferSizes(node_role_t nodeRole, buffer_sizes_t bufferSizes)
{
	string scopePath;
	uint8_t offset = 0;
	buffer_sizes_t::iterator it;
	list_size_t listSize = bufferSizes.size();
	pair<buffer_name_t, buffer_size_t> listPair;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(nodeRole, II_BUFFER_SIZES);
	size_t dataLength = sizeof(epoch) + sizeof(listSize) + listSize *
			(sizeof(listPair.first) + sizeof(listPair.second));
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] list size
	memcpy(data + offset, &listSize, sizeof(listSize));
	offset += sizeof(listSize);

	// [3...n] pairs
	for (it = bufferSizes.begin(); it != bufferSizes.end(); it++)
	{
		memcpy(data + offset, &it->first, sizeof(listPair.first));
		offset += sizeof(listPair.first);
		memcpy(data + offset, &it->second, sizeof(listPair.second));
		offset += sizeof(listPair.second);
	}

	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::channelAquisitionTime(uint32_t aquisitionTime)
{
	string scopePath;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_NAP,
			II_CHANNEL_AQUISITION_TIME);
	uint32_t dataLength = sizeof(epoch) + sizeof(aquisitionTime);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	// [2] aquisition time
	memcpy(data + sizeof(epoch), &aquisitionTime, sizeof(aquisitionTime));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::cmcGroupSize(uint32_t groupSize)
{
	string scopePath;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_NAP, II_CMC_GROUP_SIZE);
	uint32_t dataLength = sizeof(epoch) + sizeof(groupSize);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	// [2] group size
	memcpy(data + sizeof(epoch), &groupSize, sizeof(groupSize));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::cpuUtilisation(node_role_t nodeRole, uint32_t endpointId,
			uint16_t utilisation)
{
	string scopePath;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(nodeRole, endpointId,
			II_CPU_UTILISATION);
	uint32_t dataLength = sizeof(epoch) + sizeof(utilisation);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	// [2] CPU utilisation
	memcpy(data + sizeof(epoch), &utilisation, sizeof(utilisation));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::e2eLatency(node_role_t nodeRole, uint32_t endpointId,
		uint16_t latency)
{
	string scopePath;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(nodeRole, endpointId,
			II_END_TO_END_LATENCY);
	size_t dataLength = sizeof(epoch) +	sizeof(latency);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	uint8_t offset = 0;
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] E2E Latency
	memcpy(data + offset, &latency, sizeof(latency));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::fileDescriptorsType(node_role_t nodeRole,
		file_descriptors_type_t fileDescriptorsType)
{
	string scopePath;
	uint8_t offset = 0;
	publishers_namespace_t::iterator it;
	uint32_t listSize = fileDescriptorsType.size();
	pair<file_descriptor_type_t, file_descriptors_t> listPair;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(nodeRole,
			II_FILE_DESCRIPTORS_TYPE);
	size_t dataLength = sizeof(epoch) + sizeof(listSize) + listSize *
			(sizeof(listPair.first) + sizeof(listPair.second));
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] list size
	memcpy(data + offset, &listSize, sizeof(listSize));
	offset += sizeof(listSize);

	// [3...n] pairs
	for (it = fileDescriptorsType.begin();
			it != fileDescriptorsType.end(); it++)
	{
		memcpy(data + offset, &it->first, sizeof(listPair.first));
		offset += sizeof(listPair.first);
		memcpy(data + offset, &it->second, sizeof(listPair.second));
		offset += sizeof(listPair.second);
	}

	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::httpRequestsFqdn(string fqdn, uint32_t numberOfRequests)
{
	string scopePath;
	uint32_t fqdnLength;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_NAP,
			II_HTTP_REQUESTS_FQDN);
	size_t dataLength = sizeof(epoch) +	sizeof(fqdnLength) + fqdn.length() +
			sizeof(uint32_t) /*number of requests*/;
	uint8_t *data = (uint8_t *)malloc(dataLength);
	uint8_t offset = 0;
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] FQDN length
	fqdnLength = fqdn.length();
	memcpy(data + offset, &fqdnLength, sizeof(fqdnLength));
	offset += sizeof(fqdnLength);
	// [3] FQDN
	memcpy(data + offset, fqdn.c_str(), fqdn.length());
	offset += fqdn.length();
	// [4] # of HTTP requests
	memcpy(data + offset, &numberOfRequests, sizeof(uint32_t));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::matchesNamespace(
		matches_namespace_t matchesNamespace)
{
	string scopePath;
	uint16_t offset = 0;
	matches_namespace_t::iterator it;
	list_size_t listSize = matchesNamespace.size();
	pair<root_scope_t, matches_t> listPair;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_RV,
			II_MATCHES_NAMESPACE);
	size_t dataLength = sizeof(epoch) + sizeof(list_size_t) + listSize *
			(sizeof(root_scope_t) + sizeof(matches_t));
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] list size
	memcpy(data + offset, &listSize, sizeof(list_size_t));
	offset += sizeof(list_size_t);

	// [3...n] pairs
	for (it = matchesNamespace.begin(); it != matchesNamespace.end();
			it++)
	{
		memcpy(data + offset, &it->first, sizeof(root_scope_t));
		offset += sizeof(root_scope_t);
		memcpy(data + offset, &it->second, sizeof(matches_t));
		offset += sizeof(matches_t);
	}

	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::networkLatencyFqdn(uint32_t fqdn, uint16_t networkLatency)
{
	string scopePath;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_NAP,
			II_NETWORK_LATENCY_FQDN);
	size_t dataLength = sizeof(epoch) + sizeof(fqdn) + sizeof(networkLatency);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	uint8_t offset = 0;
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] hashed FQDN
	memcpy(data + offset, &fqdn, sizeof(fqdn));
	offset += sizeof(fqdn);
	// [3] Network latency
	memcpy(data + offset, &networkLatency, sizeof(networkLatency));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::packetJitterCid(node_role_t nodeRole,
		node_identifier_t nodeId, packet_jitter_cid_t packetJitterCid)
{
	string scopePath;
	uint16_t offset = 0;
	packet_jitter_cid_t::iterator it;
	list_size_t listSize = packetJitterCid.size();
	pair<content_identifier_t, packet_jitter_t> listPair;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(nodeRole, nodeId,
			II_PACKET_JITTER_CID);
	size_t dataLength = sizeof(epoch) + sizeof(list_size_t) + listSize *
			(sizeof(listPair.first) + sizeof(listPair.second));
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] list size
	memcpy(data + offset, &listSize, sizeof(list_size_t));
	offset += sizeof(list_size_t);

	// [3...n] pairs
	for (it = packetJitterCid.begin(); it != packetJitterCid.end(); it++)
	{
		memcpy(data + offset, &it->first, sizeof(listPair.first));
		offset += sizeof(listPair.first);
		memcpy(data + offset, &it->second, sizeof(listPair.second));
		offset += sizeof(listPair.second);
	}

	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::pathCalculationsNamespace(
		path_calculations_namespace_t calculations)
{
	string scopePath;
	uint8_t offset = 0;
	path_calculations_namespace_t::iterator it;
	uint32_t listSize = calculations.size();
	pair<root_scope_t, path_calculations_t> listPair;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_TM,
			II_PATH_CALCULATIONS_NAMESPACE);
	size_t dataLength = sizeof(epoch) + sizeof(listSize) + listSize *
			(sizeof(listPair.first) + sizeof(listPair.second));
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] list size
	memcpy(data + offset, &listSize, sizeof(listSize));
	offset += sizeof(listSize);

	// [3...n] pairs
	for (it = calculations.begin(); it != calculations.end(); it++)
	{
		memcpy(data + offset, &it->first, sizeof(listPair.first));
		offset += sizeof(listPair.first);
		memcpy(data + offset, &it->second, sizeof(listPair.second));
		offset += sizeof(listPair.second);
	}

	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::publishersNamespace(
		publishers_namespace_t publishersNamespace)
{
	string scopePath;
	uint8_t offset = 0;
	publishers_namespace_t::iterator it;
	uint32_t listSize = publishersNamespace.size();
	pair<root_scope_t, publishers_t> listPair;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_RV,
			II_PUBLISHERS_NAMESPACE);
	size_t dataLength = sizeof(epoch) + sizeof(listSize) + listSize *
			(sizeof(listPair.first) + sizeof(listPair.second));
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] list size
	memcpy(data + offset, &listSize, sizeof(listSize));
	offset += sizeof(listSize);

	// [3...n] pairs
	for (it = publishersNamespace.begin();
			it != publishersNamespace.end(); it++)
	{
		memcpy(data + offset, &it->first, sizeof(listPair.first));
		offset += sizeof(listPair.first);
		memcpy(data + offset, &it->second, sizeof(listPair.second));
		offset += sizeof(listPair.second);
	}

	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::rxBytesHttp(node_identifier_t nodeId, bytes_t rxBytes)
{
	string scopePath;
	uint32_t epoch = time(0);
	uint8_t offset = 0;
	// only NAPs can send this date point. Hence, it is implicitely assumed that
	// the role is NODE_ROLE_NAP
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_NAP, nodeId,
			II_RX_BYTES_HTTP);
	size_t dataLength = sizeof(epoch) + sizeof(bytes_t);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] Rx bytes
	memcpy(data + offset, &rxBytes, sizeof(bytes_t));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::rxBytesIp(node_identifier_t nodeId, bytes_t rxBytes)
{
	string scopePath;
	uint32_t epoch = time(0);
	uint8_t offset = 0;
	// only NAPs can send this date point. Hence, it is implicitely assumed that
	// the role is NODE_ROLE_NAP
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_NAP, nodeId,
			II_RX_BYTES_IP);
	size_t dataLength = sizeof(epoch) + sizeof(bytes_t);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] Rx bytes
	memcpy(data + offset, &rxBytes, sizeof(bytes_t));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::rxBytesIpMulticast(node_identifier_t nodeId,
		ip_version_t ipVersion, bytes_t rxBytes)
{
	string scopePath;
	uint32_t epoch = time(0);
	uint8_t offset = 0;
	// only NAPs can send this date point. Hence, it is implicitely assumed that
	// the role is NODE_ROLE_NAP
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_NAP, nodeId,
			II_RX_BYTES_IP_MULTICAST);
	size_t dataLength = sizeof(epoch) + sizeof(ip_version_t) + sizeof(bytes_t);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] IP version
	memcpy(data + offset, &ipVersion, sizeof(ip_version_t));
	offset += sizeof(ip_version_t);
	// [3] Received bytes
	memcpy(data + offset, &rxBytes, sizeof(bytes_t));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::rxBytesPort(uint32_t nodeId, uint16_t portId,
			uint32_t receivedBytes)
{
	string scopePath;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(nodeId, portId, II_RX_BYTES_PORT);
	size_t dataLength = sizeof(epoch) + sizeof(receivedBytes);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	// [2] Received bytes
	memcpy(data + sizeof(epoch), &receivedBytes,	sizeof(receivedBytes));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::stateIpEndpoint(node_role_t nodeRole, string hostname,
		node_state_t state)
{
	string scopePath;
	uint32_t hostnameLength;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(nodeRole, II_STATE);
	size_t dataLength = sizeof(epoch) + sizeof(node_state_t)
			+ sizeof(hostnameLength) + hostname.length();
	uint8_t *data = (uint8_t *)malloc(dataLength);
	uint8_t offset = 0;
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] State
	memcpy(data + offset, &state, sizeof(node_state_t));
	offset += sizeof(node_state_t);
	// [3] Hostname length
	hostnameLength = hostname.length();
	memcpy(data + offset, &hostnameLength, sizeof(hostnameLength));
	offset += sizeof(hostnameLength);
	// [4] Hostname
	memcpy(data + offset, &hostname, hostnameLength);
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::stateNetworkElement(node_role_t nodeRole, uint32_t nodeId,
		node_state_t state)
{
	string scopePath;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(nodeRole, nodeId, II_STATE);
	size_t dataLength = sizeof(epoch) + sizeof(node_state_t);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	uint8_t offset = 0;
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] State
	memcpy(data + offset, &state, sizeof(node_state_t));
	offset += sizeof(node_state_t);
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::subscribersNamespace(
		subscribers_namespace_t subscribersNamespace)
{
	string scopePath;
	uint8_t offset = 0;
	publishers_namespace_t::iterator it;
	uint32_t listSize = subscribersNamespace.size();
	pair<root_scope_t, publishers_t> listPair;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_RV,
			II_SUBSCRIBERS_NAMESPACE);
	size_t dataLength = sizeof(epoch) + sizeof(listSize) + listSize *
			(sizeof(listPair.first) + sizeof(listPair.second));
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] list size
	memcpy(data + offset, &listSize, sizeof(listSize));
	offset += sizeof(listSize);

	// [3...n] pairs
	for (it = subscribersNamespace.begin();
			it != subscribersNamespace.end(); it++)
	{
		memcpy(data + offset, &it->first, sizeof(listPair.first));
		offset += sizeof(listPair.first);
		memcpy(data + offset, &it->second, sizeof(listPair.second));
		offset += sizeof(listPair.second);
	}

	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::txBytesHttp(node_identifier_t nodeId, bytes_t txBytes)
{
	string scopePath;
	uint32_t epoch = time(0);
	uint8_t offset = 0;
	// only NAPs can send this date point. Hence, it is implicitely assumed that
	// the role is NODE_ROLE_NAP
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_NAP, nodeId,
			II_TX_BYTES_HTTP);
	size_t dataLength = sizeof(epoch) + sizeof(bytes_t);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] Tx bytes
	memcpy(data + offset, &txBytes, sizeof(bytes_t));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::txBytesIp(node_identifier_t nodeId, bytes_t txBytes)
{
	string scopePath;
	uint32_t epoch = time(0);
	uint8_t offset = 0;
	// only NAPs can send this date point. Hence, it is implicitely assumed that
	// the role is NODE_ROLE_NAP
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_NAP, nodeId,
			II_TX_BYTES_IP);
	size_t dataLength = sizeof(epoch) + sizeof(bytes_t);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] Tx bytes
	memcpy(data + offset, &txBytes, sizeof(bytes_t));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::txBytesIpMulticast(node_identifier_t nodeId,
		ip_version_t ipVersion, bytes_t txBytes)
{
	string scopePath;
	uint32_t epoch = time(0);
	uint8_t offset = 0;
	// only NAPs can send this date point. Hence, it is implicitely assumed that
	// the role is NODE_ROLE_NAP
	scopePath = _namespaceHelper.getScopePath(NODE_ROLE_NAP, nodeId,
			II_TX_BYTES_IP_MULTICAST);
	size_t dataLength = sizeof(epoch) + sizeof(ip_version_t) + sizeof(bytes_t);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	offset += sizeof(epoch);
	// [2] IP version
	memcpy(data + offset, &ipVersion, sizeof(ip_version_t));
	offset += sizeof(ip_version_t);
	// [3] Tx bytes
	memcpy(data + offset, &txBytes, sizeof(bytes_t));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}

void Statistics::txBytesPort(uint32_t nodeId, uint16_t portId,
		uint32_t transmittedBytes)
{
	string scopePath;
	uint32_t epoch = time(0);
	scopePath = _namespaceHelper.getScopePath(nodeId, portId, II_TX_BYTES_PORT);
	size_t dataLength = sizeof(epoch) + sizeof(transmittedBytes);
	uint8_t *data = (uint8_t *)malloc(dataLength);
	// [1] EPOCH
	memcpy(data, &epoch, sizeof(epoch));
	// [2] Transmitted bytes
	memcpy(data + sizeof(epoch), &transmittedBytes,	sizeof(transmittedBytes));
	_namespaceHelper.prepareDataToBePublished(scopePath, data, dataLength);
	free(data);
}
