/*
 * tcpclientread.hh
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

#ifndef NAP_PROXIES_HTTP_TCPCLIENTREAD_HH_
#define NAP_PROXIES_HTTP_TCPCLIENTREAD_HH_

#include <log4cxx/logger.h>
#include <mutex>
#include <thread>

#include <configuration.hh>
#include <proxies/http/headerparser.hh>
#include <namespaces/namespaces.hh>
#include <monitoring/statistics.hh>
#include <proxies/http/tcpclienthelper.hh>
#include <types/nodeid.hh>
#include <types/typedef.hh>

using namespace log4cxx;
using namespace proxies::http::headerparser;
using namespace std;

namespace proxies {

namespace http {

namespace tcpclient {

/*!
 * \brief Implementation of the "read() function in a thread" TCP client
 */
class TcpClientRead: public HeaderParser, TcpClientHelper
{
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	TcpClientRead(Configuration &configuration, Namespaces &namespaces,
			Statistics &statistics, reverse_lookup_t *reverseLookup,
			socket_fds_t *socketFds, socket_state_t *socketState,
			mutex &tcpClientMutex, bool *run);
	/*!
	 * \brief Virtual destructor
	 */
	~TcpClientRead();
	/*!
	 * \brief Functor to run the read() call in a thread
	 */
	void operator()(socket_fd_t socketFd, socket_fd_t remoteSocketFd,
			NodeId nodeId, bool mpdRequest);
private:
	Configuration &_configuration;
	Namespaces &_namespaces;
	Statistics &_statistics;
	reverse_lookup_t *_reverseLookup;
	reverse_lookup_t::iterator _reverseLookupIt;/*!< Iterator for
	_reverseLookup map */
	socket_fds_t *_socketFds;
	socket_fds_t::iterator _socketFdsIt;/*!< Iterator for _socketFdMap */
	socket_state_t *_socketState;
	socket_state_t::iterator _socketStateIt;/*!< Iterator for _socketState map*/
	mutex &_mutex;
	bool *_run;
	/*!
	 * \brief Delete a socket FD from the socket state map
	 *
	 * \param socketFd The FD to be deleted
	 */
	void _deleteSocketState(socket_fd_t &socketFd);
	/*!
	 * \brief Set socket state
	 */
	void _setSocketState(socket_fd_t &socketFd, bool state);
};

} /* namsepace tcpclient */

} /* namsepace http */

} /* namespace proxies */

#endif /* NAP_PROXIES_HTTP_TCPCLIENTREAD_HH_ */
