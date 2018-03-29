/*
 * publishersnamespace.hh
 *
 *  Created on: 21 Sep 2017
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

#ifndef MOLY_PRIMITIVES_PUBLISHERSNAMESPACE_HH_
#define MOLY_PRIMITIVES_PUBLISHERSNAMESPACE_HH_

#include "../molyhelper.hh"
#include "../typedef.hh"

/*!
 * \brief Implementation of the PUBLISHERS_NAMESPACE_M
 */
class PublishersNamespace: public MolyHelper
{
public:
	/*!
	 * \brief Constructor for a process
	 */
	PublishersNamespace(publishers_namespace_t publishersNamespace);
	/*!
	 * \brief Constructor for MONA
	 */
	PublishersNamespace(uint8_t *pointer, uint32_t size);
	/*!
	 * \brief Destructor
	 */
	~PublishersNamespace();
	/*!
	 * \brief Obtain the channel aquisition time
	 *
	 * \return Channel aquisition time
	 */
	publishers_namespace_t publishersNamespace();
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
	 * pointer()
	 */
	size_t size();
private:
	publishers_namespace_t _publishersNamespace;
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

#endif /* MOLY_PRIMITIVES_PUBLISHERSNAMESPACE_HH_ */
