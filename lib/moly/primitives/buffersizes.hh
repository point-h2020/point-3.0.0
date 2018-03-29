/*
 * buffersizes.hh
 *
 *  Created on: 17 Oct 2017
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

#ifndef MOLY_PRIMITIVES_BUFFERSIZES_HH_
#define MOLY_PRIMITIVES_BUFFERSIZES_HH_

#include "../molyhelper.hh"
#include "../typedef.hh"

using namespace std;

/*!
 * \brief Implementation of the primitive BUFFER_SIZES_M
 */
class BufferSizes: private MolyHelper
{
public:
	/*!
	 * \brief Constructor for processes
	 */
	BufferSizes(node_role_t nodeRole, buffer_sizes_t bufferSizes);
	/*!
	 * \brief Constructor for MONAs
	 */
	BufferSizes(uint8_t *pointer, uint32_t pointerSize);
	/*!
	 * \brief Destructor
	 */
	~BufferSizes();
	/*!
	 * \brief Obtain the channel aquisition time
	 *
	 * \return Channel aquisition time
	 */
	buffer_sizes_t bufferSizes();
	/*!
	 * \brief Obtain the role of the node which reports buffer sizes
	 */
	node_role_t nodeRole();
	/*!
	 * \brief Pointer to the packet
	 *
	 * \return
	 */
	uint8_t * pointer();
	/*!
	 * \brief Obtain the official primitive name
	 *
	 * \return Primitive name according to the specification
	 */
	string primitiveName();
	/*!
	 * \brief Print out the content of the primitive
	 *
	 * The print out order is determined by the MOLY specification
	 */
	string print();
	/*!
	 * \brief Size
	 *
	 * \return Return the size of the pointer returned by
	 * UpdateLinkState::pointer()
	 */
	uint32_t size();
private:
	node_role_t _nodeRole;/*!< Node role */
	buffer_sizes_t _bufferSizes;/*!< List of buffer sizes*/
	buffer_sizes_t::iterator _it;
	uint8_t *_pointer;/*!< Pointer to the packet for MONA */
	uint32_t _pointerSize;/*!< Size of _pointer for MONA */
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

#endif /* MOLY_PRIMITIVES_BUFFERSIZES_HH_ */
