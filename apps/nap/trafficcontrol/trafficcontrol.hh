/*
 * trafficcontrol.hh
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

#ifndef NAP_TRAFFICCONTROL_TRAFFICCONTROL_HH_
#define NAP_TRAFFICCONTROL_TRAFFICCONTROL_HH_

#include <log4cxx/logger.h>

#include <configuration.hh>
#include <trafficcontrol/dropping.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace log4cxx;
using namespace trafficcontrol::dropping;

namespace trafficcontrol {
/*!
 * \brief Traffic control implementation
 */
class TrafficControl: public Dropping {
	static log4cxx::LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	TrafficControl(Configuration &configuration);
	/*!
	 * \brief Destructor
	 */
	~TrafficControl();
	/*!
	 * \brief Handle a packet
	 *
	 * This method performs all sorts of traffic control. More information can
	 * be expected once a traffic control other than dropping has been
	 * implemented.
	 *
	 * \return Boolean indicating of packet can be published
	 */
	bool handle();
};

} /* namespace trafficcontrol */

#endif /* NAP_TRAFFICCONTROL_TRAFFICCONTROL_HH_ */
