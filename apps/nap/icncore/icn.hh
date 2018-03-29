/*
 * icn.hh
 *
 *  Created on: 25 Apr 2016
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

#ifndef NAP_ICN_HH_
#define NAP_ICN_HH_

#include <blackadder.hpp>
#include <log4cxx/logger.h>
#include <mutex>
#include <thread>

#include <configuration.hh>
#include <types/enumerations.hh>
#include <monitoring/statistics.hh>
#include <namespaces/namespaces.hh>
#include <transport/transport.hh>
#include <types/icnid.hh>
#include <vector>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace transport;

namespace icn
{
/*!
 * \brief Implementation of the ICN handler
 */
class Icn
{
	static log4cxx::LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	Icn(Blackadder *icnCore, Configuration &configuration,
			Namespaces &namespaces, Transport &transport,
			Statistics &statistics, bool *run);
	/*!
	 * \brief Destructor
	 */
	~Icn();
	/*!
	 * \brief Functor to run this method in a boost thread
	 */
	void operator()();
private:
	Blackadder *_icnCore; /*!< Pointer to Blackadder instance */
	Configuration &_configuration; /*!< Reference to Configuration class */
	Namespaces &_namespaces; /*!< Reference to ICN Namespaces */
	Transport &_transport; /*!< Reference to Transport class */
	Statistics &_statistics;/*!< Reference to Statistics class */
	bool *_run;
};

} /* namespace icn */

#endif /* NAP_ICN_HH_ */
