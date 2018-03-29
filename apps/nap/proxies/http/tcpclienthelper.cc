/*
 * tcpclienthelper.cpp
 *
 *  Created on: 17 Aug 2017
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

#include "tcpclienthelper.hh"

using namespace proxies::http::tcpclient;

LoggerPtr TcpClientHelper::logger(Logger::getLogger("proxies.http.tcpclient"));

TcpClientHelper::TcpClientHelper(reverse_lookup_t *reverseLookup,
		mutex &tcpClientMutex)
	: _reverseLookup(reverseLookup),
	  _mutex(tcpClientMutex)
{}

TcpClientHelper::~TcpClientHelper() {}

enigma_t TcpClientHelper::enigma(socket_fd_t &socketFd)
{
	enigma_t enigma = 23;
	_mutex.lock();
	_reverseLookupIt = _reverseLookup->find(socketFd);

	if (_reverseLookupIt == _reverseLookup->end())
	{
		LOG4CXX_TRACE(logger, "Socket FD " << socketFd << " not found in "
				"reverse lookup map to obtain Enigma");
	}
	else
	{
		enigma = _reverseLookupIt->second.enigma;
	}

	_mutex.unlock();
	return enigma;
}

IcnId TcpClientHelper::rCid(socket_fd_t &socketFd)
{
	IcnId rCid;
	_mutex.lock();
	_reverseLookupIt = _reverseLookup->find(socketFd);

	if (_reverseLookupIt == _reverseLookup->end())
	{
		LOG4CXX_TRACE(logger, "Socket FD " << socketFd << " not found in "
				"reverse lookup map to obtain rCID");
	}
	else
	{
		rCid = _reverseLookupIt->second.rCId;
	}

	_mutex.unlock();
	return rCid;
}
