/*
 * process.cc
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

#include "process.hh"

Process::Process()
{
	_bootstrapSocket = -1;
	struct sockaddr_nl src_addr;
	int ret;
	_reportingSocket = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);

	if (_reportingSocket < 0)
	{
#ifdef MOLY_DEBUG
		cout << "((MOLY)) Reporting socket towards the "
			"agent could not be created: "	<< strerror(errno) << endl;
#endif
		return;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	memset(&_agentSocketAddress, 0, sizeof(_agentSocketAddress));
	// SRC Address
	src_addr.nl_family = AF_NETLINK;
	_pid = getpid() + (rand() % 100 + 5);
	src_addr.nl_pid = _pid;
	src_addr.nl_groups = 0;  /* not in mcast groups */
	src_addr.nl_pad = 0;
	ret = bind(_reportingSocket, (struct sockaddr*) &src_addr, sizeof(src_addr));

	if (ret < 0)
	{
#ifdef MOLY_DEBUG
		cout << "((MOLY)) Could not bind to reporting "
				"socket using PID " << _pid	<< ": " << strerror(errno)
				<< "\n Re-try every second" << endl;
#endif

		while (ret < 0)
		{
			sleep(1);
			_pid = getpid() + (rand() % 100 + 5);
			src_addr.nl_pid = _pid;
			ret = bind(_reportingSocket, (struct sockaddr*) &src_addr,
					sizeof(src_addr));
		}
	}

#ifdef MOLY_DEBUG
	cout << "((MOLY)) Bound to SRC PID " << _pid << endl;
#endif
	//DST netlink socket
	_agentSocketAddress.nl_family = AF_NETLINK;
	_agentSocketAddress.nl_pid = PID_MOLY_LISTENER;
	_agentSocketAddress.nl_groups = 0;
	_agentSocketAddress.nl_pad = 0;
}

Process::~Process()
{
	if (_bootstrapSocket != -1)
	{
#ifdef MOLY_DEBUG
		cout << "((MOLY)) Closing bootstrapping socket (FD = "
				<< _bootstrapSocket << ")\n";
#endif
		close(_bootstrapSocket);
	}

	if (_reportingSocket != -1)
	{
#ifdef MOLY_DEBUG
		cout << "((MOLY)) Closing sending socket (FD = "
				<< _reportingSocket << ")\n";
#endif
		close(_reportingSocket);
	}
}

bool Process::addLink(uint16_t linkId, uint32_t destinationNodeId,
		uint32_t sourceNodeId, uint16_t sourcePortId, link_type_t linkType,
		string name)
{
	AddLink addLink(linkId, destinationNodeId, sourceNodeId, sourcePortId,
			linkType, name);
	return _send(PRIMITIVE_TYPE_ADD_LINK, addLink.pointer(), addLink.size());
}

bool Process::addNode(string name, uint32_t nodeId, node_role_t nodeRole)
{
	AddNode addNode(name, nodeId, nodeRole);
	return _send(PRIMITIVE_TYPE_ADD_NODE, addNode.pointer(), addNode.size());
}

bool Process::addPort(uint32_t nodeId, uint16_t portId, string portName)
{
	AddPort addPort(nodeId, portId, portName);
	return _send(PRIMITIVE_TYPE_ADD_PORT, addPort.pointer(), addPort.size());
}

bool Process::bufferSizes(node_role_t nodeRole, buffer_sizes_t bufferSizes)
{
	BufferSizes bufferSizesM(nodeRole, bufferSizes);
	return _send(PRIMITIVE_TYPE_BUFFER_SIZES, bufferSizesM.pointer(),
			bufferSizesM.size());
}

bool Process::channelAquisitionTime(uint32_t aquisitionTime)
{
	ChannelAquisitionTime channelAquisitionTime(aquisitionTime);
	return _send(PRIMITIVE_TYPE_CHANNEL_AQUISITION_TIME,
			channelAquisitionTime.pointer(), channelAquisitionTime.size());
}

