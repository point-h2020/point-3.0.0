/*
 * filesystemchecker.cc
 *
 *  Created on: 1 Jan 2017
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

#include "filesystemchecker.hh"

FilesystemChecker::FilesystemChecker(log4cxx::LoggerPtr logger,
		string directory, Scopes &scopes, Blackadder *icnCore)
: _logger(logger),
  _directory(directory),
  _scopes(scopes),
  _icnCore(icnCore)
{}

FilesystemChecker::~FilesystemChecker() {}

void FilesystemChecker::operator ()()
{
	LOG4CXX_DEBUG(_logger, "Filesystem checker enabled for directory "
			<< _directory);
	uint16_t nodeId;
	uint16_t dataPoint;

	while (!boost::this_thread::interruption_requested())
	{
		path p(_directory);

		try
		{
			if (exists(p) && is_directory(p))
			{
				ostringstream oss;
				copy(directory_iterator(p), directory_iterator(),
						ostream_iterator<directory_entry>(oss));

				if (oss.str().length() != 0)
				{
					LOG4CXX_TRACE(_logger, "New file found: " << oss.str());
					string file;
					//get parantheses-free path+filename
					file.assign(oss.str());
					vector<string> strs;
					boost::split(strs, file, boost::is_any_of("\""));
					string toBeDeleted = strs[1];
					strs.clear();
					//get filename only
					string filename;
					boost::split(strs, file, boost::is_any_of("/"));
					filename = strs[strs.size() - 1];
					//get filename values
					boost::split(strs, filename, boost::is_any_of("-"));
					dataPoint = (uint16_t)atoi(strs[0].c_str());
					nodeId = (uint16_t)atoi(strs[1].c_str());

					// /monitoring/nodes/nodeType/nodeId/dataPoint
					IcnId cid;
					cid.rootNamespace(NAMESPACE_MONITORING);

					if (!_scopes.published(cid))
					{
						_scopes.add(cid);
						LOG4CXX_TRACE(_logger, "Publish root namespace "
								<< cid.print());
						_icnCore->publish_scope(cid.binId(), cid.binEmpty(),
								DOMAIN_LOCAL, NULL, 0);
						_scopes.setPublicationState(cid, true);
					}

					cid.append(TOPOLOGY_NODES);

					if (!_scopes.published(cid))
					{
						_scopes.add(cid);
						LOG4CXX_TRACE(_logger, "Publish scope path "
								<< cid.print());
						_icnCore->publish_scope(cid.binId(),
								cid.binPrefixId(), DOMAIN_LOCAL, NULL, 0);
						_scopes.setPublicationState(cid, true);
					}

					// now switch over the data point to determine node type
					cid.append(_nodeRole(dataPoint));

					if (!_scopes.published(cid))
					{
						_scopes.add(cid);
						LOG4CXX_TRACE(_logger, "Publish scope path "
								<< cid.print());
						_icnCore->publish_scope(cid.binId(),
								cid.binPrefixId(), DOMAIN_LOCAL, NULL,
								0);
						_scopes.setPublicationState(cid, true);
					}

					// NID
					cid.append(nodeId);

					if (!_scopes.published(cid))
					{
						_scopes.add(cid);
						LOG4CXX_TRACE(_logger, "Publish scope path "
								<< cid.print());
						_icnCore->publish_scope(cid.binId(),
								cid.binPrefixId(), DOMAIN_LOCAL, NULL, 0);
						_scopes.setPublicationState(cid, true);
					}

					// data point
					cid.append((uint32_t)_attribute(dataPoint));

					// check first that CID is known to MOOSE
					if (!_scopes.published(cid))
					{
						_scopes.add(cid);
						_scopes.setPublicationState(cid, true);
					}

					//subscribe
					if (!_scopes.subscribed(cid))
					{
						LOG4CXX_TRACE(_logger, "Enable data point " << dataPoint
								<< " on NID " << nodeId);
						_icnCore->subscribe_info(cid.binId(), cid.binPrefixId(),
								DOMAIN_LOCAL, NULL, 0);
						_scopes.setSubscriptionState(cid, true);
					}
					//unsubscribe
					else if (_scopes.subscribed(cid))
					{
						LOG4CXX_TRACE(_logger, "Disable data point "
								<< dataPoint << " on NID " << nodeId);
						_icnCore->unsubscribe_info(cid.binId(),
								cid.binPrefixId(), DOMAIN_LOCAL, NULL, 0);
						_scopes.setSubscriptionState(cid, false);
					}
					else
					{
						LOG4CXX_ERROR(_logger, "Action could not be read "
								"(neither enable nor disable)");
					}

					//delete file
					if (!boost::filesystem::remove(toBeDeleted))
					{
						LOG4CXX_ERROR(_logger, oss.str() << " could not be "
								"removed ");
					}
					else
					{
						LOG4CXX_TRACE(_logger, oss.str() << " removed");
					}
				}
			}
			else
			{
				LOG4CXX_FATAL(_logger, _directory << " does not exist or has"
						"the wrong attr");
				return;
			}
		}
		catch (const filesystem_error& ex)
		{
			LOG4CXX_ERROR(_logger, ex.what());
		}
		// sleep for a second and check again
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}
}

bampers_info_item_t FilesystemChecker::_attribute(uint16_t dataPoint)
{
	bampers_info_item_t infoItem;

	switch (dataPoint)
	{
	case ATTRIBUTE_GW_LIST_OF_FQDNS:
	case ATTRIBUTE_NAP_LIST_OF_FQDNS:
		break;
	case ATTRIBUTE_GW_PREFIXES:
	case ATTRIBUTE_NAP_PREFIXES:
		break;
	case ATTRIBUTE_GW_HTTP_REQUESTS:
	case ATTRIBUTE_NAP_HTTP_REQUESTS:
		infoItem = II_HTTP_REQUESTS_FQDN;
		break;
	case ATTRIBUTE_GW_HTTP_REQ_RES_RATIO:
	case ATTRIBUTE_NAP_HTTP_REQ_RES_RATIO:
		infoItem = II_CMC_GROUP_SIZE;
		break;
	case ATTRIBUTE_GW_INCOMING_BYTES:
	case ATTRIBUTE_NAP_INCOMING_BYTES:
		break;
	case ATTRIBUTE_GW_OUTGOING_BYTES:
	case ATTRIBUTE_NAP_OUTGOING_BYTES:
		break;
	case ATTRIBUTE_GW_STATE:
	case ATTRIBUTE_NAP_STATE:
	case ATTRIBUTE_FN_STATE:
	case ATTRIBUTE_RV_STATE:
	case ATTRIBUTE_TM_STATE:
	case ATTRIBUTE_LNK_STATE:
	case ATTRIBUTE_SV_STATE:
	case ATTRIBUTE_UE_STATE:
		infoItem = II_STATE;
		break;
	case ATTRIBUTE_FN_NAME:
	case ATTRIBUTE_UE_NAME:
		infoItem = II_NAME;
		break;
	case ATTRIBUTE_NAP_NETWORK_LATENCY:
		infoItem = II_NETWORK_LATENCY_FQDN;
		break;
	case ATTRIBUTE_FN_LINK_ID:
	case ATTRIBUTE_RV_NUMBER_OF_STATES:
	case ATTRIBUTE_RV_NUMBER_OF_MATCH_REQUESTS:
	case ATTRIBUTE_RV_CPU_LOAD:
	case ATTRIBUTE_RV_CPU_MEMORY_USAGE:
	case ATTRIBUTE_RV_CPU_THREADS:
	case ATTRIBUTE_SV_FQDN:
	case ATTRIBUTE_TM_NUMBER_OF_UNICAST_FIDS:
	case ATTRIBUTE_TM_NUMBER_OF_PATH_REQUESTS:
	case ATTRIBUTE_TM_AVERAGE_PATH_LENGTH:
	case ATTRIBUTE_TM_MIN_PATH_LENGTH:
	case ATTRIBUTE_TM_MAX_PATH_LENGTH:
	case ATTRIBUTE_TM_CPU_LOAD:
	case ATTRIBUTE_TM_CPU_MEMORY_USAGE:
	case ATTRIBUTE_TM_CPU_THREADS:
	case ATTRIBUTE_UE_DASH_JITTER:
	case ATTRIBUTE_UE_DASH_DROPPED_FRAMES:
	case ATTRIBUTE_LNK_DESTINATIONS:
	case ATTRIBUTE_LNK_BYTES_TIME_INTERVAL:
	case ATTRIBUTE_LNK_BUFFER_OCCUPANCY:
	case ATTRIBUTE_LNK_BUFFER_FULL_EVENT:
		LOG4CXX_WARN(_logger, "Data point " << dataPoint << " not implemented");
		infoItem = II_UNKNOWN;
		break;
	default:
		LOG4CXX_ERROR(_logger, "Data point " << dataPoint
				<< " unknown. Nothing to be enabled or "
						"disabled");
		infoItem = II_UNKNOWN;
	}

	return infoItem;
}
node_role_t FilesystemChecker::_nodeRole(uint16_t dataPoint)
{
	node_role_t nodeType;

	switch (dataPoint)
	{
	case ATTRIBUTE_GW_LIST_OF_FQDNS:
	case ATTRIBUTE_GW_PREFIXES:
	case ATTRIBUTE_GW_HTTP_REQUESTS:
	case ATTRIBUTE_GW_HTTP_REQ_RES_RATIO:
	case ATTRIBUTE_GW_INCOMING_BYTES:
	case ATTRIBUTE_GW_OUTGOING_BYTES:
	case ATTRIBUTE_GW_STATE:
		nodeType = NODE_ROLE_GW;
		break;
	case ATTRIBUTE_FN_NAME:
	case ATTRIBUTE_FN_LINK_ID:
	case ATTRIBUTE_FN_STATE:
		nodeType = NODE_ROLE_FN;
		break;
	case ATTRIBUTE_NAP_LIST_OF_FQDNS:
	case ATTRIBUTE_NAP_PREFIXES:
	case ATTRIBUTE_NAP_HTTP_REQUESTS:
	case ATTRIBUTE_NAP_HTTP_REQ_RES_RATIO:
	case ATTRIBUTE_NAP_INCOMING_BYTES:
	case ATTRIBUTE_NAP_OUTGOING_BYTES:
	case ATTRIBUTE_NAP_STATE:
	case ATTRIBUTE_NAP_NETWORK_LATENCY:
		nodeType = NODE_ROLE_NAP;
		break;
	case ATTRIBUTE_RV_NUMBER_OF_STATES:
	case ATTRIBUTE_RV_NUMBER_OF_MATCH_REQUESTS:
	case ATTRIBUTE_RV_CPU_LOAD:
	case ATTRIBUTE_RV_CPU_MEMORY_USAGE:
	case ATTRIBUTE_RV_CPU_THREADS:
	case ATTRIBUTE_RV_STATE:
		nodeType = NODE_ROLE_RV;
		break;
	case ATTRIBUTE_SV_FQDN:
	case ATTRIBUTE_SV_STATE:
		nodeType = NODE_ROLE_SERVER;
		break;
	case ATTRIBUTE_TM_NUMBER_OF_UNICAST_FIDS:
	case ATTRIBUTE_TM_NUMBER_OF_PATH_REQUESTS:
	case ATTRIBUTE_TM_AVERAGE_PATH_LENGTH:
	case ATTRIBUTE_TM_MIN_PATH_LENGTH:
	case ATTRIBUTE_TM_MAX_PATH_LENGTH:
	case ATTRIBUTE_TM_CPU_LOAD:
	case ATTRIBUTE_TM_CPU_MEMORY_USAGE:
	case ATTRIBUTE_TM_CPU_THREADS:
	case ATTRIBUTE_TM_STATE:
		nodeType = NODE_ROLE_TM;
		break;
	case ATTRIBUTE_UE_NAME:
	case ATTRIBUTE_UE_DASH_JITTER:
	case ATTRIBUTE_UE_DASH_DROPPED_FRAMES:
	case ATTRIBUTE_UE_STATE:
		nodeType = NODE_ROLE_UE;
		break;
	case ATTRIBUTE_LNK_DESTINATIONS:
	case ATTRIBUTE_LNK_BYTES_TIME_INTERVAL:
	case ATTRIBUTE_LNK_STATE:
	case ATTRIBUTE_LNK_BUFFER_OCCUPANCY:
	case ATTRIBUTE_LNK_BUFFER_FULL_EVENT:
		cout << "Node role LNK has been removed... setting nodeType to UNKNWON"
				<< endl;
		nodeType = NODE_ROLE_UNKNOWN;
		break;
	default:
		LOG4CXX_ERROR(_logger, "Unknown data point/attribute " << dataPoint
				<< ". Node type will be 0");
		nodeType = NODE_ROLE_UNKNOWN;
	}

	return nodeType;
}
