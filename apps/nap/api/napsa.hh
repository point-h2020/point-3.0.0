/*
 * napsa.hh
 *
 *  Created on: 24 May 2016
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

#ifndef NAP_API_NAPSA_HH_
#define NAP_API_NAPSA_HH_

#include <errno.h>
#include <linux/netlink.h>
#include <log4cxx/logger.h>
#include <string.h>
#include <sys/socket.h>

#include <types/enumerations.hh>
#include <types/ipaddress.hh>
#include <namespaces/namespaces.hh>
#include <monitoring/statistics.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace namespaces;
using namespace log4cxx;

namespace api
{

namespace napsa
{
/*!
 * \brief Implementation of the NAP_SA API
 *
 * This class is only called in a thread if the respective variable has been set
 * in nap.cfg
 */
class NapSa
{
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	NapSa(Namespaces &namespaces, Statistics &statistics);
	/*!
	 * \brief Deconstructor
	 */
	~NapSa();
	/*!
	 * \brief Functor to listen for messages from surrogate agent
	 */
	void operator()();
private:
	Namespaces &_namespaces;/*!< Reference to namespaces */
	Statistics &_statistics;/*!< Reference to statistics class */
	int _listeningSocket; /*!< The NAP listening socket for SA messages */
};

} /* namespace napsa */

} /* namesopace api */

#endif /* NAP_API_NAPSA_HH_ */
