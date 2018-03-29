/*
 * http.hh
 *
 *  Created on: 16 Apr 2016
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

#ifndef NAP_NAMESPACES_HTTP_HH_
#define NAP_NAMESPACES_HTTP_HH_

#include <blackadder.hpp>
#include <boost/date_time.hpp>
#include <log4cxx/logger.h>
#include <list>
#include <map>
#include <mutex>
#include <thread>

#include <configuration.hh>
#include <monitoring/statistics.hh>
#include <namespaces/httptypedef.hh>
#include <namespaces/buffercleaners/httpbuffercleaner.hh>
#include <transport/transport.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace log4cxx;
using namespace monitoring::statistics;
using namespace std;

namespace namespaces
{

namespace http
{
/*!
 * \brief Implementation of the HTTP handler
 */
class Http
{
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 *
	 * \param icnCore Pointer to Blackadder API
	 * \param configuration Reference to configuration class
	 * \param transport Reference to transport class
	 * \param statistics Reference to statistics class
	 * \param run Pointer to run boolean from main()
	 */
	Http(Blackadder *icnCore, Configuration &configuration,
			Transport &transport, Statistics &statistics, bool *run);
	/*!
	 * \brief Destructor
	 */
	~Http();
	/*!
	 * \brief Add NID and rCID to reverse look up map _reversNidTorCIdLookUp
	 *
	 * \param nodeId The NID which should be added
	 * \param rCId The rCID which should be added to the list of known rCIDs for
	 * this NID
	 */
	void addReversNidTorCIdLookUp(string &nodeId, IcnId &rCId);
	/*!
	 * \brief Closing a CMC group using rCID and Enigma information
	 *
	 * Not only does this method close a CMC group properly it also signals this
	 * action to all cNAPs which allows them to inform their HTTP proxy to
	 * shutdown the TCP socket.
	 *
	 * If a TCP client socket has been shutdown this method must be called in
	 * order to remove this session key (socket FD) from the list of active
	 * IP endpoint sessions.
	 *
	 * \param rCid The rCID which identifies a particular CMC group
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The session key which identified a particular CMC group
	 */
	void closeCmcGroup(IcnId rCid, enigma_t enigma, sk_t sessionKey);
	/*!
	 * \brief Delete the session key entry for possible future communications
	 * with TCP server instances
	 *
	 * If a TCP server socket has been shutdown this method must be called in
	 * order to remove this session key (socket FD) from the list of active
	 * IP endpoint sessions.
	 *
	 * \param sessionKey The session key which should be deleted
	 */
	void deleteSessionKey(sk_t &sessionKey);
	/*!
	 * \brief End an LTP session towards an sNAP
	 *
	 * This method cleans up the list of sockets in the HTTP handlers known for
	 * rCID > enigma > socket FD mapping
	 *
	 * \param cid The CID (/http/fqdn)
	 * \param rCid The rCID (/http/url)
	 * \param sessionKey The session key (socket FD)
	 */
	void deleteSession(string fqdn, string resource, sk_t sessionKey);
	/*!
	 * \brief Handle HTTP requests
	 *
	 * \param fqdn The FQDN of the request
	 * \param resource The web resource of this request
	 * \param httpMethod HTTP method
	 * \param packet Pointer to the packet
	 * \param packetSize Length of the packet
	 * \param socketFd Socket file descriptor which will be converted to a
	 * session key in LTP
	 */
	void handleRequest(string &fqdn, string &resource,
			http_methods_t httpMethod, uint8_t *packet, uint16_t &packetSize,
			socket_fd_t socketFd);
	/*!
	 * \brief Handle HTTP responses
	 *
	 * This method blocks if CMC group can not be formed due to whatever reason.
	 * If the CMC group formation fails after 7 attempts (7 * 200ms) this method
	 * returns false;
	 *
	 * \param rCId The rCID to which the HTTP response belongs to
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The session key which identifies this particular
	 * session among multiple rCID > Enigma tuples
	 * \param firstPacket Boolean indicating if this was the first packet
	 * received from the server
	 * \param packet Pointer to the packet
	 * \param packetSize Length of the packet
	 */
	bool handleResponse(IcnId rCId, enigma_t enigma, sk_t &sessionKey,
			bool &firstPacket, uint8_t *packet, uint16_t &packetSize);
protected:
	/*!
	 * \brief Obtain the number of active CMC groups
	 *
	 * \param cid The CID for which the number of CMC groups should be retrieved
	 *
	 * \return Return the number of active CMC groups
	 */
	uint16_t activeCmcGroups(IcnId &cid);
	/*!
	 * \brief Close all potential CMC groups for a particular CID
	 *
	 * Supporting eNAP for surrogacy switching
	 *
	 * \param cid The CID for which the DNS local announcement has arrived
	 */
	void closePotentialCmcGroups(IcnId &cid);
	/*!
	 * \brief End a session by shutting down all sockets towards IP endpoints
	 *
	 * \param rCid The rCID used to look up the CID for which all sockets shall
	 * be closed
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The SK indentifying the LTP session
	 */
	void endSession(IcnId &rCid, enigma_t &enigma, sk_t &sessionKey);
	/*!
	 * \brief Set the forwarding state for a particular CID
	 *
	 * \param cId Reference to the CID for which the forwarding state
	 * should be set
	 * \param state The forwarding state
	 */
	void forwarding(IcnId &cId, bool state);
	/*!
	 * \brief Set the forwarding state for a particular NID
	 *
	 * \param nodeId Reference to the NID for which the forwarding state should
	 * be set (state kept in LTP, not in HTTP handler)
	 * \param state The forwarding state
	 */
	void forwarding(NodeId &nodeId, bool state);
	/*!
	 * \brief Act upon a received DNSlocal message
	 *
	 * \param hashedFqdn The hashed FQDN received in the DNSlocal message
	 */
	void handleDnsLocal(uint32_t &hashedFqdn);
	/*!
	 * \brief Initialise HTTP handler
	 */
	void initialise();
	/*!
	 * \brief Lock working with the potential CMC group map (surrogacy)
	 */
	void lockPotentialCmcGroupMembership();
	/*!
	 * \brief Obtain if any ICN-related communication should be paused for a
	 * particular CID (DNSlocal and resilience use cases)
	 *
	 * \param cid The CID for which the pausing state should be retrieved
	 *
	 * \return The pausing state
	 */
	bool pausing(IcnId &cid);
	/*!
	 * \brief Set the pausing state of an (r)CID
	 *
	 * \param icnId The CID/rCID for which a pausing state should be set
	 * \param state The pausing state itself
	 */
	void pausing(IcnId &cid, bool state);
	/*!
	 * \brief Publish a buffered HTTP request packet
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
	 * \brief Send HTTP response to IP endpoint
	 *
	 * When receiving the HTTP request the HTTP handler kept a mapping of the
	 * session key (socket file descriptor) to rCid > Enigma in order to be able
	 * to look up the TCP endpoint which is awaiting this HTTP response
	 *
	 * \param rCid The rCid which is used to retrieve the correct socket FD from
	 * _ipEndpointSessions
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param packet Pointer to the HTTP response packet
	 * \param packetSize Size of the HTTP response packet
	 */
	void sendToEndpoint(IcnId &rCid, enigma_t &enigma, uint8_t *packet,
			uint16_t packetSize);
	/*!
	 * \brief Subscribe to a particular FQDN
	 *
	 * \param cid The CID for which the HTTP handler shall subscribe to
	 */
	void subscribeToFqdn(IcnId &cid);
	/*!
	 * \brief Uninitialise HTTP handler
	 */
	void uninitialise();
	/*!
	 * \brief Lock working with the potential CMC group map (surrogacy)
	 */
	void unlockPotentialCmcGroupMembership();
	/*!
	 * \brief Unsubscribe from a particular FQDN
	 *
	 * \param cid The CID supposed to be used to subscribe from
	 */
	void unsubscribeFromFqdn(IcnId &cid);
private:
	Blackadder *_icnCore; /*!< Pointer to Blackadder instance */
	Configuration &_configuration; /*!< Reference to configuration */
	Transport &_transport;/*!< Reference to transport class */
	Statistics &_statistics; /*<! Reference to statistics class*/
	bool *_run;/*!< from main thread is any SIG* had been caught */
	map<cid_t, IcnId> _cIds;
	map<cid_t, IcnId>::iterator _cIdsIt;
	map<cid_t, IcnId> _rCIds;
	map<cid_t, IcnId>::iterator _rCIdsIt;
	//unordered_map<cid_t, unordered_map<cid_t, IcnId>> _cid2rCidLookup;/*!< Store
	//the list of rCIDs currently active for CID (DNSlocal purposes) */
	//unordered_map<cid_t, unordered_map<cid_t, IcnId>>::iterator
	//_cid2rCidLookupIt;/*!< Iterator for _cid2rCidLookup map */
	//boost::mutex _cid2rCidLookupMutex;/*!< mutex for _cid2rCidLookup map */
	map<nid_t, bool> _nIds; /*!< map<NID, startPublishReceived> This
	boolean is used to indicate that the FID for this particular NID has been
	received.*/
	map<nid_t, bool>::iterator _nIdsIt; /*!< Iterator for _nIds map*/
	std::mutex _nIdsMutex; /*!< Mutex for _nIds */
	std::mutex _mutexIcnIds;
	map<cid_t, map<enigma_t, list<socket_fd_t>>> _ipEndpointSessions;/*!<
	map<rCID, map<Enigma, list<sessionKey>>> where the session key is the socket
	FD of TCP server instances*/
	map<cid_t, map<enigma_t, list<socket_fd_t>>>::iterator _ipEndpointSessionsIt;
	/*!< Iterator for _ipEndpointSessions map*/
	std::mutex _ipEndpointSessionsMutex;/*!< mutex for _ipEndpointSessions map
	*/
	packet_buffer_requests_t _packetBufferRequests; /*! <Buffer for HTTP
	messages to be published  */
	packet_buffer_requests_t::iterator _packetBufferRequestsIt;/*!< Iterator for
	_packetBufferRequests map */
	std::mutex _packetBufferRequestsMutex;/*!< mutex for _packetBufferRequests
	map */
	potential_cmc_groups_t _potentialCmcGroups;/*!< List of NIDs for which an
	HTTP request has been received but not necessarily the entire message. The
	boolean indicates whether or not the entire HTTP request has been received
	so that the HTTP response can be sent */
	potential_cmc_groups_t::iterator _potentialCmcGroupsIt; /*!< Iterator for
	_potentialCmcGroups map*/
	std::mutex _potentialCmcGroupsMutex; /*!< Mutex for
	_potentialMulticastGroup map */
	bool _potentialCmcGroupsOpen;
	cmc_groups_t _cmcGroups;/*!< map to hold all locked CMC groups */
	cmc_groups_t::iterator _cmcGroupsIt; /*!< Iterator for _cmcGroups */
	std::mutex _cmcGroupsMutex; /*!< Mutex for _cmcGroups map operation */
	map<nid_t, list<cid_t>> _reverseNidTorCIdLookUp;/*!< map<NID,
	list<rCIDs> Used to store the list of rCIDs a particular NID is awaiting a
	response for. When START_PUBLISH_iSUB arrives it only has NID */
	map<nid_t, list<cid_t>>::iterator _reverseNidTorCIdLookUpIt;
	std::mutex _reverseNidTorCIdLookUpMutex;/*!< mutex for
	_reversNidTorCIdLookUp map access */
	/*!
	 * \brief Add an IP endpoint session to the list of known sessions
	 *
	 * This method adds the session key to the _ipEndpointSessions map (if it
	 * does not exist yet) using rCID > Enigma > list<session keys> relation.
	 *
	 * \param rCId The rCID to which this session key belongs
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param socketFd The session key which should be added (socket FD)
	 */
	void _addIpEndpointSession(IcnId &rCId, enigma_t &enigma,
			socket_fd_t socketFd);
	/*!
	 * \brief Buffer an HTTP request packet
	 *
	 * \param cId The CID
	 * \param rCId The rCID
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The SK
	 * \param httpMethod The HTTP method of the request
	 * \param packet Pointer to the HTTP request packet
	 * \param packetSize The size of packet
	 */
	void _bufferRequest(IcnId &cId, IcnId &rCId, enigma_t &enigma,
			sk_t &sessionKey, http_methods_t httpMethod, uint8_t *packet,
			uint16_t &packetSize);
	/*!
	 * \brief Create a new CMC group
	 *
	 * Gets only called when a new CMC group is created after discovering NIDs
	 * from the potential CMC group map. This method works on the _cmcGroups map
	 *
	 * \param rCid rCID for which the CMC group should be created
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The session key for which the CMC should be created
	 * \param nodeIds List of NIDs which should be added to this group
	 */
	void _createCmcGroup(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey, list<NodeId> &nodeIds);
	/*!
	 * \brief Delete packet from _packetBufferRequests
	 *
	 * \param cid The CID for which packets should be looked up
	 * \param rCid The rCID for which packets should be looked up
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The SK for which the packet should be deleted
	 */
	void _deleteBufferedRequest(IcnId &cid, IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey);
	/*!
	 * \brief Delete packet from _packetBufferResponses
	 *
	 * This method uses the _packetBufferResponses STL map which has an STL
	 * stack to handle HTTP responses larger than the TCP client buffer size
	 *
	 * \param rCid The rCID for which packets should be looked up
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 */
	void _deleteBufferedResponses(IcnId &rCid, enigma_t &enigma);
	/*!
	 * \brief Delete CID > rCID mapping
	 *
	 * \param cid The CID which is used as the key in the map
	 * \param rCid The rCID which should be deleted as a value for the CID
	 */
	/// TODO obsolete void _deleteCid2rCidMapping(IcnId &cid, IcnId &rCid);
	/*!
	 * \brief Delete the list of rCIDs for the given NID awaits
	 *
	 * Map _reverseNidTorCIdLookUp is used
	 *
	 * \param nodeId The NID for which the reverse look up map should be deleted
	 */
	void _deleterCids(NodeId &nodeId);
	/*!
	 * \brief Obtain the state whether or not the NAP has received a
	 * START_PUBLISH_iSUB event for a particular NID
	 *
	 * \param nodeId The NID for which the forwarding state should be checked
	 *
	 * \return Boolean indicating whether or not the forwarding state for the
	 * given NID is enabled
	 */
	bool _forwardingEnabled(nid_t nodeId);
	/*!
	 * \brief Obtain the list of rCIDs the given NID awaits
	 *
	 * \param nodeId The NID for which the reverse look up map should be checked
	 *
	 * \return A list of rCIDs
	 */
	list<uint32_t> _getrCids(NodeId &nodeId);
	/*!
	 * \brief Obtain the list of NIDs in a locked CMC group
	 *
	 * This method first checks if a CMC group exists for the given rCID and
	 * Enigma combination. If not, it will walk through the potential list of CMC
	 * group members and adds any NID for which a START_PUBLISH_iSUB has been
	 * received to a new CMC group.
	 *
	 * Note, if the group is created from Http::publishFromBuffer there is no
	 * session key available at the time and the first one found for rCID > Enigma
	 * is being used while the other entries in the ICN buffer are simply
	 * deleted. Also, in order to accommodate for a scenario where the
	 * START_PUBLISH_iSUB event is received while the HTTP response message
	 * fragments are being received from the TCP proxy, the sessionKey variable
	 * is being set to the one found first in the ICN buffer.
	 *
	 * \param rCid The rCID for which the list of NIDs should be obtained
	 * https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The session key for which the list of NIDs should be
	 * obtained
	 * \param firstPacket Boolean indicating whether or not this was the first
	 * packet received from the server
	 * \param nodeIds List of NIDs which are awaiting HTTP response
	 *
	 * \return Boolean indicating if the HTTP response must be buffered
	 */
	bool _getCmcGroup(IcnId &rCid, enigma_t &enigma, sk_t &sessionKey,
			bool firstPacket, list<NodeId> &nodeIds);
	/*!
	 * \brief Obtain the number of session keys stored for a particular rCID >
	 * Enigma mapping
	 *
	 * \param rCID The rCID for which the number of session keys should be
	 * checked
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 *
	 * \return The number of session keys stored
	 */
	uint8_t _numberOfSessionKeys(IcnId &rCId, enigma_t &enigma);
};

} /* namespace http */

} /* namespace namespaces */

#endif /* NAP_NAMESPACES_HTTP_HH_ */