bool Process::cmcGroupSize(uint32_t groupSize)
{
	CmcGroupSize cmcGroupSize(groupSize);
	return _send(PRIMITIVE_TYPE_CMC_GROUP_SIZE, cmcGroupSize.pointer(),
			cmcGroupSize.size());
}

bool Process::cpuUtilisation(node_role_t nodeRole, uint16_t utilisation)
{
	CpuUtilisation cpuUtilisation(nodeRole, utilisation);
	return _send(PRIMITIVE_TYPE_CPU_UTILISATION, cpuUtilisation.pointer(),
			cpuUtilisation.size());
}

bool Process::cpuUtilisation(node_role_t nodeRole, uint32_t endpointId,
		uint16_t utilisation)
{
	CpuUtilisation cpuUtilisation(nodeRole, endpointId, utilisation);
	return _send(PRIMITIVE_TYPE_CPU_UTILISATION, cpuUtilisation.pointer(),
			cpuUtilisation.size());
}

bool Process::endToEndLatency(node_role_t nodeType, uint32_t endpointId,
			uint16_t latency)
{
	E2eLatency e2eLatency(nodeType, endpointId, latency);
	return _send(PRIMITIVE_TYPE_END_TO_END_LATENCY, e2eLatency.pointer(),
			e2eLatency.size());
}

bool Process::fileDescriptorsType(node_role_t nodeRole,
		file_descriptors_type_t fileDescriptorsType)
{
	FileDescriptorsType fileDescriptorsTypeM(nodeRole, fileDescriptorsType);
	return _send(PRIMITIVE_TYPE_FILE_DESCRIPTORS_TYPE,
			fileDescriptorsTypeM.pointer(), fileDescriptorsTypeM.size());
}

bool Process::httpRequestsFqdn(string fqdn, uint32_t httpRequests)
{
	HttpRequestsFqdn httpRequestsFqdn(fqdn, httpRequests);
	return _send(PRIMITIVE_TYPE_HTTP_REQUESTS_FQDN,
			httpRequestsFqdn.pointer(), httpRequestsFqdn.size());
}

bool Process::linkState(uint16_t linkId, uint32_t destinationNodeId,
		uint32_t sourceNodeId, uint16_t sourcePortId, link_type_t linkType,
		link_state_t state)
{
	LinkState linkState(linkId, destinationNodeId, sourceNodeId, sourcePortId,
			linkType, state);
	return _send(PRIMITIVE_TYPE_LINK_STATE, linkState.pointer(),
			linkState.size());
}

bool Process::matchesNamespace(matches_namespace_t matches)
{
	MatchesNamespace matchesNamespace(matches);
	return _send(PRIMITIVE_TYPE_MATCHES_NAMESPACE,
			matchesNamespace.pointer(), matchesNamespace.size());
}

bool Process::networkLatencyFqdn(uint32_t fqdn, uint16_t latency)
{
	NetworkLatencyFqdn networkLatencyFqdn(fqdn, latency);
	return _send(PRIMITIVE_TYPE_NETWORK_LATENCY_FQDN,
			networkLatencyFqdn.pointer(), networkLatencyFqdn.size());
}

bool Process::nodeState(node_role_t nodeRole, uint32_t nodeId,
		node_state_t state)
{
	NodeState nodeState(nodeRole, nodeId, state);
	return _send(PRIMITIVE_TYPE_NODE_STATE, nodeState.pointer(),
				nodeState.size());
}

bool Process::publishersNamespace(publishers_namespace_t pubsNamespace)
{
	PublishersNamespace publishersNamespace(pubsNamespace);
	return _send(PRIMITIVE_TYPE_PUBLISHERS_NAMESPACE,
			publishersNamespace.pointer(), publishersNamespace.size());
}

