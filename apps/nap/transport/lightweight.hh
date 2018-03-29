/*
 * lightweight.hh
 *
 *  Created on: 22 Apr 2016
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

#ifndef NAP_TRANSPORT_LIGHTWEIGHT_HH_
#define NAP_TRANSPORT_LIGHTWEIGHT_HH_

#include <blackadder.hpp>
#include <boost/date_time.hpp>
#include <log4cxx/logger.h>
#include <forward_list>
#include <mutex>
#include <time.h>
#include <thread>

#include <configuration.hh>
#include <types/enumerations.hh>
#include <types/icnid.hh>
#include <transport/buffercleaners/ltpbuffercleaner.hh>
#include <transport/buffercleaners/ltpreversecidcleaner.hh>
#include <transport/lightweighttimeout.hh>
#include <transport/lightweighttypedef.hh>
#include <types/nodeid.hh>
#include <monitoring/statistics.hh>
#include <trafficcontrol/trafficcontrol.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define MAX_PAYLOAD_LENGTH 65535

using namespace configuration;
using namespace log4cxx;
using namespace monitoring::statistics;
using namespace trafficcontrol;

namespace transport
{

namespace lightweight
{
/*!
 * \brief Implementation of the lightweight transport protocol for
 * HTTP-over-ICN pub/subs
 */
