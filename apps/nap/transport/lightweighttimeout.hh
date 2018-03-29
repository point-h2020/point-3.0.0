/*
 * lightweighttimeout.hh
 *
 *  Created on: 20 Jul 2016
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

#ifndef NAP_TRANSPORT_LIGHTWEIGHTTIMEOUT_HH_
#define NAP_TRANSPORT_LIGHTWEIGHTTIMEOUT_HH_

#include <blackadder.hpp>
#include <boost/date_time.hpp>
#include <log4cxx/logger.h>
#include <map>
#include <mutex>
#include <thread>

#include <configuration.hh>
#include <types/enumerations.hh>
#include <types/icnid.hh>
#include <transport/lightweighttypedef.hh>
#include <types/nodeid.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace log4cxx;
using namespace std;

namespace transport
{

namespace lightweight
{
/*!
 * \brief Implementation of the LTP timer as a non-blocking operation
 */
class LightweightTimeout
{
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor for HTTP requests in sNAP
	 *
	 * Expensive class instantiation, but only required if START_PUBLISH has not
	 * been received
	 *
	 * \param cId Reference to the CID under which WE has been published
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param icnCore
	 */
	LightweightTimeout(IcnId cId, IcnId rCId, enigma_t enigma,
			uint16_t sessionKey, uint16_t sequenceNumber, Blackadder *icnCore,
			std::mutex &icnCoreMutex, uint16_t rtt, proxy_packet_buffer_t *proxyPacketBuffer,
			std::mutex *proxyPacketBufferMutex, window_ended_requests_t *windowEnded,
			std::mutex *windowEndedMutex);
	/*!
	 * \brief Constructor for LTP RST timeout in cNAP
	 *
	 * Called when LTP RST is sent and LTP RSTED is awaited from sNAP
	 *
	 * \param cid The CID under which the LTP RST has been published
	 * \param rCid The rCID for which the LTP RSTED is expected to arrive (map
	 * key in reset map)
	 * \param icnCore Pointer to ICN core (Blackadder) to republish LTP RST if
	 * timeout has been reached
	 * \param icnCoreMutex Reference mutex for icnCore pointer to avoid two
	 * threads accessing the BA API simultaneously
	 * \param rtt The RTT to be used for timeouts
	 * \param Pointer to reset map which holds state about received LTP RSTs
	 * \param resettedMutex Reference to mutex for thread-safe operations on
	 * resetted
	 */
	LightweightTimeout(IcnId cid, IcnId rCid, Blackadder *icnCore,
			std::mutex &icnCoreMutex, uint16_t rtt, void *resetted,
			std::mutex *resettedMutex);
	/*!
	 * \brief Destructor
	 */
	~LightweightTimeout();
	/*!
	 * \brief Functor
	 */
	void operator()();
private:
	IcnId _cId;
	IcnId _rCId;
	ltp_hdr_data_t _ltpHeaderData;
	NodeId _nodeId;
	Blackadder *_icnCore;
	std::mutex &_icnCoreMutex;
	uint16_t _rtt;
	proxy_packet_buffer_t *_proxyPacketBuffer; /*!< Pointer to proxy packet
	buffer */
	proxy_packet_buffer_t::iterator _proxyPacketBufferIt;/*!< Iterator for
	packetBuffer map */
	std::mutex *_proxyPacketBufferMutex;/*!< mutex for _packetBuffer map*/
	resetted_t *_resetted;/*!< map<cid, map<Enigma, map<NID, map<RST received */
	std::mutex *_resettedMutex;/*!< mutex for _resetted map*/
	window_ended_requests_t *_windowEndedRequests;/*!<
	map<rCID, map<Enigma, map<SK, WE received>> LTP CTRL-WED map for HTTP requests
	sessions (cNAP > sNAP)*/
	map<cid_t, map<enigma_t, map<sk_t, bool>>>::iterator _windowEndedRequestsIt;/*!<
	Iterator for _windowEndedRequests map */
	map<cid_t, map<enigma_t, map<sk_t, bool>>> *_windowEndedResponses;
	/*!<map<rCID, map<Enigma, map<NID, WE received>>> LTP CTRL WE*/
	map<cid_t, map<enigma_t, map<sk_t, bool>>>::iterator
	_windowEndedResponsesIt;/*!<Iterator for _windowEndedRequests map */
	std::mutex *_windowEndedMutex;/*!<Reference to the mutex allowing
	transaction safe operation*/
	bool _ltpReset;/*!< Boolean to keep knowledge which constructor has been
	called*/
	/*!
	 * \brief Check for LTP RST instead
	 *
	 * The respective boolean _ltpReset has been set in the constructor
	 */
	void _checkForLtpRsted();
	/*!
	 * \brief Delete all objects created in constructors
	 */
	void _deleteObjects();
	/*!
	 * \brief Delete a buffered proxy packet from LTP buffer
	 *
	 * \param rCId The rCID for which the proxy packet buffer should be look up
	 * \param ltpHeader The LTP header holding all the required information to
	 * delete the right packet
	 */
	void _deleteProxyPacket(IcnId &rCId, ltp_hdr_ctrl_wed_t &ltpHeader);
	/*!
	 * \brief Re-publish WED CTRL message
	 */
	void _rePublishWindowEnd();
	/*!
	 * \brief Re-publish RST message
	 */
	void _rePublishReset();
};

} /* namespace lightweight */

} /* namespace transport */

#endif /* NAP_TRANSPORT_LIGHTWEIGHTTIMEOUT_HH_ */
