/*
 * bampers.hh
 *
 *  Created on: Nov 25, 2015
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of BlAckadder Monitoring wraPpER clasS (BAMPERS).
 *
 * BAMPERS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * BAMPERS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * BAMPERS. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_BAMPERS_HH_
#define LIB_BAMPERS_HH_

#include <blackadder.hpp>
#include <boost/thread.hpp>

#include "cidanalyser.hh"
#include "datapoints/link.hh"
#include "datapoints/node.hh"
#include "datapoints/port.hh"
#include "datapoints/statistics.hh"
#include "namespace.hh"
#include "types/icnid.hh"

/*!
 * \brief Main class used by the monitoring agent
 *
 * This sub-class allows the monitoring agent to access all public classes
 * referenced below.
 */
class Bampers : public Link, public Node, public Port, public Statistics
{
public:
	/*!
	 * \brief Constructor For monitoring agent and monitoring server
	 */
	Bampers(Namespace &namespaceHelper);
	/*!
	 * \brief Destructor
	 */
	~Bampers();
	/*!
	 * \brief Shutting down Blackadder
	 */
	void shutdown(Blackadder *blackadder);
private:
	Namespace &_namespaceHelper; /*!< Reference to the class Namespace */
	Blackadder *_blackadder;/*!< Pointer to Blackadder API */
	/*!
	 * \brief
	 */
	void startIcnEventHandlerThread();
};

#endif /* :IB_BAMPERS_HH_ */
