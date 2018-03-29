/*
 * ipbuffercleaner.hh
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

#ifndef NAP_NAMESPACES_BUFFERCLEANERS_IPBUFFERCLEANER_HH_
#define NAP_NAMESPACES_BUFFERCLEANERS_IPBUFFERCLEANER_HH_

#include <boost/date_time.hpp>
#include <log4cxx/logger.h>
#include <mutex>
#include <unordered_map>

#include <configuration.hh>
#include <monitoring/statistics.hh>
#include <namespaces/iptypedef.hh>
#include <types/icnid.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define ENIGMA 23 	// https://en.wikipedia.org/wiki/23_enigma ... and don't
					//forget, it's also Sebastian's bday

using namespace configuration;
using namespace monitoring::statistics;
using namespace std;

namespace cleaners {

namespace ipbuffer {
/*!
 * \brief Implementation of the IP handler buffer cleaner
 */
class IpBufferCleaner
{
static log4cxx::LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	IpBufferCleaner(void *buffer, void *bufferMutex,
			Configuration &configuration, Statistics &statistics, bool *run);
	/*!
	 * \brief Destructor
	 */
	~IpBufferCleaner();
	/*!
	 * \brief Functor to place this into a thread
	 */
	void operator()();
private:
	packet_buffer_t *_buffer;
	packet_buffer_t::iterator _bufferIt;/*!<
	Iterator for buffer map */
	Configuration &_configuration;/*!< Reference to configuration class */
	Statistics &_statistics;/*!< Reference to statistics class */
	bool *_run;/*!< from main thread is any SIG* had been caught */
	std::mutex *_mutex;/*!< Reference to IP packet buffer mutex */
};

} /* namespace ipbuffer */

} /* namespace cleaners */

#endif /* NAP_NAMESPACES_BUFFERCLEANERS_IPBUFFERCLEANER_HH_ */
