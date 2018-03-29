/*
 * httpbuffercleaner.hh
 *
 *  Created on: 7 Jul 2016
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

#ifndef NAMESPACES_BUFFERCLEANERS_HTTPBUFFERCLEANER_HH_
#define NAMESPACES_BUFFERCLEANERS_HTTPBUFFERCLEANER_HH_

#include <boost/date_time.hpp>
#include <log4cxx/logger.h>
#include <mutex>

#include <configuration.hh>
#include <namespaces/httptypedef.hh>
#include <monitoring/statistics.hh>
#include <types/icnid.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace log4cxx;
using namespace monitoring::statistics;

namespace cleaners {

namespace httpbuffer {
/*!
 * \brief HTTP buffer cleaner
 */
class HttpBufferCleaner {
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 *
	 * \param configuration Reference to class Configuration
	 * \param statistics Reference to class Statistics
	 * \param bufferRequests Reference to HTTP request buffer
	 * \param mutexRequests Reference to HTTP request buffer mutex
	 * \param bufferResponses Reference to HTTP response buffer
	 * \param mutexResposnes Reference to HTTP response buffer mutex
	 * \param run Stop thread (SIG* received)
	 */
	HttpBufferCleaner(Configuration &configuration, Statistics &statistics,
			packet_buffer_requests_t &bufferRequests,
			std::mutex &mutexRequests,
			//packet_buffer_responses_t &bufferResponses,
			//std::mutex &mutexResposnes,
			bool *run);
	/*!
	 * \brief Destructor
	 */
	~HttpBufferCleaner();
	/*!
	 * \brief Functor to place this into a thread
	 */
	void operator()();
private:
	Configuration &_configuration;
	Statistics &_statistics;
	packet_buffer_requests_t &_bufferRequests; /*!< Buffer for
	HTTP requests messages to be published  */
	packet_buffer_requests_t::iterator _bufferRequestsIt;/*!<
	Iterator for _packetBuffer map */
	std::mutex &_mutexRequests;/*!< Reference to HTTP requests packet buffer
	mutex */
	//packet_buffer_responses_t &_bufferResponses; /*!< Buffer for
	//HTTP responses messages to be published  */
	//packet_buffer_responses_t::iterator _bufferResponsesIt;/*!< Iterator for
	//_bufferResponses*/
	//std::mutex &_mutexResponses;/*!< Reference to HTTP responses packet buffer
	//mutex */
	bool *_run;/*!< from main thread is any SIG* had been caught */
	/*!
	 * \brief Clean up HTTP request buffer
	 */
	void _cleanUpRequests();
	/*!
	 * \brief Clean up HTTP response buffer
	 */
	void _cleanUpResponses();
};

} /* namespace httpbuffer */

} /* namespace cleaners */

#endif /* NAMESPACES_BUFFERCLEANERS_HTTPBUFFERCLEANER_HH_ */
