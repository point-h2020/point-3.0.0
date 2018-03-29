/*
 * surrogatetcpclient.cc
 *
 *  Created on: 20 Aug 2016
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

#include "surrogatetcpclient.hh"

using namespace proxies::http::surrogatetcpclient;

LoggerPtr SurrogateTcpClient::logger(
		Logger::getLogger("proxies.http.surrogatetcpclient"));

SurrogateTcpClient::SurrogateTcpClient(Configuration &configuration,
		int clientSocketFd, int surrogateSocketFd)
	: _configuration(configuration),
	  _clientSocketFd(clientSocketFd),
	  _surrogateSocketFd(surrogateSocketFd)
{}

SurrogateTcpClient::~SurrogateTcpClient() {}

void SurrogateTcpClient::operator()()
{
	fd_set rset;
	uint8_t *packet = (uint8_t *)malloc(
			_configuration.tcpClientSocketBufferSize());
	int bytesWritten;
	FD_SET(_surrogateSocketFd, &rset);
	while (FD_ISSET(_surrogateSocketFd, &rset))
	{
		bytesWritten = read(_surrogateSocketFd, packet,
				_configuration.tcpClientSocketBufferSize());
		if (bytesWritten == -1)
		{
			LOG4CXX_DEBUG(logger, "Surrogate socket error (FD "
					<< _surrogateSocketFd << "): " << strerror(errno));
			break;
		}
		if (bytesWritten == 0)
		{
			LOG4CXX_DEBUG(logger, "Socket closed by surrogate (FD "
					<< _surrogateSocketFd << ")");
			break;
		}
		LOG4CXX_TRACE(logger, "HTTP response of length " << bytesWritten
				<< " received via surrogate socket FD " << _surrogateSocketFd);
		// send HTTP response to IP endpoint
		bytesWritten = write(_clientSocketFd, packet, bytesWritten);
		if (bytesWritten == -1)
		{
			LOG4CXX_DEBUG(logger, "HTTP response of length "
					<< bytesWritten << " could not be sent to IP endpoint using "
					"FD " << _clientSocketFd << ": " << strerror(errno));
			break;
		}
		else
		{
			LOG4CXX_TRACE(logger, "HTTP response of length " << bytesWritten
					<< " sent off to IP endpoint using client socket FD "
					<< _clientSocketFd);
		}
	}
	free(packet);
	shutdown(_clientSocketFd, SHUT_RDWR);
}
