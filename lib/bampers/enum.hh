/**
 * enum.hh
 *
 * Created on: Nov 25, 2015
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of BlAckadder Monitoring wraPpER clasS (BAMPERS).
 *
 * BAMPERS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * BAMPERS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * BAMPERS. If not, see <http://www.gnu.org/licenses/>.
 */

#include <moly/enum.hh>
#include <stdint.h>
#include <string>
#include "typedef.hh"
#define ID_LEN 16
using namespace std;

/*!
 * \brief Primitives for application > MA API
 */
enum BampersPrimitives
{
	LINK_ADDED, /*!< /monitoring/topology/links/linkId/destinationId/linkType/
	sourceNodeId */
	LINK_STATUS_CHANGED, /*!< /monitoring/topology/links/linkId/destinationId/
	linkType/state */
	LINK_REMOVED /*!< /monitoring/topology/links/linkId/destinationId/linkType*/
};

/*!
 * \brief Information items
 */
enum bampers_info_item_t {
	II_UNKNOWN,
	II_BUFFER_SIZES,
	II_CHANNEL_AQUISITION_TIME,
	II_CMC_GROUP_SIZE,
	II_CPU_UTILISATION,
	II_END_TO_END_LATENCY,
	II_FILE_DESCRIPTORS_TYPE,
	II_FQDN,
	II_HTTP_REQUESTS_FQDN,
	II_MATCHES_NAMESPACE,
	II_NAME,
	II_NETWORK_LATENCY_FQDN,
	II_PACKET_DROP_RATE,
	II_PACKET_ERROR_RATE,
	II_PACKET_JITTER_CID,
	II_PATH_CALCULATIONS_NAMESPACE,
	II_PUBLISHERS_NAMESPACE,
	II_PREFIX,
	II_RX_BYTES_HTTP,
	II_RX_BYTES_IP,
	II_RX_BYTES_IP_MULTICAST,
	II_RX_BYTES_PORT,
	II_ROLE,
	II_STATE,
	II_SUBSCRIBERS_NAMESPACE,
	II_TX_BYTES_HTTP,
	II_TX_BYTES_IP,
	II_TX_BYTES_IP_MULTICAST,
	II_TX_BYTES_PORT
};

/*!
 * \brief Topology
 */
enum BampersTopologyTypes {
	TOPOLOGY_NODES,//0
	TOPOLOGY_LINKS,//1
	TOPOLOGY_PORTS//2
};
