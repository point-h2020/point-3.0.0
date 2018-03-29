/*
 * monitoringtypedef.hh
 *
 *  Created on: 27 Sep 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of Blackadder.
 *
 * Blackadder is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
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

#ifndef NAP_MONITORING_MONITORINGTYPEDEFS_HH_
#define NAP_MONITORING_MONITORINGTYPEDEFS_HH_

#include <moly/enum.hh>
#include <string>
#include <unordered_map>

#include <types/ipaddress.hh>

using namespace std;
/*!
 * \brief Data describing an IP endpoint (UE or server)
 */
struct ip_endpoint_t
{
	IpAddress ipAddress;/*!< IP address of the endpoint */
	node_role_t nodeType;/*!< type of endpoint (UE || server) */
	bool surrogate;/*!< If surrogate, FQDN field is empty, as the NAP only has
	the hashed FQDN when receiving the message from SA through NAP-SA API*/
	std::string fqdn;/*!< If server: FQDN it servers */
	uint16_t port;/*!< If server: Port on which the server is reachable */
	bool reported;/*!< Keep track if endpoint has been already reported */
};

typedef unordered_map<uint32_t, ip_endpoint_t> ip_endpoints_t ; /*!< map<IP
address, struct> */

typedef map<string, uint32_t> http_requests_per_fqdn_t ;

#endif /* NAP_MONITORING_MONITORINGTYPEDEFS_HH_ */
