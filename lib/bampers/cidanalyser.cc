/*
 * cidanalyser.cc
 *
 *  Created on: 03 June 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of BAMPERS.
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

#include "cidanalyser.hh"
#include "enum.hh"

CIdAnalyser::CIdAnalyser()
{
	_endpointId = 0;
	_linkId = 0;
	_linkType = LINK_TYPE_UNKNOWN;
	_destinationNodeId = 0;
	_nodeId = 0;
	_nodeRole = NODE_ROLE_UNKNOWN;
	_portId = 0;
	_sourceNodeId = 0;
	_sourcePortId = 0;
}

CIdAnalyser::CIdAnalyser(IcnId icnId)
	: _icnId(icnId)
{
	if (nodesScope() && _icnId.str().length() > (4 * ID_LEN))
	{
		_endpointId = atol(_icnId.str().substr(4 * ID_LEN, ID_LEN).c_str());
	}
	else
	{
		_endpointId = 0;
	}

	if (linksScope() && _icnId.str().length() > (2 * ID_LEN))
	{
		_linkId = atoi(_icnId.str().substr(2 * ID_LEN, ID_LEN).c_str());
	}
	else
	{
		_linkId = 0;
	}

	if (linksScope() && _icnId.str().length() > (6 * ID_LEN))
	{
		_linkType = (link_type_t)atoi(
				_icnId.str().substr(6 * ID_LEN, ID_LEN).c_str());
	}
	else
	{
		_linkType = LINK_TYPE_UNKNOWN;
	}

	if (linksScope() && _icnId.str().length() > (3 * ID_LEN))
	{
		_destinationNodeId =
				atol(_icnId.str().substr(3 * ID_LEN, ID_LEN).c_str());
	}
	else
	{
		_destinationNodeId = 0;
	}

	if (portsScope() && _icnId.str().length() > (2 * ID_LEN))
	{
		_nodeId = atol(_icnId.str().substr(2 * ID_LEN, ID_LEN).c_str());
	}
	else if (nodesScope() && _icnId.str().length() > (3 * ID_LEN))
	{
		_nodeId = atol(_icnId.str().substr(3 * ID_LEN, ID_LEN).c_str());
	}
	else
	{
		_nodeId = 0;
	}

	if (portsScope() && _icnId.str().length() > (3 * ID_LEN))
	{
		_portId = atoi(_icnId.str().substr(3 * ID_LEN, ID_LEN).c_str());
	}
	else
	{
		_portId = 0;
	}

	if (linksScope() && _icnId.str().length() > (4 * ID_LEN))
	{
		_sourceNodeId = atol(_icnId.str().substr(4 * ID_LEN, ID_LEN).c_str());
	}
	else
	{
		_sourceNodeId = 0;
	}

	if (linksScope() && _icnId.str().length() > (5 * ID_LEN))
	{
		_sourcePortId = atoi(_icnId.str().substr(5 * ID_LEN, ID_LEN).c_str());
	}
	else
	{
		_sourcePortId = 0;
	}

	if (nodesScope() && _icnId.str().length() > (2 * ID_LEN))
	{
		_nodeRole = (node_role_t)atoi(_icnId.str().substr(2 * ID_LEN,
				ID_LEN).c_str());
	}
	else
	{
		_nodeRole = NODE_ROLE_UNKNOWN;
	}
}

CIdAnalyser::~CIdAnalyser() {}

uint32_t CIdAnalyser::destinationNodeId()
{
	return _destinationNodeId;
}

uint32_t CIdAnalyser::endpointId()
{
	return _endpointId;
}

uint16_t CIdAnalyser::linkId()
{
	return _linkId;
}

bool CIdAnalyser::linksScope()
{
	if (atoi(_icnId.str().substr(ID_LEN, ID_LEN).c_str()) == TOPOLOGY_LINKS)
	{
		return true;
	}

	return false;
}

uint8_t CIdAnalyser::linkType()
{
	return _linkType;
}

uint32_t CIdAnalyser::nodeId()
{
	return _nodeId;
}

bool CIdAnalyser::nodesScope()
{
	if (atoi(_icnId.str().substr(ID_LEN, ID_LEN).c_str()) == TOPOLOGY_NODES)
	{
		return true;
	}

	return false;
}

node_role_t CIdAnalyser::nodeRole()
{
	return _nodeRole;
}

uint16_t CIdAnalyser::portId()
{
	return _portId;
}

bool CIdAnalyser::portsScope()
{
	if (atoi(_icnId.str().substr(ID_LEN, ID_LEN).c_str()) == TOPOLOGY_PORTS)
	{
		return true;
	}

	return false;
}

uint8_t CIdAnalyser::primitive()
{
	 /* primitive enumeration defined in MOLY */
	primitive_type_t primitive = PRIMITIVE_TYPE_UNKNOWN;

	switch (_scopeTypeLevel1())
	{
	case TOPOLOGY_NODES:
		_nodeRole = (node_role_t)atoi(_icnId.str().substr(2 * 16, 16).c_str());
		_nodeId = atoi(_icnId.str().substr(3 * 16, 16).c_str());
#ifdef BAMPERS_DEBUG
		cout << "((BAMPERS)) "
				<< "Scope level 1 = TOPOLOGY_NODES (0)"
				<< " | Node Role = " << (int)_nodeRole
				<< " | Node ID   = " << (int)_nodeId << endl;
#endif
		break;
	case TOPOLOGY_LINKS:
	{
		_linkId = atoi(_icnId.str().substr(2 * 16, 16).c_str());
		_destinationNodeId = atoi(_icnId.str().substr(3 * 16, 16).c_str());
		_sourceNodeId = atoi(_icnId.str().substr(4 * 16, 16).c_str());
		_sourcePortId = atoi(_icnId.str().substr(5 * 16, 16).c_str());
		_linkType = (link_type_t)atoi(_icnId.str().substr(6 * 16, 16).c_str());
#ifdef BAMPERS_DEBUG
		cout << "((BAMPERS)) "
				<< "Scope Level 1: TOPOLOGY_LINKS (1) "
				<< "| Link ID: " << _linkId
				<< "| DST NID: " << _destinationNodeId
				<< "| SRC NID: " << _sourceNodeId
				<< "| SRC Port ID: " << _sourcePortId
				<< "| Link Type: " << (int)_linkType << endl;
#endif
		break;
	}
	case TOPOLOGY_PORTS:
	{
		_nodeId = atoi(_icnId.str().substr(2 * 16, 16).c_str());
		_portId = atoi(_icnId.str().substr(3 * 16, 16).c_str());
#ifdef BAMPERS_DEBUG
		cout << "((BAMPERS)) "
				<< "Scope Level 1 = TOPOLOGY_PORTS (1) "
				<< "| Node ID = " << _nodeId
				<< "| Port ID = " << _portId << endl;
#endif
		break;
	}
	default:
		cout << "Unknown scope type level 1: " << _scopeTypeLevel1() << endl;
	}

	switch(_informationItem())
	{
	case II_BUFFER_SIZES:
		primitive = PRIMITIVE_TYPE_BUFFER_SIZES;
		break;
	case II_CHANNEL_AQUISITION_TIME:
		primitive = PRIMITIVE_TYPE_CHANNEL_AQUISITION_TIME;
		break;
	case II_CMC_GROUP_SIZE:
		primitive = PRIMITIVE_TYPE_CMC_GROUP_SIZE;
		break;
	case II_CPU_UTILISATION:
		primitive = PRIMITIVE_TYPE_CPU_UTILISATION;
		break;
	case II_END_TO_END_LATENCY:
		primitive = PRIMITIVE_TYPE_END_TO_END_LATENCY;
		break;
	case II_FILE_DESCRIPTORS_TYPE:
		primitive = PRIMITIVE_TYPE_FILE_DESCRIPTORS_TYPE;
		break;
	case II_FQDN:
		break;
	case II_HTTP_REQUESTS_FQDN:
		primitive = PRIMITIVE_TYPE_HTTP_REQUESTS_FQDN;
		break;
	case II_MATCHES_NAMESPACE:
		primitive = PRIMITIVE_TYPE_MATCHES_NAMESPACE;
		break;
	case II_NAME:
		if (_scopeTypeLevel1() == TOPOLOGY_NODES)
		{
			primitive = PRIMITIVE_TYPE_ADD_NODE;
		}
		else if (_scopeTypeLevel1() == TOPOLOGY_LINKS)
		{
			primitive = PRIMITIVE_TYPE_ADD_LINK;
		}
		else if (_scopeTypeLevel1() == TOPOLOGY_PORTS)
		{
			primitive = PRIMITIVE_TYPE_ADD_PORT;
		}
		break;
	case II_NETWORK_LATENCY_FQDN:
		primitive = PRIMITIVE_TYPE_NETWORK_LATENCY_FQDN;
		break;
	case II_PACKET_DROP_RATE:
		primitive = PRIMITIVE_TYPE_PACKET_DROP_RATE;
		break;
	case II_PACKET_ERROR_RATE:
		primitive = PRIMITIVE_TYPE_PACKET_ERROR_RATE;
		break;
	case II_PACKET_JITTER_CID:
		primitive = PRIMITIVE_TYPE_PACKET_JITTER_CID;
		break;
	case II_PATH_CALCULATIONS_NAMESPACE:
		primitive = PRIMITIVE_TYPE_PATH_CALCULATIONS_NAMESPACE;
		break;
	case II_PUBLISHERS_NAMESPACE:
		primitive = PRIMITIVE_TYPE_PUBLISHERS_NAMESPACE;
		break;
	case II_RX_BYTES_HTTP:
		primitive = PRIMITIVE_TYPE_RX_BYTES_HTTP;
		break;
	case II_RX_BYTES_IP:
		primitive = PRIMITIVE_TYPE_RX_BYTES_IP;
		break;
	case II_RX_BYTES_IP_MULTICAST:
		primitive = PRIMITIVE_TYPE_RX_BYTES_IP_MULTICAST;
		break;
	case II_RX_BYTES_PORT:
		primitive = PRIMITIVE_TYPE_RX_BYTES_PORT;
		break;
	case II_ROLE:
		break;
	case II_STATE:
		if (_scopeTypeLevel1() == TOPOLOGY_NODES)
		{
			primitive = PRIMITIVE_TYPE_NODE_STATE;
		}
		else if (_scopeTypeLevel1() == TOPOLOGY_LINKS)
		{
			primitive = PRIMITIVE_TYPE_LINK_STATE;
		}
		else if (_scopeTypeLevel1() == TOPOLOGY_PORTS)
		{
			primitive = PRIMITIVE_TYPE_PORT_STATE;
		}
		break;
	case II_SUBSCRIBERS_NAMESPACE:
		primitive = PRIMITIVE_TYPE_SUBSCRIBERS_NAMESPACE;
		break;
	case II_TX_BYTES_HTTP:
		primitive = PRIMITIVE_TYPE_TX_BYTES_HTTP;
		break;
	case II_TX_BYTES_IP:
		primitive = PRIMITIVE_TYPE_TX_BYTES_IP;
		break;
	case II_TX_BYTES_IP_MULTICAST:
		primitive = PRIMITIVE_TYPE_TX_BYTES_IP_MULTICAST;
		break;
	case II_TX_BYTES_PORT:
		primitive = PRIMITIVE_TYPE_TX_BYTES_PORT;
		break;
	default:
		cout << "ERROR: Unknown information item: " << _informationItem()
		<< endl;
		primitive = PRIMITIVE_TYPE_UNKNOWN;
	}

	return primitive;
}

uint32_t CIdAnalyser::sourceNodeId()
{
	return _sourceNodeId;
}

uint16_t CIdAnalyser::_informationItem()
{
	return atoi(_icnId.str().substr(_icnId.str().length() - 16, 16).c_str());
}

uint8_t CIdAnalyser::_scopeTypeLevel1()
{
	return atoi(_icnId.str().substr(16, 16).c_str());
}
