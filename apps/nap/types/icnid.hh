/*
 * icnid.hh
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

#ifndef NAP_TYPES_ICNID_HH_
#define NAP_TYPES_ICNID_HH_

#include <blackadder_enums.hpp>
#include <boost/date_time.hpp>
#include <iomanip>
#include <iostream>
#include <string>

#include "ipaddress.hh"
#include "routingprefix.hh"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace std;

/*!
 * \brief
 */
class IcnId {
public:
	/*!
	 * \brief Default constructor
	 *
	 * Properly initialise the class with the equal sign operator which sets the
	 * CID
	 */
	IcnId();
	/*!
	 * \brief Initialising the class with a routing prefix
	 *
	 * This constructor automatically converts the routing prefix into a valid
	 * C ID suitable for the IP handler
	 */
	IcnId(RoutingPrefix routingPrefix);
	/*!
	 * \brief Create a CID from a hashed FQDN only (/http/hashedFqdn)
	 *
	 * \param hashedFqdn Hashed version of an FQDN
	 */
	IcnId(uint32_t hashedFqdn);
	/*!
	 * \brief Initialising the class with a routing prefix and IP address
	 *
	 * This constructor automatically generates a valid CID for the IP
	 * handler out of a routing prefix and the IP address underneath
	 */
	IcnId(RoutingPrefix routingPrefix, IpAddress ipAddress);
	/*!
	 * \brief Initialising the class with a enumerated root namespace and a
	 * corresponding information item
	 *
	 * \param rootNamespace One of the root namespaces of the enumeration
	 * RootNamespaces in blackadder_enums.hpp
	 * \param informationItem One of the corresponding information items of the
	 * InformationItems enumeration
	 */
	IcnId(root_namespaces_t rootNamespace, information_items_t informationItem);
	/*!
	 * \brief Initialising the class with an FQDN
	 *
	 * This constructor automatically generates a valid CID for the HTTP handler
	 * out of an FQDN. The CID's purpose is for publishing an HTTP request to
	 * the network.
	 */
	IcnId(string fqdn);
	/*!
	 * \brief Initialising the class with an FQDN and an HTTP resource
	 *
	 * This constructor automatically generates a valid CID for the HTTP handler
	 * out of an FQDN and an HTTP resource. The CID's purpose is for publishing
	 * an HTTP response the network.
	 */
	IcnId(string fqdn, string resource);
	/*!
	 * \brief Forcefully set access timestamp
	 */
	void access();
	/*!
	 * \brief
	 *
	 * \return
	 */
	const string binEmpty();
	/*!
	 * \brief
	 *
	 * \return
	 */
	const string binIcnId();
	/*!
	 * \brief
	 *
	 * \return
	 */
	const string binId();
	/*!
	 * \brief
	 *
	 * \return
	 */
	const string binPrefixId();
	/*!
	 * \brief
	 *
	 * \return
	 */
	const string binRootScopeId();
	/*!
	 * \brief Obtain the scope ID for a given scope level in binary format
	 *
	 * This method takes advantage of hex_to_chararray from Blackadder.
	 *
	 * \param scopeLevel The scope level which should be converted to binary
	 *
	 * \return The scope ID of the scope level requested in binary format
	 */
	const string binScopeId(unsigned int scopeLevel);
	/*!
	 * \brief Obtain the scope path up to the level provided as an argument in
	 * binary format
	 *
	 * In order to publish the entire scope path into RV each publish_scope()
	 * primitive requires the scope ID which should reside under a father scope.
	 * The father scope can be obtained through this method by simply using the
	 * scope level as an integer number. The number provided determines the
	 * length of the scope path *including* the scope level the integer number
	 * refers to. The method starts counting at 1 for the root scope.
	 *
	 * \param scopeLevel The scope level up to which the CID should be
	 * converted to binary
	 *
	 * \return The scope path in binary format
	 */
	const string binScopePath(unsigned int scopeLevel);
	/*!
	 * \brief
	 *
	 * \return
	 */
	const string id();
	/*!
	 * \brief Check if ICN ID is empty
	 *
	 * \return true if ICN ID does not hold any scope ID at least
	 */
	bool empty();
	/*!
	 * \brief Obtain the state whether or not a FID has been already requested
	 * for the CID this object holds
	 */
	bool fidRequested();
	/*!
	 * \brief Set the state that an FID has been requested for this CID this
	 * object holds
	 *
	 * \param state The state which should be stored
	 */
	void fidRequested(bool state);
	/*!
	 * \brief
	 *
	 * \return
	 */
	bool forwarding();
	/*!
	 * \brief
	 *
	 * \return void
	 */
	void forwarding(bool status);
	/*!
	 * \brief Obtian the time stamp of when this ICN ID has been accessed last
	 *
	 * \return The time when this ICN ID was last accessed
	 */
	const boost::posix_time::ptime lastAccessed();
	/*!
	 * \brief Obtain the length of the content identifier in binary format
	 *
	 * \return The CID length
	 */
	uint32_t length();
	/*!
	 * \brief Populating class with new content identifier
	 *
	 * When the empty constructor IcnId() has been called this overloading
	 * operator method allows to assign a new ICN ID.
	 *
	 * \param str Reference to the string which holds the ICN ID
	 *
	 * \return void
	 */
	void operator=(string &str);
	/*!
	 * \brief Obtain if all operations should be paused for this ICN ID
	 *
	 * \return boolean indicating if all operations should be paused
	 */
	bool pausing();
	/*!
	 * \brief Set the pausing state
	 *
	 * \param state The state indicating if all operations should be paused
	 */
	void pausing(bool state);
	/*!
	 * \brief
	 *
	 * \return
	 */
	const string prefixId();
	/*!
	 * \brief
	 *
	 * \return
	 */
	const string print();
	/*!
	 * \brief
	 *
	 * \return
	 */
	const string printPrefixId();
	/*!
	 * \brief Print the FQDN in human readable format
	 *
	 * \return The FQDN as given when ICN ID was initialised
	 */
	const string printFqdn();
	/*!
	 * \brief Print the scope path in a human readable format (/123/567/891)
	 *
	 * This method creates injects a forward slash after every 16 characters to
	 * improve on the readability of an ICN ID. Furthermore, the method requires
	 * a parameter which determines the number of scope levels until the human
	 * readable scope path should be generated. For instance, if an ICN ID is
	 * five levels long and '3' is given, the first and second is returned.
	 *
	 * \return The scope path in a human readable format
	 */
	const string printScopePath(size_t scopeLevel);
	/*!
	 * \brief Obtain the root namespace identifier
	 *
	 * \return
	 */
	uint16_t rootNamespace();
	/*!
	 * \brief
	 *
	 * \return
	 */
	const string rootScopeId();
	/*!
	 * \brief Obtain the scope ID for a given scope level
	 *
	 * \param scopeLevel The scope level which should be extracted
	 *
	 * \return The scope ID of the scope level requested
	 */
	const string scopeId(unsigned int scopeLevel);
	/*!
	 * \brief Obtain the number of scope levels this CID holds
	 *
	 * \return The number of scope levels
	 */
	size_t scopeLevels();
	/*!
	 * \brief Obtain the scope path up to the level provided as an argument
	 *
	 * In order to publish the entire scope path into RV each publish_scope()
	 * primitive requires the scope ID which should reside under a father scope.
	 * The father scope can be obtained through this method by simply using the
	 * scope level as an integer number. The number provided determines the
	 * length of the scope path *including* the scope level the integer number
	 * refers to. The method starts counting at 1 for the root scope.
	 *
	 * \param scopeLevel The scope level up to which the CID should be
	 * extracted
	 *
	 * \return The scope path requested
	 */
	const string scopePath(unsigned int scopeLevel);
	/*!
	 * \brief
	 *
	 * \return
	 */
	string str();
	/*!
	 * \brief
	 *
	 * \return
	 */
	uint32_t uint();
	/*!
	 * \brief
	 *
	 * \return
	 */
	uint32_t uintId();
private:
	boost::posix_time::ptime _lastAccessed;/*!< The timestamp when this ICN ID
	was used last */
	//std::mutex *_ageMutex;/*!< Mutex to guarantee transaction safe operations on
	//_age*/
	bool _fidRequested;/*!< store state if pubish_info(CID) has been called.
	Note, this boolean gets reset to false if START_PUBLISH arrives*/
	bool _forwarding;/*!< Forwarding policy (START_PUBLISH received yes/no)*/
	bool _pausing;/*!< Pausing policy for this particular ICN ID (DNSlocal use
	cases)*/
	string _icnIdString; /*!< */
	uint32_t _icnIdHashed; /*!< */
	uint16_t _rootNamespace;/*!< The root namespace identifier */
	string _fqdn;
	string _resource;
	/*!
	 * \brief Set timestamp for accessing this ICN ID
	 */
	void _access();
	/*!
	 * \brief
	 */
	void _hashIcnId();
	/*!
	 * \brief Hash string
	 *
	 * \return Integer representation of given string
	 */
	uint32_t _hashString(string str);
	/*!
	 * \brief calling a set of methods to initialise the ICN ID
	 */
	void _initialise();
	/*!
	 * \brief
	 */
	uint8_t _scopeLevels(string str);
	/*!
	 * \brief
	 */
	void _setRootNamespace();
};

#endif /* NAP_TYPES_ICNID_HH_ */
