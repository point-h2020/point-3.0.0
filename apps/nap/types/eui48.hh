/*
 * eui48.hh
 *
 *  Created on: 8 Sep 2016
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

#ifndef NAP_TYPES_EUI48_HH_
#define NAP_TYPES_EUI48_HH_

#include <sstream>
#include <string>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace std;

/*!
 * \brief EUI48 Class to handle MAC addresses
 */
class Eui48 {
public:
	/*!
	 * \brief Constructor
	 */
	Eui48();
	/*!
	 * \brief Overloaded constructor to allow initialising a MAC address using
	 * the format without any separator, e.g. 0a1b2c3d4e5f
	 */
	Eui48(char *mac);
	/*!
	 * \brief Destructor
	 */
	~Eui48();
	/*!
	 * \brief Initialising this class with an EUI48 address in form of a char
	 * pointer
	 *
	 * \param eui48 The EUI48 address which should be assigned to this class
	 */
	void operator=(char *eui48);
	/*!
	 * \brief Initialising this class with an EUI48 address in form of a char
	 * pointer
	 *
	 * \param eui48 The EUI48 address which should be assigned to this class
	 */
	void operator=(const char *eui48);
	/*!
	 * \brief Initialising this class with an EUI48 address in form of a string
	 * reference
	 *
	 * \param eui48 The EUI48 address which should be assigned to this class
	 */
	void operator=(string &eui48);
	/*!
	 * \brief Obtain the EUI48 address as a string (human readable format),
	 * e.g. 0a:1b:2c:3d:4d:5e:6f
	 *
	 * \return String
	 */
	string str();
private:
	string _eui48String;
};

#endif /* NAP_TYPES_EUI48_HH_ */
