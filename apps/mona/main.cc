/**
 * agent.cc
 *
 * Created on: Nov 25, 2015
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the ICN application MOnitOring Agent (MONA) which
 * comes with Blackadder.
 *
 * MONA is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MONA is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MONA. If not, see <http://www.gnu.org/licenses/>.
 */
#include <blackadder.hpp>
#include <bampers/bampers.hh>
#include <boost/thread.hpp>
#include <mapiagent.hpp>
#include <moly/moly.hh>
#include <signal.h>

Blackadder *ba;
uint8_t *data = (uint8_t *)malloc(65535);

void sigfun(int sig) {
	(void) signal(sig, SIG_DFL);
	cout << "((MONA)) " << "Disconnecting from Blackadder\n";
	ba->disconnect();
	free(data);
	delete ba;
	exit(0);
}
#ifdef MONITORING_TRIGGER_BUGFIX
int main (int argc, char* argv[])
#else
int main ()
#endif
{
	Moly moly;
	MapiAgent mapiAgent;
	uint32_t nodeId;

#ifdef MONITORING_TRIGGER_BUGFIX
	int sleepTime = 5;
#endif
	mapiAgent.set_MapiInstance(true);

	if (getuid() != 0)
	{
		cout << "((MONA)) " << "This application must run with root (sudo) privileges\n";
		return EXIT_FAILURE;
	}

	if (!mapiAgent.get_nodeid(nodeId))
	{
		cout << "((MONA)) " << "Obtaining NID via MAPI failed\n";
		return EXIT_FAILURE;
	}
#ifdef MONA_DEBUG
	cout << "((MONA)) " << "Node ID obtained: " << nodeId << endl;
#endif
#ifdef MONITORING_TRIGGER_BUGFIX
	if (argc > 1)
	{
		sleepTime = atoi(argv[1]);
	}

	cout << "((MONA)) " << "Sleep for " << sleepTime << " seconds before telling all "
			"applications to start reporting\n";
	sleep(sleepTime);
#endif

	if (!moly.Agent::initialiseListener())
	{
		cout << "((MONA)) " << "Initialising MOLY failed\n";
		return EXIT_FAILURE;
	}

#ifdef MONA_DEBUG
	cout << "((MONA)) " << "MOLY initialised\n";
#endif
	ba = Blackadder::Instance(true);
	(void) signal(SIGINT, sigfun);
	cout << "((MONA)) " << "Blackadder instantiated\n";
	uint8_t messageType;
	uint32_t dataLength;
	// Initialise namespace for BAMPERS
	Namespace namespaceHelper(ba, nodeId);
	// Initialise agent with the NodeID
	Bampers bampers(namespaceHelper);
	// Start listening for messages
	cout << "((MONA)) " << "Start listening for monitoring data from processes\n";

	for ( ; ; )
	{
		if (moly.Agent::receive(messageType, data, dataLength))
		{
			switch (messageType)
			{
			case PRIMITIVE_TYPE_ADD_LINK:
			{
				AddLink addLink(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << "ADD_LINK received: "
						<< addLink.print() << endl;
#endif
				if (addLink.sourceNodeId() == 0 ||
						addLink.destinationNodeId() == 0)
				{
#ifdef MONA_DEBUG
					cout << "((MONA)) " << "Node ID must not be 0. Discarding message: ";
					cout << "((MONA)) " << addLink.print() << endl;
#endif
					break;
				}

				bampers.Link::add(addLink.linkId(), addLink.destinationNodeId(),
						addLink.sourceNodeId(), addLink.sourcePortId(),
						addLink.linkType(), addLink.name());
				break;
			}
			case PRIMITIVE_TYPE_ADD_NODE:
			{
				AddNode addNode(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) ADD_NODE_M received: " << addNode.print()
						<< endl;
#endif
				bampers.Node::add(addNode.nodeRole(), addNode.name(),
						addNode.nodeId());
				break;
			}
			case PRIMITIVE_TYPE_ADD_PORT:
			{
				AddPort addPort(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) ADD_PORT_M received: " << addPort.print()
						<< endl;
#endif
				bampers.Port::add(addPort.nodeId(), addPort.portId(),
						addPort.portName());
				break;
			}
			case PRIMITIVE_TYPE_BUFFER_SIZES:
			{
				BufferSizes bufferSizes(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << bufferSizes.primitiveName()
						<< " received: " << bufferSizes.print() << endl;
#endif
				bampers.Statistics::bufferSizes(bufferSizes.nodeRole(),
						bufferSizes.bufferSizes());
				break;
			}
			case PRIMITIVE_TYPE_CHANNEL_AQUISITION_TIME:
			{
				ChannelAquisitionTime aquisitionTime(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) CHANNEL_AQUISITION_TIME_M received: "
						<< aquisitionTime.print() << endl;
#endif
				bampers.Statistics::channelAquisitionTime(
						aquisitionTime.channelAquisitionTime());
				break;
			}
			case PRIMITIVE_TYPE_CMC_GROUP_SIZE:
			{
				CmcGroupSize cmcGroupSize(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) CMC_GROUP_SIZE_M received: "
						<< cmcGroupSize.print() << endl;
#endif
				bampers.Statistics::cmcGroupSize(cmcGroupSize.groupSize());
				break;
			}
			case PRIMITIVE_TYPE_CPU_UTILISATION:
			{
				CpuUtilisation cpuUtilisation(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) PRIMITIVE_TYPE_CPU_UTILISATION_M received: "
						<< cpuUtilisation.print() << endl;
#endif
				bampers.Statistics::cpuUtilisation(cpuUtilisation.nodeRole(),
						cpuUtilisation.endpointId(),
						cpuUtilisation.cpuUtilisation());
				break;
			}
			case PRIMITIVE_TYPE_END_TO_END_LATENCY:
			{
				E2eLatency e2eLatency(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) PRIMITIVE_TYPE_END_TO_END_LATENCY_M received: "
						<< e2eLatency.print() << endl;
#endif
				bampers.Statistics::e2eLatency(e2eLatency.nodeRole(),
						e2eLatency.endpointId(), e2eLatency.latency());
				break;
			}
			case PRIMITIVE_TYPE_FILE_DESCRIPTORS_TYPE:
			{
				FileDescriptorsType fileDescriptorsTypeM(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << fileDescriptorsTypeM.primitiveName()
						<< " received: " << fileDescriptorsTypeM.print() << endl;
#endif
				bampers.Statistics::fileDescriptorsType(
						fileDescriptorsTypeM.nodeRole(),
						fileDescriptorsTypeM.fileDescriptorsType());
				break;
			}
			case PRIMITIVE_TYPE_HTTP_REQUESTS_FQDN:
			{
				HttpRequestsFqdn httpRequestsFqdn(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) PRIMITIVE_TYPE_HTTP_REQUESTS_FQDN_M received: "
						<< httpRequestsFqdn.print() << endl;
#endif
				bampers.Statistics::httpRequestsFqdn(
						httpRequestsFqdn.fqdn(),
						httpRequestsFqdn.httpRequests());
				break;
			}
			case PRIMITIVE_TYPE_LINK_STATE:
			{
				LinkState linkState(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << "LINK_STATE received: "
						<< linkState.print() << endl;
#endif
				bampers.Link::state(linkState.linkId(),
						linkState.destinationNodeId(), linkState.sourceNodeId(),
						linkState.sourcePortId(), linkState.linkType(),
						linkState.state());
				break;
			}
			case PRIMITIVE_TYPE_MATCHES_NAMESPACE:
			{
				MatchesNamespace matchesNamespace(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << matchesNamespace.primitiveName()
						<< matchesNamespace.print() << endl;
#endif
				bampers.Statistics::matchesNamespace(
						matchesNamespace.matchesNamespace());
				break;
			}
			case PRIMITIVE_TYPE_NETWORK_LATENCY_FQDN:
			{
				NetworkLatencyFqdn networkLatencyFqdn(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << "NETWORK_LATENCY_FQDN_M received: "
						<< networkLatencyFqdn.print() << endl;
#endif
				bampers.Statistics::networkLatencyFqdn(
						networkLatencyFqdn.fqdn(),
						networkLatencyFqdn.latency());
				break;
			}
			case PRIMITIVE_TYPE_NODE_STATE:
			{
				NodeState nodeState(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) NODE_STATE_M received: "
						<< nodeState.print() << endl;
#endif
				bampers.Statistics::stateNetworkElement(nodeState.nodeRole(),
						nodeState.nodeId(), nodeState.state());
				break;
			}
			case PRIMITIVE_TYPE_PACKET_DROP_RATE:
			{
				PacketDropRate packetDropRate(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << "PACKET_DROP_RATE_M received: "
						<< packetDropRate.print() << endl;
#endif
				bampers.Port::packetDropRate(packetDropRate.nodeId(),
						packetDropRate.portId(), packetDropRate.rate());
				break;
			}
			case PRIMITIVE_TYPE_PACKET_ERROR_RATE:
			{
				PacketErrorRate packetErrorRate(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << "PACKET_ERROR_RATE_M received: "
						<< packetErrorRate.print() << endl;
#endif
				bampers.Port::packetErrorRate(packetErrorRate.nodeId(),
						packetErrorRate.portId(), packetErrorRate.rate());
				break;
			}
			case PRIMITIVE_TYPE_PACKET_JITTER_CID:
			{
				PacketJitterCid packetJitterCid(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << packetJitterCid.primitiveName()
						<< " received: " << packetJitterCid.print() << endl;
#endif
				bampers.Statistics::packetJitterCid(
						packetJitterCid.nodeRole(),
						packetJitterCid.nodeIdentifier(),
						packetJitterCid.packetJitterCid());
				break;
			}
			case PRIMITIVE_TYPE_PATH_CALCULATIONS_NAMESPACE:
			{
				PathCalculationsNamespace pathCalculations(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << pathCalculations.primitiveName()
						<< " received: " << pathCalculations.print() << endl;
#endif
				bampers.Statistics::pathCalculationsNamespace(
						pathCalculations.pathCalculationsNamespace());
				break;
			}
			case PRIMITIVE_TYPE_PORT_STATE:
			{
				PortState portState(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << portState.primitiveName()
						<< " received: "
						<< portState.print() << endl;
#endif
				bampers.Port::state(portState.nodeId(), portState.portId(),
						portState.state());
				break;
			}
			case PRIMITIVE_TYPE_PUBLISHERS_NAMESPACE:
			{
				PublishersNamespace publishersNamespace(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) PUBLISHERS_NAMESPACE_M received: "
						<< publishersNamespace.print() << endl;
#endif
				bampers.Statistics::publishersNamespace(
						publishersNamespace.publishersNamespace());
				break;
			}
			case PRIMITIVE_TYPE_RX_BYTES_HTTP:
			{
				RxBytesHttp rxBytesHttp(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << rxBytesHttp.primitiveName()
						<< rxBytesHttp.print() << endl;
#endif
				bampers.Statistics::rxBytesHttp(rxBytesHttp.nodeId(),
						rxBytesHttp.rxBytes());
				break;
			}
			case PRIMITIVE_TYPE_RX_BYTES_IP:
			{
				RxBytesIp rxBytesIp(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << rxBytesIp.primitiveName()
						<< " received: " << rxBytesIp.print() << endl;
#endif
				bampers.Statistics::rxBytesIp(rxBytesIp.nodeId(),
						rxBytesIp.rxBytes());
				break;
			}
			case PRIMITIVE_TYPE_RX_BYTES_IP_MULTICAST:
			{
				RxBytesIpMulticast rxBytesIpMulticast(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << rxBytesIpMulticast.primitiveName()
						<< " received: " << rxBytesIpMulticast.print() << endl;
#endif
				bampers.Statistics::rxBytesIpMulticast(
						rxBytesIpMulticast.nodeId(),
						rxBytesIpMulticast.ipVersion(),
						rxBytesIpMulticast.rxBytes());
				break;
			}
			case PRIMITIVE_TYPE_RX_BYTES_PORT:
			{
				RxBytesPort rxBytesPort(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << "RX_BYTES_PORT_M received: "
						<< rxBytesPort.print() << endl;
#endif
				bampers.Statistics::rxBytesPort(rxBytesPort.nodeId(),
						rxBytesPort.portId(), rxBytesPort.bytes());
				break;
			}
			case PRIMITIVE_TYPE_SUBSCRIBERS_NAMESPACE:
			{
				SubscribersNamespace subscribersNamespace(data,	dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << "NUMBER_OF_SUBSCRIBERS_NAMESPACE_M "
						<< "received: " << subscribersNamespace.print() << endl;
#endif
				bampers.subscribersNamespace(
						subscribersNamespace.subcribersNamespace());
				break;
			}
			case PRIMITIVE_TYPE_TX_BYTES_HTTP:
			{
				TxBytesHttp txBytesHttp(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << txBytesHttp.primitiveName()
						<< " received: " << txBytesHttp.print() << endl;
#endif
				bampers.Statistics::txBytesHttp(txBytesHttp.nodeId(),
						txBytesHttp.txBytes());
				break;
			}
			case PRIMITIVE_TYPE_TX_BYTES_IP:
			{
				TxBytesIp txBytesIp(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << txBytesIp.primitiveName()
						<< " received: " << txBytesIp.print() << endl;
#endif
				bampers.Statistics::txBytesIp(txBytesIp.nodeId(),
						txBytesIp.txBytes());
				break;
			}
			case PRIMITIVE_TYPE_TX_BYTES_IP_MULTICAST:
			{
				TxBytesIpMulticast txBytesIpMulticast(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << txBytesIpMulticast.primitiveName()
						<< " received: " << txBytesIpMulticast.print() << endl;
#endif
				bampers.Statistics::txBytesIpMulticast(
						txBytesIpMulticast.nodeId(),
						txBytesIpMulticast.ipVersion(),
						txBytesIpMulticast.txBytes());
				break;
			}
			case PRIMITIVE_TYPE_TX_BYTES_PORT:
			{
				TxBytesPort txBytesPort(data, dataLength);
#ifdef MONA_DEBUG
				cout << "((MONA)) " << "TX_BYTES_PORT_M received: "
						<< txBytesPort.print() << endl;
#endif
				bampers.Statistics::txBytesPort(txBytesPort.nodeId(),
						txBytesPort.portId(), txBytesPort.bytes());
				break;
			}
			default:
				cout << "((MONA)) " << "Unknown message type: '"
				<< (int)messageType << "'\n";
			}
		}
	}

	free(data);
	delete ba;
	return 0;
}
