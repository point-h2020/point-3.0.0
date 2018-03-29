/*
 * namespace.hh
 *
 *  Created on: Dec 4, 2015
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

#ifndef LIB_BAMPERS_NAMESPACE_HH_
#define LIB_BAMPERS_NAMESPACE_HH_

#include <blackadder.hpp>
#include <boost/thread/mutex.hpp>
#include <cstring>
#include <iomanip>
#include <functional>
#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>

#include "enum.hh"
#include "types/icnid.hh"

using namespace std;

/*!
 * \brief Database to store monitoring data
 *
 */
class Namespace {
public:
	/*!
	 * \brief Constructor
	 *
	 * \param blackadder Pointer to BA instance
	 * \param nodeId The NID on which this BAMPERS instance is running
	 */
	Namespace(Blackadder *blackadder, uint32_t nodeId);
	/*!
	 * \brief Destructor
	 */
	~Namespace();
	/*!
	 * \brief
	 */
	void addPublishedScopePath(IcnId *icnId);
	/*!
	 * \brief publish_info() method
	 */
	void advertiseInformationItem(IcnId *icnId);
	/*!
	 * \brief
	 */
	Blackadder * getBlackadderInstance();
	/*!
	 * \brief
	 *
	 * The memory of the void data pointer must be allocated before calling this
	 * method. 1500 bytes is recommended at this pointer in time.
	 */
	bool getDataFromBuffer(IcnId *icnId, uint8_t *data, uint32_t &dataSize);
	/*!
	 * \brief
	 *
	 * Scope path: /Monitoring/Nodes/
	 *
	 * \param nodeRole
	 * \param infoItem
	 */
	string getScopePath(node_role_t nodeRole, bampers_info_item_t infoItem);
	/*!
	 * \brief Port-related info items
	 *
	 * Scope path: /monitoring/ports/nodeId/portId/infoItem
	 */
	string getScopePath(uint32_t nodeId, uint16_t portId,
			bampers_info_item_t infoItem);
	/*!
	 * \brief
	 *
	 * Scope path: /monitoring/links/linkId/destinationNodeId/linkType/
	 *
	 * \param linkId The unqiue link identifier
	 * \param destinationNodeId The destination node ID of the link
	 * \param sourceNodeId The source node ID of the link
	 * \param sourcePort The source port of the link
	 * \param linkType The technology used for the link
	 * \param infoItem The information item identifier
	 *
	 * \return The scope path to be used to interact with BA API
	 */
	string getScopePath(uint16_t linkId, uint32_t destinationNodeId,
			uint32_t sourceNodeId, uint16_t sourcePort, link_type_t linkType,
			bampers_info_item_t infoItem);
	/*!
	 * \brief For /monitoring/nodes/nodeRole/nodeId/informationItem
	 *
	 * \param nodeRole The node type
	 * \param nodeId The node id
	 * \param infoItem The information item (data point)
	 */
	string getScopePath(node_role_t nodeRole, uint32_t nodeId,
			bampers_info_item_t infoItem);
	/*!
	 * \brief Required steps before Blackadder::publish_data() can be called
	 *
	 * This method checks its internal database if the scope path has been
	 * published to the RV, if a subscriber is available and if all the
	 * preconditions have been met it calls another method to finally publish
	 * the data to Blackadder using Blackadder::publish_date().
	 *
	 * \param scopePath The scope path under which data should be published
	 * \param data Pointer to the date to be published
	 * \param dataLength Length of the data pointer
	 */
	void prepareDataToBePublished(string scopePath, uint8_t *data,
			uint32_t dataLength);
	/*!
	 * \brief Overwrites the previous one
	 */
	void prepareDataToBePublished(IcnId *icnId, uint8_t *data,
				uint32_t dataLength);
	/*!
	 * \brief
	 */
	bool publishedScopePath(IcnId *icnId);
	/*!
	 * \brief Remove a scope path from the list of published scope paths
	 *
	 * If the monitoring server unsubscribes from a scope path it will not only
	 * be marked as "no subscribers" available, it will be also completely
	 * removed from the list of published scope paths.
	 *
	 * This method unpublishes the the scope path in the domain local RV and
	 * deletes any state hold in BAMPERS related to this scope path.
	 *
	 * \param scopePath The scope path which has to be removed
	 */
	void removePublishedScopePath(IcnId *icnId);
	/*!
	 * \brief
	 */
	void setSubscriberAvailability(IcnId *icnId, bool state);
	/*!
	 * \brief
	 */
	bool subscriberAvailable(IcnId *icnId);
private:
	Blackadder *_blackadder;
	uint32_t _localNodeId; /*!< The node ID of the ICN node where BAMPERS is
	initialised on */
	struct data_t
	{
		uint8_t *data;
		uint32_t dataSize;
	};
	unordered_map<string, bool> _publishedScopePaths; /*!< */
	unordered_map<string, bool>::iterator _itPublishedScopePaths;
	unordered_map<string, stack<data_t>> _buffer;
	unordered_map<string, stack<data_t>>::iterator _itBuffer;
	boost::mutex _mutexPublishedScopePaths; /*!< */
	boost::mutex _mutexBuffer; /*!< */
	/*!
	 * \brief
	 */
	void _addDataToBuffer(IcnId *icnId, uint8_t *data, uint32_t dataSize);
	/*!
	 * \brief
	 */
	void _addPublishedScopePath(IcnId *icnId);
	/*!
	 *
	 */
	//uint32_t _hashScopePath(string scopePath);
	/*!
	 * \brief
	 */
	void _publishScopePath(IcnId *icnId);
};

#endif /* LIB_BAMPERS_NAMESPACE_HH_ */
