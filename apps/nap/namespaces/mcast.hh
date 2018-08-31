/*
 * mcast.hh
 *
 *  Created on: 15 Apr 2017
 *      Author: Xenofon Vasilakos <xvas-at-aueb.gr>
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

#ifndef _NAP_NAMESPACES_IGMP_HH_
#define _NAP_NAMESPACES_IGMP_HH_

#include <blackadder.hpp>
#include <boost/thread.hpp>
#include <log4cxx/logger.h>
#include <unordered_map>
#include <map>
#include <algorithm>

#include <transport/transport.hh>
#include <types/icnid.hh>
#include <types/routingprefix.hh>
#include <namespaces/buffercleaners/ipbuffercleaner.hh>
#include <namespaces/iptypedef.hh>

#include "/usr/include/netinet/igmp.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include <chrono>
#include <ctime>
#include <thread>

#include <configuration.hh>
#include <monitoring/statistics.hh>

using namespace cleaners::ipbuffer;
using namespace configuration;
using namespace std;
using namespace transport;

namespace namespaces {

namespace mcast {

class MCast
{
	static log4cxx::LoggerPtr logger;

protected:
	/*!
	 * \brief Publish IGMP scopes for data and control messages */
	void initialise();
	/*!
	 * \brief Unsubscribe from routing prefix and unpublish the scope path  */
	void uninitialise();
	/*!
	 * \brief Set the forwarding state for a particular CID
	 * \param cid Reference to the CID (either for the <CTRL> or <DATA> channels)
	 * for which the forwarding state should be set
	 * \param state The forwarding state */
	void forwarding(IcnId &cId, bool state);

public:
	/*!
	 * \brief Constructor  */
	MCast(Blackadder *icnCore, Configuration &configuration,
			Transport &transport, Statistics &statistics, bool *run);
	/*!
	 * \brief Destructor  */
	~MCast();
	/*!
	 * \brief TODO
	 */
	void handle(IpAddress &sourceIpAddress, IpAddress &destinationIpAddress,
			uint8_t *packet);
	/*!
	 * \brief Handle multicast packet received at the demux of this sNAP
	 * \param sourceIpAddress Reference to the source address of multicast data packets
	 * \param destinationIpAddress Reference to the destination multicast address
	 * \param packet Pointer to the packet with mcast data to be published by this sNAP
	 * \param packetSize Reference to the size of the packet
	 */
	void handleMcastDataAtSNap(IpAddress & sourceIpAddress,
			IpAddress &destinationIpAddress, uint8_t *packet,
			uint16_t &packetSize);
	/*!
	 * \brief Handle a PUBLISHED_DATA BA event at cNAP
	 * \param icnId The content identifier of the event
	 */
	void handlePUBLISHED_DATA(IcnId icnId, Event *event);
	/*!
	 * \brief Handle a PUBLISHED_DATA_iSUB BA event at sNAP
	 * \param rCId The implicit subscription content identifier of the event
	 */
	void handlePUBLISHED_DATA_iSUB(IcnId rCId, Event *event);
	/*!
	 * \brief Handle a START_PUBLISH BA event at cNAP
	 * \param icnId The content identifier of the event */
	void handleSTART_PUBLISH(IcnId icnId);
	/*!
	 * \brief Handle a START_PUBLISH_iSUB BA event at sNAP
	 * \param rCId The implicit subscription content identifier of the event
	 */
	void handleSTART_PUBLISH_iSUB(IcnId rCId);
	/*!
	 * \brief Subscribe to a scope
	 *
	 * This method implements the subscription to an ICN ID by using the
	 * subscribe_scope() primitive of the Blackaddder API.
	 *
	 * \param icnId The content identifierentifier to which the subscription
	 * should be issued to
	 */
	void subscribeScope(IcnId &icnId);

