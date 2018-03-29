/*
 * reporter.hh
 *
 *  Created on: 12 Oct 2017
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the ICN application MOnitOring SErver (MOOSE) which
 * comes with Blackadder.
 *
 * MOOSE is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOOSE is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOOSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SERVER_MONITORING_REPORTER_HH_
#define SERVER_MONITORING_REPORTER_HH_

#include <log4cxx/logger.h>
#include <mapiagent.hpp>
#include <moly/moly.hh>
#include <thread>

#include "statistics.hh"

namespace moose {

class Reporter {
public:
	Reporter(log4cxx::LoggerPtr logger, moose::Statistics &statistics);
	virtual ~Reporter();
	/*!
	 * \brief Functor to run in thread
	 */
	void operator()();
private:
	log4cxx::LoggerPtr _logger;/*!<  */
	moose::Statistics &_statistics;/*!< Reference to Statistics class*/
};

} /* namespace moose */

#endif /* SERVER_MONITORING_REPORTER_HH_ */
