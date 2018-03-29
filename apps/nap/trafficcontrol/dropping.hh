/*
 * dropping.hh
 *
 *  Created on: 24 Aug 2016
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

#ifndef NAP_TRAFFICCONTROL_DROPPING_HH_
#define NAP_TRAFFICCONTROL_DROPPING_HH_

#include <boost/thread/mutex.hpp>
#include <log4cxx/logger.h>

#include <configuration.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace log4cxx;

namespace trafficcontrol {

namespace dropping {
/*!
 * \brief Implementation of packet dropping control
 */
class Dropping {
	static log4cxx::LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	Dropping(Configuration &configuration);
	/*!
	 * \brief Destructor
	 */
	~Dropping();
protected:
	/*!
	 * \brief Obtain whether or not a packet shall be dropped
	 */
	bool dropPacket();
private:
	Configuration &_configuration;/*!< Reference to configuration class holding
	all settings the user provided */
};

} /* namespace dropping */

} /* namespace trafficcontrol */

#endif /* NAP_TRAFFICCONTROL_DROPPING_HH_ */