class Lightweight: public TrafficControl
{
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	Lightweight(Blackadder *icnCore, Configuration &configuration,
			std::mutex &icnCoreMutex, Statistics &statistics, bool *run);
	/*!
	 * \brief Destructor
	 */
	virtual ~Lightweight();
	/*!
	 * \brief Obtain the CID from a given rCID
	 *
	 * \param rCid The rCID for which LTP should check if it is known
	 * \param cid The CID which corresponds to the rCID
	 *
	 * \return Boolean indicating if rCID was found and cid reference holds the
	 * CID
	 */
	bool cidLookup(IcnId &rCid, IcnId &cid);
	/*!
	 * \brief Clean up LTP buffer
	 *
	 * \param cid The CID
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The SK
	 */
	void cleanUpBuffers(IcnId &cid, enigma_t &enigma, sk_t &sessionKey);
	/*!
	 * \brief Initialise LTP with pointers to maps residing in the HTTP
	 * handler (for separation of concern reasons).
	 *
	 * LTP requires knowledge about the forwarding policy of particular
	 * NIDs indicated by the ICN core through START_PUBLISH_iSUB which
	 * is handled by HTTP handler. However, in order to send CTRL data
	 * LTP requires knowledge whether or not the START_PUBLISH_iSUB has
	 * been received for a particular NID.
	 * The pointer to the CMC group map is passed to LTP, as only LTP
	 * understands the structure of its protocol and fills up the map of
	 * potential CMC groups on behalf of the HTTP handler.
	 *
	 * \param potentialCmcGroup Pointer to potential CMC group map
	 * \param potentialCmcGroupMutex Pointer to mutex for transaction-safe
	 * operations on potentialCmcGroup map
	 * \param knownNIds Pointer to knownNIds map
	 * \param knownNIdsMutex Pointer to mutex for transaction-safe operations on
	 * knownNIds map
	 */
	void initialise(void *potentialCmcGroup, void *potentialCmcGroupMutex,
			void *knownNIds, void *knownNIdsMutex, void *cmcGroup,
			void *cmcGroupMutex);
	/*!
	 * \brief Allow HTTP handler to set forwarding state
	 *
	 * In a DNSlocal scenario received LTP RSTED messages from the sNAP at the
	 * cNAP determines whether or not a new FID is requested via
	 * unpublish_info() & publish_info(). However, if DNSlocal has been
	 * completed the HTTP handler must pass this information into the LTP class
	 * so that any future LTP RSTED do not cause a new "FID request". This has
	 * been experienced when multiple LTP RST were issued for the same rCID
	 * (timeout) due to a congested link.
	 *
	 * This method sets the forwarding and fidRequested boolean in the
	 * _cIdReverseLookUp map
	 *
	 * \param cid The CID for which the START_PUBLISH
	 * \param state The forwarding state for the given CID
	 */
	void forwarding(IcnId &cid, bool state);
	/*!
	 * \brief Publish HTTP responses (co-incidental multicast)
	 *
	 * \param cId Reference to the content identifier
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The session key under which this packet should be
	 * published
	 * \param packet Pointer to the data
	 * \param dataSize Pointer to the length of the packet
	 */
	void publish(IcnId &rCid, enigma_t &enigma, sk_t &sessionKey,
			list<NodeId> &nodeIds, uint8_t *data, uint16_t &dataSize);
	/*!
	 * \brief Publish HTTP responses (co-incidental multicast)
	 *
	 * Blocking method. This method is called when packet from TCP server can be
	 * immediately published to sNAP (
	 *
	 * \param cId Reference to the content identifier
	 * \param iSubIcnId Reference to the iSub content identifier
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param packet Pointer to the data
	 * \param dataSize Pointer to the length of the packet
	 *
	 * \return Boolean indicating if packet has been successfully
	 * published
	 */
	void publish(IcnId &cId, IcnId &iSubIcnId, enigma_t &enigma,
			sk_t &sessionKey, uint8_t *data, uint16_t &dataSize);
	/*!
	 * \brief Publish HTTP session ended (co-incidental multicast)
	 *
	 * \param cId Reference to the content identifier
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The session key under which this packet should be
	 * published
	 */
	void publishEndOfSession(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey);
	/*!
	 * \brief Publish HTTP session end (from cNAP to sNAP)
	 *
	 * \param cid The CID
	 * \param rCid The rCID
	 * \param sessionKey The session key (socket FD of the TCP server)
	 */
	void publishEndOfSession(IcnId cid, IcnId rCid, sk_t sessionKey);
	/*!
	 * \brief Publish HTTP requests
	 *
	 * Non-blocking method. It calls a thread to check for successful delivery
	 *
	 * \param cId Reference to the content identifier
	 * \param rCId Reference to the iSub content identifier (rCID)
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param packet Pointer to the data
	 * \param dataSize Pointer to the length of the packet
	 */
	void publishFromBuffer(IcnId &cId, IcnId &rCId, enigma_t &enigma,
			sk_t &sessionKey, uint8_t *data, uint16_t &dataSize);
	/*!
	 * \brief Publish LTP reset
	 *
	 * Upon reception of a DNSlocal announcement for a particular FQDN all
	 * active LTP sessions for the announced FQDN must be reset
	 *
	 * \param cid The CID under whcih the LTP RST should be published
	 * \param rCid The rCID for the PUBLISH_DATA_iSUB
	 */
	void publishReset(IcnId &cid, IcnId &rCid);
	/*!
	 * \brief Cleaning up states from an HTTP session within LTP
	 *
	 * After LTP CTRL-SED has been received by sNAP this method allows the HTTP
	 * handler to clean up states such as the reverse CID to rCID mapping
	 *
	 * \param cid The CID for which states and packet buffers should be cleaned
	 * up
	 * \param rCid The rCID for which states and packet buffers should be
	 * cleaned up
	 */
	//void sessionCleanUp(IcnId &cid, IcnId &rCid);//TODO remove
protected:
	/*!
	 * \brief Handle an incoming ICN message (iSub) which is using LTP
	 *
	 * This implementation of the NAP uses LTP for the HTTP handler
	 * only.
	 *
	 * \param cId The CID under which the packet has been published
	 * \param packet Pointer to the ICN payload
	 */
	tp_states_t handle(IcnId &cId, uint8_t *packet, enigma_t &enigma,
			sk_t &sessionKey);
	/*!
	 * \brief Handle an incoming ICN message (iSub) which is using LTP
	 *
	 * This implementation of the NAP uses LTP for the HTTP handler
	 * only.
	 *
	 * \param cId The CID under which the packet has been published
	 * \param rCId The rCID under which the response must be
	 * published
	 * \param nodeIdStr The NID which must be used when publishing the
	 * response
	 * \param packet Pointer to the ICN payload
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey If WE has been received and all fragments were
	 * received this variable holds the corresponding session key
	 *
	 * \return The TP state indicating if packet can be sent to IP endpoint
	 */
	tp_states_t handle(IcnId &cId, IcnId &rCId, string &nodeIdStr,
			uint8_t *packet, enigma_t &enigma, sk_t &sessionKey);
	/*!
	 * \brief Retrieve a packet from _icnPacketBuffer map
	 *
	 * In order to call the method handle() retrieveIcnPacket must have returned
	 * TP_STATE_ALL_FRAGMENTS_RECEIVED. This method also cleans up the packet
	 * from the packet buffer
	 *
	 * This method uses the _icnPacketBuffer STL map
	 *
	 * \param rCId The rCID for which the packet should be retrieved from the
	 * buffer
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param nodeIdStr The NID for which the packet should be retrieved from
	 * the buffer
	 * \param sessionKey The SK for which the packet should be retrieved from
	 * the buffer
	 * \param packet Pointer to the memory to which the packet will be copied
	 * \param packetSize The number of octets representing the packet size
	 *
	 * \return boolean indicating if the packet has been successfully retrieved
	 * from ICN buffer
	 */
	bool retrieveIcnPacket(IcnId &rCId, enigma_t &enigma, string &nodeIdStr,
			sk_t &sessionKey, uint8_t *packet, uint16_t &packetSize);
private:
	Blackadder *_icnCore;/*!< Pointer to the Blackadder instance */
	Configuration &_configuration;
	std::mutex &_icnCoreMutex;/*!< Reference to ICN core mutex */
	Statistics &_statistics;/*!< Reference to the Statistics class*/
	reverse_cid_lookup_t _cIdReverseLookUp;/*!< map<rCID, cId> When	LTP CTRL
	arrives from sNAP (under rCID) the CID (FQDN) is required for potential
	retransmissions*/
	reverse_cid_lookup_t::iterator _cIdReverseLookUpIt;/*!< Iterator for
	_cIdReverseLookUp*/
	std::mutex _cIdReverseLookUpMutex; /*!< Mutex for _cIdReverseLookUp map
	operations */
	icn_packet_buffer_t _icnPacketBuffer; /*!< Packet buffer for incoming ICN
	packets */
	icn_packet_buffer_t::iterator _icnPacketBufferIt; /*!< Iterator
	for _icnPacketBuffer map*/
	std::mutex _icnPacketBufferMutex; /*!< Mutex for _icnPacketBuffer map*/
	proxy_packet_buffer_t _proxyPacketBuffer; /*!< Packet buffer for incoming
	HTTP packets from the HTTP proxy (both req+resp with rCID as the map key */
	proxy_packet_buffer_t::iterator _proxyPacketBufferIt;/*!< Iterator for
	packetBuffer map */
	std::mutex _proxyPacketBufferMutex;/*!< mutex for _packetBuffer map*/
	ltp_packet_buffer_t _ltpPacketBuffer;/*!< map<rCID, map<enigma, map<SK,
	map<Sequence, Packet>>>> Packet buffer for sent datagrams over LTP (in case
	NACK comes back)*/
	std::mutex _ltpPacketBufferMutex;/*!< mutex for transaction-safe
	operations on _ltpPacketBuffer */
	ltp_session_activity_t _ltpSessionActivity;/*!< Keep knowledge if LTP packet
	buffer is currently used for a particular LTP session*/
	std::mutex _ltpSessionActivityMutex; /*!< Mutex for _ltpSessionActivity*/
	/*
	 * (Potential) CMC Group
	 */
	cmc_groups_t *_cmcGroups;/*!< Pointer to Http::_cmcGroups map */
	cmc_groups_t::iterator _cmcGroupsIt; /*!< Iterator for _cmcGroups */
	std::mutex *_cmcGroupsMutex; /*!< Pointer to Http::_cmcGroupsMutex mutex*/
	potential_cmc_groups_t * _potentialCmcGroups; /*!< Pointer to
	Http::_potentialCmcGroups */
	potential_cmc_groups_t::iterator _potentialCmcGroupsIt; /*!< Iterator for
	_potentialCmcGroups map*/
	std::mutex *_potentialCmcGroupsMutex; /*!< Pointer to
	Http::_potentialCmcGroups mutex*/
	map<nid_t, bool> *_knownNIds; /*!< map<NID, startPublishReceived> This
	boolean is used to indicate that the FID for this particular NID has been
	received.*/
	map<nid_t, bool>::iterator _knownNIdsIt; /*!<Iterator for _fidKnown map*/
	std::mutex *_knownNIdsMutex; /*!< Mutex for _knownNIds */
	/*
	 * LTP Reset
	 */
	resetted_t _resetted;/*!< LTP RST received */
	resetted_t::iterator _resettedIt; /*!< Iterator for _resetted */
	std::mutex _resettedMutex;/*!< Mutex for _resetted map */
	vector<std::thread> _resettedThreads;
	forward_list<uint16_t> _rtts;/*!< Round trip time list*/
	std::mutex _rttsMutex;/*!< Mutex for writing _rtt*/
	uint16_t _rttMultiplier;/*!< Multiplier for LTP-CTRL timeout using RTT */
	/*
	 * LTP Session Ended
	 */
	session_ended_responses_t _sessionEndedResponses;
	/*!< map<rCID, map<enigma, map<NID, map<SK, SED received>>>>*/
	session_ended_responses_t::iterator _sessionEndedResponsesIt;
	/*!< Iterator for _sessionEndedResponses map*/
	std::mutex _sessionEndedResponsesMutex;/*!< Mutex for operations on
	_sessionEndedResponses map*/
	unordered_map<cid_t, unordered_map<sk_t, bool>>
	_sessionEndedResponsesUnicast;/*!< To keep track about sent LTP CTRL SEs */
	std::mutex _sessionEndedResponsesUnicastMutex;/*!< Mutex for
	_sessionEndedResponsesUnicast map*/
	/*
	 * LTP Window Ended
	 */
	window_ended_requests_t _windowEndedRequests;
	/*!<map<rCID, map<enigma, map<Session Key, WED received>>> */
	window_ended_requests_t::iterator
	_windowEndedRequestsIt;/*!<Iterator for _windowEndedRequests map*/
	std::mutex _windowEndedRequestsMutex;/*!< Mutex for _windowEndedRequests
	maps */
	map<cid_t, map<enigma_t, map<nid_t, map<sk_t, bool>>>> _windowEndedResponses;
	/*!<map<rCID, map<enigma, map<NID, map<Session Key, WED received>>> */
	map<cid_t, map<enigma_t, map<nid_t, map<sk_t, bool>>>>::iterator
	_windowEndedResponsesIt;/*!<Iterator for _windowEndedResponses map*/
	std::mutex _windowEndedResponsesMutex;/*!< Mutex for _windowEndedResponses
	maps */
	map<cid_t, map<enigma_t, map<sk_t, map<nid_t, bool>>>> _windowUpdate;
	/*!< map to check if WUD has been received */
	map<cid_t, map<enigma_t, map<sk_t, map<nid_t, bool>>>>::iterator
	_windowUpdateIt; /*!< Iterator for operations on _windowUpdate map*/
	std::mutex _windowUpdateMutex; /*!< Mutex for transaction-safe operations
	on _windowUpdate map */
	map<cid_t, map<enigma_t, map<sk_t, nack_group_t>>> _nackGroups;/*!<
	map<rCID, map<enigma, map<SK, nackGroup>>> Store the NIDs which sent a NACK so
	that if all NIDs have responded to the WE message (either WED or NACK) the
	NAP can eventually re-submit the range of missing segments */
	map<cid_t, map<enigma_t, map<sk_t, nack_group_t>>>::iterator
	_nackGroupsIt; /*!< Iterator for _nackGroups mak */
	std::mutex _nackGroupsMutex;/*!< mutex for _nackGroups map */
	/*!
	 * \brief Add a NACK to _nackGroups
	 *
	 * \param rCid The rCID under which the NACK message has been received
	 * \param ltpHeaderNack The LTP NACK header received
	 * \param nodeId The NID from which the NACK message has been received
	 */
	void _addNackNodeId(IcnId &rCid, ltp_hdr_ctrl_nack_t &ltpHeaderNack,
			NodeId &nodeId);
	/*!
	 * \brief Add NID to list of known NIDs and their forwarding states
	 *
	 * This method checks the _fidKnown map if the given NID exists as a key.
	 * The local ICN core preserves a list of NIDs and their FID to allow the
	 * realisation of implicit subscriptions (iSub).
	 *
	 * \param nodeId The NID to be added
	 */
	void _addNodeId(NodeId &nodeId);
	/*!
	 * \brief Add a NID to the _windowUpdate map
	 *
	 * This methods adds the given NID to the _windowUpdate map and sets its
	 * state to false.
	 *
	 * \param rCid The rCID under which the NID should be added
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The SK under which the NID should be added
	 * \param nodeId The NID to be added
	 */
	void _addNodeIdToWindowUpdate(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey, NodeId &nodeId);
	/*!
	 * \brief Add rCID > CID look-up
	 *
	 * When sNAP replies only the rCID is inluded. Hence, the NAP requires to
	 * keep a reverse look up mapping
	 *
	 * \param cid The CID of the request
	 * \param rCid The rCID of hte request
	 */
	void _addReverseLookUp(IcnId &cid, IcnId &rCid);
	/*!
	 * \brief Add rCID > SK > false to _sessionEndedResponsesUnicast map
	 *
	 * For a cNAP to keep track about a sent SE to an sNAP
	 *
	 * \param rCid The rCID used as the primary key
	 * \param sessionKey The session key
	 */
	void _addSessionEnd(IcnId &rCid, sk_t &sessionKey);
	/*!
	 * \brief Add rCID > enigma > NID > SK > false to _sessionEndedResponses map
	 *
	 * For an sNAP to keep track about sent SEs to cNAPs in a CMC group
	 *
	 * \param rCid The rCID used as the primary key
	 * \param ltpHdrCtrlSe The LTP SE header comprising enigma and SK
	 * \param nodeIds The list of NIDs from which a SED is expected
	 */
	void _addSessionEnd(IcnId &rCid, ltp_hdr_ctrl_se_t &ltpHdrCtrlSe,
			list<NodeId> nodeIds);
	/*!
	 * \brief Add sent LTP CTRL-WE message to _windowEndedRequests map
	 *
	 * \param rCid The rCID which should be added
	 * \param ltpHeaderCtrlWe The LTP CTRL-WE header holding the required enigma
	 * and session key values
	 */
	void _addWindowEnd(IcnId &rCid, ltp_hdr_ctrl_we_t &ltpHeaderCtrlWe);
	/*!
	 * \brief Add sent LTP CTRL-WE message to _windowEndedResponses map
	 *
	 * \param rCid The rCID which should be added
	 * \param nodeIds List of NIDs this WE has been published to
	 * \param ltpHeaderCtrlWe The LTP CTRL-WE header holding the required enigma
	 * and session key values
	 */
	void _addWindowEnd(IcnId &rCid, list<NodeId> &nodeIds,
			ltp_hdr_ctrl_we_t &ltpHeaderCtrlWe);
	/*!
	 * \brief Add packet received from ICN core to LTP buffer
	 *
	 * This method adds the actual packet to the LTP buffer always using rCID as
	 * the map's key for both HTTP requests and responses
	 *
	 * Note (to myself probably): do NOT delete packetDescriptor.first memory
	 * after inserting it into the map. This will delete the packet in the map
	 * too. Funny enough, when erasing a map entry the map.erase() method
	 * deletes the memory though.
	 *
	 * \param rCId iSub CID (URL)
	 * \param nodeId Node ID of the publisher
	 * \param ltpHeader LTP header for data
	 * \param packet Pointer to packet
	 * \param packetSize Size of the allocated packet memory
	 */
	void _bufferIcnPacket(IcnId &rCId, NodeId &nodeId,
			ltp_hdr_data_t &ltpHeader, uint8_t *packet);
	/*!
	 * \brief Buffer a sent LTP packet for NACK scenarios
	 *
	 * Uses _ltpPacketBuffer map
	 *
	 * \param rCid The rCid for which the packet should be buffered
	 * \param ltpHeaderData The LTP information required to create rCID > enigma >
	 * SK > Sequence relational map
	 * \param packet Pointer to the packet
	 * \param packetSize Total size of the packet (must be multiple of 8)
	 */
	void _bufferLtpPacket(IcnId &rCid,
			ltp_hdr_data_t &ltpHeaderData, uint8_t *packet,
			uint16_t packetSize);
	/*!
	 * \brief Add packet received from proxy (via HTTP handler) to LTP buffer
	 *
	 * This method adds the actual packet to the LTP buffer (_proxyPacketBuffer
	 * map) and also stores the iSub CID <> CID mapping for reverse look-ups to
	 * the _cIdReverseLookUp map
	 *
	 * \param cId Content ID (FQDN) - only used for better loggin output
	 * \param rCId iSub CID (URL) - key in proxy buffer map
	 * \param ltpHeaderData LTP header for data
	 * \param packet Pointer to packet
	 * \param packetSize Size of the allocated packet memory
	 */
	void _bufferProxyPacket(IcnId &cId, IcnId &rCId,
			ltp_hdr_data_t &ltpHeaderData, uint8_t *packet,
			uint16_t &packetSize);
	/*!
	 * \brief Check if locked CMC group does not exist anymore or has no members
	 *
	 * Note, when a NID is removed from a locked CMC group the corresponding
	 * methods delete empty groups.
	 *
	 * The group iterates over the _cmcGroups STL map
	 *
	 * \param rCid The rCID which should be looked up
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The SK which should be looked up
	 *
	 * \return True if group is empty or does not exist (anymore)
	 */
	bool _cmcGroupEmpty(IcnId &rCid, enigma_t &enigma, sk_t &sessionKey);
	/*!
	 * \brief Delete an entire HTTP packet from the LTP packet storage (sNAP)
	 *
	 * Uses _ltpPacketBuffer map
	 *
	 * \param rCid The rCID under which the packet has been buffered
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The SK under which the packet has been buffered
	 */
	void _deleteBufferedLtpPacket(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey);
	/*!
	 * \brief Delete buffered ICN packet from ICN packet buffer
	 *
	 * \param rCID The rCID map key under which the packet had been buffered
	 * \param nid The NID map key under which the packet had been buffered
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sk The SK map key under which the packet had been buffered
	 */
	void _deleteBufferedIcnPacket(IcnId &rCid, enigma_t &enigma, NodeId &nid,
			sk_t &sk);
	/*!
	 * \brief This method cleans up the LTP buffer entry for a given rCID > enigma
	 * > SK
	 *
	 * After the completion of an LTP session the actual entry stays in the LTP
	 * buffer for further packets to be stored and deleted. Only once the socket
	 * towards the server is closed the actual map keys are deleted
	 *
	 * \param rCid The rCID which should be looked up in _ltpPacketBuffer
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The SK which should be looked up in _ltpPacketBuffer
	 */
	void _deleteLtpBufferEntry(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey);
	/*!
	 * \brief Delete an entire NACK group
	 *
	 * \param rCid The rCID identifying the NACK group
	 * \param ltpHeaderNack The LTP header holding enigma and SK to identify the
	 * NACK group to be deleted
	 */
	void _deleteNackGroup(IcnId &rCid, ltp_hdr_ctrl_nack_t &ltpHeaderNack);
	/*!
	 * \brief Delete a buffered proxy packet from LTP buffer
	 *
	 * \param rCId The rCID for which the proxy packet buffer should be look up
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The SK for which the packet(s) should be deleted
	 */
	void _deleteProxyPacket(IcnId &rCId, enigma_t enigma, sk_t sessionKey);
	/*!
	 * \brief Delete a rCID > SK entry from _sessionEndedResponsesUnicast map
	 *
	 * \param rCid The rCID under which the other values are stored
	 * \param sessionKey  The SK
	 */
	void _deleteSessionEnd(IcnId &rCid, sk_t &sessionKey);
	/*!
	 * \brief Delete a rCID > enigma > NID(s) > SK entry from
	 * _sessionEndedResponses map
	 *
	 * \param rCid The rCID under which the other values are stored
	 * \param ltpHdrCtrlSed  The LTP CTRL SED header which holds enigma and SK
	 * \param nodeIds The list of NIDs for which the SKs should be deleted
	 */
	void _deleteSessionEnd(IcnId &rCid, ltp_hdr_ctrl_se_t &ltpHdrCtrlSe,
			list<NodeId> &nodeIds);