bool Process::packetDropRate(uint32_t nodeId, uint16_t portId, uint32_t rate)
{
	PacketErrorRate packetErrorRate(nodeId, portId, rate);
	return _send(PRIMITIVE_TYPE_PACKET_DROP_RATE, packetErrorRate.pointer(),
			packetErrorRate.size());
}

bool Process::packetErrorRate(uint32_t nodeId, uint16_t portId, uint32_t rate)
{
	PacketErrorRate packetErrorRate(nodeId, portId, rate);
	return _send(PRIMITIVE_TYPE_PACKET_ERROR_RATE, packetErrorRate.pointer(),
			packetErrorRate.size());
}

bool Process::packetJitterCid(node_role_t nodeRole, node_identifier_t nodeId,
		packet_jitter_cid_t packetJitters)
{
	PacketJitterCid packetJitterCid(nodeRole, nodeId, packetJitters);
	return _send(PRIMITIVE_TYPE_PACKET_JITTER_CID,
			packetJitterCid.pointer(), packetJitterCid.size());
}

bool Process::pathCalculations(path_calculations_namespace_t calculations)
{
	PathCalculationsNamespace pathCalculations(calculations);
	return _send(PRIMITIVE_TYPE_PATH_CALCULATIONS_NAMESPACE,
			pathCalculations.pointer(), pathCalculations.size());
}

bool Process::portState(uint32_t nodeId, uint16_t portId, port_state_t state)
{
	PortState portState(nodeId, portId, state);
	return _send(PRIMITIVE_TYPE_PORT_STATE, portState.pointer(),
			portState.size());
}

bool Process::rxBytesHttp(node_identifier_t nodeId,	bytes_t rxBytes)
{
	RxBytesHttp rxBytesHttp(nodeId, rxBytes);
	return _send(PRIMITIVE_TYPE_RX_BYTES_HTTP, rxBytesHttp.pointer(),
			rxBytesHttp.size());
}

bool Process::rxBytesIp(node_identifier_t nodeId, bytes_t rxBytes)
{
	RxBytesIp rxBytesIp(nodeId, rxBytes);
	return _send(PRIMITIVE_TYPE_RX_BYTES_IP, rxBytesIp.pointer(),
			rxBytesIp.size());
}

bool Process::rxBytesIpMulticast(node_identifier_t nodeId,
		ip_version_t ipVersion,	bytes_t rxBytes)
{
	RxBytesIpMulticast rxBytesIpMulticast(nodeId, ipVersion, rxBytes);
	return _send(PRIMITIVE_TYPE_RX_BYTES_IP_MULTICAST,
			rxBytesIpMulticast.pointer(), rxBytesIpMulticast.size());
}

bool Process::rxBytesPort(uint32_t nodeId, uint16_t portId, uint32_t bytes)
{
	RxBytesPort rxBytesPort(nodeId, portId, bytes);
	return _send(PRIMITIVE_TYPE_RX_BYTES_PORT, rxBytesPort.pointer(),
			rxBytesPort.size());
}

bool Process::startReporting()
{
	int bytesRead;
	struct sockaddr_nl srcAddr, agentAddress;
	struct nlmsghdr *nlh = NULL;
	struct iovec iov;
	struct msghdr msg;
	int ret;
	_bootstrapSocket = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);

	if (_bootstrapSocket < 0)
	{
#ifdef MOLY_DEBUG
		cout << "((MOLY)) Bootstrap socket could not be "
				"created\n";
#endif
		return false;
	}

#ifdef MOLY_DEBUG
	cout << "((MOLY)) Bootstrap socket (FD = "
			<< _bootstrapSocket << ") towards monitoring agent created\n";
