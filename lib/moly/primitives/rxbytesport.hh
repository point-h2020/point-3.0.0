/*
 * rxbytesport.hh
 *
 *  Created on: 20 Apr 2017
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

#ifndef MOLY_PRIMITIVES_RXBYTESPORT_HH_
#define MOLY_PRIMITIVES_RXBYTESPORT_HH_

#include "../typedef.hh"

using namespace std;

/*!
 * \brief Implementation of the RX_BYTES_PORT_M primitive
 */
class RxBytesPort {
public:
	/*!
	 * \brief Constructor for processes
	 */
	RxBytesPort(uint32_t nodeId, uint16_t portId, uint32_t bytes);
	/*!
	 * \brief Constructor for MONAs
	 */
	RxBytesPort(uint8_t* pointer, size_t size);
	/*!
	 * \brief Destructor
	 */
	~RxBytesPort();
	/*!
	 * \brief Obtain the RX bytes number stored in this instance
	 */
	uint32_t bytes();
	/*!
	 * \brief Obtain the NID
	 */
	uint32_t nodeId();
	/*!
	 * \brief Pointer to the packet
	 *
	 * \return
	 */
	uint8_t * pointer();
	/*!
	 * \brief Obtain the port ID
	 */
	uint16_t portId();
	/*!
	 * \brief Print out the content of the RX_BYTES primitive
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
private:
	uint32_t _nodeId; /*!< The destination NID on which the port resides*/
	uint16_t _portId; /*!< The port ID */
	uint32_t _bytes;/*!< RX bytes [bytes] */
	uint8_t * _pointer;/*!< Pointer to MOLY packet */
	size_t _size;/*!< size of _pointer */
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

#endif /* MOLY_PRIMITIVES_RXBYTESPORT_HH_ */
