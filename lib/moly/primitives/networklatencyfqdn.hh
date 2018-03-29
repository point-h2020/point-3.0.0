/*
 * networklatencyperfqdn.hh
 *
 *  Created on: 9 Nov 2016
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

#ifndef MOLY_PRIMITIVES_NETWORKLATENCYFQDN_HH_
#define MOLY_PRIMITIVES_NETWORKLATENCYFQDN_HH_

#include "../typedef.hh"

/*!
 * \brief Implementation of the NETWORK_LATENCY_FQDN primitive
 */
class NetworkLatencyFqdn {
public:
	/*!
	 * \brief Constructor
	 *
	 * \param fqdn The hashed version of the FQDN
	 * \param latency The latency to be reported for the FQDN
	 */
	NetworkLatencyFqdn(uint32_t fqdn, uint16_t latency);
	/*!
	 * \brief Constructor to read packet
	 */
	NetworkLatencyFqdn(uint8_t * pointer, size_t size);
	/*!
	 * \brief Destructor
	 */
	~NetworkLatencyFqdn();
	/*!
	 * \brief Obtain the hashed FQDN
	 */
	uint32_t fqdn();
	/*!
	 * \brief Obtain the latency stored in this class
	 */
	uint16_t latency();
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
	uint32_t _fqdn;
	uint16_t _latency;
	uint8_t * _pointer;
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

#endif /* MOLY_PRIMITIVES_NETWORKLATENCYFQDN_HH_ */
