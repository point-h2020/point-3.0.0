/*
 * statistics.cc
 *
 *  Created on: 2 Sep 2016
 *      Author: Sebastian Robitzsch
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

#include "statistics.hh"

using namespace monitoring::statistics;

LoggerPtr Statistics::logger(Logger::getLogger("monitoring.statistics"));

Statistics::Statistics()
{
	_bufferSizeHttpHandlerRequests.first = 0;
	_bufferSizeHttpHandlerRequests.second = 0;
	_bufferSizeHttpHandlerResponses.first = 0;
	_bufferSizeHttpHandlerResponses.second = 0;
	_bufferSizeIpHandler.first = 0;
	_bufferSizeIpHandler.second = 0;
	_cmcGroupSizes.first = 0;
	_cmcGroupSizes.second = 0;
	_rxHttpBytes = 0 ;
	_rxIpBytes = 0;
	_tcpSockets = 0;
	_txHttpBytes = 0;
	_txIpBytes = 0;
    _rxIGMPBytes = 0;
    _txIGMPBytes = 0;
}

uint32_t Statistics::averageCmcGroupSize()
{
    uint32_t averageCmcGroupSize;
    _mutex.lock();

    if (_cmcGroupSizes.first != 0)
    {
        averageCmcGroupSize = _cmcGroupSizes.second * 10 / _cmcGroupSizes.first;
    }
    else
    {
        averageCmcGroupSize = 0;
    }

    _cmcGroupSizes.first = 0;
    _cmcGroupSizes.second = 0;
    _mutex.unlock();
    return averageCmcGroupSize;
}

unordered_map<uint32_t, uint16_t> Statistics::averageNetworkDelayPerFqdn()
{
    unordered_map<uint32_t, uint16_t> latencyPerFqdn;
    forward_list<uint16_t>::iterator fListIt;
    uint32_t rttSum;
    uint16_t listSize;
    uint16_t latency;
    _mutex.lock();
    for (_rttPerFqdnIt = _rttPerFqdn.begin();
            _rttPerFqdnIt != _rttPerFqdn.end(); _rttPerFqdnIt++)
    {
        listSize = 0;
        rttSum = 0;
        // Get RTT sum
        for (fListIt = _rttPerFqdnIt->second.begin();
                fListIt != _rttPerFqdnIt->second.end(); fListIt++)
        {
            rttSum += *fListIt;
            listSize++;
        }
        // Calculate & insert sum of not 0
        if (rttSum != 0)
        {
            latency = ((rttSum / listSize) / 2.0) * 10.0;

            //if RTT was 1, latency is 0. Set it to 1
            if (latency == 0)
            {
                latency = 1;
            }

            latencyPerFqdn.insert(pair<uint32_t, uint16_t>(_rttPerFqdnIt->first,
                    latency));
            // Now reset all values for this FQDN
            _rttPerFqdnIt->second.clear();
        }
    }
    _mutex.unlock();
    return latencyPerFqdn;
}

void Statistics::bufferSizeHttpHandlerRequests(uint32_t bufferSize)
{
	_mutex.lock();
	_bufferSizeHttpHandlerRequests.first += bufferSize;
	_bufferSizeHttpHandlerRequests.second++;
	_mutex.unlock();
}

uint32_t Statistics::bufferSizeHttpHandlerRequests()
{
	uint32_t bufferSize = 0;
	_mutex.lock();

	if (_bufferSizeHttpHandlerRequests.second != 0)
	{
		bufferSize = _bufferSizeHttpHandlerRequests.first /
				_bufferSizeHttpHandlerRequests.second;
		_bufferSizeHttpHandlerRequests.first = 0;
		_bufferSizeHttpHandlerRequests.second = 0;
	}

	_mutex.unlock();
	LOG4CXX_TRACE(logger, "HTTP handler buffer size (requests): " << bufferSize);
	return bufferSize;
}

void Statistics::bufferSizeHttpHandlerResponses(uint32_t bufferSize)
{
	_mutex.lock();
	_bufferSizeHttpHandlerResponses.first += bufferSize;
	_bufferSizeHttpHandlerResponses.second++;
	_mutex.unlock();
}

uint32_t Statistics::bufferSizeHttpHandlerResponses()
{
	uint32_t bufferSize = 0;
	_mutex.lock();

	if (_bufferSizeHttpHandlerResponses.second != 0)
	{
		bufferSize = _bufferSizeHttpHandlerResponses.first /
				_bufferSizeHttpHandlerResponses.second;
		_bufferSizeHttpHandlerResponses.first = 0;
		_bufferSizeHttpHandlerResponses.second = 0;
	}

	_mutex.unlock();
	LOG4CXX_TRACE(logger, "HTTP handler buffer size (responses): "
			<< bufferSize);
	return bufferSize;
}

void Statistics::bufferSizeIpHandler(uint32_t bufferSize)
{
    _mutex.lock();
    _bufferSizeIpHandler.first += bufferSize;
    _bufferSizeIpHandler.second++;
    _mutex.unlock();
}

uint32_t Statistics::bufferSizeIpHandler()
{
	uint32_t bufferSize = 0;
	_mutex.lock();

	if (_bufferSizeIpHandler.second != 0)
	{
		bufferSize = _bufferSizeIpHandler.first / _bufferSizeIpHandler.second;
		_bufferSizeIpHandler.first = 0;
		_bufferSizeIpHandler.second = 0;
	}

	_mutex.unlock();
	LOG4CXX_TRACE(logger, "IP handler buffer size: " << bufferSize);
	return bufferSize;
}

uint32_t Statistics::bufferSizeLtp()
{
	uint32_t bufferSize = 0;
	_mutex.lock();

	if (_bufferSizeLtp.second != 0)
	{
		bufferSize = _bufferSizeLtp.first / _bufferSizeLtp.second;
		_bufferSizeLtp.first = 0;
		_bufferSizeLtp.second = 0;
	}

	_mutex.unlock();
	LOG4CXX_TRACE(logger, "LTP buffer size: " << bufferSize);
	return bufferSize;
}

void Statistics::bufferSizeLtp(uint32_t bufferSize)
{
	_mutex.lock();
	_bufferSizeLtp.first += bufferSize;
	_bufferSizeLtp.second++;
	_mutex.unlock();
}

void Statistics::cmcGroupSize(uint32_t cmcGroupSize)
{
    _mutex.lock();
    _cmcGroupSizes.first++;
    _cmcGroupSizes.second += cmcGroupSize;
    _mutex.unlock();
}

void Statistics::ipEndpointAdd(IpAddress ipAddress)
{
    ip_endpoints_t::iterator ipEndpointsIt;

    if (ipAddress.str().length() == 0)
    {
        LOG4CXX_DEBUG(logger, "IP address is empty. IP endpoint is not going to"
                " be reported");
        return;
    }

    _mutex.lock();
    ipEndpointsIt = _ipEndpoints.find(ipAddress.uint());

    // new IP endpoint
    if (ipEndpointsIt == _ipEndpoints.end())
    {
        ip_endpoint_t ipEndpoint;
        ipEndpoint.ipAddress = ipAddress;
        ipEndpoint.nodeType = NODE_ROLE_UE;
        ipEndpoint.surrogate = false;
        ipEndpoint.reported = false;
        _ipEndpoints.insert(pair<uint32_t, ip_endpoint_t>(ipAddress.uint(),
                ipEndpoint));
        LOG4CXX_TRACE(logger, "New UE added. IP address " << ipAddress.str());
    }

    _mutex.unlock();
}

void Statistics::ipEndpointAdd(IpAddress ipAddress, uint16_t port)
{
    ip_endpoints_t::iterator ipEndpointsIt;

    if (ipAddress.str().length() == 0)
    {
        LOG4CXX_DEBUG(logger, "IP address is empty. IP endpoint is not going to"
                " be reported");
        return;
    }

    _mutex.lock();
    ipEndpointsIt = _ipEndpoints.find(ipAddress.uint());

    // new IP endpoint
    if (ipEndpointsIt == _ipEndpoints.end())
    {
        ip_endpoint_t ipEndpoint;
        ipEndpoint.ipAddress = ipAddress;
        ipEndpoint.nodeType = NODE_ROLE_SERVER;
        ipEndpoint.surrogate = true;
        ipEndpoint.port = port;
        ipEndpoint.reported = false;
        _ipEndpoints.insert(pair<uint32_t, ip_endpoint_t>(ipAddress.uint(),
                ipEndpoint));
        LOG4CXX_TRACE(logger, "New surrogate server added on "
                << ipAddress.str() << ":" << port)
    }

    _mutex.unlock();
}

void Statistics::ipEndpointAdd(IpAddress ipAddress, string fqdn, uint16_t port)
{
    ip_endpoints_t::iterator ipEndpointsIt;

    if (ipAddress.str().length() == 0)
    {
        LOG4CXX_DEBUG(logger, "IP address is empty. IP endpoint is not going to"
                " be reported");
        return;
    }

    _mutex.lock();
    ipEndpointsIt = _ipEndpoints.find(ipAddress.uint());

    // new IP endpoint
    if (ipEndpointsIt == _ipEndpoints.end())
    {
        ip_endpoint_t ipEndpoint;
        ipEndpoint.ipAddress = ipAddress;
        ipEndpoint.nodeType = NODE_ROLE_SERVER;
        ipEndpoint.surrogate = false;
        ipEndpoint.fqdn = fqdn;
        ipEndpoint.port = port;
        ipEndpoint.reported = false;
        _ipEndpoints.insert(pair<uint32_t, ip_endpoint_t>(ipAddress.uint(),
                ipEndpoint));
        LOG4CXX_TRACE(logger, "New server added. FQDN " << fqdn << ", IP "
                "address " << ipAddress.str() << ":" << port);
    }

    _mutex.unlock();
}

ip_endpoints_t Statistics::ipEndpoints()
{
    ip_endpoints_t currentIpEndpoints;
    ip_endpoints_t::iterator ipEndpointsIt;
    _mutex.lock();
    ipEndpointsIt = _ipEndpoints.begin();

    while (ipEndpointsIt != _ipEndpoints.end())
    {
        currentIpEndpoints.insert(pair<uint32_t, ip_endpoint_t>(
                ipEndpointsIt->first, ipEndpointsIt->second));

        // hasn't been reported
        if (!ipEndpointsIt->second.reported)
        {
            ipEndpointsIt->second.reported = true;
            LOG4CXX_TRACE(logger, "IP endpoint "
                    << ipEndpointsIt->second.ipAddress.str() << " marked as "
                    "being reported");
        }

        ipEndpointsIt++;
    }

    _mutex.unlock();
    return currentIpEndpoints;
}

http_requests_per_fqdn_t Statistics::httpRequestsPerFqdn()
{
    http_requests_per_fqdn_t httpRequestsPerFqdn;
    _mutex.lock();
    httpRequestsPerFqdn = _httpRequestsPerFqdn;
    _httpRequestsPerFqdn.clear();
    _mutex.unlock();
    return httpRequestsPerFqdn;
}

void Statistics::httpRequestsPerFqdn(string fqdn, uint32_t numberOfRequests)
{
    http_requests_per_fqdn_t::iterator httpRequestsPerFqdnIt;
    _mutex.lock();
    httpRequestsPerFqdnIt = _httpRequestsPerFqdn.find(fqdn);
    // FQDN key does not exist
    if (httpRequestsPerFqdnIt == _httpRequestsPerFqdn.end())
    {
        _httpRequestsPerFqdn.insert(pair<string, uint32_t>(fqdn,
                numberOfRequests));
    }
    // FQDN does exist
    else
    {
        httpRequestsPerFqdnIt->second += numberOfRequests;
    }
    _mutex.unlock();
}

void Statistics::roundTripTime(IcnId cid, uint16_t rtt)
{
    if (rtt == 0)
    {
        rtt = 1;
    }
    _mutex.lock();
    _rttPerFqdnIt = _rttPerFqdn.find(cid.uintId());//only get the information
    // item which is the hashed FQDN
    // FQDN not found
    if (_rttPerFqdnIt == _rttPerFqdn.end())
    {
        forward_list<uint16_t> rtts;
        rtts.push_front(rtt);
        _rttPerFqdn.insert(pair<uint32_t, forward_list < uint16_t >> (cid.uintId(),
                rtts));
        LOG4CXX_TRACE(logger, "New hashed FQDN " << cid.uintId() << " added to"
                " RTT statistics table");
        _mutex.unlock();
        return;
    }
    // simply add the new rtt value
    _rttPerFqdnIt->second.push_front(rtt);
    _mutex.unlock();
}

uint32_t Statistics::rxHttpBytes()
{
	uint32_t rxHttpBytes = 0;
	_mutex.lock();
	rxHttpBytes = _rxHttpBytes;
	_rxHttpBytes = 0;
	_mutex.unlock();
	LOG4CXX_TRACE(logger, "RX HTTP bytes counter of " << rxHttpBytes
			<< " obtained and counter reset");
	return rxHttpBytes;
}

void Statistics::rxHttpBytes(int *rxBytes)
{
	_mutex.lock();
	_rxHttpBytes += *rxBytes;
	LOG4CXX_TRACE(logger, "RX IP bytes counter increased by " << *rxBytes
			<< " to " << _rxHttpBytes << " bytes");
	_mutex.unlock();
}

uint32_t Statistics::rxIpBytes()
{
	uint32_t rxIpBytes = 0;
	_mutex.lock();
	rxIpBytes = _rxIpBytes;
	_rxIpBytes = 0;
	_mutex.unlock();
	LOG4CXX_TRACE(logger, "RX IP bytes counter of " << rxIpBytes
			<< " obtained and counter reset");
	return rxIpBytes;
}

void Statistics::rxIpBytes(uint16_t *rxBytes)
{
	_mutex.lock();
	_rxIpBytes += *rxBytes;
	LOG4CXX_TRACE(logger, "RX IP bytes counter increased by " << *rxBytes
			<< " to " << _rxIpBytes << " bytes");
	_mutex.unlock();
}

void Statistics::tcpSocket(int sockets)
{
    _mutex.lock();

    // just in case
    if (_tcpSockets == 0 && sockets < 0)
    {
        _tcpSockets = 0;
    } else
    {
        _tcpSockets += sockets;
    }

    _mutex.unlock();
}

uint16_t Statistics::tcpSockets()
{
	uint16_t tcpSockets;
	_mutex.lock();
	tcpSockets = _tcpSockets;
	_mutex.unlock();
	LOG4CXX_TRACE(logger, "TCP file descriptor counter of " << tcpSockets
			<< " obtained and reset");
	return tcpSockets;
}

uint32_t Statistics::txHttpBytes()
{
	uint32_t txHttpBytes = 0;
	_mutex.lock();
	txHttpBytes = _txHttpBytes;
	_txHttpBytes = 0;
	_mutex.unlock();
	LOG4CXX_TRACE(logger, "TX HTTP bytes counter of " << txHttpBytes
			<< " obtained and counter reset");
	return txHttpBytes;
}

void Statistics::txHttpBytes(int *txBytes)
{
	_mutex.lock();
	_txHttpBytes += *txBytes;
	LOG4CXX_TRACE(logger, "TX HTTP bytes counter increased by " << *txBytes
			<< " to " << _txHttpBytes << " bytes");
	_mutex.unlock();
}

uint32_t Statistics::txIpBytes()
{
	uint32_t txIpBytes = 0;
	_mutex.lock();
	txIpBytes = _txIpBytes;
	_txIpBytes = 0;
	_mutex.unlock();
	LOG4CXX_TRACE(logger, "TX IP bytes counter of " << txIpBytes
			<< " obtained and counter reset");
	return txIpBytes;
}

void Statistics::txIpBytes(int *txBytes)
{
	_mutex.lock();
	_txIpBytes += *txBytes;
	LOG4CXX_TRACE(logger, "TX IP bytes counter increased by " << *txBytes
			<< " to " << _txIpBytes << " bytes");
	_mutex.unlock();
}

uint32_t Statistics::averageChannelAcquisitionTime()
{
    uint32_t sum = 0;
    uint32_t size = 0;
    _mutex.lock();
    _channelAcquisitionTimeIt = _channelAcquisitionTime.begin();

    while (_channelAcquisitionTimeIt != _channelAcquisitionTime.end())
    {
        size++;
        sum += _channelAcquisitionTimeIt->second;
        ++_channelAcquisitionTimeIt;
    }

    _channelAcquisitionTime.clear();
    _mutex.unlock();

    if (size != 0)
    {
    	return ((uint32_t) sum / size);
    }

    return 0;
}

uint32_t Statistics::counterRxIGMPBytes()
{
    uint32_t rxBytes;
    _mutex.lock();
    rxBytes = _rxIGMPBytes;
    _rxIGMPBytes = 0;
    _mutex.unlock();
    return rxBytes;
}

uint32_t Statistics::counterTxIGMPBytes()
{
    uint32_t txBytes;
    _mutex.lock();
    txBytes = _txIGMPBytes;
    _txIGMPBytes = 0;
    _mutex.unlock();
    return txBytes;
}
 
void Statistics::channelAcquisitionTime(uint32_t mcastGrpAddr, uint32_t channelAcquisitionTime)
{
    _mutex.lock();

    auto _channelAcquisitionTimeIt = _channelAcquisitionTime.find(mcastGrpAddr);
    // mcast group key does not exist
    if (_channelAcquisitionTimeIt == _channelAcquisitionTime.end())
    {
        _channelAcquisitionTime.insert(pair<uint32_t, uint32_t>(mcastGrpAddr,
                channelAcquisitionTime));
    } // mcast group  does exist
    else
    {
        // in this case, use the average value
        _channelAcquisitionTimeIt->second += channelAcquisitionTime;
        _channelAcquisitionTimeIt->second = _channelAcquisitionTimeIt->second / 2;
    }

    _mutex.unlock();
}

void Statistics::rxIGMPBytes(uint32_t rxBytes)
{
    _mutex.lock();
    _rxIGMPBytes += rxBytes;
    _mutex.unlock();
}

void Statistics::txIGMPBytes(uint32_t txBytes)
{
    _mutex.lock();
    _txIGMPBytes += txBytes;
    _mutex.unlock();
}