#endif
	memset(&msg, 0, sizeof(msg));
	memset(&srcAddr, 0, sizeof(srcAddr));
	memset(&agentAddress, 0, sizeof(agentAddress));
	srcAddr.nl_family = AF_NETLINK;
	srcAddr.nl_pid = getpid() + (rand() % 100 + 5);
	srcAddr.nl_pad = 0;
	srcAddr.nl_groups = 0;
	ret = bind(_bootstrapSocket, (struct sockaddr*) &srcAddr, sizeof(srcAddr));

	if (ret == -1)
	{
#ifdef MOLY_DEBUG
		cout << "((MOLY)) Could not bind to bootstrap "
				"socket using PID "<< srcAddr.nl_pid << " Trying again with "
						"different PID" << endl;
#endif
		srcAddr.nl_pid = getpid() + (rand() % 100 + 5);

		while (true)
		{
			ret = bind(_bootstrapSocket, (struct sockaddr*) &srcAddr,
					sizeof(srcAddr));

			if (ret != -1)
			{
				break;
			}
		}
	}

#ifdef MOLY_DEBUG
	cout << "((MOLY)) Successfully bound to trigger socket " << endl;
#endif
	//DST netlink socket
	agentAddress.nl_family = AF_NETLINK;
	agentAddress.nl_pid = PID_MOLY_BOOTSTRAP_LISTENER;
	agentAddress.nl_groups = 0;
	agentAddress.nl_pad = 0;
	/* Allocate the required memory (NLH + payload) */
	nlh=(struct nlmsghdr *) malloc(NLMSG_SPACE(sizeof(srcAddr.nl_pid)));
	/* Fill the netlink message header */
	nlh->nlmsg_len = NLMSG_SPACE(sizeof(srcAddr.nl_pid));
	nlh->nlmsg_pid = srcAddr.nl_pid;
	nlh->nlmsg_flags = NLM_F_REQUEST;
	nlh->nlmsg_type = BOOTSTRAP_MY_PID;
	nlh->nlmsg_seq = rand();
	memcpy(NLMSG_DATA(nlh), &srcAddr.nl_pid, sizeof(srcAddr.nl_pid));
	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	// Creating message
	msg.msg_name = (void *)&agentAddress;
	msg.msg_namelen = sizeof(agentAddress);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
#ifdef MOLY_DEBUG
	cout << "((MOLY)) Connecting to agent on PID "
			<< (int)PID_MOLY_BOOTSTRAP_LISTENER	<< " to send my PID "
			<< nlh->nlmsg_pid << endl;
#endif
	ret = sendmsg(_bootstrapSocket, &msg, 0);

	if (ret == -1)
	{
#ifdef MOLY_DEBUG
		cout << "((MOLY)) Monitoring agent is not "
				"reachable yet. Trying again every second\n";
#endif

		while (true)
		{
			ret = sendmsg(_bootstrapSocket, &msg, 0);

			if (ret != -1)
			{
				break;
			}

			sleep(1);
		}
	}

	free(nlh);
#ifdef MOLY_DEBUG
	cout << "((MOLY)) Connected to monitoring agent. PID "
			<< srcAddr.nl_pid << " sent\n";
#endif
	// Now waiting and reading responses
	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_MESSAGE_PAYLOAD));
	memset(nlh, 0, NLMSG_SPACE(MAX_MESSAGE_PAYLOAD));
	iov.iov_base = (void*)nlh;
	iov.iov_len = NLMSG_SPACE(MAX_MESSAGE_PAYLOAD);

	// Now waiting for intial OK
	while (true)
	{
		bytesRead = recvmsg(_bootstrapSocket, &msg, 0);

		if (bytesRead <= 0)
		{
#ifdef MOLY_DEBUG
			cout << "((MOLY)) Trigger socket crashed "
					<< strerror(errno) << endl;
#endif
			close(_bootstrapSocket);
			free(nlh);
			return false;
		}

		switch (nlh->nlmsg_type)
		{
		case BOOTSTRAP_OK:
#ifdef MOLY_DEBUG
			cout << "((MOLY)) Monitoring agent confirmed "
					"connection. Waiting for start reporting trigger from "
					"monitoring agent on PID " << nlh->nlmsg_pid << endl;
#endif
			break;
		case BOOTSTRAP_START_REPORTING:
#ifdef MOLY_DEBUG
			cout << "((MOLY)) Start reporting trigger received\n";
#endif
			close(_bootstrapSocket);
			free(nlh);
			return true;
		case BOOTSTRAP_ERROR:
#ifdef MOLY_DEBUG
			cout << "((MOLY)) Connection to agent established but boostrapping "
					"failed\n";
#endif
			close(_bootstrapSocket);
			free(nlh);
			return false;
		}
	}

	close(_bootstrapSocket);
	free(nlh);
	return false;
}