private:
	bool isClientNAP, isServerNAP; // mode operation parameters of NAP
	unsigned MESSAGE_PROCESSING_LAG; // delay time before clearing state after leave message (ns)
	unsigned LEAVE_TIMER; // time between receiving leave from client and sending leave to sNAP (ms)
	Blackadder *_icnCore; /*!< Pointer to Blackadder instance */
	Configuration &_configuration; /*!< Reference to Configuration class */
	Transport &_transport; /*!< Reference to Transport class */
	Statistics &_statistics; /*!< Reference to statistics class */
	bool *_run; /*!< from main thread is any SIG* had been caught */
	IcnId* _rootScope_ptr;
	IcnId* _dataScope_ptr;
	IcnId* _controlScope_ptr;
	// statistics section:
	unordered_map<uint32_t, clock_t> _statJoin2FirstDatumMap; /*!<  cNAP: mcast
	IP -> time in clock ticks since join. If 0, then the time until first packet has been measured*/
	// [sNAP] Multicast address DB at the sNAP with advertised
	// Data scope IDs (notated as <DATA>)
	unordered_map<uint32_t, IcnId> _dataScopeIds;
	unordered_map<uint32_t, IcnId>::iterator _dataScopeIdsIt;
	boost::mutex _mutexDataScopeIds;
	//TODO this can be moved to the DB DATA CID state
	// the data forwarding state, set to true when receiving a START_PUBLISH
	map<uint32_t, bool> _dataFwdState;
	map<uint32_t, bool>::iterator _dataFwdStateIt;
	boost::mutex _mutexDataFwdState;
	// [sNAP] NIDs of joined cNAPs (on behlaf of the IP clients) kept at sNAP.
	// Non empty list of cNAP NIDs denotes an active multicast group at the sNAP
	// YT: when all clients leave the map entry will not be deleted!! Only the
	// list (the map's value) will be empty..
	unordered_map<uint32_t, list<string> *> _sNAPCounterOfCNapIDs;
	unordered_map<uint32_t, list<string> *>::iterator _sNAPCounterOfCNapIDsIt;
	boost::mutex _mutexSNapCounter;
	// [cNAP] Multicast group address DB with Control scope IDs
	// (notated as <CTRL>) corresponding to currently joined multicast addresses by locally attached UEs
	unordered_map<uint32_t, IcnId> _ctrlScopeIds;
	unordered_map<uint32_t, IcnId>::iterator _ctrlScopeIdsIt;
	boost::mutex _mutexCtrlScopeIds;
	// [cNAP] MCast address DB at the cNAP with
	//1. counter of unanswered queries sent from this cNAP,
	// where "unanswered" implies no membership reports sent back to
	// make the counter equal to zero.
	unordered_map<uint32_t, uint32_t> _mcastDB_cNap;
	unordered_map<uint32_t, uint32_t>::iterator _mcastDB_cNapIt;
	//2. pointers to
	// to sockets maintined for mcasting publications to locally
	// joined IP clients
	unordered_map<uint32_t, pair<int, struct sockaddr_in *>> _mcastDB_cNapIPSockets;
	unordered_map<uint32_t, pair<int, struct sockaddr_in *>> ::iterator _mcastDB_cNapIPSocketsIt;
	boost::mutex _mutexMcastDB_cNap;
	// Essentially, set this to false only to end the detached threads
	// for maintaining this DB, stats, etc...
	bool _runDetachedThds;
	thread _snd2IPClientsThd;
	thread _statsThd;
	//TODO this can be moved to the DB CTRL CID state
	// the control data forwarding state, set to true when receiving a START_PUBLISH
	map<uint32_t, bool> _ctrlFwdState;
	map<uint32_t, bool>::iterator _ctrlFwdStateIt;
	boost::mutex _mutexCtrlFwdState;
	// [cNAP] Source address of currently joined clients at a cNAP
	// kept for rejecting redundant Join/Leave requests
	// and Leave requests by IP clients unregistered to an MCast
	unordered_map<uint32_t, int> _clientCounter; // mcast address -> how many client UEs
	boost::mutex _mutexClientCounter;
	// [sNAP/cNAP] Used for bufferring both control and data packets until
	// a proper BA event (e.g. a START_PUBLISH) takes place
	packet_buffer_t _packetBuffer; /*!< Buffer for mcasted packets */
	packet_buffer_t::iterator _bufferIt; /*!< Iterator for _buffer map */
	boost::mutex _mutexBuffer; /*!< Mutex for transaction safe operations on _buffer */
	boost::thread *_mcastBufferThread;
	// What follows is subject to the properties file loaded with _loadSNapProps()
	// supported multicast group IDs by this sNAP
	// and their corresponding port number
	unordered_map<string, uint16_t> _sNapMCastIPs;
	// list of ignored source IPs for IGMP messages
	vector<string> _ignoreIGMPFrom;
	// list of ignored source IPs for multicast data packets
	vector<string> _ignoreMCastDataFrom;
	// milliseconds until next general query
	uint32_t GENERAL_QUERY_TIMER;
	// milliseconds until next redudant IGMP (report or leave) message to IPTV
	uint32_t _sNapToIPTVServerTimer;
	// IGMP messages can be redundantly sent more than once, as there is no
	// transport protocol assumed for IGMP
	uint32_t REDUNDANT_IGMP_NUM;
	// upon receiving a leave IGMP message, send group specific queries
	// for the group reported as left, and use this variable to drop the
	// entire multicast group after 2-3 queries go unanswered.
	uint32_t DROM_AFTER_QUER_NUM;
	// general query IP address
	string GENERAL_QUERY_IP, _groupLeaveIP;
	/*!
	 * \brief TODO
	 */
	void _loadSNapProps();
	/*!
	 * \brief buffer this data or cotrol channel packet.
	 * \param icnId The CID of the packet (data or control)
	 * \param packet A pointer to the data to buffer
	 * \param packetSize the size of the packet to buffer
	 */
	void _bufferPacket(IcnId &icnId, uint8_t *packet, uint16_t &packetSize);
	/*!
	 * \brief delete from buffer.
	 * \param icnId The CID of the packet (data or control) to delete
	 */
	void _deleteBufferedPacket(IcnId &cid);
	/*!
	 * \brief separates cotrol from data channel CIDs
	 * \param icnId The CID to separate
	 * \return 1 for cotrol, 2 for data, -1 for anything else
	 */
	int _cntrl1_data2(IcnId &cId);
	/*!
	 * \brief sends Query IGMP messages to IP clients
	 * \param mcast the multicast group for explicit group queries; allows NULL
	 * for general queries
	 */
	void _cNAP2IPClientQueries(IpAddress mcast = IpAddress("0.0.0.0"));
	/*!
	 * \brief timer on the cNAP side for periodic method execution.
	 * See method _cNAP2IPClientQueries
	 * \param period in milliseconds
	 */
	void _cNapGeneralQueriesTimer(uint32_t period);
	/*!
	 * \brief TODO
	 *
	 * \param multicastIpAddress The multicast group address
	 * \return
	 *  true: if group received a member report after receiving leave group and
	 *  sending group query
	 * 	false: if group did not receive a member report after receiving leave
	 * 	group and sending group query
	 */
	void _removeGroupAfterLeave(IpAddress groupAddr);
	/*!
	 * \brief Increase the counter of joined cNAPs in this sNAP.
	 *
	 * \param multicastIpAddress The multicast IP address
	 * \param cNapNodeId The node id of the cNAP sending that wants to join
	 * \return
	 * number of joined cNAPs after registering this cNAP or
	 * -1 if the cNAP is already registered or
	 * -2 if the multicast group address is not valid for this sNAP */
	int _incSNapCounterFor(IpAddress &multicastIpAddress, string &cNapNodeId);
	/*!
	 * \brief Decrease the counter of joined cNAPs in this sNAP.
	 * \param multicastIpAddress The multicast IP address
	 * \param cNapNodeId The node id of the cNAP sending that wants to join
	 * \return
	 * number of joined cNAPs after unregistering this cNAP or
	 * -1 if the cNAP is not registered or
	 * -2 if the multicast group address is not valid for this sNAP */
	int _decrSNapCounterFor(IpAddress &multicastIpAddress, string &cNapNodeId);
	/*!
	 * \brief TODO
	 */
	void _forwardingData(IcnId &dataCid, bool state);
	/*!
	 * \brief TODO
	 */
	void _forwardingCtrl(IcnId &ctrlCid, bool state);
	/*!
	 * \brief TODO
	 */
	void _tryClearCNapStateFor(uint8_t *packet, IcnId* ctrlCid_ptr,
			IpAddress* multicastIpAddress_ptr = NULL);
	/*!
	 * \brief TODO
	 */
	void _extractFromPacket(uint8_t *packet, unsigned int packetSize,
			IpAddress & sourceIpAddress, IpAddress &multicastIpAddress,
			char** data = NULL, u_int16_t * dataSz = NULL);
	/*!
	 * \brief TODO
	 */
	void _prepareIgmpPacket(IpAddress &multicastIpAddress,
			const unsigned char igmpHdrType,  uint8_t *packet);
	/*!
	 * \brief TODO
	 */
	void _startPublishFromCtrlBuffer(IcnId ctrlCid);
	/*!
	 * \brief TODO
	 */
	void _publishIGMPCtrl_data_isub(IcnId &ctrlCid, IcnId &dataCid,
			uint8_t * packet, unsigned int packetSize);
	/*!
	 * \brief TODO
	 */
	void _publishFromDataBuffer(IcnId &dataCid);
	/*!
	 * \brief TODO
	 */
	void _tryPublishData(IcnId &dataCid, uint8_t * packet,
			unsigned int packetSize, list<string> * _cNapNids = NULL,
			IpAddress * multicastIpAddress = NULL);
	/*!
	 * \brief Handle multicast packet received at cNAP
	 * \param event Pointer reference to the underlying BA event (PUBLISHED_DATA)
	 */
	void _handleMCastDataAtCNap(Event * event);
	/*!
	 * \brief Handle at sNAP side the published packet to <CTRL> for a cNAP,
	 * with implicit subscription to <DATA>
	 * \param event Pointer reference to the underlying BA event at sNAP
	 * (PUBLISHED_DATA_iSUB)
	 */
	void _handleCtrlPub_dataISub(Event * event);
	/*!
	 * \brief IGMP checksum
	 * According top RFC 2236, section "2.3.  Checksum"
	 *    The checksum is the 16-bit one's complement of the one's complement
	 *    sum of the whole IGMP message (the entire IP payload). For computing
	 *    the checksum, the checksum field is set to zero.  When transmitting
	 *    packets, the checksum MUST be computed and inserted into this field.
	 *    When receiving packets, the checksum MUST be verified before
	 *    processing a packet. */
	u_int16_t _cksum(u_int16_t * data, int32_t length);
	/*!
	 * \brief TODO
	 */
	void _sNapToIPTVServer(IpAddress mcast, uint8_t type);
	/*!
	 * \brief TODO
	 */
	void _handleIGMPQuery(IpAddress & sourceIpAddress,
			IpAddress &destinationIpAddress, struct igmp * igmpHdr);
	/*!
	 * \brief Handle IGMP client membership report (join) request packet
	 * according to definition
	 * \param destinationIpAddress Reference to the destination multicast IGMP
	 * address
	 * \param packet Pointer to the IGMP packet
	 * \param packetSize Reference to the size of the IGMP packet
	 * \param configuration the NAP configuration */
	void _handleIGMPMemReport(IpAddress & sourceIpAddress,
			IpAddress &destinationIpAddress, struct igmp * igmpHdr);
	/*!
	 * \brief Handle IGMP client leave request packet according to definition
	 * \param destinationIpAddress Reference to the destination multicast IGMP address
	 * \param packet Pointer to the IGMP packet
	 * \param packetSize Reference to the size of the IGMP packet
	 * \param configuration the NAP configuration */
	void _handleIGMPLeave(IpAddress & sourceIpAddress,
			IpAddress &multicastIpAddress, struct igmp * igmpHdr);
	/*!
	 * \brief Forces this cNAP to leave this group and updates the
	 * corresponding sNAP(s) via the ICN n/w
	 * \param multicastIpAddress the multicast group */
	void _forceLeave(IpAddress &multicastIpAddress);
	/*!
	 * \brief TODO
	 */
	bool _mcastDB_cNapErase(IpAddress * multicastIpAddress_ptr);
};

} /* namespace mcast */

} /* namespace namespaces */

#endif /* _NAP_NAMESPACES_IGMP_HH_ */
