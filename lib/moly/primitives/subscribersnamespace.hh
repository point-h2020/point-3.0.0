/*
 * subscriberspernamespace.hh
 *
 *  Created on: 25 Sep 2017
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

#ifndef MOLY_PRIMITIVES_SUBSCRIBERSNAMESPACE_HH_
#define MOLY_PRIMITIVES_SUBSCRIBERSNAMESPACE_HH_

#include "../molyhelper.hh"
#include "../typedef.hh"

/*!
 * \brief Implementation of the SUBSCRIBERS_NAMESPACE_M primitive
 */
class SubscribersNamespace: public MolyHelper
{
public:
	/*!
	 * \brief Constructor for process
	 *
	 * \param numOfSubsPerNamespace The number of subscribers per namespace as
	 * a list of pairs<root scope, counter>
	 */
	SubscribersNamespace(subscribers_namespace_t subcribersNamespace);
	/*!
	 * \brief Constructor for MONA
	 *
	 * \param pointer Pointer to the packet
	 * \param size Size of the packet
	 */
	SubscribersNamespace(uint8_t *pointer, uint32_t size);
	/*!
	 * \brief Destructor
	 */
	~SubscribersNamespace();
	/*!
	 * \brief Obtain the number of subscribers
	 *
	 * \return Number of subscribers per namespace as a list of pairs
	 */
	subscribers_namespace_t subcribersNamespace();
	/*!
	 * \brief Pointer to the packet
	 *
	 * \return
	 */
	uint8_t * pointer();
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
	subscribers_namespace_t _subcribersNamespace;
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

#endif /* MOLY_PRIMITIVES_SUBSCRIBERSNAMESPACE_HH_ */
