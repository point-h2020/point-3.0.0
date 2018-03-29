/*
 * httpproxy.hh
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

#ifndef NAP_PROXIES_HTTP_HTTPPROXY_HH_
#define NAP_PROXIES_HTTP_HTTPPROXY_HH_

//#include <boost/thread.hpp>
#include <log4cxx/logger.h>
#include <sstream>
#include <thread>
#include <vector>

#include <configuration.hh>
#include <monitoring/statistics.hh>
#include <namespaces/namespaces.hh>
#include <proxies/http/tcpserver.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace log4cxx;
using namespace monitoring::statistics;
using namespace namespaces;
using namespace std;

namespace proxies {

namespace http {

class HttpProxy
{
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	HttpProxy(Configuration &configuration, Namespaces &namespaces,
			Statistics &statistics, bool *run);
	/*!
	 * \brief Destructor
	 */
	~HttpProxy();
	/*!
	 * \brief Remove iptables rule to forward TCP packets destined to Port 80 to
	 * the transparent proxy port. This method also closes the TCP listening
	 * socket.
	 *
	 * \return void
	 */
	void tearDown();
	/*!
	 * \brief Functor
	 *
	 * This functor is required to call a method from main.cc which initialises
	 * the HTTP proxy
	 */
	void operator()();
private:
	Configuration &_configuration; /*!< Reference to configuration class
	*/
	Namespaces &_namespaces;/*!< Reference to namespaces */
	Statistics &_statistics;/*!< Reference to statistics class */
	bool *_run;/*!< SIG* received and termination requested */
	int _tcpListener;/*!< Socket FD listening for new TCP clients*/
};

} /* namespace http */

} /* namespace proxies */

#endif /* NAP_PROXIES_HTTP_HTTPPROXY_HH_ */
