/*
 * addnode.hh
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

#ifndef MOLY_PRIMITIVES_ADDNODE_HH_
#define MOLY_PRIMITIVES_ADDNODE_HH_

#include "../typedef.hh"
/*!
 * \brief Implementation of the ADD_NODE primitive
 */
class AddNode {
public:
	/*!
	 * \brief Constructor to write packet
	 */
	AddNode(string name, uint32_t nodeId, node_role_t nodeRole);
	/*!
	 * \brief Constructor to read packet
	 */
	AddNode(uint8_t * pointer, size_t size);
	/*!
	 * \brief Deconstructor
	 */
	~AddNode();
	/*!
	 * \brief Obtain the name
	 */
	string name();
	/*!
	 * \brief Obtain the node ID
	 */
	uint32_t nodeId();
	/*!
	 * \brief Pointer to the packet
	 *
	 * \return Pointer to packet
	 */
	uint8_t * pointer();
	/*!
	 * \brief Print out the content of the ADD_NODE primitive
	 *
	 * The print out order is determined by the MOLY specification
	 */
	string print();
	/*!
	 * \brief Size
	 *
	 * \return The size of the memory _pointer is pointing to
	 */
	size_t size();
	/*!
	 * \brief Obtain the node type
	 */
	node_role_t nodeRole();
private:
	string _name;
	uint32_t _nodeId;
	node_role_t _nodeRole;
	uint8_t * _pointer;
	size_t _size;
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

#endif /* MOLY_PRIMITIVES_ADDNODE_HH_ */
