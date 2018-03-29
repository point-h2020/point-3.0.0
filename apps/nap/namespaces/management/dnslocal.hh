/*
 * dnslocal.hh
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

#ifndef NAP_NAMESPACES_MANAGEMENT_DNSLOCAL_HH_
#define NAP_NAMESPACES_MANAGEMENT_DNSLOCAL_HH_

#include <blackadder.hpp>
#include <log4cxx/logger.h>
#include <mutex>

#include <configuration.hh>
#include <types/icnid.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace log4cxx;

namespace namespaces {

namespace management {

namespace dnslocal {
/*!
 * \brief Implementation of the DNS local namespace which is used by the NAP-SA
 * interface.
 *
 * For information on this namespace can be found in the documentation under
 * /doc/tex
 */
class DnsLocal {
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 *
	 * This method basically subscribes to /management/dnsLocal to be able to
	 * receive triggers about flushing FIDs stored in the local ICN core by re-
	 * publishing the HTTP CIDs
	 */
	DnsLocal(Blackadder *icnCore, std::mutex &icnCoreMutex, Configuration
			&configuration);
	/*!
	 * \brief Destructor
	 */
	~DnsLocal();
protected:
	/*!
	 * \brief Announcement of a particular FQDN to all NAPs to flush their FID
	 * in case they are publishing to this FQDN
	 *
	 * \param hashedFqdn The FQDN which should be announced
	 */
	void announce(uint32_t hashedFqdn);
	/*!
	 * \brief Set forwarding for /managmeent/dnslocal CID
	 *
	 * \param state The state which should be set for the DNSlocal CID
	 */
	void forwarding(bool state);
private:
	Blackadder *_icnCore; /*!< Pointer to Blackadder instance */
	std::mutex &_icnCoreMutex;/*!< Mutex to lock writing operations on the
	Blackadder API */
	Configuration &_configuration;/*!< Reference to the Configuration class */
	IcnId _cid;/*!< The CID under which DNS local announcement are made */
};

} /* namespace dnslocal */

} /* namespace management */

} /* namespace namespaces */

#endif /* NAP_NAMESPACES_MANAGEMENT_DNSLOCAL_HH_ */
