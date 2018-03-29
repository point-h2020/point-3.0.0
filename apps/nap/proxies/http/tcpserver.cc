/*
 * tcpserver.cc
 *
 *  Created on: 20 Dec 2015
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
 * Blackadder. If not, see <http://www.gnu.org/licenses/>.
 */

#include <thread>

#include <proxies/http/tcpserver.hh>
#include <proxies/http/surrogatetcpclient.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define ENIGMA 23 	// https://en.wikipedia.org/wiki/23_enigma ... and don't
					//forget, it's also Sebastian's bday

using namespace proxies::http::surrogatetcpclient;
using namespace proxies::http::tcpserver;

LoggerPtr TcpServer::logger(Logger::getLogger("proxies.http.tcpserver"));

TcpServer::TcpServer(Configuration &configuration, Namespaces &namespaces,
		Statistics &statistics,	bool *run)
	: _configuration(configuration),
	  _namespaces(namespaces),
	  _statistics(statistics),
	  _run(run)
{
	_localSurrogateFd = -1;
	_httpRequestMethod = HTTP_METHOD_UNKNOWN;
}

TcpServer::~TcpServer() {
}

void TcpServer::operator ()(socket_fd_t socketFd, IpAddress ipAddress)
{
	std::thread *surrogateTcpClientThread;
	fd_set readSet;
	char packet[_configuration.tcpServerSocketBufferSize()];
	bzero(packet, _configuration.tcpServerSocketBufferSize());
	string httpRequest;
	int bytesRead;
	uint16_t packetSize;
	string fqdn, resource;
	sk_t sessionKey = (sk_t)socketFd;
	http_methods_t httpMethod;
	LOG4CXX_TRACE(logger, "New active TCP session with IP endpoint "
			<< ipAddress.str() << " via socket FD " << socketFd);
	FD_ZERO(&readSet);//Initialising descriptor
	FD_SET(socketFd, &readSet); // Set descriptor

	// check again that FD is >= 0
	if (socketFd == -1)
	{
		LOG4CXX_ERROR(logger, "TCP socket FD is -1. Not starting to read from "
				"it then");
		return;
	}

	_statistics.tcpSocket(1);

	while (FD_ISSET(socketFd, &readSet) && *_run)
	{
		bytesRead = read(socketFd, packet,
				_configuration.tcpServerSocketBufferSize());

		if (bytesRead <= 0)
		{
			switch(errno)
			{
			case 0:
				LOG4CXX_TRACE(logger, "Socket " << socketFd << " closed "
						"correctly by client " << ipAddress.str() << ": "
						<< strerror(errno));
				break;
			case ECONNRESET:
				LOG4CXX_DEBUG(logger, "Socket " << socketFd << " reset by "
						"client " << ipAddress.str() << ": "
						<< strerror(errno));
				break;
			default:
				LOG4CXX_DEBUG(logger,  "Socket " << socketFd << " closed "
						"unexpectedly by client " << ipAddress.str() << ": "
						<< strerror(errno));
			}

			break;
		}

		packetSize = bytesRead;
		LOG4CXX_TRACE(logger, "HTTP request of length " << bytesRead
				<< " received from " << ipAddress.str() << " via socket FD "
				<< socketFd);
		_statistics.rxHttpBytes(&bytesRead);

		// If surrogate is running locally, check if this request must be
		// relayed to localhost
		if (_configuration.localSurrogacy() && readFqdn(packet,
				packetSize).compare(_configuration.localSurrogateFqdn()) == 0)
		{
			LOG4CXX_TRACE(logger, "HTTP request to "
					<< _configuration.localSurrogateFqdn() << " gets relayed to"
							" local surrogate");

			// No socket FD available. Create one
			if (_localSurrogateFd == -1)
			{
				_localSurrogateFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

				if (_localSurrogateFd == -1)
				{
					LOG4CXX_WARN(logger, "Socket could not be created: "
							<< strerror(errno));
					shutdown(socketFd, SHUT_RDWR);
					return;
				}

				struct sockaddr_in serverAddress;
				bzero(&serverAddress, sizeof(serverAddress));
				serverAddress.sin_family = AF_INET;
				serverAddress.sin_port = htons(
						_configuration.localSurrogatePort());
				IpAddress ipAddress("127.0.0.1");
				inet_pton(AF_INET, ipAddress.str().c_str(),
						&serverAddress.sin_addr);

				// Connect to server
				if (connect(_localSurrogateFd,
						(struct sockaddr *) &serverAddress,
						sizeof(serverAddress)) < 0)
				{
					LOG4CXX_ERROR(logger, "TCP client socket towards "
							<< ipAddress.str() << " could not be established: "
							<< strerror(errno));
					close(_localSurrogateFd);
					shutdown(socketFd, SHUT_RDWR);
					return;
				}
			}

			// send off HTTP request to surrogate on localhost
			int bytesWritten = -1;
			bytesWritten = write(_localSurrogateFd, packet, packetSize);

			if (bytesWritten == -1)
			{
				LOG4CXX_DEBUG(logger, "HTTP request of length "
						<< packetSize << " could not be sent to IP endpoint"
						" using FD " << _localSurrogateFd << ": "
						<< strerror(errno));
				close(_localSurrogateFd);
				shutdown(socketFd, SHUT_RDWR);
				return;
			}
			else
			{
				LOG4CXX_TRACE(logger, "HTTP request of length "
						<< bytesWritten << " sent off to IP endpoint using "
						"socket FD " << _localSurrogateFd);
			}

			// create TCP listener for HTTP response
			SurrogateTcpClient surrogateTcpClient(_configuration, socketFd,
					_localSurrogateFd);
			surrogateTcpClientThread = new std::thread(surrogateTcpClient);
			surrogateTcpClientThread->detach();
			delete surrogateTcpClientThread;
		}
		else
		{
			httpMethod = readHttpRequestMethod(packet, packetSize);

			// POST or PUT and 1st++ TCP segment which only has data and no HTTP
			// header
			if (httpMethod == HTTP_METHOD_UNKNOWN)
			{
				LOG4CXX_TRACE(logger, "HTTP request is spawn over multiple TCP "
						"segments. Using HTTP method " << _httpRequestMethod
						<< ", FQDN " << _httpRequestFqdn << " and resource "
						<< _httpRequestResource);
				httpMethod = _httpRequestMethod;
				fqdn = _httpRequestFqdn;
				resource = _httpRequestResource;
			}
			else
			{
				LOG4CXX_TRACE(logger, "Entire HTTP request:\n" << packet);
				_httpRequestMethod = httpMethod;
				fqdn = readFqdn(packet, packetSize);
				_httpRequestFqdn = fqdn;
				resource = readResource(packet, packetSize);
				_httpRequestResource = resource;
			}

			//only send it if FQDN was found (resource will be at least '/')
			if (fqdn.length() > 0)
			{
				_namespaces.Http::handleRequest(fqdn, resource, httpMethod,
						(uint8_t *)packet, packetSize, socketFd);
			}
			else
			{
				LOG4CXX_DEBUG(logger, "FQDN is empty. Dropping packet");
			}
		}

		bzero(packet, _configuration.tcpServerSocketBufferSize());
	}

	if (!*_run)
	{
		LOG4CXX_DEBUG(logger, "Termination requested ... shutting down socket "
				<< socketFd << " with SHUT_RDWR");
		shutdown(socketFd, SHUT_RDWR);
	}

	_namespaces.Http::deleteSession(fqdn, resource, sessionKey);
	// FIXME HTTP session awareness not implemented. Using sleep to not close
	// the socket immediately
	//this_thread::sleep_for(chrono::seconds(ENIGMA));
	LOG4CXX_TRACE(logger, "Closing socket FD " << socketFd);
	close(socketFd);
	_statistics.tcpSocket(-1);
}
