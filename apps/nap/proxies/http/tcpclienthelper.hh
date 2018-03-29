/*
 * tcpclienthelper.hh
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

#ifndef NAP_PROXIES_HTTP_TCPCLIENTHELPER_HH_
#define NAP_PROXIES_HTTP_TCPCLIENTHELPER_HH_

#include <log4cxx/logger.h>
#include <mutex>
#include <thread>

#include <proxies/http/httpproxytypedef.hh>
#include <types/icnid.hh>
#include <types/typedef.hh>

using namespace log4cxx;
using namespace std;

namespace proxies {

namespace http {

namespace tcpclient {

/*!
 * \brief Helper methods related to the TCP client implementation
 */
class TcpClientHelper
{
	static LoggerPtr logger;/*!< log4cxx instance*/
public:
	/*!
	 * \brief Constructor
	 */
	TcpClientHelper(reverse_lookup_t *reverseLookup, mutex &tcpClientMutex);
	/*!
	 * \brief Destructor
	 */
	~TcpClientHelper();
	/*!
	 * \brief Obtain the Enigma from the _reverseLookup map
	 *
	 * \param socketFd The socket FD for which the Enigma should be obtained
	 *
	 * \return Reference to Enigma
	 */
	enigma_t enigma(socket_fd_t &socketFd);
	/*!
	 * \brief Obtain the rCID from the _reverseLookup map
	 *
	 * \param socketFd The socket FD which should be looked up
	 *
	 * \return rCID which has been stored for this socket FD
	 */
	IcnId rCid(socket_fd_t &socketFd);
private:
	reverse_lookup_t *_reverseLookup;
	reverse_lookup_t::iterator _reverseLookupIt;/*!< Iterator for
	_reverseLookup map */
	std::mutex &_mutex; /*!< Mutex for all shared maps operations*/
};

} /* namsepace tcpclient */

} /* namsepace http */

} /* namespace proxies */

#endif /* NAP_PROXIES_HTTP_TCPCLIENTHELPER_HH_ */