bool Process::subscribersNamespace(subscribers_namespace_t subsNamespace)
{
	SubscribersNamespace subscribersNamespace(subsNamespace);
	return _send(PRIMITIVE_TYPE_SUBSCRIBERS_NAMESPACE,
			subscribersNamespace.pointer(), subscribersNamespace.size());
}

bool Process::txBytesHttp(node_identifier_t nodeId,	bytes_t txBytes)
{
	TxBytesHttp txBytesHttp(nodeId, txBytes);
	return _send(PRIMITIVE_TYPE_TX_BYTES_HTTP, txBytesHttp.pointer(),
			txBytesHttp.size());
}

bool Process::txBytesIp(node_identifier_t nodeId, bytes_t txBytes)
{
	TxBytesIp txBytesIp(nodeId, txBytes);
	return _send(PRIMITIVE_TYPE_TX_BYTES_IP, txBytesIp.pointer(),
			txBytesIp.size());
}

bool Process::txBytesIpMulticast(node_identifier_t nodeId,
		ip_version_t ipVersion, bytes_t rxBytes)
{
	TxBytesIpMulticast txBytesIpMulticast(nodeId, ipVersion, rxBytes);
	return _send(PRIMITIVE_TYPE_TX_BYTES_IP_MULTICAST,
			txBytesIpMulticast.pointer(), txBytesIpMulticast.size());
}

bool Process::txBytesPort(uint32_t nodeId, uint16_t portId, uint32_t bytes)
{
	TxBytesPort txBytesPort(nodeId, portId, bytes);
	return _send(PRIMITIVE_TYPE_TX_BYTES_PORT, txBytesPort.pointer(),
			txBytesPort.size());
}

bool Process::_send(uint8_t messageType, void *data, uint32_t dataSize)
{
	struct msghdr msg;
	int ret;
	struct iovec iov;
	struct nlmsghdr *nlh = NULL;
	memset(&msg, 0, sizeof(msg));
	/* Allocate the required memory (NLH + payload) */
	nlh=(struct nlmsghdr *) malloc(NLMSG_SPACE(dataSize));
	bzero(nlh, NLMSG_SPACE(dataSize));
	/* Fill the netlink message header */
	nlh->nlmsg_len = NLMSG_SPACE(dataSize);
	nlh->nlmsg_pid = _pid;
	nlh->nlmsg_flags = NLM_F_REQUEST;
	nlh->nlmsg_type = messageType;
	nlh->nlmsg_seq = rand();
	memcpy(NLMSG_DATA(nlh), data, dataSize);
	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	// Creating message and send it off
	msg.msg_name = (void *)&_agentSocketAddress;
	msg.msg_namelen = sizeof(_agentSocketAddress);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	_reportingSocketMutex.lock();
	ret = sendmsg(_reportingSocket, &msg, 0);

	if (ret < 0)
	{
		_reportingSocketMutex.unlock();
#ifdef MOLY_DEBUG
		cout << "((MOLY)) Could not send off data to the monitoring agent: "
				<< strerror(errno) << endl;
#endif
		free(nlh);
		return false;
	}

	_reportingSocketMutex.unlock();
#ifdef MOLY_DEBUG
	cout << "((MOLY)) Message of length " << nlh->nlmsg_len << " (Payload "
			<< dataSize << ") bytes sent to MONA" << endl;
#endif
	free(nlh);
	return true;
}
