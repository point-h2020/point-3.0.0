/*
 * enum.hh
 *
 *  Created on: 13 Jan 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *      		Mays Al-Naday <mfhaln@essex.ac.uk>
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

#ifndef ENUM_HH_
#define ENUM_HH_

#include <stdint.h>

typedef uint16_t state_t;

/*!
 * \brief Message types for bootstrapping of processes and agents
 */
enum BoostrappingMessageTypes
{
	BOOTSTRAP_OK,
	BOOTSTRAP_ERROR,
	BOOTSTRAP_MY_PID,
	BOOTSTRAP_START_REPORTING,
	BOOTSTRAP_SERVER_UP/*!< local BAMPERS instance informs agnet that trigger
	has been received */
};

/*!
 * \brief Buffers that can be reported by a process
 */
enum buffers_t
{
	BUFFER_NAP_IP_HANDLER,
	BUFFER_NAP_HTTP_HANDLER_ICN,
	BUFFER_NAP_HTTP_HANDLER_REQUESTS,
	BUFFER_NAP_HTTP_HANDLER_RESPONSES,
	BUFFER_NAP_LTP,
	BUFFER_MOOSE_MESSAGE_STACK
};

/*!
 * \brief Socket file descriptor types
 */
enum descriptor_types_t
{
	DESCRIPTOR_TYPE_UNKNOWN,
	DESCRIPTOR_TYPE_IPC,
	DESCRIPTOR_TYPE_FILE,
	DESCRIPTOR_TYPE_RAW_IP,
	DESCRIPTOR_TYPE_TCP_SOCKET,
	DESCRIPTOR_TYPE_UDP_SOCKET
};

/*!
 * \brief Link states
 */
enum link_state_t {
	LINK_STATE_UNKNOWN,
	LINK_STATE_DOWN,
	LINK_STATE_UP
};

/*!
 * \brief Link types
 */
enum link_type_t {
	LINK_TYPE_UNKNOWN,/*!< 0 */
	LINK_TYPE_802_3,/*!< 1 */
	LINK_TYPE_802_11,/*!< 2 */
	LINK_TYPE_802_11_A,/*!< 3 */
	LINK_TYPE_802_11_B,/*!< 4 */
	LINK_TYPE_802_11_G,/*!< 5 */
	LINK_TYPE_802_11_N,/*!< 6 */
	LINK_TYPE_802_11_AA,/*!< 7 */
	LINK_TYPE_802_11_AC,/*!< 8 */
	LINK_TYPE_SDN_802_3_Z, /*!< 9: IEEE spec deployed by pica8 SDN switches for
	1G ports*/
	LINK_TYPE_SDN_802_3_AE,/*!< 10: IEEE spec deployed by pica8 SDN switches for
	10G optical ports*/
	LINK_TYPE_GPRS,/*!< 11 */
	LINK_TYPE_UMTS,/*!< 12 */
	LINK_TYPE_LTE,/*!< 13 */
	LINK_TYPE_LTE_A,/*!< 14 */
	LINK_TYPE_OPTICAL,/*!< 15 */

};
/*!
 * \brief MOLY primitives
 */
enum primitive_type_t
{
	PRIMITIVE_TYPE_UNKNOWN,
	PRIMITIVE_TYPE_ADD_LINK,
	PRIMITIVE_TYPE_ADD_NODE,
	PRIMITIVE_TYPE_ADD_PORT,
	PRIMITIVE_TYPE_BUFFER_SIZES,
	PRIMITIVE_TYPE_CHANNEL_AQUISITION_TIME,
	PRIMITIVE_TYPE_CMC_GROUP_SIZE,
	PRIMITIVE_TYPE_CPU_UTILISATION,
	PRIMITIVE_TYPE_END_TO_END_LATENCY,
	PRIMITIVE_TYPE_FILE_DESCRIPTORS_TYPE,
	PRIMITIVE_TYPE_HTTP_REQUESTS_FQDN,
	PRIMITIVE_TYPE_LINK_STATE,
	PRIMITIVE_TYPE_MATCHES_NAMESPACE,
	PRIMITIVE_TYPE_NETWORK_LATENCY_FQDN,
	PRIMITIVE_TYPE_NODE_STATE,
	PRIMITIVE_TYPE_PACKET_DROP_RATE,
	PRIMITIVE_TYPE_PACKET_ERROR_RATE,
	PRIMITIVE_TYPE_PACKET_JITTER_CID,
	PRIMITIVE_TYPE_PATH_CALCULATIONS_NAMESPACE,
	PRIMITIVE_TYPE_PORT_STATE,
	PRIMITIVE_TYPE_PUBLISHERS_NAMESPACE,
	PRIMITIVE_TYPE_REMOVE_LINK,
	PRIMITIVE_TYPE_REMOVE_NODE,
	PRIMITIVE_TYPE_REMOVE_PORT,
	PRIMITIVE_TYPE_RX_BYTES_HTTP,
	PRIMITIVE_TYPE_RX_BYTES_IP,
	PRIMITIVE_TYPE_RX_BYTES_IP_MULTICAST,
	PRIMITIVE_TYPE_RX_BYTES_PORT,
	PRIMITIVE_TYPE_SUBSCRIBERS_NAMESPACE,
	PRIMITIVE_TYPE_TX_BYTES_HTTP,
	PRIMITIVE_TYPE_TX_BYTES_IP,
	PRIMITIVE_TYPE_TX_BYTES_IP_MULTICAST,
	PRIMITIVE_TYPE_TX_BYTES_PORT
};

/*!
 * \brief Node roles
 */
enum node_role_t {
	NODE_ROLE_UNKNOWN,
	NODE_ROLE_FN,
	NODE_ROLE_GW,
	NODE_ROLE_MOOSE,
	NODE_ROLE_NAP,
	NODE_ROLE_RV,
	NODE_ROLE_SERVER,
	NODE_ROLE_TM,
	NODE_ROLE_UE
};

/*!
 * \brief Node states
 */
enum node_state_t
{
	NODE_STATE_UNKNOWN,
	NODE_STATE_DOWN,
	NODE_STATE_UP,
	NODE_STATE_INACTIVE/*!< For IP endpoints if they have not been seen in a
	given interval*/
};

/*!
 * \brief Port states
 */
enum port_state_t
{
	PORT_STATE_UNKNOWN,
	PORT_STATE_DOWN,
	PORT_STATE_UP
};

/*!
 * \brief Surrogate server states
 */
enum surrogate_state_t {
	SURROGATE_STATE_UNKNOWN,/*!< 0 */
	SURROGATE_STATE_BOOTED,/*!< 1 */
	SURROGATE_STATE_CONNECTED,/*!< 2 */
	SURROGATE_STATE_DISCONNECTED,/*!< 3 */
	SURROGATE_STATE_DOWN,/*!< 4 */
	SURROGATE_STATE_NON_BOOTED,/*!< 5 */
	SURROGATE_STATE_NON_STATE,/*!< 6 */
	SURROGATE_STATE_PLACED,/*!< 7 */
	SURROGATE_STATE_UP/*!< 8 */
};

#endif /* ENUM_HH_ */
