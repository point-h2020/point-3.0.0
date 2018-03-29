/*
 * process.hh
 *
 *  Created on: 20 Feb 2016
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

#ifndef MOLY_PROCESS_HH_
#define MOLY_PROCESS_HH_

#include <blackadder_enums.hpp>
#include <boost/thread/mutex.hpp>
#include "enum.hh"
#include <errno.h>
#include <iostream>
#include <linux/netlink.h>
#include "primitives.hh"
#include <sys/socket.h>
#include "typedef.hh"

using namespace std;

/*!
 * \brief Implementation of boostrapping class for processes
 */
class Process {
public:
	/*!
	 * \brief Constructor
	 */
	Process();
	/*!
	 * \brief Destructor
	 */
	~Process();
	/*!
	 * \brief Wrapper for ADD_LINK primitive
	 *
	 * STATE_UP by default
	 *
	 */
	bool addLink(uint16_t linkId, uint32_t destinationNodeId,
			uint32_t sourceNodeId, uint16_t sourcePortId, link_type_t linkType,
			string name);
	/*!
	 * \brief Wrapper for ADD_NODE_M primitive
	 */
	bool addNode(string name, uint32_t nodeId, node_role_t nodeRole);
	/*!
	 * \brief Wrapper for ADD_PORT_M primitive
	 */
	bool addPort(uint32_t nodeId, uint16_t portId, string portName);
	/*!
	 * \brief Wrapper for BUFFER_SIZES_M primitives
	 *
	 * \param nodeRole The role of the node which reports buffer sizes
	 * \param bufferSizes list of buffer sizes
	 */
	bool bufferSizes(node_role_t nodeRole, buffer_sizes_t bufferSizes);
	/*!
	 * \brief Wrapper for CHANNEL_AQUISITION_TIME_M primitive
	 *
	 * \param aquisitionTime The CAT as an unsigned integer to a base of 10*-1
	 */
	bool channelAquisitionTime(uint32_t aquisitionTime);
	/*!
	 * \brief Wrapper for CMC_GROUP_SIZE_M primitive
	 *
	 * \param groupSize The CMC group size to be reported
	 *
	 * \return Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool cmcGroupSize(uint32_t groupSize);
	/*!
	 * \brief Wrapper for CPU_UTILISATION_M primitive
	 *
	 * This primitive allows to report the CPU utilisation of a node role other
	 * than IP endpoints
	 *
	 * \param nodeRole The node's role
	 * \param utilisation The CPU utilisation in % with a base of 0.01
	 *
	 * \return Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool cpuUtilisation(node_role_t nodeRole, uint16_t utilisation);
	/*!
	 * \brief Wrapper for CPU_UTILISATION_M primitive
	 *
	 * This primitive allows to report the CPU utilisation of an IP endpoint
	 *
	 * \param nodeRole The node's role
	 * \param endpointId The endpoint identifier (IP address in network byte
	 * order)
	 * \param utilisation The CPU utilisation in % with a base of 0.01
	 *
	 * \return Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool cpuUtilisation(node_role_t nodeRole, uint32_t endpointId,
			uint16_t utilisation);
	/*!
	 * \brief Wrapper for END_TO_END_LATENCY_M primitive
	 *
	 * \param nodeType The node type (UE || SV)
	 * \param endpointId The endpoint ID of the UE||SV (network byte order of
	 * its IP address)
	 * \param latency The latency to be reported
	 */
	bool endToEndLatency(node_role_t nodeType, uint32_t endpointId,
			uint16_t latency);
	/*!
	 * \brief Wrapper for HTTP_REQUESTS_FQDN_M primitive
	 *
	 * \param fqdn The FQDN
	 * \param httpRequests The number of HTTP requests traversed the node
	 *
	 * \return Boolean indicating whether or not the primitive call was
	 * successful
	 */
	bool httpRequestsFqdn(string fqdn, uint32_t httpRequests);
	/*!
	 * \brief Wrapper for the FILE_DESCRIPTORS_TYPE_M primitive
	 *
	 * \param nodeRole The role of the node which reports the number of open
	 * file descriptors
	 * \param fileDescriptorsType List of file descriptors and their types
	 */
	bool fileDescriptorsType(node_role_t nodeRole,
			file_descriptors_type_t fileDescriptorsType);
	/*!
	 * \brief Wrapper for LINK_STATE_M primitive
	 *
	 * \param linkId The link identifier
	 * \param destinationNodeId The DST NID
	 * \param sourceNodeId The SRC NID
	 * \param sourcePortId The SRC port ID
	 * \param linkType The link type
	 * \param state The new state
	 *
	 * \return  Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool linkState(uint16_t linkId, uint32_t destinationNodeId,
			uint32_t sourceNodeId, uint16_t sourcePortId, link_type_t linkType,
			link_state_t state);
	/*!
	 * \brief Wrapper for MATCHES_NAMESPACE_M primitive
	 *
	 * \param matches List of matches per namespace
	 */
	bool matchesNamespace(matches_namespace_t matches);
	/*!
	 * \brief Wrapper for NETWORK_LATENCY_FQDN_M primitive
	 *
	 * \param fqdn The FQDN for which the latency is reported
	 * \param latency The network latency to be reported
	 *
	 * \return  Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool networkLatencyFqdn(uint32_t fqdn, uint16_t latency);
	/*!
	 * \brief Wrapper for NODE_STATE_M primitive
	 *
	 * \param nodeType The node's role/type
	 * \param nodeId The NID
	 * \param state The new state
	 *
	 * \return  Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool nodeState(node_role_t nodeRole, uint32_t nodeId, node_state_t state);
	/*!
	 * \brief Wrapper for PACKET_DROP_RATE_M primitive
	 *
	 * \param nodeId The NID of where the port resides
	 * \param portId The port ID for which the packet drop rate is supposed to
	 * be reported
	 * \param rate The rate
	 *
	 * \return  Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool packetDropRate(uint32_t nodeId, uint16_t portId, uint32_t rate);
	/*!
	 * \brief Wrapper for PACKET_ERROR_RATE_M primitive
	 *
	 * \param nodeId The NID of where the port resides
	 * \param portId The port ID for which the packet error rate is supposed to
	 * be reported
	 * \param rate The rate
	 *
	 * \return  Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool packetErrorRate(uint32_t nodeId, uint16_t portId, uint32_t rate);
	/*!
	 * \brief Wrapper for PACKET_JITTER_CID_M primitive
	 *
	 * \param nodeRole The role of the node where the packet jitter was measured
	 * \param nodeId The node ID of the node where the packet jitter was
	 * measured
	 * \param packetJitters list of pairs <cid, jitter>
	 *
	 * \return Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool packetJitterCid(node_role_t nodeRole, node_identifier_t nodeId,
			packet_jitter_cid_t packetJitters);
	/*!
	 * \brief Wrapper for NUM_OF_PATH_CALCULATIONS_M primitive
	 *
	 * \param pathCalculations List of rootScope<>numOfCalcs
	 *
	 * \return  Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool pathCalculations(path_calculations_namespace_t pathCalculations);
	/*!
	 * \brief Wrapper for PORT_STATE_M primitive
	 *
	 * \param nodeId The NID
	 * \param portId The port
	 * \param state The new state
	 *
	 * \return  Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool portState(uint32_t nodeId, uint16_t portId, port_state_t state);
	/*!
	 * \brief Wrapper for PUBLISHERS_NAMESPACE_M primitive
	 *
	 * \param pubsNamespace List of publishers per namespace
	 *
	 * \return  Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool publishersNamespace(publishers_namespace_t pubsNamespace);
	/*!
	 * \brief Wrapper for RX_BYTES_HTTP_M primitive
	 *
	 * \param nodeId The node ID of the network element on which the port
	 * resides
	 * \param rxBytes The amount of RX bytes
	 *
	 * \return Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool rxBytesHttp(node_identifier_t nodeId, bytes_t rxBytes);
	/*!
	 * \brief Wrapper for RX_BYTES_IP_M primitive
	 *
	 * \param nodeId The node ID of the network element on which the port
	 * resides
	 * \param rxBytes The amount of RX bytes
	 *
	 * \return Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool rxBytesIp(node_identifier_t nodeId, bytes_t rxBytes);
	/*!
	 * \brief Wrapper for RX_BYTES_IP_MULTICAST_M primitive
	 *
	 * \param nodeId The node ID of the network element on which the port
	 * resides
	 * \param ipVersion The protocol version
	 * \param rxBytes The amount of RX bytes
	 *
	 * \return Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool rxBytesIpMulticast(node_identifier_t nodeId, ip_version_t ipVersion,
			bytes_t rxBytes);
	/*!
	 * \brief Wrapper for RECEIVED_BYTES_PORT_M primitive
	 *
	 * \param nodeId The node ID of the network element on which the port
	 * resides
	 * \param portId The port identifier for which the number of bytes are
	 * reported
	 * \param bytes The amount of RX bytes
	 *
	 * \return Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool rxBytesPort(uint32_t nodeId, uint16_t portId, uint32_t bytes);
	/*!
	 * \brief Blocking method waiting for the start trigger message from the
	 * monitoring server
	 *
	 * In order to not shoot monitoring data too soon towards the monitoring
	 * agent, the application that requires to report monitoring data must wait
	 * for the trigger.
	 */
	bool startReporting();
	/*!
	 * \brief Wrapper for SUBSCRIBERS_NAMESPACE_M primitive
	 *
	 * \param subsPerNamespace List of subscribers per namespace
	 *
	 * \return  Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool subscribersNamespace(subscribers_namespace_t subsPerNamespace);
	/*!
	 * \brief Wrapper for TX_BYTES_HTTP_M primitive
	 *
	 * \param nodeId The node ID of the network element on which the port
	 * resides
	 * \param rxBytes The amount of TX bytes
	 *
	 * \return Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool txBytesHttp(node_identifier_t nodeId, bytes_t txBytes);
	/*!
	 * \brief Wrapper for TX_BYTES_IP_M primitive
	 *
	 * \param nodeId The node ID of the network element on which the port
	 * resides
	 * \param rxBytes The amount of TX bytes
	 *
	 * \return Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool txBytesIp(node_identifier_t nodeId, bytes_t txBytes);
	/*!
	 * \brief Wrapper for TX_BYTES_IP_MULTICAST_M primitive
	 *
	 * \param nodeId The node ID of the network element on which the port
	 * resides
	 * \param ipVersion The protocol version
	 * \param rxBytes The amount of RX bytes
	 *
	 * \return Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool txBytesIpMulticast(node_identifier_t nodeId, ip_version_t ipVersion,
			bytes_t rxBytes);
	/*!
	 * \brief Wrapper for TX_BYTES_IP_MULTICAST_M primitive
	 *
	 * \param nodeId The node ID of the network element on which the port
	 * resides
	 * \param portId The port identifier for which the number of bytes are
	 * reported
	 * \param bytes The amount of RX bytes
	 *
	 * \return Boolean indicating whether or not the data point has been
	 * successfully reported
	 */
	bool txBytesPort(uint32_t nodeId, uint16_t portId, uint32_t bytes);
protected:
	int _reportingSocket;
	uint32_t _pid;
	struct sockaddr_nl _agentSocketAddress;
	boost::mutex _reportingSocketMutex;
private:
	int _bootstrapSocket;
	/*!
	 * \brief Send off the generated message
	 */
	bool _send(uint8_t messageType, void *data, uint32_t dataSize);
};

#endif /* MOLY_PROCESS_HH_ */

