/*
 * tcpclient.cc
 *
 *  Created on: 28 Jul 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of Blackadder.
 *
 * Blackadder is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
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

#include "tcpclient.hh"

using namespace proxies::http::tcpclient;

LoggerPtr TcpClient::logger(Logger::getLogger("proxies.http.tcpclient"));

TcpClient::TcpClient(Configuration &configuration, Namespaces &namespaces,
		Statistics &statistics, reverse_lookup_t *reverseLookup,
		socket_fds_t *socketFds, socket_state_t *socketState,
		mutex &tcpClientMutex, bool *run)
	: DnsResolutions(run),
	  TcpClientHelper(reverseLookup, tcpClientMutex),
	  _configuration(configuration),
	  _namespaces(namespaces),
	  _statistics(statistics),
	  _reverseLookup(reverseLookup),
	  _socketFds(socketFds),
	  _socketState(socketState),
	  _mutex(tcpClientMutex),
	  _run(run)
{
	_wildcardCid = IcnId("*");
}

TcpClient::~TcpClient(){}

void TcpClient::operator ()(IcnId cid, IcnId rCid, enigma_t enigma,
		sk_t remoteSocketFd, string nodeIdStr, uint8_t *packet,
		uint16_t packetSize)
{
	uint8_t *p = (uint8_t *)malloc(packetSize);

	if (p == NULL)
	{
		LOG4CXX_ERROR(logger, "malloc failed for establishing TCP client thread"
				" with CID " << cid.print() << " rCID " << rCid.print());
		free(p);
		return;
	}

	memcpy(p, packet, packetSize);

	// first copy the packet and several variables to thread safe memory
	// locations
	socket_fd_t socketFd;
	NodeId nodeId(nodeIdStr);
	bool callRead;

	socketFd = _tcpSocket(cid, nodeId, remoteSocketFd, callRead, p, packetSize);

	if (socketFd == -1)
	{
		LOG4CXX_WARN(logger, "Socket could not be created to server for CID "
				<< cid.print());
		free(p);
		return;
	}

	// creating read thread if this is a new socket determined in _tcpSocket
	if (callRead)
	{
		TcpClientRead tcpClientRead(_configuration, _namespaces, _statistics,
				_reverseLookup, _socketFds, _socketState, _mutex, _run);

		std::thread *readThread = new std::thread(tcpClientRead, socketFd,
				remoteSocketFd, nodeId, callRead);
		readThread->detach();
		delete readThread;
	}

	// Send off the data using the new FD _nextTcpClientFd
	int bytesWritten = -1;

	if (!_socketWriteable(socketFd))
	{
		LOG4CXX_TRACE(logger, "Socket FD " << socketFd << " not writeable "
				"anymore. Packet of length " << packetSize << " will be "
						"dropped");
		free(p);
		return;
	}

	_addReverseLookup(socketFd, rCid, enigma);

	bytesWritten = send(socketFd, p, packetSize, MSG_NOSIGNAL);
	//write(socketFd, p, packetSize);

	if (bytesWritten <= 0)
	{
		LOG4CXX_INFO(logger, "HTTP request of length " << packetSize
				<< " could not be sent to IP endpoint using FD "
				<< socketFd	<< ": " << strerror(errno));
		free(p);
		return;
	}

	_statistics.txHttpBytes(&bytesWritten);
	LOG4CXX_TRACE(logger, "HTTP request of length " << bytesWritten
			<< " sent off to IP endpoint using socket FD "
			<< socketFd);
	free(p);
}

void TcpClient::_addReverseLookup(int &socketFd, IcnId &rCId,
		enigma_t &enigma)
{
	//_reverseLookupMutex->lock();
	_mutex.lock();
	_reverseLookupIt = _reverseLookup->find(socketFd);

	if (_reverseLookupIt == _reverseLookup->end())
	{
		rcid_enigma_t socketFdStruct;
		socketFdStruct.enigma = enigma;
		socketFdStruct.rCId = rCId;
		_reverseLookup->insert(pair<socket_fd_t, rcid_enigma_t>(socketFd,
				socketFdStruct));
		LOG4CXX_TRACE(logger, "Reverse look-up for socket FD " << socketFd
				<< " added with rCID " << rCId.print() << " and Enigma "
				<< enigma);
	}
	else
	{
		LOG4CXX_TRACE(logger, "Reverse look-up already exists for socket FD "
				<< socketFd << " > rCID "  << rCId.print() << " > Enigma "
				<< enigma);
	}

	//_reverseLookupMutex->unlock();
	_mutex.unlock();
}

void TcpClient::_setSocketState(socket_fd_t &socketFd, bool state)
{
	_mutex.lock();
	_socketStateIt = _socketState->find(socketFd);

	if (_socketStateIt == _socketState->end())
	{
		_socketState->insert(pair<socket_fd_t, bool>(socketFd, state));
		LOG4CXX_TRACE(logger, "New socket FD " << socketFd << " added to list "
				"of known sockets with state " << state);
		_mutex.unlock();
		return;
	}

	_socketStateIt->second = state;
	LOG4CXX_TRACE(logger, "Socket FD " << socketFd << " updated with state "
			<< state);
	_mutex.unlock();
}

bool TcpClient::_socketWriteable(int &socketFd)
{
	fd_set writeSet;
	bool state = false;
	FD_ZERO(&writeSet);
	FD_SET(socketFd, &writeSet);

	if(!FD_ISSET(socketFd, &writeSet))
	{
		LOG4CXX_TRACE(logger, "TCP socket FD " << socketFd << " not writeable "
				"anymore");
		return false;
	}

	//_socketStateMutex->lock();
	_mutex.lock();
	_socketStateIt = _socketState->find(socketFd);

	if (_socketStateIt != _socketState->end())
	{
		state = _socketStateIt->second;
	}
	else
	{
		LOG4CXX_TRACE(logger, "Socket FD " << socketFd << " not found in socket"
				" state map");
	}

	//_socketStateMutex->unlock();
	_mutex.unlock();
	return state;
}

int TcpClient::_tcpSocket(IcnId &cId, NodeId &nodeId,
		sk_t &remoteSocketFd, bool &callRead, uint8_t *packet,
		uint16_t &packetSize)
{
	callRead = false;
	socket_fd_t socketFd = -1;
	string fqdn;
	unordered_map<sk_t, socket_fd_t>::iterator rSksIt;
	fqdn = readFqdn(packet, packetSize);
	// First check if socket already exists for this HTTP session
	//_socketFdsMutex->lock();
	_mutex.lock();
	_socketFdsIt = _socketFds->find(nodeId.uint());

	//FQDN not found (continuation of existing HTTP REQ session)
	if (fqdn.empty())
	{
		// NID found
		if (_socketFdsIt != _socketFds->end() && !fqdn.empty())
		{
			rSksIt = _socketFdsIt->second.find(remoteSocketFd);

			// remote socket FD found
			if (rSksIt != _socketFdsIt->second.end())
			{
				socketFd = rSksIt->second;
				//_socketFdsMutex->unlock();
				_mutex.unlock();
				LOG4CXX_TRACE(logger, "Existing local socket FD " << socketFd
						<< " used for sending off this HTTP request from NID "
						<< nodeId.uint() << " for CID " << cId.print());
				return socketFd;
			}
			else
			{
				LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " > SFD "
						<< remoteSocketFd << " does not exist in socketFds "
						"map");
			}
		}
		else
		{
			LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " does not exist "
					"in socket FDs map (NID > SFD)");
		}
	}

	/* DO NOT unlock socketFdsMutex, as this would allow the second received
	 * HTTP fragment (over ICN) to think there's no socket available and a new
	 * one must be created. This is because the socket creation takes slightly
	 * longer then receiving the next fragment and retrieving it from the ICN
	 * buffer
	 */
	callRead = true;//as new socket is created and the functor must call read(()
	bool endpointFound = false;
	IpAddress ipAddressEndpoint;
	uint16_t port;
	list<pair<IcnId, pair<IpAddress, uint16_t>>> ipEndpoints;
	list<pair<IcnId, pair<IpAddress, uint16_t>>>::iterator ipEndpointsIt;
	ipEndpoints = _configuration.fqdns();
	// get IP address for this CID (FQDN)
	ipEndpointsIt = ipEndpoints.begin();

	while(ipEndpointsIt != ipEndpoints.end())
	{
		ipEndpointsIt->first;

		if (ipEndpointsIt->first.uint() == cId.uint())
		{
			ipAddressEndpoint = ipEndpointsIt->second.first;
			port = ipEndpointsIt->second.second;
			endpointFound = true;
			break;
		}

		ipEndpointsIt++;
	}

	// if no IP, check if the CID is a wildcard match and this NAP is ICN GW
	if (!endpointFound && _configuration.icnGatewayHttp())
	{
		// check if an IP is known from previous DNS requests
		if (checkDns(fqdn, ipAddressEndpoint))
		{
			endpointFound = true;
		}
		else
		{
			port = 80;
			endpointFound = resolve(fqdn, ipAddressEndpoint);
		}
	}

	// if IP address still not known, exit here
	if (!endpointFound)
	{
		LOG4CXX_INFO(logger, "IP endpoint for " << cId.print() << " was "
				"not configured");
		_mutex.unlock();
		return socketFd;
	}

	// create socket
	socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (socketFd == -1)
	{
		LOG4CXX_WARN(logger, "Socket could not be created: "<< strerror(errno));
		_mutex.unlock();
		return socketFd;
	}

	struct sockaddr_in serverAddress;
	bzero(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	inet_pton(AF_INET, ipAddressEndpoint.str().c_str(),
			&serverAddress.sin_addr);

	// Connect to server
	if (connect(socketFd, (struct sockaddr *) &serverAddress,
			sizeof(serverAddress)) == -1)
	{
		LOG4CXX_INFO(logger, "TCP client socket towards "
				<< ipAddressEndpoint.str() << ":" << port << " could not be "
						"established: "	<< strerror(errno));
		close(socketFd);
		//_socketFdsMutex->unlock();
		_mutex.unlock();
		return socketFd;
	}

	LOG4CXX_TRACE(logger, "TCP socket opened with FD " << socketFd
			<< " towards " << ipAddressEndpoint.str() << ":" << port);
	// add new socket to socket FD map
	_socketFdsIt = _socketFds->find(nodeId.uint());

	// NID does not exists
	if (_socketFdsIt == _socketFds->end())
	{
		unordered_map<sk_t, socket_fd_t> rSks;
		rSks.insert(pair<sk_t, socket_fd_t> (remoteSocketFd, socketFd));
		_socketFds->insert(pair<nid_t, unordered_map<sk_t, socket_fd_t>>
				(nodeId.uint(), rSks));
		LOG4CXX_TRACE(logger, "New local socket " << socketFd << " towards "
				<< ipAddressEndpoint.str() << ":" << port << " added to "
				"socketFds map for new NID " << nodeId.uint() << " > rSFD "
				<< remoteSocketFd);
		//_socketFdsMutex->unlock();
		_mutex.unlock();
		_setSocketState(socketFd, true);
		return socketFd;
	}

	// NID exists
	rSksIt = _socketFdsIt->second.find(remoteSocketFd);

	// rSFD  does not exist
	if (rSksIt == _socketFdsIt->second.end())
	{
		_socketFdsIt->second.insert(pair<sk_t, socket_fd_t> (remoteSocketFd,
				socketFd));
		LOG4CXX_TRACE(logger, "New local socket " << socketFd << " towards "
				<< ipAddressEndpoint.str() << ":" << port << " added to "
				"socketFds map for known NID " << nodeId.uint() << " but new "
				"rSFD "	<< remoteSocketFd);
	}
	else
	{
		if (rSksIt->second == socketFd)
		{
			LOG4CXX_TRACE(logger, "Socket FD " << socketFd << " already known "
					"for NID " << nodeId.uint() << " > rSFD "
					<< remoteSocketFd);
		}
		// Updating socket entry
		else
		{
			rSksIt->second = socketFd;
			LOG4CXX_TRACE(logger, "Socket FD " << socketFd << " updated in NID "
					<< nodeId.uint() << " > rSFD " << remoteSocketFd
					<< " socket map");
		}
	}

	//_socketFdsMutex->unlock();
	_mutex.unlock();
	_setSocketState(socketFd, true);
	_statistics.tcpSocket(1);
	return socketFd;
}
