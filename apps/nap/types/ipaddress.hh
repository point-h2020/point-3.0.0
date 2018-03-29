/*
 * ipaddress.hh
 *
 *  Created on: 15 Feb 2016
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

#ifndef NAP_TYPES_IPADDRESS_HH_
#define NAP_TYPES_IPADDRESS_HH_

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <sstream>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace std;

class IpAddress {
public:
	/*!
	 * \brief Constructor without any arguments
	 *
	 * In order to properly initialise this class, use the overloaded '='
	 * operator implementation after calling this constructor
	 */
	IpAddress();
	/*!
	 * \brief
	 */
	IpAddress(struct in_addr *ipAddress);
	/*!
	 * \brief Constructor for IP address as string
	 *
	 * \param ipAddress The IP address as a string
	 */
	IpAddress(string ipAddress);
	/*!
	 * \brief Constructor for IP address in network byte order
	 *
	 * \param ipAddress The IP address in network byte order
	 */
	IpAddress(uint32_t ipAddress);
	/*!
	 * \brief Returns the network byte order as in_addr struct
	 *
	 * \return Network byte order IP address as type struct in_addr
	 */
	struct in_addr inAddress();
	/*!
	 * \brief Set IP address using human readable format
	 */
	void operator=(string &ipAddress);
	/*!
	 * \brief Set IP address using network byte order
	 */
	void operator=(uint32_t &ipAddress);
	/*!
	 * \brief Obtain the IP address as an Internet socket address
	 *
	 * \return The IP address
	 */
	struct sockaddr_in socketAddress();
	/*!
	 * \brief Obtain IP address in human friendly format (a.b.c.d)
	 *
	 * \return The class-full representation of the IP address as a string
	 */
	string str();
	/*!
	 * \brief Obtain IP address in network byte order
	 *
	 * \return The IP address in network byte order
	 */
	uint32_t uint();
	uint32_t *uintPointer();
private:
	uint32_t _ipAddressUint;/*!< Network byte order of the IP address */
	string _ipAddressString;/*!< Class-full representation of the IP address */
};

#endif /* NAP_TYPES_IPADDRESS_HH_ */
