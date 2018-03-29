/*
 * routingprefix.hh
 *
 *  Created on: 17 April 2016
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

#ifndef APPS_NAP_TYPES_ROUTINGPREFIX_HH_
#define APPS_NAP_TYPES_ROUTINGPREFIX_HH_

#include <iostream>
#include <string>

#include "ipaddress.hh"
#include "netmask.hh"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace std;

/*!
 * \brief Routing prefix class
 */
class RoutingPrefix {
public:
	/*!
	 * \brief Constructor without any argument
	 *
	 * Netmask and network address must be added using networkAddress(string)
	 * and netmask(string) methods
	 */
	RoutingPrefix();
	/*!
	 * \brief Constructor to add network address and its netmask in human
	 * readable format
	 */
	RoutingPrefix(string networkAddress, string netmask);
	/*!
	 * \brief Obtain the netmask of this routing prefix
	 */
	Netmask netmask();
	/*!
	 * \brief Set the netmask for this routing prefix
	 */
	void netmask(string netmask);
	/*!
	 * \brief Obtaining the set network address for this routing prefix
	 *
	 * \return Returns the network address of type IpAddress()
	 */
	IpAddress networkAddress();
	/*!
	 * \brief
	 */
	void networkAddress(string networkAddress);
	/*!
	 * \brief Obtain the routing prefix in network byte order
	 *
	 * This method returns the network address with the netmask applied
	 *
	 * \return Routing prefix in network byte order
	 */
	uint32_t uint();
	/*!
	 * \brief Obtain the routing prefix in human readable format
	 *
	 * This method allows to obtain the routing prefix in human readable format,
	 * i.e., a.b.c.d/XX where a.b.c.d is the network address and XX the netmask
	 * in CIDR format
	 *
	 * \return Routing prefix in human readable format
	 */
	string str();
private:
	IpAddress _networkAddress;
	Netmask _netmask;
	string _routingPrefixString;
	uint32_t _routingPrefixUint; /*!< The routing prefix obtained by applying
	netmask on the network address when calling the constructor */
};

#endif /* APPS_NAP_TYPES_ROUTINGPREFIX_HH_ */
