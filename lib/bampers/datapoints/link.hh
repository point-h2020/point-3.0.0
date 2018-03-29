/*
 * link.hh
 *
 *  Created on: 19 Dec 2015
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of BlAckadder Monitoring wraPpER clasS (BAMPERS).
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

#ifndef LIB_BAMPERS_DATAPOINTS_LINK_HH_
#define LIB_BAMPERS_DATAPOINTS_LINK_HH_

#include "../namespace.hh"

class Link {
public:
	/*!
	 * \brief Constructor
	 */
	Link(Namespace &namespaceHelper);
	/*!
	 * \brief Destructor
	 */
	~Link();
	/*!
	 * \brief Report the existence of a new link between two nodes
	 *
	 * This method allows the topology manager to report a new link to the
	 * monitoring server.
	 *
	 * The namespace scope path used is:
	 *
	 * /monitoring/links/linkId/destinationNodeId/sourceNodeId/sourcePort/linkType/name
	 *
	 * \param linkId The new link to be reported
	 * \param destinationNodeId The destination node ID of the new link
	 * \param sourceNodeId The source node ID of the new link
	 * \param sourcePort
	 * \param linkType The link type as specified in enum.hh -> link_type_t
	 * \param linkName The cu
	 */
	void add(uint16_t linkId, uint32_t destinationNodeId, uint32_t sourceNodeId,
			uint16_t sourcePort, link_type_t linkType, string linkName);
	/*!
	 * \brief Update the state of a link in the network
	 *
	 * This method allows the topology manager to update the state of a
	 * previously added link ID. The scope used is:
	 *
	 * /monitoring/links/linkId/dNodeId/sNodeId/sPort/state
	 */
	void state(uint16_t linkId, uint32_t destinationNodeId,
			uint32_t sourceNodeId, uint16_t sourcePortId, link_type_t linkType,
			link_state_t state);
private:
	Namespace &_namespaceHelper; /*!< Reference to the class Namespace */
};

#endif /* LIB_BAMPERS_DATAPOINTS_LINK_HH_ */
