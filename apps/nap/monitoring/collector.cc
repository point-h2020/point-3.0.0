/*
 * collector.cc
 *
 *  Created on: 3 Feb 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of Blackadder.
 *
 * Blackadder is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Blackadder is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Blackadder.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <moly/moly.hh>

#include "collector.hh"

using namespace monitoring::collector;

LoggerPtr Collector::logger(Logger::getLogger("monitoring.collector"));

Collector::Collector(Configuration &configuration, Statistics &statistics,
		bool *run)
	: _configuration(configuration),
	  _statistics(statistics),
	  _run(run)
{}

Collector::~Collector() {}

void Collector::operator ()()
{
	Moly moly;
	http_requests_per_fqdn_t httpRequestsPerFqdn;
	float averageLatency = 0; // values are all with a base of 0.1
	int averageLatencySum;
	int averageLatencyCounter;
	LOG4CXX_INFO(logger, "Statistics collector thread started");

	if (!moly.Process::startReporting())
	{
		LOG4CXX_ERROR(logger, "Bootstrapping process with monitoring agent "
				"failed");
		return;
	}

	LOG4CXX_DEBUG(logger, "'Start reporting' trigger from MONA received");
	string nodeName;
	node_role_t nodeRole;

	// FIXME get IP GW, surrogate and Server node types properly implemented
	if (_configuration.icnGatewayIp() || _configuration.icnGatewayHttp())
	{
		nodeRole = NODE_ROLE_GW;
		nodeName = "ICN GW";
	}
	else
	{
		nodeRole = NODE_ROLE_NAP;
		nodeName = "NAP ";
		nodeName.append(_configuration.hostRoutingPrefix().str());
	}

	// addNode
	if (moly.Process::addNode(nodeName, _configuration.nodeId().uint(),
			nodeRole))
	{
		LOG4CXX_TRACE(logger, "moly.ADD_NODE(" << nodeName << ", "
				<< _configuration.nodeId().uint() << ", " << nodeRole
				<< ") sent");
	}
	else
	{
		LOG4CXX_WARN(logger, "Sending moly.ADD_NODE to monitoring agent "
				"failed");
	}

	while (*_run)
	{
		sleep(_configuration.molyInterval());

		if (!moly.Process::cmcGroupSize(_statistics.averageCmcGroupSize()))
		{
			LOG4CXX_WARN(logger, "Average CMC group could not be reported")
		}

		// Number of HTTP requests per FQDN
		httpRequestsPerFqdn = _statistics.httpRequestsPerFqdn();
		http_requests_per_fqdn_t::iterator httpRequestsPerFqdnIt;

		if (httpRequestsPerFqdn.size() > 0)
		{
			for (httpRequestsPerFqdnIt = httpRequestsPerFqdn.begin();
					httpRequestsPerFqdnIt != httpRequestsPerFqdn.end();
					httpRequestsPerFqdnIt++)
			{
				if(!moly.Process::httpRequestsFqdn(
						httpRequestsPerFqdnIt->first,
						httpRequestsPerFqdnIt->second))
				{
					LOG4CXX_WARN(logger, "Statistics for FQDN "
							<< httpRequestsPerFqdnIt->first << " could not be"
							" sent");
				}
			}
		}

		// latency in the network
		averageLatency = 0;
		averageLatencySum = 0;
		averageLatencyCounter = 0;
		unordered_map <uint32_t, uint16_t> latencyPerFqdn;
		unordered_map <uint32_t, uint16_t>::iterator latencyPerFqdnIt;
		latencyPerFqdn = _statistics.averageNetworkDelayPerFqdn();

		for (latencyPerFqdnIt = latencyPerFqdn.begin();
				latencyPerFqdnIt != latencyPerFqdn.end(); latencyPerFqdnIt++)
		{
			if (latencyPerFqdnIt->second != 0)
			{
				if (!moly.networkLatencyFqdn(latencyPerFqdnIt->first,
						latencyPerFqdnIt->second))
				{
					LOG4CXX_WARN(logger, "Reporting network latency of "
							<< latencyPerFqdnIt->second << "ms for hashed FQDN "
							<< latencyPerFqdnIt->first << " failed");
				}

				LOG4CXX_TRACE(logger, "Reporting network latency of "
						<< latencyPerFqdnIt->second << "ms for hashed FQDN "
						<< latencyPerFqdnIt->first);
				averageLatencySum = averageLatencySum +
						(int)latencyPerFqdnIt->second;
				averageLatencyCounter = averageLatencyCounter + 1;
			}
		}

		// calculate average latency
		if (averageLatencyCounter != 0)
		{
			averageLatency = averageLatencySum / averageLatencyCounter;
		}

		// report newly added IP endpoints
		ip_endpoints_t ipEndpoints = _statistics.ipEndpoints();
		if (!ipEndpoints.empty() && !_configuration.icnGatewayIp())
		{
			ip_endpoints_t::iterator ipEndpointsIt;
			ipEndpointsIt = ipEndpoints.begin();

			while (ipEndpointsIt != ipEndpoints.end())
			{
				if (!ipEndpointsIt->second.reported)
				{
					ostringstream nodeName;

					switch (ipEndpointsIt->second.nodeType)
					{
					case NODE_ROLE_UE:
					case NODE_ROLE_SERVER:
						nodeName << ipEndpointsIt->second.ipAddress.str();
						break;
					default:
						LOG4CXX_WARN(logger, "Unknown node type for IP endpoint"
								<< " "<< ipEndpointsIt->second.ipAddress.str());
					}

					// add node and activate the node
					if (!moly.addNode(nodeName.str(),
							_configuration.nodeId().uint(),
							ipEndpointsIt->second.nodeType))
					{
						LOG4CXX_WARN(logger, "IP endpoint reporting failed for"
								<< " " << nodeName.str());
					}
					else
					{
						LOG4CXX_TRACE(logger, "IP endpoint '" << nodeName.str()
								<< "' reported to MA");
						ipEndpointsIt->second.reported = true;
					}
				}

				//TODO HACK! report average E2E latency for this UE/SV
				if (averageLatency != 0)
				{
					if (!moly.endToEndLatency(ipEndpointsIt->second.nodeType,
							ipEndpointsIt->second.ipAddress.uint(),
							averageLatency))
					{
						LOG4CXX_WARN(logger, "E2E latency of "
								<< averageLatency / 10.0
								<< "ms for IP endpoint "
								<< ipEndpointsIt->second.ipAddress.str()
								<< " could not be sent to MONA");
					}

					LOG4CXX_TRACE(logger, "E2E latency of "
							<< averageLatency / 10.0
							<< "ms for IP endpoint "
							<< ipEndpointsIt->second.ipAddress.str()
							<< " reported");
				}

				ipEndpointsIt++;
			}
		}

		// cleaning up
		httpRequestsPerFqdn.clear();

		// RX/TX bytes counters
		if (!moly.txBytesHttp(_configuration.nodeId().uint(),
				_statistics.txHttpBytes()))
		{
			LOG4CXX_WARN(logger, "TX HTTP bytes could not be sent to MONA");
		}

		if (!moly.rxBytesHttp(_configuration.nodeId().uint(),
				_statistics.rxHttpBytes()))
		{
			LOG4CXX_WARN(logger, "RX HTTP bytes could not be sent to MONA");
		}

		if (!moly.txBytesIp(_configuration.nodeId().uint(),
				_statistics.txIpBytes()))
		{
			LOG4CXX_WARN(logger, "TX IP bytes could not be sent to MONA");
		}

		if (!moly.rxBytesIp(_configuration.nodeId().uint(),
				_statistics.rxIpBytes()))
		{
			LOG4CXX_WARN(logger, "RX IP bytes could not be sent to MONA");
		}

		//buffer sizes
		buffer_sizes_t bufferSizes;
		bufferSizes.push_back(pair<buffer_name_t, buffer_size_t>(
				BUFFER_NAP_IP_HANDLER,_statistics.bufferSizeIpHandler()));
		bufferSizes.push_back(pair<buffer_name_t, buffer_size_t>(
				BUFFER_NAP_HTTP_HANDLER_REQUESTS,
				_statistics.bufferSizeHttpHandlerRequests()));
		bufferSizes.push_back(pair<buffer_name_t, buffer_size_t>(
				BUFFER_NAP_HTTP_HANDLER_RESPONSES,
				_statistics.bufferSizeHttpHandlerResponses()));
		bufferSizes.push_back(pair<buffer_name_t, buffer_size_t>(
				BUFFER_NAP_LTP, _statistics.bufferSizeLtp()));

		if (!moly.bufferSizes(NODE_ROLE_NAP, bufferSizes))
		{
			LOG4CXX_WARN(logger, "Buffer sizes could not be sent to MONA");
		}

		file_descriptors_type_t fileDescriptorsType;
		fileDescriptorsType.push_back(pair<file_descriptor_type_t,
				file_descriptors_t>(DESCRIPTOR_TYPE_TCP_SOCKET,
						_statistics.tcpSockets()));

		if (!moly.fileDescriptorsType(NODE_ROLE_NAP, fileDescriptorsType))
		{
			LOG4CXX_WARN(logger, "Number of file descritpors could not be "
					"sent to MONA");
		}

	}

	LOG4CXX_INFO(logger, "Statistics collector thread stopped");
}
