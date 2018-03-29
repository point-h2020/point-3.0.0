/*
 * netmask.hh
 *
 *  Created on: 17 Apr 2016
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

#ifndef NAP_TYPES_NETMASK_HH_
#define NAP_TYPES_NETMASK_HH_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>
#include <string>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace std;

/*!
 * \brief Implementation of the netmask class
 */
class Netmask {
public:
	Netmask();
	/*!
	 * \brief Constructor with netmask as its argument
	 */
	Netmask(string netmask);
	/*!
	 * \brief Obtain the netmask in CIDR format
	 *
	 * \return The netmask in CIDR format
	 */
	uint8_t cidr();
	/*!
	 * \brief Overloading assignment operator =
	 *
	 * This method allows to initialise the netmask using the empty constructor
	 * and assign the netmask using '=', e.g.:
	 */
	void operator=(string &netmask);
	/*!
	 * \brief Overloading assigment operator '=' to initialise this class with
	 * an uint32_t
	 */
	void operator=(uint32_t &netmask);
	/*!
	 * \brief
	 */
	uint32_t uint();
	/*!
	 * \brief
	 */
	string str();
private:
	uint32_t _netmaskUint; /*!< */
	string _netmaskString; /*!< */
	uint8_t _cidrUint; /*!< Class-full inter-domain routing representation of the
	netmask */
	/*!
	 * \brief Convert human friendly netmask to CIDR
	 */
	void _cidr();
};

#endif /* NAP_TYPES_NETMASK_HH_ */
