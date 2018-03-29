/*
 * icnid.hh
 *
 *  Created on: 03 June 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of BAMPERS.
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

#ifndef APPS_MONITORING_SERVER_TYPES_ICNID_HH_
#define APPS_MONITORING_SERVER_TYPES_ICNID_HH_

#include <blackadder.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

/*!
 * \brief Implementation of a helper class to work with CIDs and scope paths.
 *
 * The class has been called ICN ID, as it can handle individual IDs, scope
 * paths and full CIDs
 */
class IcnId {
public:
	/*!
	 * \brief Constructor
	 */
	IcnId();
	/*!
	 * \brief Destructor
	 */
	~IcnId();
	/*!
	 * \brief append ICN ID by an identifier
	 */
	void append(uint32_t identifier);
	/*!
	 * \brief Obtain an empty string in binary format
	 *
	 * This method basically calls hex_to_charray(string())
	 */
	string binEmpty();
	/*!
	 * \brief Obtain binary representation of the entire ICN ID
	 */
	string binIcnId();
	/*!
	 * \brief Obtain the binary format representation of the ID
	 *
	 * This method takes the last 16 characters of the ICN ID and converts it
	 * to the binary format required by the Blackadder API
	 */
	string binId();
	/*!
	 * \brief Obtain the binary format representation of the prefix ID
	 *
	 * This method takes the ICN ID, except the last 16 characters, and converts
	 * it to the binary format required by the Blackadder API
	 */
	string binPrefixId();
	/*!
	 * \brief Obtain the binary format representation of the root scope ID
	 *
	 * This method takes the first 16 characters of the ICN ID and converts it
	 * to a binary format representation.
	 */
	string binRootScopeId();
	/*!
	 * \brief Obtain the last 16 characters of the ICN ID
	 */
	string id();
	/*!
	 * \brief Overloading of the equal sign operator for type const char
	 */
	void operator=(const char *c);
	/*!
	 * \brief Overloading of the equal sign operator for type string
	 *
	 * The overloading allows to set the ICN ID. For instance, when receiving an
	 * event from Blackadder, the resulting string of hex_to_chararray can be
	 * assigned to the ICN ID using this overloading method.
	 */
	void operator=(string &str);
	/*!
	 * \brief Obtain the prefix ID
	 */
	string prefixId();
	/*!
	 * \brief Print out the ICN ID in human readable format
	 *
	 * This method adds a forward slash every 16 characters (length of an ID) to
	 * indicate scope levels
	 */
	string print();
	/*!
	 * \brief Create a new ICN ID using a predefined namespace
	 *
	 * Blackadder comes with a system-wide enumeration of root namespaces which
	 * can be used here to create the root scope ID of an ICN ID. Note, this
	 * method overwrites the entire ICN ID
	 */
	void rootNamespace(root_namespaces_t rootNamespace);
	/*!
	 * \brief Obtain the ID of the root scope
	 *
	 * The first 16 characters of the ICN ID are returned
	 */
	string rootScopeId();
	/*!
	 * \brief Obtain the number of scope levels the ICN ID holds
	 */
	uint8_t scopeLevels();
	/*!
	 * \brief Obtain the ICN ID as a string
	 */
	string str();
	/*!
	 * \brief Obtain the hashed version of the ICN ID
	 */
	uint32_t uint();
private:
	string _icnIdString = string();
	uint32_t _icnIdHashed = 0;
	/*!
	 * \brief Private method to hash the text stored in _icnIdString
	 */
	void _hashIcnId();
};

#endif /* APPS_MONITORING_SERVER_TYPES_ICNID_HH_ */
