/*
 * tcpclientread.cc
 *
 *  Created on: 17 Aug 2017
 *      Author: user
 */

#include "tcpclientread.hh"

using namespace proxies::http::tcpclient;

LoggerPtr TcpClientRead::logger(Logger::getLogger("proxies.http.tcpclient"));

TcpClientRead::TcpClientRead(Configuration &configuration,
		Namespaces &namespaces, Statistics &statistics,
		reverse_lookup_t *reverseLookup, socket_fds_t *socketFds,
		socket_state_t *socketState, mutex &tcpClientMutex, bool *run)
	: TcpClientHelper(reverseLookup, tcpClientMutex),
	  _configuration(configuration),
	  _namespaces(namespaces),
	  _statistics(statistics),
	  _reverseLookup(reverseLookup),
	  _socketFds(socketFds),
	  _socketState(socketState),
	  _mutex(tcpClientMutex),
	  _run(run)
{}

TcpClientRead::~TcpClientRead() {}

void TcpClientRead::operator()(socket_fd_t socketFd, socket_fd_t remoteSocketFd,
		NodeId nodeId, bool mpdRequest)
{
	fd_set writeSet;
	uint16_t packetSize;
	uint8_t *p = (uint8_t *)malloc(_configuration.tcpClientSocketBufferSize());
	bool firstPacket = true;
	sk_t sessionKey = (sk_t)socketFd;
	FD_SET(socketFd, &writeSet);
	int bytesReceived;

	while (FD_ISSET(socketFd, &writeSet))
	{
		bzero(p, _configuration.tcpClientSocketBufferSize());
		bytesReceived = read(socketFd, p,
				_configuration.tcpClientSocketBufferSize());

		if (bytesReceived <= 0)
		{
			_setSocketState(socketFd, false);

			if (errno > 0)
			{
				LOG4CXX_DEBUG(logger,  "Socket " << socketFd << " closed "
						"unexpectedly: " << strerror(errno));
			}
			else
			{
				LOG4CXX_TRACE(logger,  "Socket " << socketFd << " closed: "
						<< strerror(errno));
			}

			LOG4CXX_TRACE(logger, "Shutting down socket " << socketFd
					<< " completely");
			shutdown(socketFd, SHUT_RDWR);
			LOG4CXX_TRACE(logger, "Closing socket " << socketFd);
			close(socketFd);
			_statistics.tcpSocket(-1);
			break;
		}

		packetSize = bytesReceived;
		LOG4CXX_TRACE(logger, "Packet of length " << packetSize << " received "
				"via socket FD " << socketFd);
		_statistics.rxHttpBytes(&bytesReceived);

		if (!_namespaces.Http::handleResponse(rCid(socketFd),
				enigma(socketFd), sessionKey, firstPacket, p,
				packetSize))
		{
			LOG4CXX_TRACE(logger, "Response read from socket FD " << socketFd
					<< " was not sent to cNAP(s) for rCID "
					<< rCid(socketFd).print() << " > Enigma "
					<< enigma(socketFd) << " > SK " << sessionKey);
			break;
		}

		firstPacket = false;
	}

	_deleteSocketState(socketFd);
	// Now clean up states in the HTTP handler
	_namespaces.Http::closeCmcGroup(rCid(socketFd), enigma(socketFd),
			sessionKey);
	// Cleaning up _reverseLookup map
	_mutex.lock();
	_reverseLookupIt = _reverseLookup->find(socketFd);

	if (_reverseLookupIt == _reverseLookup->end())
	{
		LOG4CXX_TRACE(logger, "Socket FD " << socketFd << " cannot be found in"
				" reverse lookup map");
		//_reverseLookupMutex->unlock();
		_mutex.unlock();
		free(p);
		return;
	}

	_reverseLookup->erase(_reverseLookupIt);
	// Cleaning up remote socket FD
	unordered_map<sk_t, int>::iterator rSksIt;
	_socketFdsIt = _socketFds->find(nodeId.uint());

	// NID found
	if (_socketFdsIt != _socketFds->end())
	{
		rSksIt = _socketFdsIt->second.find(remoteSocketFd);

		// remote socket FD found
		if (rSksIt != _socketFdsIt->second.end())
		{
			_socketFdsIt->second.erase(rSksIt);
			LOG4CXX_TRACE(logger, "Remote socket FD (SK) " << remoteSocketFd
					<< " deleted for NID " << nodeId.uint() << " > "
					<< _socketFdsIt->first);
		}

		// If this was the last rSFD, delete the entire NID entry
		if (_socketFdsIt->second.empty())
		{
			LOG4CXX_TRACE(logger, "That was the last remote socket FD (SK) for "
					"NID " << nodeId.uint() << ". " << "Deleting map key "
					<< _socketFdsIt->first);
			_socketFds->erase(_socketFdsIt);
		}
	}

	_mutex.unlock();
	free(p);
}

void TcpClientRead::_deleteSocketState(socket_fd_t &socketFd)
{
	_mutex.lock();
	_socketStateIt = _socketState->find(socketFd);

	if (_socketStateIt != _socketState->end())
	{
		_socketState->erase(_socketStateIt);
		LOG4CXX_TRACE(logger, "Socket FD " << socketFd << " deleted from list "
				"of writable socket FDs");
	}

	_mutex.unlock();
}

void TcpClientRead::_setSocketState(socket_fd_t &socketFd, bool state)
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
