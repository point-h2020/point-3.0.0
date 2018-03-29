/*
 * cidanalyser.hh
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

#ifndef LIB_BAMPERS_CIDANALYSER_HH_
#define LIB_BAMPERS_CIDANALYSER_HH_

#include <iostream>
#include <moly/enum.hh>
#include <sstream>

#include "types/icnid.hh"

using namespace std;

/*!
 * \brief Class to parse a content identifier and the data received
 */
class CIdAnalyser {
public:
	/*!
	 * \brief Constructor
	 */
	CIdAnalyser();
	/*!
	 * \brief Constructor with IcnId as argument
	 */
	CIdAnalyser(IcnId icnId);
	/*!
	 * \brief Destructor
	 */
	~CIdAnalyser();
	/*!
	 * \brief Obtain the destination node ID from the CID
	 *
	 * /monitoring/links/linkId/dstNodeId/srcNodeId/srcPortId/linkType
	 */
	uint32_t destinationNodeId();
	/*!
	 * \brief Obtain the endpoint identifier from the ICN ID
	 *
	 * /monitoring/nodes/nodeRole/nodeId/endpointId
	 */
	uint32_t endpointId();
	/*!
	 * \brief Obtain the link ID from the ICN ID (bit position)
	 *
	 * /monitoring/links/linkId/
	 */
	uint16_t linkId();
	/*!
	 * \brief Obtain if this ICN ID points to /monitoring/TOPOLOGY_NODES branch
	 *
	 * \return Boolean
	 */
	bool linksScope();
	/*!
	 * \brief Obtain the link type from the ICN ID
	 *
	 * /monitoring/links/linkId/dstNodeId/srcNodeId/srcPortId/linkType
	 */
	uint8_t linkType();
	/*!
	 * \brief
	 *
	 * /monitoring/nodes/nodeType/nodeId/
	 */
	uint32_t nodeId();
	/*!
	 * \brief Obtain if this ICN ID points to /monitoring/TOPOLOGY_LINKS branch
	 *
	 * \return Boolean
	 */
	bool nodesScope();
	/*!
	 * \brief Returns the scope ID value representing the node's role
	 *
	 * /monitoring/nodes/nodeRole/
	 */
	node_role_t nodeRole();
	/*!
	 * \brief In case default constructor was used this method overloads the
	 * equal sign operator to assign a new CID to the analyser class
	 *
	 * \param str The CID as a string after converting it from hex to char
	 */
	//void operator=(string &str);
	/*!
	 * \brief Obtain the primitive type and fill up the corresponding fields
	 *
	 * All primitive types are derived from the MOnitoring LibrarY (MOLY)
	 * specification in <moly/enum.hh>
	 */
	uint8_t primitive();
	/*!
	 * \brief Obtain the port ID
	 */
	uint16_t portId();
	/*!
	 * \brief Obtain if this ICN ID points to /monitoring/TOPOLOGY_PORTS branch
	 *
	 * \return Boolean
	 */
	bool portsScope();
	/*!
	 * \brief Obtain the source node ID from the CID
	 *
	 * /monitoring/links/linkId/dstNodeId/srcNodeId/srcPortId/linkType
	 */
	uint32_t sourceNodeId();
private:
	uint32_t _destinationNodeId;
	uint32_t _endpointId;
	IcnId _icnId;
	uint16_t _linkId;
	link_type_t _linkType;
	node_role_t _nodeRole;
	uint32_t _nodeId;
	uint32_t _sourceNodeId;
	uint16_t _sourcePortId;
	uint16_t _portId;
	uint16_t _informationItem();
	uint8_t _scopeTypeLevel1();
};

#endif /* LIB_BAMPERS_CIDANALYSER_HH_ */
