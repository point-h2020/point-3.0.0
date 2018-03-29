/*
 * pathcalculations.hh
 *
 *  Created on: 19 Sep 2017
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

#ifndef MOLY_PRIMITIVES_PATHCALCULATIONS_HH_
#define MOLY_PRIMITIVES_PATHCALCULATIONS_HH_

#include "../molyhelper.hh"
#include "../typedef.hh"

/*!
 * \brief Implementation of the PATH_CALCULATIONS_NAMESPACE_M primitive
 */
class PathCalculationsNamespace: public MolyHelper
{
public:
	/*!
	 * \brief Constructor for processes
	 */
	PathCalculationsNamespace(
			path_calculations_namespace_t pathCalculationsNamespace);
	/*!
	 * \brief Constructor for MONA
	 */
	PathCalculationsNamespace(uint8_t *pointer, uint32_t size);
	/*!
	 * \brief Destructor
	 */
	~PathCalculationsNamespace();
	/*!
	 * \brief Obtain the channel aquisition time
	 *
	 * \return Channel aquisition time
	 */
	path_calculations_namespace_t pathCalculationsNamespace();
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
	 * \brief Print out the content of the CMC_GROUP_SIZE primitive
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
	size_t size();
private:
	path_calculations_namespace_t _pathCalculationsNamespace;/*!< The
	number of path calculations per root namespace */
	path_calculations_namespace_t::iterator it;
	uint8_t * _pointer; /*!< Pointer to the generated packet */
	size_t _size; /*!< Size of the generated packet */
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

#endif /* MOLY_PRIMITIVES_PATHCALCULATIONS_HH_ */
