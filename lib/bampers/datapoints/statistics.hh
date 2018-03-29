/*
 * statistics.hh
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

#ifndef LIB_BAMPERS_DATAPOINTS_STATISTICS_HH_
#define LIB_BAMPERS_DATAPOINTS_STATISTICS_HH_

#include <moly/typedef.hh>

#include "../namespace.hh"
/*!
 * \brief Statistics class
 */
class Statistics {
public:
	/*!
	 * \brief Constructor
	 *
	 * \param namespaceHelper Reference to namespace helper class
	 */
	Statistics(Namespace &namespaceHelper);
	/*!
	 * \brief Destructor
	 */
	virtual ~Statistics();
	/*!
	 * \brief Send buffer sizes under
	 *
	 * /monitoring/nodes/nodeRole/nodeId/bufferSize
	 */
	void bufferSizes(node_role_t nodeRole, buffer_sizes_t bufferSizes);
	/*!
	 * \brief Send channel aquisition time under
	 *
	 * /monitoring/nodes/nodeRole/nodeId/channelAquisitionTime
	 */
	void channelAquisitionTime(uint32_t aquisitionTime);
	/*!
	 * \brief Send coincidental multicast group size under
	 * /topology/nodes/nap/nodeId/cmcGrouSize
	 *
	 * \param groupSize The averaged group size over an interval
	 */
	void cmcGroupSize(uint32_t groupSize);
	/*!
	 * \brief Send CPU utilisation under
	 *
	 * /monitoring/nodes/nodeRole/napNid/endpointId/cpuUtilisation
	 *
	 * or
	 *
	 * /monitoring/nodes/nodeRole/napNid/cpuUtilisation
	 *
	 * depending on the nodeRole
	 *
	 * \param nodeRole The node role
	 * \param endpointId The endpoint ID (0 if not UE or SV)
	 * \param utilisation CPU utilisation in % with a base of 0.01
	 */
	void cpuUtilisation(node_role_t nodeRole, uint32_t endpointId,
			uint16_t utilisation);
	/*!
	 * \brief Implementation of the END_TO_END_LATENCY primitive
	 *
	 * \param nodeRole The role of the node
	 * \param endpointID The ID of the endpoint
	 * \param latency The network latency
	 */
	void e2eLatency(node_role_t nodeRole, uint32_t endpointId, uint16_t latency);
	/*!
	 * \brief Implementation of the FILE_DESCRIPTORS_TYPE_B primitive
	 */
	void fileDescriptorsType(node_role_t nodeRole,
			file_descriptors_type_t fileDescriptorsType);
	/*!
	 * \brief TODO
	 */
	void httpRequestsFqdn(string fqdn, uint32_t numberOfRequests);
	/*!
	 * \brief Implementation of the MATCHES_PER_NAMESPACE_B primitive
	 *
	 * \param matchesNamespace List of pub-sub matches per namespace
	 */
	void matchesNamespace(matches_namespace_t matchesNamespace);
	/*!
	 * \brief Send the average network latency over the last reporting period
	 * under /topology/nodes/nap/nodeId/networkLatency
	 *
	 * \param networkLatency The average network latency over the last reporting
	 * interval
	 */
	void networkLatencyFqdn(uint32_t fqdn, uint16_t networkLatency);
	/*!
	 * \brief Implementation of the PACKET_JITTER_PER_CID_B primitive
	 *
	 * \brief
	 */
	void packetJitterCid(node_role_t nodeRole, node_identifier_t nodeId,
			packet_jitter_cid_t packetJitterCid);
	/*!
	 * \brief Send number of path calculations per root scope ID
	 *
	 * \param calculations The number of calculations per root scope ID
	 */
	void pathCalculationsNamespace(
			path_calculations_namespace_t calculations);
	/*!
	 * \brief Send the number of publishers per root scope ID
	 *
	 * \param publishersNamespace The number of publishers per namespace
	 */
	void publishersNamespace(publishers_namespace_t publishersNamespace);
	/*!
	 * \brief Implementation of RX_BYTES_HTTP_B
	 *
	 * \param nodeId The NID where the bytes where measured
	 * \param txBytes The number of received bytes
	 */
	void rxBytesHttp(node_identifier_t nodeId, bytes_t txBytes);
	/*!
	 * \brief Implementation of RX_BYTES_IP_B
	 *
	 * \param nodeId The NID where the bytes where measured
	 * \param txBytes The number of received bytes
	 */
	void rxBytesIp(node_identifier_t nodeId, bytes_t txBytes);
	/*!
	 * \brief Implementation of RX_BYTES_IP_MULTICAST_B
	 *
	 * \param nodeId The destination node ID of the link
	 * \param ipVersion The verison of the protocol
	 * \param txBytes The number of received bytes
	 */
	void rxBytesIpMulticast(node_identifier_t nodeId, ip_version_t ipVersion,
			bytes_t rxBytes);
	/*!
	 * \brief Send received bytes under /monitoring/ports/nodeId/portId/rxBytes
	 *
	 * This method enables an ICN application to publish received bytes for
	 * a particular port ID.
	 *
	 * \param nodeId The node ID the port resides on
	 * \param portId The port identifier
	 * \param receivedBytes The number of transmitted bytes
	 */
	void rxBytesPort(uint32_t nodeId, uint16_t portId,
			uint32_t receivedBytes);
	/*!
	 * \brief Update the state of an IP endpoint in the network
	 */
	void stateIpEndpoint(node_role_t nodeRole, string hostname,
			node_state_t state);
	/*!
	 * \brief Update the state of an ICN network element
	 *
	 * \param nodeRole The node type (role)
	 * \param nodeId The NID for which the state is supposed to be updated
	 * \param state The new state of this node type
	 */
	void stateNetworkElement(node_role_t nodeRole, uint32_t nodeId,
			node_state_t state);
	/*!
	 * \brief Send the number of publishers per root scope ID
	 *
	 * \param subscribersNamespace The number of subscribers per namespace
	 */
	void subscribersNamespace(
			subscribers_namespace_t subscribersNamespace);
	/*!
	 * \brief Implementation of TX_BYTES_HTTP_B
	 *
	 * \param nodeId The NID where the bytes where measured
	 * \param txBytes The number of transmitted bytes
	 */
	void txBytesHttp(node_identifier_t nodeId, bytes_t txBytes);
	/*!
	 * \brief Implementation of TX_BYTES_IP_B
	 *
	 * \param nodeId The NID where the bytes where measured
	 * \param txBytes The number of transmitted bytes
	 */
	void txBytesIp(node_identifier_t nodeId, bytes_t txBytes);
	/*!
	 * \brief Implementation of TX_BYTES_IP_MULTICAST_B
	 *
	 * \param nodeId The destination node ID of the link
	 * \param ipVersion The verison of the protocol
	 * \param txBytes The number of transmitted bytes
	 */
	void txBytesIpMulticast(node_identifier_t nodeId, ip_version_t ipVersion,
			bytes_t txBytes);
	/*!
	 * \brief Send transmitted bytes under /Topology/Links/LID/NodeId/TxBytes
	 *
	 * This method enables an ICN application to publish transmitted bytes over
	 * a particular Link ID.
	 *
	 * \param nodeId The destination node ID of the link
	 * \param transmittedBytes The number of transmitted bytes
	 */
	void txBytesPort(uint32_t nodeId, uint16_t portId,
			uint32_t transmittedBytes);
private:
	Namespace &_namespaceHelper; /*!< Reference to the class Namespace */
};

#endif /* LIB_BAMPERS_DATAPOINTS_STATISTICS_HH_ */
