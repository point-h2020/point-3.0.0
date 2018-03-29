/*
 * nodestate.hh
 *
 *  Created on: 20 Jan 2017
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

#ifndef MOLY_PRIMITIVES_NODESTATE_HH_
#define MOLY_PRIMITIVES_NODESTATE_HH_

#include "../typedef.hh"

/*!
 * \brief Implementation of the NODE_STATE primitive
 */
class NodeState {
public:
	/*!
	 * \brief Constructor to compose a packet
	 */
	NodeState(node_role_t nodeRole, uint32_t nodeId, node_state_t state);
	/*!
	 * \brief Constructor to decompose a packet
	 */
	NodeState(uint8_t * pointer, size_t size);
	/*!
	 * \brief Destructor
	 */
	~NodeState();
	/*!
	 * \brief Obtain NID
	 */
	uint32_t nodeId();
	/*!
	 * \brief Obtain node role
	 */
	node_role_t nodeRole();
	/*!
	 * \brief Pointer to the packet
	 *
	 * \return
	 */
	uint8_t * pointer();
	/*!
	 * \brief Print out the content of the NODE_STATE primitive
	 *
	 * The print out order is determined by the MOLY specification
	 */
	string print();
	/*!
	 * \brief Pointer size
	 *
	 * \return Return the size of the pointer returned by NodeState::pointer()
	 */
	size_t size();
	/*!
	 * \brief Obtain the state
	 */
	node_state_t state();
private:
	node_role_t _nodeRole;
	uint32_t _nodeId;
	node_state_t _state;
	uint8_t *_pointer;
	size_t _size;
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

#endif /* MOLY_PRIMITIVES_NODESTATE_HH_ */
