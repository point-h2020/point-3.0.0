/*
 * collector.hh
 *
 *  Created on: 3 Feb 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of Blackadder.
 *
 * Blackadder is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
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

#ifndef NAP_MONITORING_COLLECTOR_HH_
#define NAP_MONITORING_COLLECTOR_HH_

#include <log4cxx/logger.h>

#include <configuration.hh>
#include <monitoring/statistics.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace log4cxx;
using namespace monitoring::statistics;

namespace monitoring {

namespace collector {
/*!
 * \brief Collect statistics from the NAP and report it to the monitoring agent
 *
 * The reporting towards the monitoring agent is implemented using MOLY
 */
class Collector {
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 *
	 * \param configuration Reference to configuration class initialised in main
	 */
	Collector(Configuration &configuration, Statistics &statistics, bool *run);
	/*!
	 * \brief Destructor
	 */
	~Collector();
	/*!
	 * \brief Functor
	 *
	 * Required to put this collector into a thread
	 */
	void operator()();
private:
	Configuration &_configuration;/*!< Reference to configuration class */
	Statistics &_statistics;/*!< Reference to statistics class */
	bool *_run;/*!< from main thread is any SIG* had been caught */
};

} /* namespace collector */

} /* namespace monitoring */

#endif /* NAP_MONITORING_COLLECTOR_HH_ */
