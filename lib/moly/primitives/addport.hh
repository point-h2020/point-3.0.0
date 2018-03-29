/*
 * addport.hh
 *
 *  Created on: 3 May 2017
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

#ifndef MOLY_PRIMITIVES_ADDPORT_HH_
#define MOLY_PRIMITIVES_ADDPORT_HH_

#include "../typedef.hh"

using namespace std;

/*!
 * \brief Implementation of the ADD_PORT primitive
 */
class AddPort {
public:
	/*!
	 * \brief Constructor I
	 */
	AddPort(node_identifier_t nodeId, port_identifier_t portId, string portName);
	/*!
	 * \brief Constructor II
	 */
	AddPort(uint8_t *pointer, uint32_t pointerSize);
	/*!
	 * \brief Destructor
	 */
	~AddPort();
	/*!
	 * \brief Free up allocated memory
	 */
	void cleanUp();
	/*!
	 * \brief Obtain the node ID
	 */
	node_identifier_t nodeId();
	/*!
	 * \brief Pointer to the packet
	 */
	uint8_t * pointer();
	/*!
	 * \brief Obtain the port ID
	 */
	port_identifier_t portId();
	/*!
	 * \brief Obtain the port name
	 */
	string portName();
	/*!
	 * \brief Print out the content of the ADD_PORT primitive
	 *
	 * The print out order is determined by the MOLY specification
	 */
	string print();
	/*!
	 * \brief Size
	 */
	size_t size();
private:
	node_identifier_t _nodeId;
	port_identifier_t _portId;
	string _portName;
	uint8_t *_pointer;
	uint32_t _pointerSize;
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

#endif /* MOLY_PRIMITIVES_ADDPORT_HH_ */
