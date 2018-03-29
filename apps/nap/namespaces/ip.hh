/*
 * ip.hh
 *
 *  Created on: 15 Apr 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
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

#ifndef NAP_NAMESPACES_IP_HH_
#define NAP_NAMESPACES_IP_HH_

#include <blackadder.hpp>
#include <log4cxx/logger.h>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <configuration.hh>
#include <monitoring/statistics.hh>
#include <namespaces/buffercleaners/ipbuffercleaner.hh>
#include <namespaces/iptypedef.hh>
#include <transport/transport.hh>
#include <types/icnid.hh>
#include <types/routingprefix.hh>
#include <types/typedef.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace cleaners::ipbuffer;
using namespace configuration;
using namespace monitoring::statistics;
using namespace std;
using namespace transport;

namespace namespaces {

namespace ip {
/*!
 * \brief Implementation of the IP handler
 */
class Ip
{
	static log4cxx::LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	Ip(Blackadder *icnCore, Configuration &configuration,
			Transport &transport, Statistics &statistics, bool *run);
	/*!
	 * \brief Destructor
	 */
	~Ip();
	/*!
	 * \brief Handle IP packet according to the IP-over-ICN definition
	 *
	 * \param destinationIpAddress Reference to the destination
	 * endpoint's IP address
	 * \param packet Pointer to the IP packet to be published
	 * \param packetSize Reference to the size of the IP packet
	 */
	void handle(IpAddress &destinationIpAddress, uint8_t *packet,
			uint16_t &packetSize);
	/*!
	 * \brief Subscribe to an information item
	 *
	 * This method implements the subscription to a CID by using the
	 * subscribe_info() primitive of the Blackaddder API. Furthermore,
	 * this method also calls Ip::_icnId() which adds the CID to the list
	 * of known CIDs in case it has not been added beforehand.
	 *
	 * \param icnId The content identifier to which the subscription
	 * should be issued to
	 *
	 * \return void
	 */
	void subscribeInfo(IcnId &icnId);
protected:
	/*!
	 * \brief Set the forwarding state for a particular CID
	 *
	 * \param cid Reference to the CID for which the forwarding state
	 * should be set
	 * \param state The forwarding state
	 */
	void forwarding(IcnId &cid, bool state);
	/*!
	 * \brief Publish IP scope path for configured prefix/host and
	 * subscribe to it
	 */
	void initialise();
	/*!
	 * \brief Publish a buffered packet
	 *
	 * If the NAP received a START_PUBLISH event from the ICN core this
	 * method checks if any packet has been buffered for the CID
	 *
	 * \param icnId The content identifier for which the IP buffer should
	 * be checked
	 *
	 * \return void
	 */
	void publishFromBuffer(IcnId &cId);
	/*!
	 * \brief Subscribe to a scope
	 *
	 * This method implements the subscription to a ICN ID by using the
	 * subscribe_scope() primitive of the Blackaddder API. Furthermore,
	 * this method also calls Ip::_icnId() which adds the ICN ID to the
	 * list of known CIDs in case it has not been added beforehand.
	 *
	 * \param icnId The content identifier to which the subscription
	 * should be issued to
	 *
	 * \return void
	 */
	void subscribeScope(IcnId &icnId);
	/*!
	 * \brief Unsubscribe from routing prefix and unpublish the scope
	 * path
	 */
	void uninitialise();
private:
	Blackadder *_icnCore;/*!< Pointer to Blackadder instance */
	Configuration &_configuration;/*!< Reference to Configuration class */
	Transport &_transport;/*!< Reference to Transport class */
	Statistics &_statistics;/*!< Reference to statistics class */
	bool *_run;/*!< from main thread is any SIG* had been caught */
	map<uint32_t, RoutingPrefix> *_routingPrefixes;
	map<uint32_t, RoutingPrefix>::reverse_iterator _routingPrefixesRevIt;
	std::mutex _mutexRoutingPrefixes;
	unordered_map<uint32_t, IcnId> _icnIds;
	unordered_map<uint32_t, IcnId>::iterator _icnIdsIt;
	std::mutex _mutexIcnIds;
	packet_buffer_t _packetBuffer; /*!< Buffer for IP packets */
	packet_buffer_t::iterator _bufferIt; /*!< Iterator for _buffer map */
	std::mutex _mutexBuffer;/*!< Mutex for transaction safe operations on
	_buffer */
	/*!
	 * \brief Add IP packet to buffer
	 *
	 * In case the START_PUBLISH notification for a particular CID has
	 * not been received the IP packet is added to a buffer. This packet
	 * buffer allows to store a single packet per CID only. If an
	 * application issues multiple IP packets to the same destination IP
	 * address and destination port, the previous buffered packet is
	 * deleted.
	 *
	 * \param icnId Reference to the CID for this particular packet
	 * \param packet Pointer to the actual packet
	 * \param packetSize Pointer to the length of the packet in bytes
	 *
	 * \return void
	 */
	void _bufferPacket(IcnId &icnId, uint8_t *packet, uint16_t &packetSize);
	/*!
	 * \brief Delete the packet buffered for a particular CID
	 *
	 * Note, this method does NOT lock the buffer mutex, as it is supposed to
	 * be called right after the packet has been published and the buffer mutex
	 * is going to be unlocked again.
	 *
	 * \param cid The CID for which the buffered packet should be deleted
	 */
	void _deleteBufferedPacket(IcnId &cid);
	/*!
	 * \brief Add new CID to IP namespace
	 *
	 * Note, this method does not lock the unordered map _icnIds which
	 * holds all known CIDs
	 *
	 * \param icnId The CID which shall be added
	 *
	 * \return void
	 */
	void _icnId(IcnId &icnId);
	/*!
	 * \brief Obtain known routing prefix for given IP address
	 *
	 * All routing prefixes have been ordered using their uint representation
	 * (networkAddress & netmask) which places the largest routing prefix (in
	 * terms of hwo many hosts the prefix comprises) before a smaller prefix.
	 * When obtaining a routing prefix using the given IP address an inverse
	 * iterator is used which starts checking for possible prefixes starting
	 * with the smallest stored routing prefix.
	 *
	 * \param ipAddress The IP address for which the routing prefix is
	 * required
	 * \param routingPrefix Reference to RoutingPrefix class which shall
	 * hold the routing prefix under to which the IP fits
	 *
	 * \return Boolean indicating whether or not a routing prefix could
	 * be obtained
	 */
	bool _routingPrefix(IpAddress &ipAddress,
			RoutingPrefix &routingPrefix);
};

} /* namespace ip */

} /* namespace namespaces */

#endif /* NAP_NAMESPACES_IP_HH_ */
