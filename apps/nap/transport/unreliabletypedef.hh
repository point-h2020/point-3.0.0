/*
 * unreliabletypedef.hh
 *
 *  Created on: 15 Sep 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of Blackadder.
 *
 * Blackadder is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Blackadder is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Blackadder.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NAP_TRANSPORT_UNRELIABLETYPEDEF_HH_
#define NAP_TRANSPORT_UNRELIABLETYPEDEF_HH_

#include <types/typedef.hh>

using namespace std;

/*!
 * \brief Unreliable transport protocol header
 */
struct utp_header_t
{
	uint16_t key; /*!< Unique key to identify a fragmented packet */
	uint16_t payloadLength; /*!< The length of the data the
	unreliable tranport protocol header carries */
	uint8_t sequence; /*!< Identity the order in which fragments
	should be re-assembled */
	uint8_t state; /*!< */
};

typedef map<uint32_t, map<uint8_t, pair<utp_header_t, uint8_t*>>>
		reassembly_packet_buffer_t ; /*!< map<uniqueKey, map<sequence,
		pair<packetHeader, packet>>> */

#endif /* NAP_TRANSPORT_UNRELIABLETYPEDEF_HH_ */
