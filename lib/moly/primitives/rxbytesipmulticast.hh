/*
 * rxbytesipmulticast.hh
 *
 *  Created on: 29 Sep 2017
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

#ifndef MOLY_PRIMITIVES_RXBYTESIPMULTICAST_HH_
#define MOLY_PRIMITIVES_RXBYTESIPMULTICAST_HH_

#include "../typedef.hh"

/*!
 * \brief Implementation of the RX_BYES_IP_MULTICAST_M primitive
 */
class RxBytesIpMulticast {
public:
	/*!
	 * \brief Constructor for processes
	 */
	RxBytesIpMulticast(node_identifier_t nodeId, ip_version_t ipVersion,
			bytes_t rxBytes);
	/*!
	 * \brief Constructor for MONAs
	 */
	RxBytesIpMulticast(uint8_t *pointer, uint32_t size);
	/*!
	 * \brief Destructor
	 */
	~RxBytesIpMulticast();
	/*!
	 * \brief Obtain the IP version
	 */
	ip_version_t ipVersion();
	/*!
	 * \brief Obtain the node ID where the packet count was measured
	 */
	node_identifier_t nodeId();
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
	 * \brief Obtian the byte counter
	 */
	bytes_t rxBytes();
	/*!
	 * \brief Size
	 *
	 * \return Return the size of the pointer returned by
	 * pointer()
	 */
	size_t size();
private:
	node_identifier_t _nodeId;
	ip_version_t _ipVersion;
	uint8_t *_pointer;
	bytes_t _rxBytes;
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

#endif /* MOLY_PRIMITIVES_RXBYTESIPMULTICAST_HH_ */
