/*
 * surrogatetcpclient.hh
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

#ifndef PROXIES_HTTP_SURROGATETCPCLIENT_HH_
#define PROXIES_HTTP_SURROGATETCPCLIENT_HH_

#include <arpa/inet.h>
#include <errno.h>
#include <log4cxx/logger.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <configuration.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace log4cxx;

namespace proxies {

namespace http {

namespace surrogatetcpclient {
/*!
 * \brief Surrogate TCP client implementation
 */
class SurrogateTcpClient {
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 *
	 * \param configuration Reference to class Configuration in order to obtain
	 * TCP client socket buffer size
	 * \param clientSocketFd The socket file descriptor for the client awaiting
	 * the HTTP response
	 * \param surrogateSocketFd The socket file descriptor for the connection to
	 * the surrogate server
	 */
	SurrogateTcpClient(Configuration &configuration, int clientSocketFd,
			int surrogateSocketFd);
	/*!
	 * \brief Destructor
	 */
	~SurrogateTcpClient();
	/*!
	 * \brief Functor to call this class in a Boost thread
	 */
	void operator()();
private:
	Configuration &_configuration;/*!< Reference to Configuration class*/
	int _clientSocketFd;/*!< The socket FD for the client */
	int _surrogateSocketFd; /*!< The socket FD for the surrogate server */
};

} /* namespace surrogatetcpclient */

} /* namespace http */

} /* namespace proxies */

#endif /* PROXIES_HTTP_SURROGATETCPCLIENT_HH_ */
