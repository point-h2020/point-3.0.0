/*
 * helper.cc
 *
 *  Created on: 28 Sep 2017
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

#include "molyhelper.hh"

MolyHelper::MolyHelper() {}

MolyHelper::~MolyHelper() {}

string MolyHelper::bufferNameToString(buffer_name_t buffer)
{
	ostringstream oss;

	switch (buffer)
	{
	case BUFFER_NAP_IP_HANDLER:
		oss << "BUFFER_NAP_IP_HANDLER";
		break;
	case BUFFER_NAP_HTTP_HANDLER_REQUESTS:
		oss << "BUFFER_NAP_HTTP_HANDLER_REQUESTS";
		break;
	case BUFFER_NAP_HTTP_HANDLER_RESPONSES:
		oss << "BUFFER_NAP_HTTP_HANDLER_RESPONSES";
		break;
	case BUFFER_NAP_LTP:
		oss << "BUFFER_NAP_LTP";
		break;
	case BUFFER_MOOSE_MESSAGE_STACK:
		oss << "BUFFER_MOOSE_MESSAGE_STACK";
		break;
	default:
		oss << "UNKNOWN";
	}

	return oss.str();
}

string MolyHelper::descriptorTypeToString(file_descriptor_type_t descriptorType)
{
	ostringstream oss;

	switch (descriptorType)
	{
	case DESCRIPTOR_TYPE_IPC:
		oss << "DESCRIPTOR_TYPE_IPC";
		break;
	case DESCRIPTOR_TYPE_FILE:
		oss << "DESCRIPTOR_TYPE_FILE";
		break;
	case DESCRIPTOR_TYPE_RAW_IP:
		oss << "DESCRIPTOR_TYPE_RAW_IP";
		break;
	case DESCRIPTOR_TYPE_TCP_SOCKET:
		oss << "DESCRIPTOR_TYPE_TCP_SOCKET";
		break;
	case DESCRIPTOR_TYPE_UDP_SOCKET:
		oss << "DESCRIPTOR_TYPE_UDP_SOCKET";
		break;
	default:
		oss << "UNKNOWN";
	}

	return oss.str();
}

string MolyHelper::nodeRoleToString(node_role_t nodeRole)
{
	ostringstream oss;

	switch (nodeRole)
	{
	case NODE_ROLE_FN:
		oss << "FN";
		break;
	case NODE_ROLE_GW:
		oss << "ICN GW";
		break;
	case NODE_ROLE_MOOSE:
		oss << "MOOSE";
		break;
	case NODE_ROLE_NAP:
		oss << "NAP";
		break;
	case NODE_ROLE_RV:
		oss << "RV";
		break;
	case NODE_ROLE_SERVER:
		oss << "Server";
		break;
	case NODE_ROLE_TM:
		oss << "TM";
		break;
	case NODE_ROLE_UE:
		oss << "UE";
		break;
	default:
		oss << "UNKNOWN";
	}

	return oss.str();
}

string MolyHelper::scopeToString(root_scope_t rootScope)
{
	ostringstream oss;

	switch(rootScope)
	{
	case NAMESPACE_IP:
		oss << "IP";
		break;
	case NAMESPACE_HTTP:
		oss << "HTTP";
		break;
	case NAMESPACE_COAP:
		oss << "CoAP";
		break;
	case NAMESPACE_MONITORING:
		oss << "Monitoring";
		break;
	case NAMESPACE_MANAGEMENT:
		oss << "Management";
		break;
	case NAMESPACE_SURROGACY:
		oss << "Surrogate";
		break;
	case NAMESPACE_LINK_STATE:
		oss << "Link State";
		break;
	case NAMESPACE_PATH_MANAGEMENT:
		oss << "Path Management";
		break;
	case NAMESPACE_EXPERIMENTATION:
		oss << "Experimentation";
		break;
	case NAMESPACE_IGMP_CTRL:
		oss << "IGMP CTRL";
		break;
	//case NAMESPACE_IGMP_DATA://FIXME all apps must use hex values!!!
	case 16:
		oss << "IGMP Data (IP Multicast)";
		break;
	//case NAMESPACE_APP_RV:
	case 17:
		oss << "App > RV";
		break;
	//case NAMESPACE_RV_TM:
	case 18:
		oss << "RV > TM";
		break;
	//case NAMESPACE_TM_APP:
	case 19:
		oss << "TM > App";
		break;
	default:
#ifdef MOLY_DEBUG
		cout << "Root scope ID" << rootScope << " unknown!" << endl;
#endif
		oss << "Unknown";
	}

	return oss.str();
}
