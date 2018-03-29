/*
 * typedef.hh
 *
 *  Created on: 29 Dec 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the ICN application MOnitOring SErver (MOOSE) which
 * comes with Blackadder.
 *
 * MOOSE is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOOSE is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOOSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef APPS_MONITORING_SERVER_TYPES_TYPEDEF_HH_
#define APPS_MONITORING_SERVER_TYPES_TYPEDEF_HH_

typedef struct dataPoints
{
	bool topology = false;/*!< Node, Link and Port names only*/
	bool statistics = false;/*!< Internal boolean to store if any statics data
	points had been configured to true */
	// data points (statistics)
	bool bufferSizes = false;
	bool channelAquisitionTime = false;
	bool cmcGroupSize = false;
	bool cpuUtilisation = false;
	bool e2elatency = false;
	bool fileDescriptorsType = false;
	bool httpRequestsFqdn = false;
	bool linkState = false;
	bool matchesNamespace = false;
	bool networkLatencyFqdn = false;
	bool nodeState = false;
	bool packetDropRate = false;
	bool packetErrorRate = false;
	bool pathCalculationsNamespace = false;
	bool portState = false;
	bool publishersNamespace = false;
	bool rxBytesHttp = false;
	bool rxBytesIp = false;
	bool rxBytesIpMulticast = false;
	bool rxBytesPort = false;
	bool subscribersNamespace = false;
	bool txBytesHttp = false;
	bool txBytesIp = false;
	bool txBytesIpMulticast = false;
	bool txBytesPort = false;
} data_points_t;

#endif /* APPS_MONITORING_SERVER_TYPES_TYPEDEF_HH_ */