	/*!
	 * \brief Enable a NID to be ready to receive an HTTP response
	 *
	 * \param rCId The rCID for which the NID should be enabled
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param nodeId The NID which should be enabled
	 */
	void _enableNIdInCmcGroup(IcnId &rCId, enigma_t &enigma,
			NodeId &nodeId);
	/*!
	 * \brief Obtain the number of NIDs that are ready to receive HTTP responses
	 *
	 * This is information if used when HTTP request supresson is enabled in
	 * sNAP to determine whether or not the HTTP proxy has already sent off one
	 * HTTP request to the server
	 *
	 * \param rCId The rCID for which the NID should be enabled
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 */
	uint32_t _enabledNIdsInPotentialCmcGroup(IcnId &rCId, enigma_t &enigma);
	/*!
	 * \brief Check if data can be published to a particular NID
	 *
	 * \param nodeId The NID which shall be checked
	 *
	 * \return Boolean indicating if NID can be used to publish data to
	 */
	bool _forwardingEnabled(NodeId &nodeId);
	/*!
	 * \brief Obtain the list of NIDs in a particular CMC group
	 *
	 * Note, this method acts on pointers to the CMC group map declared and
	 * maintained in the class Http.
	 *
	 * This method does NOT lock the CMC group's mutex
	 *
	 */
	bool _getCmcGroup(IcnId &rCid, enigma_t &enigma,
		sk_t &sessionKey, list<NodeId> &cmcGroup);
	/*!
	 * \brief Handle an incoming LTP control message
	 *
	 * \param rCId The rCID under which the packet had been published
	 * \param packet Pointer to the entire CTRL message
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey When returning TP_STATE_ALL_FRAGMENTS_RECEIVED this
	 * argument contains the session key the LTP CTRL header carried
	 */
	tp_states_t _handleControl(IcnId &rCId, uint8_t *packet,
			enigma_t &enigma, sk_t &sessionKey);
	/*!
	 * \brief Handle an incoming LTP control message (iSub)
	 *
	 * \param cId The CID under which the packet has been published
	 * \param rCId The iSub CID under which the response must be published
	 * \param nodeId The NID which must be used when publishing the response
	 * \param packet Pointer to the CTRL message
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey This field is filled out if WE has been received and
	 * all fragment were received
	 */
	tp_states_t _handleControl(IcnId &cId, IcnId &rCId, NodeId &nodeId,
			uint8_t *packet, enigma_t &enigma, sk_t &sessionKey);
	/*!
	 * \brief Handling an incoming HTTP response
	 *
	 * \param rCid The rCID under which the packet has been received
	 * \param packet Pointer to the packet
	 */
	void _handleData(IcnId &rCid, uint8_t *packet);
	/*!
	 * \brief Handle an incoming HTTP request
	 *
	 * Note, packet must not point to the entire ICN payload anymore.  Instead,
	 * it has been already moved to the correct memory field
	 *
	 * \param cId The CID under which the packet has been published
	 * \param rCId The iSub CID under which the response must be published
	 * \param nodeId The NID which must be used when publishing the response
	 * \param packet Pointer to data
	 * \param packetSize Pointer to the length of data
	 */
	void _handleData(IcnId cId, IcnId &rCId, NodeId &nodeId,
			uint8_t *packet);
	/*!
	 * \brief Check if CTRL SED has been received for rCID > SK
	 *
	 * sNAP confirms SE sent by cNAP. Map used for this
	 * _sessionEndedResponsesUnicast
	 *
	 * \param rCid The rCID for which the SED boolean should be checked
	 * \param sessionKey The SK for which the SED boolean should be checked
	 *
	 * \return Boolean indicating of all SEDs have been received
	 */
	bool _ltpCtrlSedReceived(IcnId &rCid, sk_t &sessionKey);
	/*!
	 * \brief Check if CTRL SED has been received for rCID > enigma > SK > NIDs
	 *
	 * \param rCid The rCID for which the SED boolean should be checked
	 * \param nodeIds The list of NIDs which should be checked
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The SK for which the SED boolean should be checked
	 *
	 * \return Boolean indicating of all SEDs have been received
	 */
	bool _ltpCtrlSedReceived(IcnId rCid, list<NodeId> nodeIds,
			enigma_t enigma, sk_t sessionKey);
	/*!
	 * \brief Check if a previous LTP session for the same rCID > enigma > SK is
	 * still active and the new packet from a server must wait
	 *
	 * \param rCid The rCID which should be looked up
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The SK which should be looked up
	 *
	 * \return If this method returns true another session is still active
	 */
	bool _ltpSessionActivityCheck(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey);
	/*!
	 * \brief Set the status for a particular LTP sessions
	 *
	 * \param rCid The rCID for which the status should be set
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The SK for which the status should be set
	 * \param status The status which should be written
	 */
	void _ltpSessionActivitySet(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey, bool status);
	/*!
	 * \brief Obtain the group of NIDs that have sent a NACK in response to
	 * a WE CTRL message
	 *
	 * \param rCid The rCID for which the group size should be obtained
	 * \param ltpHeaderNack The LTP header of the NACK message received
	 *
	 * \return The list of NIDs in the NACK group for this particular rCID >
	 * enigma > SK
	 */
	nack_group_t _nackGroup(IcnId &rCid, ltp_hdr_ctrl_nack_t &ltpHeaderNack);
	/*!
	 * \brief Update the RTT
	 *
	 * RTT is used by the NACK timer to determine whether or not a control
	 * message to one of the subscribers got lost
	 *
	 * \param rtt The value which should be used to update _rtt
	 */
	void _recalculateRtt(boost::posix_time::time_duration &rtt);
	/*!
	 * \brief Publish HTTP responses
	 *
	 * When this method returns the LTP CTRL-WE header is filled with the last
	 * sequence number and the randomly generated session key
	 *
	 * \param rCId Reference to the iSub content identifier
	 * \param ltpHeaderCtrlWe Reference to the LTP CTRL-WE header which must
	 * hold the enigma
	 * \param nodeIds List of NIDs to which the data should be sent
	 * \param packet Pointer to the data
	 * \param dataSize Pointer to the length of the packet
	 */
	void _publishData(IcnId &rCId, ltp_hdr_ctrl_we_t &ltpHeaderCtrlWe,
			list<NodeId> &nodeIds, uint8_t *data, uint16_t &dataSize);
	/*!
	 * \brief Publish HTTP requests
	 *
	 * Called by public publish*() methods
	 *
	 * \param cId Reference to the content identifier
	 * \param rCId Reference to the iSub content identifier
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey Reference to the session key provided by the caller
	 * \param packet Pointer to the data
	 * \param dataSize Pointer to the length of the packet
	 *
	 * \return This method returns the sequence number of the last fragment to
	 * create LTP CTRL messages based on this important piece of information
	 */
	uint16_t _publishData(IcnId &cId, IcnId &rCId, enigma_t &enigma,
			sk_t &sessionKey, uint8_t *data, uint16_t &dataSize);
	/*!
	 * \brief Publish a range of segments from an HTTP request in response to
	 * previously received NACKs
	 *
	 * This method looks up the CID for the provided rCID and publishes the
	 * range of LTP fragments provided in the LTP header
	 *
	 * \param rCid The rCID under which the packet(s) shall be published
	 * \param ltpCtrlNack The LTP CTRL NACK message received from an sNAP
	 */
	void _publishDataRange(IcnId &rCid, ltp_hdr_ctrl_nack_t &ltpCtrlNack);
	/*!
	 * \brief Publish a range of segments from an HTTP response in response to
	 * previously received NACKs
	 *
	 * \param rCid The rCID under which the packet(s) shall be published
	 * \param enigma The enigma used in the LTP header to publish the packets
	 * \param sessionKey The SK used in the LTP header to publish the packets
	 * \param nackGroup The NIDs to which the range of segments shall be sent
	 */
	void _publishDataRange(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey, nack_group_t &nackGroup);
	/*!
	 * \brief Publish NACK in response to missing segments from an HTTP request
	 * session
	 *
	 * \param cid The CID under which the packet should be published
	 * \param nodeId The NID to which the NACK should be sent
	 * \param enigma The enigma to the sequence range belongs
	 * \param sessionKey The SK to which teh sequence range belongs
	 * \param firstMissingSequence The start of the range of sequence numbers
	 * which need to be re-send
	 * \param lastMissingSequence The end of the range of sequence numbers
	 * which need to be re-send
	 */
	void _publishNegativeAcknowledgement(IcnId &rCid, NodeId &nodeId,
			enigma_t &enigma, sk_t &sessionKey, seq_t &firstMissingSequence,
			seq_t &lastMissingSequence);
	/*!
	 * \brief Publish NACK in response to missing segments from an HTTP response
	 * session
	 *
	 * \param cid The CID under which the packet should be published
	 * \param rCid The rCID which will be included in the publication
	 * \param enigma The enigma to the sequence range belongs
	 * \param sessionKey The SK to which teh sequence range belongs
	 * \param firstMissingSequence The start of the range of sequence numbers
	 * which need to be re-send
	 * \param lastMissingSequence The end of the range of sequence numbers
	 * which need to be re-senduint8_t attempts
	 */
	void _publishNegativeAcknowledgement(IcnId &cid, IcnId &rCid,
			enigma_t &enigma, sk_t &sessionKey, seq_t &firstMissingSequence,
			seq_t &lastMissingSequence);
	/*!
	 * \brief Publish reset
	 *
	 * An LTP RST is published to reset an LTP sessions for surrogacy use cases
	 *
	 * \param cid The CID under which the LTP RST is supposed to be published
	 * \param rCid The rCID for the CID
	 * \param ltpHeaderCtrlRst The LTP RST header to be used
	 */
	void _publishReset(IcnId &cid, IcnId &rCid);
	/*!
	 * \brief Publish an LTP RSTED message
	 *
	 * \param rCid The rCID under which the LTP RSTED message is supposed to be
	 * published
	 * \param nid The NID used in the publish_data primitive
	 */
	void _publishResetted(IcnId &rCid, NodeId &nodeId);
	/*!
	 * \brief Publish session end notification from sNAP to cNAP(s)
	 *
	 * \param rCid The rCID under which the CTRL message shall be published
	 * \param nodeIds The NIDs to which the SE notification shall be published
	 * \param ltpHeaderCtrlSe The LTP CTRL-SE header
	 */
	void _publishSessionEnd(IcnId &rCid, list<NodeId> &nodeIds,
			ltp_hdr_ctrl_se_t &ltpHeaderCtrlSe);
	/*!
	 * \brief Publish session end notification from cNAP to sNAP
	 *
	 * Note, the enigma is not required here, as this primitive is simply used to
	 * close the socket at the sNAP towards IP endpoints
	 *
	 * \param cid The CID under which the LTP CTRL SE will be published
	 * \param rCid The rCID which will be included in the packet
	 * \param sessionKey The session key for which the session should be closed
	 */
	void _publishSessionEnd(IcnId cid, IcnId rCid, sk_t sessionKey);
	/*!
	 * \brief Publish session ended in response to a received session end
	 *
	 * sNAP to cNAP. Note that the enigma is 0 in that case
	 *
	 * \param rCid The rCid used for the publish_data_isub
	 * \param nodeId The NID to which the LTP CTRL SED message should be
	 * published
	 * \param sessionKey The SK for the LTP header
	 */
	void _publishSessionEnded(IcnId &rCid, NodeId &nodeId, sk_t &sessionKey);
	/*!
	 * \brief Publish session ended in response to a received session end
	 *
	 * cNAP to sNAP
	 *
	 * \param cid The CID under which SED will be published
	 * \param rCid The rCid used for the publish_data_isub
	 * \param enigma The enigma for the LTP header
	 * \param sessionKey The SK for the LTP header
	 */
	void _publishSessionEnded(IcnId &cid, IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey);
	/*!
	 * \brief Publish CTRL-WE message to a subscriber via iSub
	 *
	 * \param rCId The rCID under which the CTRL message should be published
	 * \param nodeIds The list of NIDs in the CMC group
	 * \param ltpHeader The LTP CTRL-WE header with the sequence number and the
	 * enigma
	 */
	void _publishWindowEnd(IcnId &rCId, list<NodeId> &nodeIds,
			ltp_hdr_ctrl_we_t &ltpHeader);
	/*!
	 * \brief Publish CTRL-WE message to a subscriber via iSub
	 *
	 * \param cId The CID under which the CTRL WE message is published
	 * \param rCId The iSub CId used when this method calls
	 * Blackadder::publish_data_iSub
	 * \param enigma The enigma included in the CTRL WE message
	 * \param sessionKey The session key for this HTTP session
	 * \param sequenceNumber The sequence number of the last segment published
	 */
	void _publishWindowEnd(IcnId &cId, IcnId &rCId, enigma_t &enigma,
			sk_t &sessionKey, seq_t &sequenceNumber);
	/*!
	 * \brief Publish CTRL-WED message to the sNAP in response to a received
	 * CTRL-WE message
	 *
	 * \param cid The CID under which the packet should be published
	 * \param rCid The rCID used to use implicit subscription
	 * \param ltpHeaderCtrlWed The LTP CTRL-WED header which already carries
	 * enigma and session key
	 */
	void _publishWindowEnded(IcnId &cid, IcnId &rCid,
			ltp_hdr_ctrl_wed_t &ltpHeaderCtrlWed);
	/*!
	 * \brief Send CTRL WED message to a publisher subscribed to the rCID
	 *
	 * \param rCId The CID under which the message gets published
	 * \param nodeId The NID of the NAP waiting for the WED message
	 * \param enigma The enigma included in the LTP header
	 * \param sessionKey The session key for this HTTP session
	 */
	void _publishWindowEnded(IcnId &rCId, NodeId &nodeId,
			enigma_t &enigma, sk_t &sessionKey);
	/*!
	 * \brief Send CTRL WU message to a list of subscribers (via CMC)
	 *
	 * \param rCid The rCID under which the packet is going to be published
	 * \param enigma The enigma used in the LTP header
	 * \param sessionKey The sesion key used in the LTP header
	 * \param nodeIds List of NIDs to which the packet should be published
	 */
	void _publishWindowUpdate(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey, list<NodeId> &nodeIds);
	/*!
	 * \brief Publish CTRL-WUD message to the sNAP in response to a received
	 * CTRL-WU message
	 *
	 * \param cid The CID under which the packet should be published
	 * \param rCid The rCID used to use implicit subscription
	 * \param ltpHeaderControlWud The enigma and SK used for the LTP CTRL-WUD
	 * header
	 */
	void _publishWindowUpdated(IcnId &cid, IcnId &rCid,
			ltp_hdr_ctrl_wud_t &ltpHeaderControlWud);
	/*!
	 * \brief Set LTP CTRL-SED in _sessionEndedResponseUnicast map
	 *
	 * For sNAPs confirming SE from cNAPs (socket closed)
	 *
	 * \param rCid The rCID (map key) for which the LTP CTRL-SED has been
	 * received
	 * \param sessionKey The SK (map key) for which the LTP CTRL-SED has been
	 * received
	 * \param state The SED state supposed to be set
	 */
	void _setSessionEnded(IcnId &rCid, sk_t &sessionKey, bool state);
	/*!
	 * \brief Remove a particular NID from both potential and locked CMC groups
	 *
	 * This method is called when LTP CTRL RST has been received
	 *
	 * \param rCid The rCID for which the given NID should be removed
	 * \param nodeId The NID which should be remove is present
	 * \param enigmaSkPairs Once the NID has been found in the list of locked CMC
	 * groups this variable holds the corresponding enigmas & SKs to clean up
	 * other buffers
	 */
	void _removeNidFromCmcGroups(IcnId &rCid, NodeId &nodeId,
			list< pair<enigma_t,sk_t> > *enigmaSkPairs);
	/*!
	 * \brief Remove a list of NIDs from the list of NIDs in a locked CMC group
	 *
	 * This method works on the _cmcGroups map
	 *
	 * \param rCid The rCID for which the list of NIDs is supposed to be removed
	 * \param enigma The enigma for which the list of NIDS is supposed to be removed
	 * \param SK The SK for which the list of NIDS is supposed to be removed
	 * \param nodeIds The list of NIDs which are supposed to be removed
	 */
	void _removeNidsFromCmcGroup(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey, list<NodeId> &nodeIds);
	/*!
	 * \brief Remove a list of NIDs from the list of NIDs from where WEDs are
	 * expected for a particular rCID > enigma > SK
	 *
	 * This method works on the _windowUpdate map
	 *
	 * \param rCid The rCID for which the list of NIDs is supposed to be removed
	 * \param enigma The enigma for which the list of NIDS is supposed to be removed
	 * \param SK The SK for which the list of NIDS is supposed to be removed
	 * \param nodeIds The list of NIDs which are supposed to be removed
	 */
	void _removeNidsFromWindowUpdate(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey, list<NodeId> &nodeIds);
	/*!
	 * \brief Obtain the current median over _rtts list
	 *
	 * \return RTT in ms
	 */
	uint16_t _rtt();
	/*!
	 * \brief Update the round trip time (if applicable)
	 *
	 * \param rtt An obtained RTT which should be checked to be updated in ms
	 */
	void _rtt(uint16_t rtt);
	/*!
	 * \brief Obtain the list of NIDs for which a WED was not received
	 *
	 * When publishing HTTP responses and running out of credit a CTRL-WU is
	 * published to all subscribers. This method allows the sNAP to check for
	 * which NID the WED had been received.
	 *
	 * Note, _windowUpdateMutex is not used in this private method, as it is
	 * called after the mutex was locked when an sNAP was running out of traffic
	 *
	 * \param rCid The rCID for which the list of NIDs should be obtained
	 * \param enigma The enigma for which the list of NIDs should be obtained
	 * \param sessionKey The SK for which the list of NIDs should be obtained
	 *
	 * \return A list with the NIDs
	 */
	list<NodeId> _wudsNotReceived(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey);
};

} /* namespace lightweight */

} /* namespace transport */

#endif /* NAP_TRANSPORT_LEIGHTWEIGHT_HH_ */
