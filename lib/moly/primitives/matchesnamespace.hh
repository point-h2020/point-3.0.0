/*
 * matchesnamespace.hh
 *
 *  Created on: 28 Sep 2017
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

#ifndef MOLY_PRIMITIVES_MATCHESNAMESPACE_HH_
#define MOLY_PRIMITIVES_MATCHESNAMESPACE_HH_

#include "../molyhelper.hh"
#include "../typedef.hh"

/*!
 * \brief Implementation of the MOLY primitive MATCHES_PER_NAMESPACE_M
 */
class MatchesNamespace: public MolyHelper
{
public:
	/*!
	 * \brief Constructor for Processes
	 */
	MatchesNamespace(matches_namespace_t matchesNamespace);
	/*!
	 * \brief Constructor for MONAs
	 */
	MatchesNamespace(uint8_t *pointer, uint32_t size);
	/*!
	 * \brief Destructor
	 */
	~MatchesNamespace();
	/*!
	 * \brief Obtain the matches per namespace
	 *
	 * \return pub/sub matches
	 */
	matches_namespace_t matchesNamespace();
	/*!
	 * \brief Pointer to the packet
	 *
	 * \return Pointer
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
	 * \return Return the size of the pointer from pointer()
	 */
	size_t size();
private:
	matches_namespace_t _matchesNamespace;
	matches_namespace_t::iterator it;
	uint8_t *_pointer;
	uint32_t _size;
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

#endif /* MOLY_PRIMITIVES_MATCHESNAMESPACE_HH_ */
