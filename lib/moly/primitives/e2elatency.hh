/*
 * e2elatency.hh
 *
 *  Created on: 3 Feb 2017
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

#ifndef MOLY_PRIMITIVES_E2ELATENCY_HH_
#define MOLY_PRIMITIVES_E2ELATENCY_HH_

#include "../typedef.hh"

using namespace std;

/*!
 * \brief Implementation of the E2E_LATENCY primitive
 */
class E2eLatency {
public:
	/*!
	 * \brief Constructor
	 */
	E2eLatency(node_role_t nodeRole, uint32_t endpointId, uint16_t latency);
	/*!
	 * \brief Constructor to read packet
	 */
	E2eLatency(uint8_t * pointer, size_t size);
	/*!
	 * \brief Destructor
	 */
	~E2eLatency();
	/*!
	 * \brief The endpoint ID of the node
	 */
	uint32_t endpointId();
	/*!
	 * \brief Obtain the latency stored in this class
	 */
	uint16_t latency();
	/*!
	 * \brief Obtain the node's role
	 */
	node_role_t nodeRole();
	/*!
	 * \brief Pointer to the packet
	 *
	 * \return
	 */
	uint8_t * pointer();
	/*!
	 * \brief Print out the content of the NETWORK_LATENCY primitive
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
	node_role_t _nodeRole;/*!< Role of the node (UE || SV)*/
	uint32_t _endpointId;/*!< The IP address in network byte order */
	uint16_t _latency;/*!< E2E latency in [ms] */
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

#endif /* MOLY_PRIMITIVES_E2ELATENCY_HH_ */
