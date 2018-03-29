/*
 * cputilisation.hh
 *
 *  Created on: 16 Feb 2017
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

#ifndef MOLY_PRIMITIVES_CPUUTILISATION_HH_
#define MOLY_PRIMITIVES_CPUUTILISATION_HH_

#include "../typedef.hh"

/*!
 * \brief Implementation of the CPU_UTILISATION primitive (for all node roles)
 */
class CpuUtilisation {
public:
	/*!
	 * \brief Constructor to compose a packet
	 */
	CpuUtilisation(node_role_t nodeRole, uint16_t cpuUtilisation);
	/*!
	 * \brief Constructor to compose a packet
	 */
	CpuUtilisation(node_role_t nodeRole, uint32_t endpointId,
			uint16_t cpuUtilisation);
	/*!
	 * \brief Constructor to decompose a packet
	 */
	CpuUtilisation(uint8_t * pointer, size_t size);
	/*!
	 * \brief Destructor
	 */
	~CpuUtilisation();
	/*!
	 * \brief Obtain the CPU utilisation
	 */
	uint16_t cpuUtilisation();
	/*!
	 * \brief Obtain the endpoint ID
	 */
	uint32_t endpointId();
	/*!
	 * \brief Obtain the node role provided by the user
	 *
	 * \return Node role
	 */
	node_role_t nodeRole();
	/*!
	 * \brief Pointer to the packet
	 *
	 * \return
	 */
	uint8_t * pointer();
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
	node_role_t _nodeRole; /*!< Node role */
	uint32_t _endpointId;/*!< The endpoint ID, if applicable*/
	uint16_t _cpuUtilisation;/*!< CPU utilisation in [%] with a base of 0.01 */
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

#endif /* MOLY_PRIMITIVES_CPUUTILISATION_HH_ */
