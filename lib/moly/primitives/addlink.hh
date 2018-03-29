/*
 * addlink.hh
 *
 *  Created on: 13 Jan 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the MOnitoring LibrarY (MOLY).
 *
 * MOLY is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOLY is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOLY. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MOLY_PRIMITIVES_ADDLINK_HH_
#define MOLY_PRIMITIVES_ADDLINK_HH_

#include "../typedef.hh"

/*!
 * \brief Implementation of the ADD_LINK primitive
 */
class AddLink {
public:
	/*!
	 * \brief Constructor
	 */
	AddLink(uint16_t linkId, uint32_t destinationNodeId, uint32_t sourceNodeId,
			uint16_t sourcePortId, link_type_t linkType, string name);
	/*!
	 * \brief Constructor
	 */
	AddLink(uint8_t * pointer, size_t size);
	/*!
	 * \brief Destructor
	 */
	~AddLink();
	/*!
	 * \brief Obtain the destination node ID
	 */
	uint32_t destinationNodeId();
	/*!
	 * \brief Obtain the link ID
	 */
	uint16_t linkId();
	/*!
	 * \brief Obtain the link type
	 */
	link_type_t linkType();
	/*!
	 * \brief Obtain the string
	 */
	string name();
	/*!
	 * \brief Pointer to the packet
	 */
	uint8_t * pointer();
	/*!
	 * \brief Print out the content of the ADD_LINK primitive
	 *
	 * The print out order is determined by the MOLY specification
	 */
	string print();
	/*!
	 * \brief Size
	 *
	 */
	size_t size();
	/*!
	 * \brief Obtain the source node ID
	 */
	uint32_t sourceNodeId();
	/*!
	 * \brief Obtain the source port ID
	 */
	uint16_t sourcePortId();
private:
	uint16_t _linkId;/*!< The bit position in the 256 bit mask*/
	uint32_t _destinationNodeId;
	uint32_t _sourceNodeId;
	uint16_t _sourcePortId;
	link_type_t _linkType;
	string _name;
	uint8_t * _pointer; /*!< Pointer to the generated packet */
	size_t _size; /*!< Size of the generated packet */
	uint8_t _offset;
	/*!
	 * \brief Compose packet
	 *
	 * This method composes the packet according to the specification.
	 */
	void _composePacket();
	/*!
	 * \brief Decompose packet
	 *
	 * This method decomposes the packet according to the specification.
	 */
	void _decomposePacket();
};

#endif /* MOLY_PRIMITIVES_ADDLINK_HH_ */
