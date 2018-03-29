/*
 * filesystemchecker.hh
 *
 *  Created on: 1 Jan 2017
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

#ifndef APPS_MONITORING_SERVER_FILESYSTEMCHECKER_HH_
#define APPS_MONITORING_SERVER_FILESYSTEMCHECKER_HH_

#include <algorithm>
#include <bampers/bampers.hh>
#include <blackadder.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <log4cxx/logger.h>
#include <iostream>
#include <iterator>

#include "../types/enumerations.hh"
#include "../types/scopes.hh"

using namespace boost::filesystem;
using namespace std;

/*!
 * \brief API for visualisation tool to enable/disable data points
 */
class FilesystemChecker {
public:
	/*!
	 * \brief Constructor
	 */
	FilesystemChecker(log4cxx::LoggerPtr logger, string directory,
			Scopes &scopes, Blackadder *icnCore);
	/*!
	 * \brief Destructor
	 */
	~FilesystemChecker();
	/*!
	 * \brief Functor to run in thread
	 */
	void operator()();
private:
	log4cxx::LoggerPtr _logger;/*!< Pointer to logger class*/
	string _directory;/*!< Directory to receive commands from visualisation
	tool */
	Scopes &_scopes;/*!< Reference to the scopes directory*/
	Blackadder *_icnCore;/*!< Pointer to the Blackadder API*/
	/*!
	 * \brief Obtain the enumerated BAMPERS information item to subscribe to
	 *
	 * \param dataPoint The data point (attribute) for which the BAMPERS
	 * information item should be determined
	 *
	 * \return The BAMPERS information item
	 */
	bampers_info_item_t _attribute(uint16_t dataPoint);
	/*!
	 * \brief Obtain the node's role from the attribute/data point (moly/enum.hh)
	 *
	 * \param dataPoint The data point which allows to determine the node role
	 *
	 * \return The node role
	 */
	node_role_t _nodeRole(uint16_t dataPoint);
};

#endif /* APPS_MONITORING_SERVER_FILESYSTEMCHECKER_HH_ */
