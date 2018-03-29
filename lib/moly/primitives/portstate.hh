/*
 * portstate.hh
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

#ifndef MOLY_PRIMITIVES_PORTSTATE_HH_
#define MOLY_PRIMITIVES_PORTSTATE_HH_

#include "../typedef.hh"

/*!
 * \brief Implementation of the PORT_STATE_M primitive
 */
class PortState {
public:
	/*!
	 * \brief Constructor to write packet
	 */
	PortState(uint32_t nodeId, uint16_t portId, state_t state);
	/*!
	 * \brief Constructor to read packet
	 */
	PortState(uint8_t * pointer, size_t _size);
	/*!
	 * \brief Destructor
	 */
	~PortState();
	/*!
	 * \brief Obtain the source node ID
	 */
	uint32_t nodeId();
	/*!
	 * \brief Obtain the name of this primitive
	 *
	 * \return Primitive name according to the specification
	 */
	string primitiveName();
	/*!
	 * \brief Pointer to the packet
	 *
	 * \return
	 */
	uint8_t * pointer();
	/*!
	 * \brief Print out the content of the LINK_STATE primitive
	 *
	 * The print out order is determined by the MOLY specification
	 */
	string print();
	/*!
	 * \brief Size
	 *
	 * \return Return the size of the pointer returned by PortState::pointer()
	 */
	size_t size();
	/*!
	 * \brief Obtain the source port ID
	 */
	uint16_t portId();
	/*!
	 * \brief Obtain the state
	 */
	state_t state();
private:
	uint32_t _nodeId;
	uint16_t _portId;
	state_t _state;
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

#endif /* MOLY_PRIMITIVES_PORTSTATE_HH_ */
