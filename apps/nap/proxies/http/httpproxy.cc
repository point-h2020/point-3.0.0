/*
 * httpproxy.cc
 *
 *  Created on: 24 May 2016
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

#include "httpproxy.hh"

using namespace proxies::http;
using namespace proxies::http::tcpserver;

LoggerPtr HttpProxy::logger(Logger::getLogger("proxies.http"));

HttpProxy::HttpProxy(Configuration &configuration, Namespaces &namespaces,
		Statistics &statistics, bool *run)
	: _configuration(configuration),
	  _namespaces(namespaces),
	  _statistics(statistics),
	  _run(run)
{
	_tcpListener = -1;
}

HttpProxy::~HttpProxy() {}

void HttpProxy::tearDown()
{
	LOG4CXX_INFO(logger, "Shutting down transparent HTTP proxy");
	std::ostringstream iptablesRule;
	iptablesRule << "iptables -D PREROUTING -t nat -p tcp -i "
			<< _configuration.networkDevice() << " --dport "
			<< _configuration.tcpInterceptionPort() << " -j REDIRECT --to-port "
			<< _configuration.httpProxyPort();

	if (system(iptablesRule.str().c_str()) == -1)
	{
		LOG4CXX_WARN(logger, "iptables rule could not be removed");
	}
	else
	{
		LOG4CXX_DEBUG(logger, "iptables rule removed: " << iptablesRule.str());
	}

	if (_tcpListener != -1)
	{
		LOG4CXX_INFO(logger, "Closing TCP listening socket " << _tcpListener);
		close(_tcpListener);
	}
	else
	{
		LOG4CXX_DEBUG(logger, "TCP listening socket already closed");
	}
}

void HttpProxy::operator()()
{
	ostringstream iptablesRule;

	// Just double check to not insert a rule for nothing
	if (_configuration.httpProxyPort() != 80)
	{
		iptablesRule << "iptables -t nat -A PREROUTING -p tcp -i "
				<< _configuration.networkDevice() << " --dport "
				<< _configuration.tcpInterceptionPort() << " -j REDIRECT"
				<< " --to-port " << _configuration.httpProxyPort();

		if (system(iptablesRule.str().c_str()) == -1)
		{
			LOG4CXX_FATAL(logger, "Could not set iptables rule to forward HTTP "
					"requests to port " << _configuration.httpProxyPort());
			return;
		}
		else
		{
			LOG4CXX_INFO(logger, "iptables rule set to forward HTTP requests on"
					" interface " << _configuration.networkDevice() << " to "
					"port "	<< _configuration.httpProxyPort());
		}
	}

	//activeSessions.startTimeoutChecker();
	int newSocketFd = -1;
	int ret;
	socklen_t clilen;
	struct sockaddr_in serverAddress, clientAddress;
	_tcpListener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (_tcpListener < 0)
	{
		LOG4CXX_FATAL(logger, "Opening TCP socket: " << strerror(errno));
	}

	LOG4CXX_DEBUG(logger, "TCP listening socket created. FD = "
			<< _tcpListener);
	bzero((char *) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(_configuration.httpProxyPort());
	ret = bind(_tcpListener, (struct sockaddr *) &serverAddress,
			sizeof(serverAddress));

	if (ret < 0)
	{
		LOG4CXX_WARN(logger, "Cannot bind to socket port "
				<< _configuration.httpProxyPort() << ": " << strerror(errno)
				<< ". Retrying every 1 second until bind was successful");

		while (errno == EADDRINUSE)
		{
			sleep(1);
			ret = bind(_tcpListener, (struct sockaddr *) &serverAddress,
					sizeof(serverAddress));

			if (ret != -1)
			{
				LOG4CXX_INFO(logger, "Socket finally bound to port "
						<< _configuration.httpProxyPort());
				break;
			}
		}
	}

	if (listen(_tcpListener, _configuration.backlog()) == -1)
	{
		LOG4CXX_FATAL(logger, "Cannot start socket lister on FD "
				<< _tcpListener);
		return;
	}

	clilen = sizeof(clientAddress);
	LOG4CXX_INFO(logger, "Listening for TCP connections");
	thread *tcpServerThread;
	TcpServer tcpServer(_configuration, _namespaces, _statistics, _run);

	while(*_run)
	{
		newSocketFd = accept(_tcpListener, (struct sockaddr *) &clientAddress,
				&clilen);

		if (newSocketFd <= 0)
		{
			LOG4CXX_ERROR(logger, "New TCP session socket could not be "
					<< "accepted: " << strerror(errno));
		}
		else
		{
			IpAddress ipAddress = clientAddress.sin_addr.s_addr;
			_statistics.ipEndpointAdd(ipAddress);
			tcpServerThread = new thread(tcpServer, newSocketFd, ipAddress);
			tcpServerThread->detach();
			delete tcpServerThread;
		}
	}
}
