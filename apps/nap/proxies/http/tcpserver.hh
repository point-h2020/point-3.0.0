/*
 * tcpserver.hh
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
 * Blackadder.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NAP_PROXIES_HTTP_TCPSERVER_HH_
#define NAP_PROXIES_HTTP_TCPSERVER_HH_

#include <log4cxx/logger.h>
#include <string.h>

#include <configuration.hh>
#include <types/enumerations.hh>
#include <monitoring/statistics.hh>
#include <namespaces/namespaces.hh>
#include <proxies/http/headerparser.hh>
#include <types/ipaddress.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace log4cxx;
using namespace monitoring::statistics;
using namespace namespaces;
using namespace proxies::http::headerparser;

namespace proxies
{
namespace http
{
namespace tcpserver
{
/*!
 * \brief TCP server
 *
 * This class handles all incoming TCP sessions from IP endpoints, i.e., session
 * which have been created by the IP endpoint (HTTP request messages).
 */
class TcpServer: public HeaderParser
{
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	TcpServer(Configuration &configuration, Namespaces &namespaces,
			Statistics &statistics,	bool *run);
	/*!
	 * \brief Destructor
	 */
	~TcpServer();
	/*!
	 * \brief Functor
	 */
	void operator()(socket_fd_t socketFd, IpAddress ipAddress);
private:
	Configuration &_configuration;/*!< Reference to Configuration class */
	Namespaces &_namespaces;/*!< Reference to Namespace class */
	Statistics &_statistics;/*!< Reference to Statistics class */
	string _httpRequestFqdn;/*!< In case of an HTTP request POST || PUT this
	variable allows to handle a request which is fragmented into multiple TCP
	segments*/
	bool *_run;/*!< Capture if SIG* has been requested*/
	int _localSurrogateFd; /*!< If local surrogacy has been enabled this FD
	holds the socket */
	http_methods_t _httpRequestMethod;/*!< In case of an HTTP request POST ||
	PUT this variable allows to handle a request which is fragmented into
	multiple TCP segments*/
	string _httpRequestResource;/*!< In case of an HTTP request POST ||
	PUT this variable allows to handle a request which is fragmented into
	multiple TCP segments*/
};

} /* namespace tcpserver */

} /* namespace http */

} /* namespace proxies */

#endif /* NAP_PROXIES_HTTP_TCPSERVER_HH_ */
