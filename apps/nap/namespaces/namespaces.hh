/*
 * namespaces.hh
 *
 *  Created on: 16 Apr 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 *  IGMP Handler extensions added on: 12 Jul 2017
 *      Author: Xenofon Vasilakos <xvas@aueb.gr>
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

#ifndef NAP_NAMESPACES_HH_
#define NAP_NAMESPACES_HH_

#include <blackadder.hpp>
#include <mutex>

#include <configuration.hh>
#include <monitoring/statistics.hh>
#include <namespaces/ip.hh>
#include <namespaces/mcast.hh>
#include <namespaces/http.hh>
#include <namespaces/management.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace monitoring::statistics;
using namespace namespaces::ip;
using namespace namespaces::mcast;
using namespace namespaces::http;
using namespace namespaces::management;
using namespace transport;

/*!
 * \brief Parent namespace class for all root scopes
 */
class Namespaces: public Ip, public MCast,  public Http, public Management
{
public:
	/*!
	 * \brief Constructor
	 */
	Namespaces(Blackadder *icnCore, std::mutex &icnCoreMutex,
			Configuration &configuration, Transport &transport,
			Statistics &statistics, bool *run);
	/*!
	 * \brief Notify the end of an application session
	 *
	 * In case of LTP this method allows to switch based on the namespace into
	 * the appropriate proxy to close the socket used to communicate to IP
	 * endpoints
	 *
	 * \param cid The CID under which the notification has been received. For
	 * LTP this is an rCID
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The session key for which the session shall be closed
	 */
	void endOfSession(IcnId &cid, enigma_t &enigma, uint16_t &sessionKey);
	/*!
	 * \brief Set the forwarding state for a particular CID
	 *
	 * Note, ICN CTRL messages always go through namespaces first, before
	 * calling methods from a transport protocol to pass on certain information.
	 *
	 * \param cId Reference to the CID for which the forwarding state
	 * should be set
	 * \param state The forwarding state
	 */
	void forwarding(IcnId &cId, bool state);
	/*!
	 * \brief Set the forwarding state for a particular NID (iSub)
	 *
	 * Note, ICN CTRL messages always go through namespaces first, before
	 * calling methods from a transport protocol to pass on certain information.
	 *
	 * \param nodeId Reference to the NID for which the START_PUBLISH_iSUB has
	 * been received
	 * \param state The forwarding state
	 */
	void forwarding(NodeId &nodeId, bool state);
	/*!
	 * \brief Handle published data received from the local ICN core
	 *
	 * It is important to understand that the actions implemented in this method
	 * do not necessarily relate to the namespace under which data has been
	 * received.
	 *
	 * \param cid The CID under which data has been received
	 * \param data Pointer to the data received
	 * \param dataSize The size of the received data pointer
	 */
	void handlePublishedData(IcnId &cid, void *data, uint32_t &dataSize);
	/*!
	 * \brief
	 */
	void initialise();
	/*!
	 * \brief Publish a buffered packet
	 *
	 * If the NAP received a START_PUBLISH event from the ICN core this
	 * method checks if any packet has been buffered for the CID.
	 *
	 * \param cid Reference to the CID for which the buffer should be checked
	 */
	void publishFromBuffer(IcnId &cid);
	/*!
	 * \brief Publish a buffered packet
	 *
	 * If the NAP received a START_PUBLISH_iSUB event from the ICN core this
	 * method checks if any packet has been buffered for the CID.
	 *
	 * \param nodeId Reference to the NID for which the buffer should be checked
	 */
	//void publishFromBuffer(NodeId &nodeId);
	/*!
	 * \brief Send packet to IP endpoint using the appropriate handler
	 *
	 * \param rCid The rCID for which the correct IP endpoint should be
	 * identified
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param packet Pointer to the packet
	 * \param packetSize The size of the packet pointer
	 */
	void sendToEndpoint(IcnId &rCid, enigma_t &enigma, uint8_t *packet,
			uint16_t &packetSize);
	/*!
	 * \brief Subscribe to a scope
	 *
	 * This method implements the subscription to an ICN ID by using the
	 * subscribe_scope() primitive of the Blackaddder API. Furthermore, this
	 * method also calls _icnId() which adds the ICN ID to the list of known ICN
	 * IDs in case it has not been added beforehand.
	 *
	 * \param icnId The content identifier to which the subscription should be
	 * issued to
	 */
	void subscribeScope(IcnId &icnId);
	/*!
	 * \brief Activate/Deactivate a surrogate server
	 *
	 * \param cid The CID (FQDN) which should be subscribed to/unsubscribed from
	 * \param command Activate/deactivation command
	 */
	void surrogacy(root_namespaces_t rootNamespace, uint32_t hashedFqdn,
			uint32_t ipAddress, uint16_t port, nap_sa_commands_t command);
	/*!
	 * \brief Shutdown the NAP and unsubscribe from all scope paths
	 */
	void uninitialise();
private:
	Configuration &_configuration;/*!< Reference to Configuration class */
};

#endif /* NAP_NAMESPACES_HH_ */
