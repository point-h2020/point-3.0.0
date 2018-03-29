/*
 * mysqlconnector.cc
 *
 *  Created on: 13 Feb 2016
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

#include <ctime>
#include <memory.h>
#include <moly/enum.hh>
#include <stack>

#include "../types/enumerations.hh"
#include "mysqlconnector.hh"

MySqlConnector::MySqlConnector(log4cxx::LoggerPtr logger, string serverIpAddress,
		string serverPort, string username, string password,
		string database, MessageStack &messageStack)
	: _logger(logger),
	  _serverIpAddress(serverIpAddress),
	  _serverPort(serverPort),
	  _username(username),
	  _password(password),
	  _messageStack(messageStack)
{
	try
	{
		string url;
		url.append("tcp://");
		url.append(serverIpAddress);
		url.append(":");
		url.append(serverPort);
		_sqlDriver = get_driver_instance();
		_sqlConnection = _sqlDriver->connect(url, username, password);
		LOG4CXX_INFO(logger, "Connected to visualisation server via " << url);
		_sqlConnection->setSchema(database);
	}
	catch (sql::SQLException &e)
	{
		cout << "# ERR: SQLException in " << __FILE__;
		cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		cout << "# ERR: " << e.what();
		cout << " (MySQL error code: " << e.getErrorCode();
		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
	}
}

MySqlConnector::~MySqlConnector() {}

void MySqlConnector::operator()()
{
	LOG4CXX_DEBUG(_logger, "Starting MySQL connector thread");
	IcnId icnId;
	uint8_t *data = (uint8_t *)malloc(65535);
	uint16_t dataLength = 0;

	while(!boost::this_thread::interruption_requested())
	{
		// only read message if packet has been read correctly
		if (_messageStack.readNext(icnId, data, dataLength))
		{
			_send(icnId, data, dataLength);
			bzero(data, 65535);
		}
	}

	free(data);
	delete _sqlConnection;
}

void MySqlConnector::_send(IcnId &icnId, uint8_t *data, uint32_t dataLength)
{
	sql::Statement *sqlStatement;
	CIdAnalyser cIdAnalyser(icnId);
	ElementType elementType(icnId);
	ostringstream insertOss;
	uint32_t endpointId = 0;
	// First get the EPOCH which comes in all messages
	uint32_t epoch;
	uint16_t offset = 0;
	memcpy(&epoch, data, sizeof(epoch));
	offset = sizeof(epoch);

	switch (cIdAnalyser.primitive())
	{
	case PRIMITIVE_TYPE_ADD_LINK:
	{
		ostringstream query;
		sql::ResultSet *sqlResult;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_ADD_LINK received with "
				"timestamp " << epoch);
		uint32_t linkNameLength;
		// link name length
		memcpy(&linkNameLength, data + offset, sizeof(linkNameLength));
		offset += sizeof(linkNameLength);
		// link name (not used though at the moment)
		char linkName[linkNameLength + 1];
		memcpy(linkName, data + offset, linkNameLength);
		linkName[linkNameLength] = '\0';
		// check if link is already known
		query << "SELECT * FROM links WHERE (link_id= '" << cIdAnalyser.linkId()
				<< "' AND src = '" << cIdAnalyser.sourceNodeId() << "' AND dest"
						" = '" << cIdAnalyser.destinationNodeId() << "' )";
		LOG4CXX_TRACE(_logger, "Execute query: " << query.str());
		sqlStatement = _sqlConnection->createStatement();
		sqlResult = sqlStatement->executeQuery(query.str());

		// link exists
		if (sqlResult->next())
		{
			LOG4CXX_TRACE(_logger, "Link already exists in DB. Ignore");
			delete sqlStatement;
			delete sqlResult;
			return;
		}

		insertOss << "INSERT INTO links (link_id, src, dest, link_type)"
				"VALUES ('"
				<< cIdAnalyser.linkId() << "',"
				<< (int)cIdAnalyser.sourceNodeId() << ","
				<< (int)cIdAnalyser.destinationNodeId() << ","
				<< (int)cIdAnalyser.linkType() << ")";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_ADD_NODE:
	{
		sql::ResultSet *sqlResult;
		uint32_t nodeNameLength;
		// node name length
		memcpy(&nodeNameLength, data + offset, sizeof(nodeNameLength));
		offset += sizeof(nodeNameLength);
		// node name
		char nodeName[nodeNameLength + 1];
		memcpy(nodeName, data + offset, nodeNameLength);
		nodeName[nodeNameLength] = '\0';
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_ADD_NODE received with "
				"timestamp " << epoch);

		if (cIdAnalyser.nodeRole() == NODE_ROLE_UE ||
				cIdAnalyser.nodeRole() == NODE_ROLE_SERVER)
		{
			//First check if node ID does exist in DB and if so, call update
			ostringstream query;
			struct in_addr address;
			inet_aton(nodeName, &address);
			endpointId = address.s_addr;
			query << "SELECT * FROM nodes WHERE (name= '" << nodeName
					<< "' AND (node_type = " << ELEMENT_TYPE_UE << " OR "
							"node_type = " << ELEMENT_TYPE_SERVER << "))";
			sqlStatement = _sqlConnection->createStatement();
			sqlResult = sqlStatement->executeQuery(query.str());

			if (sqlResult->next())
			{
				if (sqlResult->getString(3).compare(nodeName) == 0)
				{
					LOG4CXX_TRACE(_logger, "IP endpoint with name '" << nodeName
							<< "' already exists in DB");
					delete sqlStatement;
					delete sqlResult;
					return;
				}
			}

			insertOss << "INSERT INTO nodes (node_id, name, node_type, "
					"endpoint_id) VALUES ("
					<< cIdAnalyser.nodeId() << ", '"
					<< nodeName << "', "
					<< (int)elementType.elementType() << ", "
					<< endpointId << ");";
			sqlStatement = _sqlConnection->createStatement();
			LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
			sqlStatement->executeUpdate(insertOss.str());
			delete sqlStatement;
			break;
		}

		//First check if node ID does exist in DB and if so, call update
		ostringstream query;
		query << "SELECT * FROM nodes WHERE (node_id= " << cIdAnalyser.nodeId()
				<< " AND NOT (node_type = " << ELEMENT_TYPE_UE << " OR "
						"node_type = " << ELEMENT_TYPE_SERVER << "))";
		sqlStatement = _sqlConnection->createStatement();
		sqlResult = sqlStatement->executeQuery(query.str());

		if (sqlResult->next())
		{
			element_type_t elementType;

			// switch over the node type in the database
			switch(sqlResult->getUInt(4))
			{
			case ELEMENT_TYPE_FN:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN;
					break;
				case NODE_ROLE_GW:
					elementType = ELEMENT_TYPE_FN_GW;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_MOOSE;
					break;
				case NODE_ROLE_NAP:
					elementType = ELEMENT_TYPE_FN_NAP;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_RV;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node role "
							<< sqlResult->getUInt(4) << " with "
							<< cIdAnalyser.nodeRole() << " has not been "
							"implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_GW:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_GW:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_GW;
					break;
				case NODE_ROLE_FN:
					elementType = ELEMENT_TYPE_FN_GW;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_GW_MOOSE;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_GW_TM;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_GW_RV;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node role "
							<< sqlResult->getUInt(4) << " with "
							<< cIdAnalyser.nodeRole() << " has not been "
							"implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_MOOSE:
			{
				switch(cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_MOOSE:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_MOOSE;
					break;
				case NODE_ROLE_FN:
					elementType = ELEMENT_TYPE_FN_MOOSE;
					break;
				case NODE_ROLE_GW:
					elementType = ELEMENT_TYPE_FN_GW_MOOSE;
					break;
				case NODE_ROLE_NAP:
					elementType = ELEMENT_TYPE_FN_MOOSE_NAP;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_MOOSE_RV;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_MOOSE_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node role "
							<< sqlResult->getUInt(4) << " with "
							<< cIdAnalyser.nodeRole() << " has not been "
							"implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_NAP:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_NAP:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_NAP;
					break;
				case NODE_ROLE_FN:
					elementType = ELEMENT_TYPE_FN_NAP;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_MOOSE_NAP;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_NAP_RV;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_NAP_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node role "
							<< sqlResult->getUInt(4) << " with "
							<< cIdAnalyser.nodeRole() << " has not been "
							"implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_RV:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_RV:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_RV;
					break;
				case NODE_ROLE_FN:
					elementType = ELEMENT_TYPE_FN_RV;
					break;
				case NODE_ROLE_GW:
					elementType = ELEMENT_TYPE_FN_GW_RV;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_MOOSE_RV;
					break;
				case NODE_ROLE_NAP:
					elementType = ELEMENT_TYPE_FN_NAP_RV;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node role "
							<< sqlResult->getUInt(4) << " with "
							<< cIdAnalyser.nodeRole() << " has not been "
							"implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_SERVER:
				LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
						<< " with role " << sqlResult->getUInt(4)
						<< " already has role " << cIdAnalyser.nodeRole());
				elementType = ELEMENT_TYPE_SERVER;
				break;
			case ELEMENT_TYPE_TM:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_TM:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_TM;
					break;
				case NODE_ROLE_FN:
					elementType = ELEMENT_TYPE_FN_TM;
					break;
				case NODE_ROLE_GW:
					elementType = ELEMENT_TYPE_FN_GW_TM;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_RV_TM;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_MOOSE_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node role "
							<< sqlResult->getUInt(4) << " with "
							<< cIdAnalyser.nodeRole() << " has not been "
							"implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_UE:
				LOG4CXX_DEBUG(_logger, "Node type 'ue' has been already "
						"reported for node ID " << cIdAnalyser.nodeId());
				delete sqlStatement;
				delete sqlResult;
				return;
			case ELEMENT_TYPE_FN_GW:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_GW:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_GW;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_GW_MOOSE;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_GW_TM;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_GW_RV;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node role "
							<< sqlResult->getUInt(4) << " with "
							<< cIdAnalyser.nodeRole() << " has not been "
							"implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}
				break;
			}
			case ELEMENT_TYPE_FN_MOOSE:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_MOOSE:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_MOOSE;
					break;
				case NODE_ROLE_NAP:
					elementType = ELEMENT_TYPE_FN_MOOSE_NAP;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_MOOSE_TM;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_MOOSE_RV;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node role "
							<< sqlResult->getUInt(4) << " with "
							<< cIdAnalyser.nodeRole() << " has not been "
							"implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}
				break;
			}
			case ELEMENT_TYPE_FN_NAP:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_NAP:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_NAP;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_MOOSE_NAP;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_NAP_RV;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_NAP_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node role "
							<< sqlResult->getUInt(4) << " with "
							<< cIdAnalyser.nodeRole() << " has not been "
							"implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_TM:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_TM:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_TM;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_MOOSE_TM;
					break;
				case NODE_ROLE_NAP:
					elementType = ELEMENT_TYPE_FN_NAP_TM;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_RV_TM;
					break;
				case NODE_ROLE_GW:
					elementType = ELEMENT_TYPE_FN_GW_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type 'FN+TM'"
						" (" << sqlResult->getUInt(4) << ") with "
						<< (int)cIdAnalyser.nodeRole()
						<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_RV:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_RV:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_RV;
					break;
				case NODE_ROLE_GW:
					elementType = ELEMENT_TYPE_FN_GW_RV;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_MOOSE_RV;
					break;
				case NODE_ROLE_NAP:
					elementType = ELEMENT_TYPE_FN_NAP_RV;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type 'FN+TM'"
						" (" << sqlResult->getUInt(4) << ") with "
						<< (int)cIdAnalyser.nodeRole()
						<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_GW_TM:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_GW:
				case NODE_ROLE_TM:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_GW_TM;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_GW_MOOSE_TM;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_GW_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type 'FN+TM'"
						" (" << sqlResult->getUInt(4) << ") with "
						<< (int)cIdAnalyser.nodeRole()
						<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_MOOSE_RV:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_MOOSE:
				case NODE_ROLE_RV:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_MOOSE_RV;
					break;
				case NODE_ROLE_GW:
					elementType = ELEMENT_TYPE_FN_GW_MOOSE_RV;
				case NODE_ROLE_NAP:
					elementType = ELEMENT_TYPE_FN_MOOSE_NAP_RV;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_MOOSE_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type 'FN+TM'"
						" (" << sqlResult->getUInt(4) << ") with "
						<< (int)cIdAnalyser.nodeRole()
						<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_MOOSE_TM:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_MOOSE:
				case NODE_ROLE_TM:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_MOOSE_TM;
					break;
				case NODE_ROLE_GW:
					elementType = ELEMENT_TYPE_FN_GW_MOOSE_TM;
					break;
				case NODE_ROLE_NAP:
					elementType = ELEMENT_TYPE_FN_MOOSE_NAP_TM;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_MOOSE_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type 'FN+TM'"
						" (" << sqlResult->getUInt(4) << ") with "
						<< (int)cIdAnalyser.nodeRole()
						<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_NAP_TM:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_NAP:
				case NODE_ROLE_TM:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_NAP_TM;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_MOOSE_NAP_TM;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_NAP_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type 'FN+TM'"
						" (" << sqlResult->getUInt(4) << ") with "
						<< (int)cIdAnalyser.nodeRole()
						<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_NAP_RV:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_NAP:
				case NODE_ROLE_RV:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_NAP_RV;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_MOOSE_NAP_RV;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_NAP_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type 'FN+TM'"
						" (" << sqlResult->getUInt(4) << ") with "
						<< (int)cIdAnalyser.nodeRole()
						<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_GW_RV:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_GW:
				case NODE_ROLE_RV:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_GW_RV;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_GW_MOOSE_RV;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_GW_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type 'FN+TM'"
						" (" << sqlResult->getUInt(4) << ") with "
						<< (int)cIdAnalyser.nodeRole()
						<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_RV_TM:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_RV:
				case NODE_ROLE_TM:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_RV_TM;
					break;
				case NODE_ROLE_NAP:
					elementType = ELEMENT_TYPE_FN_NAP_RV_TM;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_MOOSE_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type 'FN+TM'"
						" (" << sqlResult->getUInt(4) << ") with "
						<< (int)cIdAnalyser.nodeRole()
						<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_GW_MOOSE_RV:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_GW:
				case NODE_ROLE_MOOSE:
				case NODE_ROLE_RV:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_GW_MOOSE_RV;
					break;
				case NODE_ROLE_TM:
					elementType = ELEMENT_TYPE_FN_GW_MOOSE_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type 'FN+TM'"
						" (" << sqlResult->getUInt(4) << ") with "
						<< (int)cIdAnalyser.nodeRole()
						<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}
			}
			case ELEMENT_TYPE_FN_GW_MOOSE_TM:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_GW:
				case NODE_ROLE_MOOSE:
				case NODE_ROLE_TM:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_GW_MOOSE_TM;
					break;
				case NODE_ROLE_RV:
					elementType = ELEMENT_TYPE_FN_GW_MOOSE_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type 'FN+TM'"
						" (" << sqlResult->getUInt(4) << ") with "
						<< (int)cIdAnalyser.nodeRole()
						<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}
				break;
			}
			case ELEMENT_TYPE_FN_NAP_RV_TM:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_NAP:
				case NODE_ROLE_RV:
				case NODE_ROLE_TM:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_NAP_RV_TM;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_MOOSE_NAP_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type 'FN+TM'"
						" (" << sqlResult->getUInt(4) << ") with "
						<< (int)cIdAnalyser.nodeRole()
						<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_GW_RV_TM:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_GW:
				case NODE_ROLE_RV:
				case NODE_ROLE_TM:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_GW_RV_TM;
					break;
				case NODE_ROLE_MOOSE:
					elementType = ELEMENT_TYPE_FN_GW_MOOSE_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type "
							<< sqlResult->getUInt(4) << " with "
							<< (int)cIdAnalyser.nodeRole()
							<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_MOOSE_RV_TM:
			{
				switch (cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_MOOSE:
				case NODE_ROLE_RV:
				case NODE_ROLE_TM:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_MOOSE_RV_TM;
					break;
				case NODE_ROLE_GW:
					elementType = ELEMENT_TYPE_FN_GW_MOOSE_RV_TM;
					break;
				case NODE_ROLE_NAP:
					elementType = ELEMENT_TYPE_FN_MOOSE_NAP_RV_TM;
					break;
				default:
					LOG4CXX_INFO(_logger, "Updating existing node type "
							<< sqlResult->getUInt(4) << " with "
							<< (int)cIdAnalyser.nodeRole()
							<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}

				break;
			}
			case ELEMENT_TYPE_FN_GW_MOOSE_RV_TM:
			{
				switch(cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_GW:
				case NODE_ROLE_MOOSE:
				case NODE_ROLE_RV:
				case NODE_ROLE_TM:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_GW_MOOSE_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type "
							<< sqlResult->getUInt(4) << " with "
							<< (int)cIdAnalyser.nodeRole()
							<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}
				break;
			}
			case ELEMENT_TYPE_FN_MOOSE_NAP_RV_TM:
			{
				switch(cIdAnalyser.nodeRole())
				{
				case NODE_ROLE_FN:
				case NODE_ROLE_MOOSE:
				case NODE_ROLE_NAP:
				case NODE_ROLE_RV:
				case NODE_ROLE_TM:
					LOG4CXX_TRACE(_logger, "Node " << cIdAnalyser.nodeId()
							<< " with role " << sqlResult->getUInt(4)
							<< " already has role " << cIdAnalyser.nodeRole());
					elementType = ELEMENT_TYPE_FN_MOOSE_NAP_RV_TM;
					break;
				default:
					LOG4CXX_WARN(_logger, "Updating existing node type "
							<< sqlResult->getUInt(4) << " with "
							<< (int)cIdAnalyser.nodeRole()
							<< " has not been implemented");
					delete sqlStatement;
					delete sqlResult;
					return;
				}
				break;
			}
			default:
				LOG4CXX_ERROR(_logger, "Unknown element type "
						<< sqlResult->getUInt(4));
				return;
			}

			string name = sqlResult->getString(3);
			size_t pos = 0;
			string nodeNameStr(nodeName);
			// check if node name must be appended
			pos = name.find(nodeNameStr, ++pos);

			if (pos == std::string::npos)
			{
				name.append(", ");
				name.append(nodeName);
				LOG4CXX_TRACE(_logger, "Node name appended to " << name);
			}

			insertOss << "UPDATE nodes SET node_type = " << elementType << ", "
					"name = '" << name << "' WHERE node_id= "
					<< cIdAnalyser.nodeId() << ";";
		}
		else
		{
			insertOss << "INSERT INTO nodes (node_id, name, node_type, "
					"endpoint_id) VALUES (" << cIdAnalyser.nodeId() << ", '"
					<< nodeName << "', "
					<< (int)cIdAnalyser.nodeRole() << ", "
					<< endpointId << ");";
		}

		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		delete sqlResult;
		break;
	}
	case PRIMITIVE_TYPE_ADD_PORT:
	{
		uint32_t portNameLength;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_ADD_PORT received with "
				"timestamp " << epoch);
		// port name length
		memcpy(&portNameLength, data + offset, sizeof(portNameLength));
		offset += sizeof(portNameLength);
		// port name
		char portName[portNameLength + 1];
		memcpy(portName, data + offset, portNameLength);
		portName[portNameLength] = '\0';

		// check first if port exists
		ostringstream query;
		query << "SELECT * FROM ports WHERE node_id = " << cIdAnalyser.nodeId()
				<< " AND port_id = " << cIdAnalyser.portId();
		LOG4CXX_TRACE(_logger, "" << query.str());
		sqlStatement = _sqlConnection->createStatement();
		sql::ResultSet *sqlResult = sqlStatement->executeQuery(query.str());

		if (sqlResult->next())
		{
			// even port name matches. log it and return
			if (sqlResult->getString(3).compare(portName) == 0)
			{
				LOG4CXX_TRACE(_logger, "Port with name '" << portName
						<< "' (NID " << cIdAnalyser.nodeId() << ", PID "
						<< cIdAnalyser.portId() << " already exists in DB");
				delete sqlStatement;
				delete sqlResult;
				return;
			}

			LOG4CXX_TRACE(_logger, "Port in DB for NID "
					<< cIdAnalyser.nodeId() << ", PID "
					<< cIdAnalyser.portId() << " has only a different name."
					" Overwriting this one");
			insertOss << "UPDATE ports SET name = '" << portName << "' WHERE "
					"node_id=" << cIdAnalyser.nodeId() << " AND port_id = " <<
					cIdAnalyser.portId();
			//insertOss << "DELETE FROM ports WHERE node_id = "
			//		<< cIdAnalyser.nodeId() << " AND port_id = "
			//		<< cIdAnalyser.portId() << ",\n";
			LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
			sqlStatement = _sqlConnection->createStatement();
			delete sqlStatement;
			delete sqlResult;
			return;
		}

		insertOss << "INSERT INTO ports (node_id, port_id, name) VALUES ("
				<< cIdAnalyser.nodeId() << ", "
				<< cIdAnalyser.portId() << ", '"
				<< portName << "');";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		delete sqlResult;
		break;
	}
	case PRIMITIVE_TYPE_BUFFER_SIZES:
	{
		pair<buffer_name_t, buffer_size_t> listPair;
		list_size_t listSize = 0;
		LOG4CXX_TRACE(_logger, "BUFFER_SIZES_B received with timestamp "
				<< epoch);
		memcpy(&listSize, data + offset, sizeof(listSize));
		offset += sizeof(listSize);

		if (listSize == 0)
		{
			break;
		}

		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
		"attr_id, endpoint_id) VALUES";

		for (list_size_t i = 0; i < listSize; i++)
		{
			memcpy(&listPair.first, data + offset, sizeof(buffer_name_t));
			offset += sizeof(buffer_name_t);
			memcpy(&listPair.second, data + offset, sizeof(buffer_size_t));
			offset += sizeof(buffer_size_t);

			if (i == 0)
			{
				insertOss << " (";
			}
			else
			{
				insertOss << ",\n(";
			}

			insertOss << epoch << ", " << cIdAnalyser.nodeId() << ", '"
					<< listPair.second << "', ";

			switch (listPair.first)
			{
			case BUFFER_NAP_IP_HANDLER:
				insertOss << (int)ATTRIBUTE_NAP_BUFFER_SIZE_IP_HANDLER	<< ", ";
				break;
			case BUFFER_NAP_HTTP_HANDLER_ICN:
				insertOss
				<< (int)ATTRIBUTE_NAP_BUFFER_SIZE_HTTP_HANDLER_ICN
				<< ", ";
				break;
			case BUFFER_NAP_HTTP_HANDLER_REQUESTS:
				insertOss
				<< (int)ATTRIBUTE_NAP_BUFFER_SIZE_HTTP_HANDLER_REQUESTS
				<< ", ";
				break;
			case BUFFER_NAP_HTTP_HANDLER_RESPONSES:
				insertOss
				<< (int)ATTRIBUTE_NAP_BUFFER_SIZE_HTTP_HANDLER_RESPONSES
				<< ", ";
				break;
			case BUFFER_NAP_LTP:
				insertOss << (int)ATTRIBUTE_NAP_BUFFER_SIZE_LTP << ", ";
				break;
			case BUFFER_MOOSE_MESSAGE_STACK:
				insertOss
					<< (int)ATTRIBUTE_MOOSE_BUFFER_SIZE_MESSAGE_STACK << ", ";
				break;
			default:
				insertOss << (int)ATTRIBUTE_UNKNOWN << ", ";
			}

			insertOss << endpointId << ")";
			LOG4CXX_TRACE(_logger, "Buffer size for "
					<< bufferNameToString(listPair.first) << ": "
					<< listPair.second);
		}

		insertOss << ";";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_CHANNEL_AQUISITION_TIME:
	{
		uint32_t channelAquisitionTime;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_CHANNEL_AQUISITION_TIME received"
				" with timestamp " << epoch);
		memcpy(&channelAquisitionTime, data + offset,
				sizeof(channelAquisitionTime));
		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
				"attr_id, endpoint_id) VALUES ("
				<< epoch << ", "
				<< (int)cIdAnalyser.nodeId() << ", '"
				<< channelAquisitionTime / 10.0 << "', "
				<< (int)ATTRIBUTE_NAP_CHANNEL_AQUISITION_TIME << ", "
				<< endpointId << ")";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_CMC_GROUP_SIZE:
	{
		uint32_t cmcGroupSize;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_CMC_GROUP_SIZE received "
				"with timestamp " << epoch);
		memcpy(&cmcGroupSize, data + offset, sizeof(cmcGroupSize));
		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
				"attr_id, endpoint_id) VALUES ("
				<< epoch << ", "
				<< (int)cIdAnalyser.nodeId() << ", '"
				<< cmcGroupSize / 10.0 << "', "
				<< (int)ATTRIBUTE_NAP_HTTP_REQ_RES_RATIO << ", "
				<< endpointId << ")";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_CPU_UTILISATION:
	{
		uint16_t cpuUtilisation;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_CPU_UTILISATION received "
				"with timestamp " << epoch);
		memcpy(&cpuUtilisation, data + offset, sizeof(cpuUtilisation));
		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
				"attr_id, endpoint_id) VALUES ("
				<< epoch << ", "
				<< (int)cIdAnalyser.nodeId() << ", '"
				<< cpuUtilisation / 100.0 << "', ";

		switch (cIdAnalyser.nodeRole())
		{
		case NODE_ROLE_FN:
			insertOss << (int)ATTRIBUTE_FN_CPU_UTILISATION << ", ";
			insertOss << 0 << ");";
			break;
		case NODE_ROLE_TM:
			insertOss << (int)ATTRIBUTE_TM_CPU_UTILISATION << ", ";
			insertOss << 0 << ");";
			break;
		case NODE_ROLE_RV:
			insertOss << (int)ATTRIBUTE_RV_CPU_UTILISATION << ", ";
			insertOss << 0 << ");";
			break;
		case NODE_ROLE_NAP:
			insertOss << (int)ATTRIBUTE_NAP_CPU_UTILISATION << ", ";
			insertOss << 0 << ");";
			break;
		case NODE_ROLE_GW:
			insertOss << (int)ATTRIBUTE_GW_CPU_UTILISATION << ", ";
			insertOss << 0 << ");";
			break;
		case NODE_ROLE_UE:
			insertOss << (int)ATTRIBUTE_UE_CPU_UTILISATION << ", ";
			insertOss << cIdAnalyser.endpointId() << ");";
			break;
		case NODE_ROLE_SERVER:
			insertOss << (int)ATTRIBUTE_SV_CPU_UTILISATION << ", ";
			insertOss << cIdAnalyser.endpointId() << ");";
			break;
		default:
			insertOss << (int)ATTRIBUTE_UNKNOWN << ", ";
		}

		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_END_TO_END_LATENCY:
	{
		uint16_t latency;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_END_TO_END_LATENCY received "
				"with timestamp " << epoch);
		memcpy(&latency, data + offset, sizeof(latency));
		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
				"attr_id, endpoint_id) VALUES ("
				<< epoch << ", "
				<< (int)cIdAnalyser.nodeId() << ", '"
				<< latency / 10.0 << "', ";

		switch (cIdAnalyser.nodeRole())
		{
		case NODE_ROLE_UE:
			insertOss << (int)ATTRIBUTE_UE_E2E_LATENCY << ", ";
			break;
		case NODE_ROLE_SERVER:
			insertOss << (int)ATTRIBUTE_UE_E2E_LATENCY << ", ";
			break;
		default:
			 insertOss << ATTRIBUTE_UE_E2E_LATENCY << ", ";
		}

		insertOss << cIdAnalyser.endpointId() << ");";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_FILE_DESCRIPTORS_TYPE:
	{
		pair<file_descriptor_type_t, file_descriptors_t> listPair;
		list_size_t listSize = 0;
		LOG4CXX_TRACE(_logger, "FILE_DESCRIPTORS_TYPE received with timestamp "
				<< epoch);
		memcpy(&listSize, data + offset, sizeof(listSize));
		offset += sizeof(listSize);

		if (listSize == 0)
		{
			break;
		}

		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
		"attr_id, endpoint_id) VALUES";

		for (list_size_t i = 0; i < listSize; i++)
		{
			memcpy(&listPair.first, data + offset,
					sizeof(file_descriptor_type_t));
			offset += sizeof(file_descriptor_type_t);
			memcpy(&listPair.second, data + offset, sizeof(file_descriptors_t));
			offset += sizeof(file_descriptors_t);

			if (i == 0)
			{
				insertOss << " (";
			}
			else
			{
				insertOss << ",\n(";
			}

			insertOss << epoch << ", " << cIdAnalyser.nodeId() << ", '"
					<< listPair.second << "', ";

			switch (listPair.first)
			{
			case DESCRIPTOR_TYPE_TCP_SOCKET:
			{
				if (cIdAnalyser.nodeRole() == NODE_ROLE_NAP ||
						cIdAnalyser.nodeRole() == NODE_ROLE_GW)
				{
					insertOss << (int)ATTRIBUTE_NAP_FILE_DESCRIPTOR_TCP	<< ", ";
				}
				else
				{
					LOG4CXX_WARN(_logger, "Node role " << cIdAnalyser.nodeRole()
							<< " not implemented for FD reporting");
					insertOss << (int)ATTRIBUTE_UNKNOWN << ", ";
				}
				break;
			}
			default:
				LOG4CXX_WARN(_logger, "Unknown file descriptor type of value "
						<< listPair.first << " received from NID "
						<< cIdAnalyser.nodeId());
				insertOss << (int)ATTRIBUTE_UNKNOWN << ", ";
			}

			insertOss << endpointId << ")";
		}

		insertOss << ";";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_HTTP_REQUESTS_FQDN:
	{
		LOG4CXX_TRACE(_logger, "HTTP_REQUESTS_FQDN_B received with timestamp "
				<< epoch);
		uint32_t httpRequests;
		uint32_t fqdnLength;
		attribute_t attributeId;
		memcpy(&fqdnLength, data + offset, sizeof(fqdnLength));
		offset += sizeof(fqdnLength);
		char fqdn[fqdnLength + 1];
		memcpy(fqdn, data + offset, fqdnLength);
		fqdn[fqdnLength] = '\0';
		offset += fqdnLength;
		memcpy(&httpRequests, data + offset, sizeof(httpRequests));
		LOG4CXX_TRACE(_logger, "# of HTTP Requests for " << fqdn << ": "
				<< httpRequests);

		switch(cIdAnalyser.nodeRole())
		{
		case NODE_ROLE_NAP:
			attributeId = ATTRIBUTE_NAP_HTTP_REQUESTS;
			break;
		case NODE_ROLE_GW:
			attributeId = ATTRIBUTE_GW_HTTP_REQUESTS;
			break;
		default:
			attributeId = ATTRIBUTE_UNKNOWN;
			LOG4CXX_WARN(_logger, "Unknown node role for HTTP_REQUESTS_FQDN");
		}

		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
				"attr_id, endpoint_id) VALUES ("
				<< epoch << ", "
				<< (int)cIdAnalyser.nodeId() << ", '"
				<< httpRequests << "', "
				<< (int)attributeId << ", "
				<< endpointId << ");";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_LINK_STATE:
	{
		link_state_t state;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_LINK_STATE "
						"received with timestamp " << epoch);
		memcpy(&state, data + offset, sizeof(state));
		insertOss << "INSERT INTO links_values (timestamp, link_id, dest, "
				"value, attr_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.linkId() << ", "
				<< (int)cIdAnalyser.destinationNodeId() << ", '"
				<< _stateStr(PRIMITIVE_TYPE_LINK_STATE, state)
				<< "', " << (int)ATTRIBUTE_LNK_STATE << ");";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_MATCHES_NAMESPACE:
	{
		pair<root_scope_t, matches_t> listPair;
		list_size_t listSize = 0;
		LOG4CXX_TRACE(_logger, "MATCHES_NAMESPACE_B received with timestamp "
				<< epoch);
		memcpy(&listSize, data + offset, sizeof(listSize));
		offset += sizeof(listSize);

		if (listSize == 0)
		{
			break;
		}

		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
		"attr_id, endpoint_id) VALUES";

		for (list_size_t i = 0; i < listSize; i++)
		{
			memcpy(&listPair.first, data + offset, sizeof(root_scope_t));
			offset += sizeof(root_scope_t);
			memcpy(&listPair.second, data + offset, sizeof(matches_t));
			offset += sizeof(matches_t);

			if (i == 0)
			{
				insertOss << " (";
			}
			else
			{
				insertOss << ",\n(";
			}

			insertOss << epoch << ", " << cIdAnalyser.nodeId() << ", '"
					<< listPair.second << "', ";

			switch (listPair.first)
			{
			case NAMESPACE_IP:
				insertOss << (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_IP	<< ", ";
				break;
			case NAMESPACE_HTTP:
				insertOss << (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_HTTP << ", ";
				break;
			case NAMESPACE_COAP:
				insertOss << (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_COAP << ", ";
				break;
			case NAMESPACE_MONITORING:
				insertOss
					<< (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_MONITORING << ", ";
				break;
			case NAMESPACE_MANAGEMENT:
				insertOss
					<< (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_MANAGEMENT << ", ";
				break;
			case NAMESPACE_SURROGACY:
				insertOss << (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_SURROGACY
						<< ", ";
				break;
			case NAMESPACE_LINK_STATE:
				insertOss << (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_LINK_STATE
						<< ", ";
				break;
			case NAMESPACE_PATH_MANAGEMENT:
				insertOss << (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_PATH_MANAGEMENT
						<< ", ";
			case NAMESPACE_EXPERIMENTATION:
				insertOss << (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_EXPERIMENTATION
						<< ", ";
				break;
			case NAMESPACE_IGMP_CTRL:
				insertOss << (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_IGMP_CTRL << ", ";
				break;
			//case NAMESPACE_IGMP_DATA:
			case 16:
				insertOss << (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_IGMP_DATA
						<< ", ";
				break;
			//case NAMESPACE_APP_RV:
			case 17:
				insertOss << (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_APP_RV << ",";
				break;
			//case NAMESPACE_RV_TM:
			case 18:
				insertOss << (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_RV_TM << ",";
				break;
			//case NAMESPACE_TM_APP:
			case 19:
				insertOss << (int)ATTRIBUTE_RV_MATCHES_NAMESPACE_TM_APP << ",";
				break;
			default:
				LOG4CXX_WARN(_logger, "Unknown root namspace " << listPair.first
						<< " for Primitive "
						<< PRIMITIVE_TYPE_MATCHES_NAMESPACE);
				insertOss << (int)ATTRIBUTE_UNKNOWN << ", ";
			}

			insertOss << endpointId << ")";
			LOG4CXX_TRACE(_logger, "Pub/sub matches root scope "
					<< scopeToString(listPair.first) << ": " << listPair.second);
		}

		insertOss << ";";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_NETWORK_LATENCY_FQDN:
	{
		uint32_t fqdn;
		uint16_t networkLatency;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_NETWORK_LATENCY_FQDN "
				"received with timestamp " << epoch);
		memcpy(&fqdn, data + offset, sizeof(fqdn));
		offset += sizeof(fqdn);
		memcpy(&networkLatency, data + offset, sizeof(networkLatency));
		LOG4CXX_TRACE(_logger, "Network latency at NAP " << cIdAnalyser.nodeId()
				<< " for hashed FQDN " << fqdn << ": " << (int)networkLatency);
		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
				"attr_id, endpoint_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.nodeId() << ", '"
				<< networkLatency / 10.0 << "', "
				<< (int)ATTRIBUTE_NAP_NETWORK_LATENCY << ", "
				<< endpointId << ");";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_NODE_STATE:
	{
		node_state_t state;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_NODE_STATE received with "
				"timestamp " << epoch);
		memcpy(&state, data + offset, sizeof(state));

		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
					"attr_id, endpoint_id) VALUES ("
					<< epoch << ", "
					<< cIdAnalyser.nodeId() << ", '"
					<< _stateStr(PRIMITIVE_TYPE_PORT_STATE, state) << "', "
					<< (int)ATTRIBUTE_FN_PORT_STATE << ", ";

		if (cIdAnalyser.nodeRole() == NODE_ROLE_UE ||
				cIdAnalyser.nodeRole() == NODE_ROLE_SERVER)
		{
			insertOss << cIdAnalyser.endpointId() << ");";
		}
		else
		{
			insertOss << "0);";
		}

		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_PACKET_DROP_RATE:
	{
		uint32_t rate;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_PACKET_DROP_RATE_B "
				"received with timestamp " << epoch);
		memcpy(&rate, data + offset, sizeof(rate));
		insertOss << "INSERT INTO ports_values (timestamp, node_id, port_id, "
				"value, attr_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.nodeId() << ", "
				<< cIdAnalyser.portId() << ", ";

		if (rate != 0)
		{
			LOG4CXX_TRACE(_logger, "Packet drop rate at NID "
					<< cIdAnalyser.nodeId() << " > Port ID "
					<< cIdAnalyser.portId()	<< " is " << 1 / rate);
			insertOss << 1 / rate << ", ";
		}
		else
		{
			LOG4CXX_TRACE(_logger, "Packet drop rate at NID "
				<< cIdAnalyser.nodeId() << " > Port ID "
				<< cIdAnalyser.portId()	<< " is 0");
			insertOss << "0" << ", ";
		}

		insertOss << (int)ATTRIBUTE_FN_PACKET_DROP_RATE_PORT << ");";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_PACKET_ERROR_RATE:
	{
		uint32_t rate;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_PACKET_ERROR_RATE_B "
				"received with timestamp " << epoch);
		memcpy(&rate, data + offset, sizeof(rate));
		insertOss << "INSERT INTO ports_values (timestamp, node_id, port_id, "
				"value, attr_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.nodeId() << ", "
				<< cIdAnalyser.portId() << ", ";

		if (rate != 0)
		{
			LOG4CXX_TRACE(_logger, "Packet error rate at NID "
					<< cIdAnalyser.nodeId() << " > Port ID "
					<< cIdAnalyser.portId() << " is " << 1 / rate);
			insertOss << 1 / rate << ", ";
		}
		else
		{
			LOG4CXX_TRACE(_logger, "Packet error rate at NID "
					<< cIdAnalyser.nodeId() << " > Port ID "
					<< cIdAnalyser.portId() << " is 0");
			insertOss << "0" << ", ";
		}

		insertOss << (int)ATTRIBUTE_FN_PACKET_ERROR_RATE_PORT << ");";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_PACKET_JITTER_CID:
	{
		pair<content_identifier_t, packet_jitter_t> listPair;
		list_size_t listSize = 0;
		LOG4CXX_TRACE(_logger, "PACKET_JITTER_CID_B received with timestamp "
				<< epoch);
		memcpy(&listSize, data + offset, sizeof(list_size_t));
		offset += sizeof(list_size_t);

		if (listSize == 0)
		{
			break;
		}

		for (list_size_t i = 0; i < listSize; i++)
		{
			memcpy(&listPair.first, data + offset, sizeof(listPair.first));
			offset += sizeof(listPair.first);
			memcpy(&listPair.second, data + offset, sizeof(listPair.second));
			offset += sizeof(listPair.second);
			insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
			"attr_id, endpoint_id) VALUES ("
			<< epoch << ", "
			<< cIdAnalyser.nodeId() << ", '"
			<< listPair.first << "/" << listPair.second << "', ";//TODO is that the right format?

			if (cIdAnalyser.nodeRole() == NODE_ROLE_FN)
			{
				insertOss << (int)ATTRIBUTE_FN_PACKET_JITTER_CID << ", ";
				LOG4CXX_TRACE(_logger, "Packet jitter on FN("
						<< cIdAnalyser.nodeId() << ") for hashed CID "
						<< listPair.first << ": " << listPair.second);
			}
			else if (cIdAnalyser.nodeRole() == NODE_ROLE_NAP)
			{
				insertOss<< (int)ATTRIBUTE_NAP_PACKET_JITTER_CID << ", ";
				LOG4CXX_TRACE(_logger, "Packet jitter on NAP("
							<< cIdAnalyser.nodeId() << ") for hashed CID "
							<< listPair.first << ": " << listPair.second);
			}
			else
			{
				insertOss << ATTRIBUTE_UNKNOWN << ", ";
			}

			insertOss << endpointId << ");\n";
		}

		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_PATH_CALCULATIONS_NAMESPACE:
	{
		pair<root_scope_t, path_calculations_t> listPair;
		list_size_t listSize = 0;
		LOG4CXX_TRACE(_logger, "PATH_CALCULATIONS_NAMESPACE_B received "
				"with timestamp " << epoch);
		memcpy(&listSize, data + offset, sizeof(list_size_t));
		offset += sizeof(list_size_t);

		if (listSize == 0)
		{
			break;
		}

		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
					"attr_id, endpoint_id) VALUES";

		for (list_size_t i = 0; i < listSize; i++)
		{
			memcpy(&listPair.first, data + offset, sizeof(root_scope_t));
			offset += sizeof(root_scope_t);
			memcpy(&listPair.second, data + offset, sizeof(path_calculations_t));
			offset += sizeof(path_calculations_t);

			if (i == 0)
			{
				insertOss << "(";
			}
			else
			{
				insertOss << ",\n(";
			}

			insertOss << epoch << ", "
					<< cIdAnalyser.nodeId() << ", '"
					<< listPair.second << "', ";

			switch (listPair.first)
			{
			case NAMESPACE_IP:
				insertOss << (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_IP
					<< ", ";
				break;
			case NAMESPACE_HTTP:
				insertOss << (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_HTTP
					<< ", ";
				break;
			case NAMESPACE_COAP:
				insertOss << (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_COAP
					<< ", ";
				break;
			case NAMESPACE_MONITORING:
				insertOss
					<< (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_MONITORING
					<< ", ";
				break;
			case NAMESPACE_MANAGEMENT:
				insertOss
					<< (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_MANAGEMENT
					<< ", ";
				break;
			case NAMESPACE_SURROGACY:
				insertOss
					<< (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_SURROGACY
					<< ", ";
				break;
			case NAMESPACE_LINK_STATE:
				insertOss
					<< (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_LINK_STATE
					<< ", ";
				break;
			case NAMESPACE_PATH_MANAGEMENT:
				insertOss
					<< (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_PATH_MANAGEMENT
					<< ", ";
				break;
			case NAMESPACE_EXPERIMENTATION:
				insertOss
					<< (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_EXPERIMENTATION
					<< ", ";
				break;
			case NAMESPACE_IGMP_CTRL:
				insertOss << (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_IGMP_CTRL
					<< ", ";
				break;
			//case NAMESPACE_IGMP_DATA:
			case 16:
				insertOss
					<< (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_IGMP_DATA
					<< ", ";
				break;
			//case NAMESPACE_APP_RV:
			case 17:
				insertOss
					<< (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_APP_RV
					<< ", ";
				break;
			//case NAMESPACE_RV_TM:
			case 18:
				insertOss
					<< (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_RV_TM
					<< ", ";
				break;
			//case NAMESPACE_TM_APP:
			case 19:
				insertOss
					<< (int)ATTRIBUTE_TM_PATH_CALCULATIONS_NAMESPACE_TM_APP
					<< ", ";
				break;
			default:
				LOG4CXX_WARN(_logger, "Unknown root namspace " << listPair.first
						<< " for Primitive "
						<< PRIMITIVE_TYPE_PATH_CALCULATIONS_NAMESPACE);
				insertOss << (int)ATTRIBUTE_UNKNOWN << ", ";
			}

			insertOss << endpointId << ")";
			LOG4CXX_TRACE(_logger, "Number of path calculations for root scope "
					<< scopeToString(listPair.first) << ": " << listPair.second);
		}

		insertOss << ";";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_PORT_STATE:
	{
		port_state_t state;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_PORT_STATE "
						"received with timestamp " << epoch);
		memcpy(&state, data + offset, sizeof(state));
		insertOss << "INSERT INTO ports_values (timestamp, node_id, port_id, "
				"value, attr_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.nodeId() << ", "
				<< cIdAnalyser.portId() << ", '"
				<< _stateStr(PRIMITIVE_TYPE_PORT_STATE, state)
				<< "', " << (int)ATTRIBUTE_FN_PORT_STATE << ");";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_PUBLISHERS_NAMESPACE:
	{
		pair<root_scope_t, publishers_t> listPair;
		list_size_t listSize = 0;
		LOG4CXX_TRACE(_logger, "PUBLISHERS_NAMESPACE_B received with "
				"timestamp " << epoch);
		memcpy(&listSize, data + offset, sizeof(listSize));
		offset += sizeof(listSize);

		if (listSize == 0)
		{
			break;
		}

		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
					"attr_id, endpoint_id) VALUES ";

		for (uint32_t i = 0; i < listSize; i++)
		{
			memcpy(&listPair.first, data + offset, sizeof(listPair.first));
			offset += sizeof(listPair.first);
			memcpy(&listPair.second, data + offset, sizeof(listPair.second));
			offset += sizeof(listPair.second);

			if (i == 0)
			{
				insertOss << "(";
			}
			else
			{
				insertOss << ",\n(";
			}

			insertOss << epoch << ", " << cIdAnalyser.nodeId() << ", '"
					<< listPair.second << "', ";

			switch (listPair.first)
			{
			case NAMESPACE_IP:
				insertOss << (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_IP;
				break;
			case NAMESPACE_HTTP:
				insertOss << (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_HTTP;
				break;
			case NAMESPACE_COAP:
				insertOss << (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_COAP;
				break;
			case NAMESPACE_MONITORING:
				insertOss << (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_MONITORING;
				break;
			case NAMESPACE_MANAGEMENT:
				insertOss << (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_MANAGEMENT;
				break;
			case NAMESPACE_SURROGACY:
				insertOss << (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_SURROGACY;
				break;
			case NAMESPACE_LINK_STATE:
				insertOss << (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_LINK_STATE;
				break;
			case NAMESPACE_PATH_MANAGEMENT:
				insertOss
					<< (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_PATH_MANAGEMENT;
				break;
			case NAMESPACE_EXPERIMENTATION:
				insertOss
					<< (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_EXPERIMENTATION;
				break;
			case NAMESPACE_IGMP_CTRL:
				insertOss << (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_IGMP_CTRL;
				break;
			//case NAMESPACE_IGMP_DATA:
			case 16:
				insertOss
					<< (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_IGMP_DATA;
				break;
			//case NAMESPACE_APP_RV:
			case 17:
				insertOss << (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_APP_RV;
				break;
			//case NAMESPACE_RV_TM:
			case 18:
				insertOss << (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_RV_TM;
				break;
			//case NAMESPACE_TM_APP:
			case 19:
				insertOss << (int)ATTRIBUTE_RV_PUBLISHERS_NAMESPACE_TM_APP;
				break;
			default:
				LOG4CXX_WARN(_logger, "Unknown root namspace " << listPair.first
						<< " for Primitive "
						<< PRIMITIVE_TYPE_PUBLISHERS_NAMESPACE);
				insertOss << (int)ATTRIBUTE_UNKNOWN;
			}

			insertOss << ", ";
			insertOss << endpointId << ")";
			LOG4CXX_TRACE(_logger, "Number of publishers in namespace "
					<< scopeToString(listPair.first) << ": " << listPair.second);
		}

		insertOss << ";";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_RX_BYTES_HTTP:
	{
		bytes_t bytes;
		LOG4CXX_TRACE(_logger, "RX_BYTES_HTTP received with timestamp "
				<< epoch);
		memcpy(&bytes, data + offset, sizeof(bytes_t));
		LOG4CXX_TRACE(_logger, "RX HTTP bytes for NID " << cIdAnalyser.nodeId()
				<< "/Role " << nodeRoleToString(cIdAnalyser.nodeRole()) << ": "
				<< bytes);
		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
				"attr_id, endpoint_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.nodeId() << ", '"
				<< bytes << "', "
				<< (int)ATTRIBUTE_NAP_RX_BYTES_HTTP
				<< ", 0);";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_RX_BYTES_IP:
	{
		bytes_t bytes;
		LOG4CXX_TRACE(_logger, "RX_BYTES_IP received with timestamp "
				<< epoch);
		memcpy(&bytes, data + offset, sizeof(bytes_t));
		LOG4CXX_TRACE(_logger, "RX IP bytes for NID " << cIdAnalyser.nodeId()
				<< "/Role " << nodeRoleToString(cIdAnalyser.nodeRole()) << ": "
				<< bytes);
		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
				"attr_id, endpoint_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.nodeId() << ", '"
				<< bytes << "', "
				<< (int)ATTRIBUTE_NAP_RX_BYTES_IP
				<< ", 0);";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_RX_BYTES_IP_MULTICAST:
	{
		bytes_t bytes;
		ip_version_t ipVersion;
		LOG4CXX_TRACE(_logger, "RX_BYTES_IP_MULTICAST received with timestamp "
				<< epoch);
		memcpy(&ipVersion, data + offset, sizeof(ip_version_t));
		offset += sizeof(ip_version_t);
		memcpy(&bytes, data + offset, sizeof(bytes_t));
		LOG4CXX_TRACE(_logger, bytes << " RX IP multicast bytes via IPv"
				<< ipVersion << " for NID " << cIdAnalyser.nodeId()	<< "/Role "
				<< nodeRoleToString(cIdAnalyser.nodeRole()));
		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
				"attr_id, endpoint_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.nodeId() << ", '"
				<< bytes << "', ";

		if (ipVersion == 4)
		{
			insertOss << (int)ATTRIBUTE_NAP_RX_BYTES_IPV4_MULTICAST;
		}
		else if (ipVersion == 6)
		{
			insertOss << (int)ATTRIBUTE_NAP_RX_BYTES_IPV6_MULTICAST;
		}
		else
		{
			LOG4CXX_INFO(_logger, "Primitive "
					<< PRIMITIVE_TYPE_RX_BYTES_IP_MULTICAST << " did comprise "
					"an unknown IP version " << ipVersion << "). Assuming V4");
			insertOss << ATTRIBUTE_NAP_RX_BYTES_IPV4_MULTICAST;
		}

		insertOss << ", " << "0);";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_RX_BYTES_PORT:
	{
		uint32_t bytes;
		LOG4CXX_TRACE(_logger, "PRIMITIVE_TYPE_RX_BYTES_PORT received with "
				"timestamp " << epoch);
		memcpy(&bytes, data + offset, sizeof(bytes));
		LOG4CXX_TRACE(_logger, bytes << " RX Bytes for NID "
				<< cIdAnalyser.nodeId()	<< " > Port ID "
				<< cIdAnalyser.portId());
		insertOss << "INSERT INTO ports_values (timestamp, node_id, port_id, "
				"value, attr_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.nodeId() << ", "
				<< cIdAnalyser.portId() << ", "
				<< bytes << ", "
				<< (int)ATTRIBUTE_FN_RX_BYTES_PORT << ");";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_SUBSCRIBERS_NAMESPACE:
	{
		pair<root_scope_t, subscribers_t> listPair;
		uint32_t listSize = 0;
		LOG4CXX_TRACE(_logger, "SUBSCRIBERS_NAMESPACE_B received with "
				"timestamp " << epoch);
		memcpy(&listSize, data + offset, sizeof(listSize));
		offset += sizeof(listSize);

		if (listSize == 0)
		{
			break;
		}

		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
					"attr_id, endpoint_id) VALUES";

		for (uint32_t i = 0; i < listSize; i++)
		{
			memcpy(&listPair.first, data + offset, sizeof(listPair.first));
			offset += sizeof(listPair.first);
			memcpy(&listPair.second, data + offset, sizeof(listPair.second));
			offset += sizeof(listPair.second);

			if (i == 0)
			{
				insertOss << " (";
			}
			else
			{
				insertOss << ",\n(";
			}

			insertOss << epoch << ", " << cIdAnalyser.nodeId() << ", '"
					<< listPair.second << "', ";

			switch (listPair.first)
			{
			case NAMESPACE_IP:
				insertOss << (int)ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_IP;
				break;
			case NAMESPACE_HTTP:
				insertOss << (int)ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_HTTP;
				break;
			case NAMESPACE_COAP:
				insertOss << (int)ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_COAP;
				break;
			case NAMESPACE_MONITORING:
				insertOss << (int)ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_MONITORING;
				break;
			case NAMESPACE_MANAGEMENT:
				insertOss << (int)ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_MANAGEMENT;
				break;
			case NAMESPACE_SURROGACY:
				insertOss << (int)ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_SURROGACY;
				break;
			case NAMESPACE_LINK_STATE:
				insertOss << (int)ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_LINK_STATE;
				break;
			case NAMESPACE_PATH_MANAGEMENT:
				insertOss << ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_PATH_MANAGEMENT;
				break;
			case NAMESPACE_EXPERIMENTATION:
				insertOss
					<< (int)ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_EXPERIMENTATION;
				break;
			case NAMESPACE_IGMP_CTRL:
				insertOss << ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_IGMP_CTRL;
				break;
			//case NAMESPACE_IGMP_DATA:
			case 16:
				insertOss << ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_IGMP_DATA;
				break;
			//case NAMESPACE_APP_RV:
			case 17:
				insertOss << ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_APP_RV;
				break;
			//case NAMESPACE_RV_TM:
			case 18:
				insertOss << ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_RV_TM;
				break;
			//case NAMESPACE_TM_APP:
			case 19:
				insertOss << ATTRIBUTE_RV_SUBSCRIBERS_NAMESPACE_TM_APP;
				break;
			default:
				LOG4CXX_WARN(_logger, "Unknown root namspace " << listPair.first
						<< " for Primitive "
						<< PRIMITIVE_TYPE_SUBSCRIBERS_NAMESPACE);
				insertOss << ATTRIBUTE_UNKNOWN;
			}

			insertOss << ", " << endpointId << ")";
			LOG4CXX_TRACE(_logger, "Number of subscribers for namespace "
					<< scopeToString(listPair.first) << ": " << listPair.second);
		}

		insertOss << ";";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_TX_BYTES_HTTP:
	{
		bytes_t bytes;
		LOG4CXX_TRACE(_logger, "TX_BYTES_HTTP received with timestamp "
				<< epoch);
		memcpy(&bytes, data + offset, sizeof(bytes_t));
		LOG4CXX_TRACE(_logger, "TX HTTP bytes for NID " << cIdAnalyser.nodeId()
				<< "/Role "	<< nodeRoleToString(cIdAnalyser.nodeRole()) << ": "
				<< bytes);
		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
				"attr_id, endpoint_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.nodeId() << ", '"
				<< bytes << "', "
				<< (int)ATTRIBUTE_NAP_TX_BYTES_HTTP
				<< ", 0);";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_TX_BYTES_IP:
	{
		bytes_t bytes;
		LOG4CXX_TRACE(_logger, "TX_BYTES_IP received with timestamp "
				<< epoch);
		memcpy(&bytes, data + offset, sizeof(bytes_t));
		LOG4CXX_TRACE(_logger, "TX IP bytes for NID " << cIdAnalyser.nodeId()
				<< "/Role "	<< nodeRoleToString(cIdAnalyser.nodeRole()) << ": "
				<< bytes);
		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
				"attr_id, endpoint_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.nodeId() << ", '"
				<< bytes << "', "
				<< (int)ATTRIBUTE_NAP_TX_BYTES_IP
				<< ", 0);";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_TX_BYTES_IP_MULTICAST:
	{
		bytes_t bytes;
		ip_version_t ipVersion;
		LOG4CXX_TRACE(_logger, "TX_BYTES_IP_MULTICAST received with timestamp "
				<< epoch);
		memcpy(&ipVersion, data + offset, sizeof(ip_version_t));
		offset += sizeof(ip_version_t);
		memcpy(&bytes, data + offset, sizeof(bytes_t));
		LOG4CXX_TRACE(_logger, bytes << " TX IP multicast bytes via IPv"
				<< ipVersion << " for NID " << cIdAnalyser.nodeId()	<< "/Role "
				<< nodeRoleToString(cIdAnalyser.nodeRole()));
		insertOss << "INSERT INTO nodes_values (timestamp, node_id, value, "
				"attr_id, endpoint_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.nodeId() << ", '"
				<< bytes << "', ";

		if (ipVersion == 4)
		{
			insertOss << (int)ATTRIBUTE_NAP_TX_BYTES_IPV4_MULTICAST;
		}
		else if (ipVersion == 6)
		{
			insertOss << (int)ATTRIBUTE_NAP_TX_BYTES_IPV6_MULTICAST;
		}

		insertOss << ", " << "0);";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	case PRIMITIVE_TYPE_TX_BYTES_PORT:
	{
		uint32_t bytes;
		LOG4CXX_TRACE(_logger, "TX_BYTES_PORT received with "
				"timestamp " << epoch);
		memcpy(&bytes, data + offset, sizeof(bytes));
		LOG4CXX_TRACE(_logger, bytes << " TX Bytes for NID "
				<< cIdAnalyser.nodeId()	<< " > Port ID "
				<< cIdAnalyser.portId());
		insertOss << "INSERT INTO ports_values (timestamp, node_id, port_id, "
				"value, attr_id) VALUES ("
				<< epoch << ", "
				<< cIdAnalyser.nodeId() << ", "
				<< cIdAnalyser.portId() << ", "
				<< bytes << ", "
				<< (int)ATTRIBUTE_FN_TX_BYTES_PORT << ");";
		sqlStatement = _sqlConnection->createStatement();
		LOG4CXX_TRACE(_logger, "Execute query: " << insertOss.str());
		sqlStatement->executeUpdate(insertOss.str());
		delete sqlStatement;
		break;
	}
	default:
		LOG4CXX_WARN(_logger, "Unsuported primitive for CID "
				<< icnId.print());
	}
}

string MySqlConnector::_stateStr(primitive_type_t primitive, uint8_t state)
{
	ostringstream ossState;

	switch (primitive)
	{
	case PRIMITIVE_TYPE_LINK_STATE:
		if (state == LINK_STATE_DOWN)
		{
			ossState << "STATE_DOWN";
		}
		else if (state == LINK_STATE_UP)
		{
			ossState << "STATE_UP";
		}
		else
		{
			ossState << "STATE_UNKNOWN";
		}

		break;
	case PRIMITIVE_TYPE_NODE_STATE:
		if (state == NODE_STATE_DOWN)
		{
			ossState << "STATE_DOWN";
		}
		else if (state == NODE_STATE_UP)
		{
			ossState << "STATE_UP";
		}
		else
		{
			ossState << "STATE_UNKNOWN";
		}

		break;
	case PRIMITIVE_TYPE_PORT_STATE:
		if (state == PORT_STATE_DOWN)
		{
			ossState << "STATE_DOWN";
		}
		else if (state == PORT_STATE_UP)
		{
			ossState << "STATE_UP";
		}
		else
		{
			ossState << "STATE_UNKNOWN";
		}

		break;
	default:
		ossState << "STATE_UNKNOWN";
	}

	return ossState.str();
}
