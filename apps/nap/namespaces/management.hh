/*
 * management.hh
 *
 *  Created on: 26 Aug 2016
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

#ifndef NAP_NAMESPACES_MANAGEMENT_HH_
#define NAP_NAMESPACES_MANAGEMENT_HH_

#include <blackadder.hpp>
#include <mutex>

#include <configuration.hh>
#include <namespaces/management/dnslocal.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace namespaces::management::dnslocal;

namespace namespaces {

namespace management {
/*!
 * \brief Implementation of the namespace management which holds many other base
 * classes
 */
class Management: public DnsLocal
{
public:
	/*!
	 * \brief Constructor
	 *
	 * \param icnCore Pointer to Blackadder instance
	 * \param icnCoreMutex Reference to mutex for transaction safe BA API
	 * write operations
	 * \param configuration Reference to the configuration class
	 */
	Management(Blackadder *icnCore, std::mutex &icnCoreMutex,
			Configuration &configuration);
	/*!
	 * \brief Destructor
	 */
	~Management();
	/*!
	 * \brief Set forwarding state for a particular CID
	 *
	 * \param cid The CID for which the forwarding state should be changed
	 * \param state The state
	 */
	void forwarding(IcnId &cid, bool state);
};

} /* namespace management */

} /* namespace namespaces */

#endif /* NAP_NAMESPACES_MANAGEMENT_HH_ */
