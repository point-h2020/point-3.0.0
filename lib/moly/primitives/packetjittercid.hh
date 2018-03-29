/*
 * packetjitterpercid.hh
 *
 *  Created on: 27 Sep 2017
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

#ifndef MOLY_PRIMITIVES_PACKETJITTERCID_HH_
#define MOLY_PRIMITIVES_PACKETJITTERCID_HH_

#include "../typedef.hh"

/*!
 * \brief Implementation of the MOLY primitive PACKET_JITTER_CID_M
 */
class PacketJitterCid {
public:
	/*!
	 * \brief Constructor for processes
	 */
	PacketJitterCid(node_role_t nodeRole, node_identifier_t nodeId,
			packet_jitter_cid_t packetJitterCid);
	/*!
	 * \brief Constructor for MONAs
	 *
	 * \param pointer Pointer to the packet
	 * \param size Size of the packet
	 */
	PacketJitterCid(uint8_t *pointer, uint32_t size);
	/*!
	 * \brief Destructor
	 */
	~PacketJitterCid();
	/*!
	 * \brief Obtain the node identifier provided by the process
	 *
	 * \return the NID
	 */
	node_identifier_t nodeIdentifier();
	/*!
	 * \brief Obtain the node role provided by the process
	 *
	 * \return The node role of type node_role_t which is directly derived from
	 * the enumeration
	 */
	node_role_t nodeRole();
	/*!
	 * \brief Obtain the channel aquisition time
	 *
	 * \return Channel aquisition time
	 */
	packet_jitter_cid_t packetJitterCid();
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
	node_role_t _nodeRole;/*!< The role of the node where the packet jitter was
	measured */
	node_identifier_t _nodeId;/*!< The identifier of the node where the packet
	jitter was measured*/
	packet_jitter_cid_t _packetJitterCid;/*!< List to hold all values*/
	uint8_t *_pointer;/*!< pointer to received packet from processes*/
	uint32_t _size;/*!< Size of _pointer*/
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

#endif /* MOLY_PRIMITIVES_PACKETJITTERCID_HH_ */
