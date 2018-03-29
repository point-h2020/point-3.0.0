/*
 * nodeid.hh
 *
 *  Created on: 4 Feb 2016
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

#ifndef NAP_TYPES_NODEID_HH_
#define NAP_TYPES_NODEID_HH_

#include <iostream>
#include <string>

#include <types/typedef.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace std;

/*!
 * \brief Implementation of the type node identifier
 */
class NodeId {
public:
	/*!
	 * \brief Constructor with no arguments. Equal sign operator must be called
	 */
	NodeId();
	/*!
	 * \brief Constructor
	 *
	 * \param nodeId The node ID as a string
	 */
	NodeId(string nodeId);
	/*!
	 * \brief Constructor
	 *
	 * \param nodeId The node ID as an unsigned integer
	 */
	NodeId(uint32_t nodeId);
	/*!
	 * \brief
	 */
	void operator=(string &nodeId);
	/*!
	 * \brief
	 */
	void operator=(uint32_t &nodeId);
	/*!
	 * \brief
	 */
	string str();
	/*!
	 * \brief
	 */
	nid_t uint();
private:
	nid_t _nodeId; /*!< */
};

#endif /* NAP_TYPES_NODEID_HH_ */
