/*
 * lightweight.cc
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

#include "../proxies/http/httpproxy.hh"
#include "../proxies/http/tcpclient.hh"
#include "lightweight.hh"
#include "lightweighttimeout.hh"
#include "lightweighttypedef.hh"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace namespaces;
using namespace proxies::http::tcpclient;
using namespace transport::lightweight;
using namespace cleaners::ltpbuffer;
using namespace cleaners::ltpreversecid;

LoggerPtr Lightweight::logger(Logger::getLogger("transport.lightweight"));

Lightweight::Lightweight(Blackadder *icnCore, Configuration &configuration,
		std::mutex &icnCoreMutex, Statistics &statistics, bool *run)
	: TrafficControl(configuration),
	  _icnCore(icnCore),
	  _configuration(configuration),
	  _icnCoreMutex(icnCoreMutex),
	  _statistics(statistics)
{
	_knownNIds = NULL;// will be set in initialise() method
	_knownNIdsMutex = NULL;// will be set in initialise() method
	_cmcGroups = NULL;// will be set in initialise() method
	_cmcGroupsMutex = NULL;// will be set in initialise() method
	_potentialCmcGroups = NULL;// will be set in initialise() method
	_potentialCmcGroupsMutex = NULL;// will be set in initialise() method
	uint16_t rttListSize = 0;

	while (rttListSize < _configuration.ltpRttListSize())
	{
		_rtts.push_front(200);// milli seconds
		rttListSize++;
	}

	_rttMultiplier = _configuration.ltpRttMultiplier();
	LtpBufferCleaner ltpBufferCleaner(_configuration, _statistics,
			&_ltpPacketBuffer, &_ltpPacketBufferMutex,	run);
	std::thread *ltpBufferCleanerThread = new std::thread(ltpBufferCleaner);
	ltpBufferCleanerThread->detach();
	delete ltpBufferCleanerThread;
	LtpReverseCidCleaner ltpReverseCidCleaner(configuration, &_cIdReverseLookUp,
			&_cIdReverseLookUpMutex);
	std::thread *ltpReverseCidCleanerThread =
			new std::thread(ltpReverseCidCleaner);
	ltpReverseCidCleanerThread->detach();
	delete ltpReverseCidCleanerThread;
}

Lightweight::~Lightweight(){}

bool Lightweight::cidLookup(IcnId &rCid, IcnId &cid)
{
	reverse_cid_lookup_t::iterator _cIdReverseLookUpIt;
	_cIdReverseLookUpMutex.lock();
	_cIdReverseLookUpIt = _cIdReverseLookUp.begin();//TODO check that u_map really doesn't have a reverse iterator

	while (_cIdReverseLookUpIt != _cIdReverseLookUp.end())
	{
		if (_cIdReverseLookUpIt->first == rCid.uint())
		{
			cid = _cIdReverseLookUpIt->second;
			_cIdReverseLookUpIt->second.access();
			_cIdReverseLookUpMutex.unlock();
			return true;
		}

		_cIdReverseLookUpIt++;
	}

	return true;
}

void Lightweight::cleanUpBuffers(IcnId &cid, enigma_t &enigma,
		sk_t &sessionKey)
{
	LOG4CXX_DEBUG(logger, "Clean up LTP packet and proxy buffer for rCID "
			<< cid.print() << " > enigma " << enigma << " > SK "
			<< sessionKey);

	while (Lightweight::_ltpSessionActivityCheck(cid, enigma, sessionKey))
	{
		// loop until LTP has finsihed
	}

	_deleteBufferedLtpPacket(cid, enigma, sessionKey);
	_deleteLtpBufferEntry(cid, enigma, sessionKey);
	_deleteProxyPacket(cid, enigma, sessionKey);
}

void Lightweight::initialise(void *potentialCmcGroup,
		void *potentialCmcGroupMutex, void *knownNIds, void *knownNIdsMutex,
		void *cmcGroups, void *cmcGroupsMutex)
{
	_potentialCmcGroups = (potential_cmc_groups_t *) potentialCmcGroup;
	_potentialCmcGroupsMutex = (std::mutex *) potentialCmcGroupMutex;
	_knownNIds = (map<uint32_t, bool> *) knownNIds;
	_knownNIdsMutex = (std::mutex *) knownNIdsMutex;
	_cmcGroups = (cmc_groups_t *)cmcGroups;
	_cmcGroupsMutex = (std::mutex *)cmcGroupsMutex;
}

void Lightweight::forwarding(IcnId &cid, bool state)
{
	_cIdReverseLookUpMutex.lock();

	// Iterate over all known rCIDs
	for (auto it = _cIdReverseLookUp.begin(); it != _cIdReverseLookUp.end();
			it++)
	{
		// set forwarding state to "state"
		if (it->second.uint() == cid.uint())
		{
			it->second.forwarding(state);
			LOG4CXX_DEBUG(logger, "Forwarding state for CID " << cid.print()
					<< " set to " << state);

			if (state)
			{
				LOG4CXX_DEBUG(logger, "FID requested state disable for CID "
						<< cid.print());
				it->second.fidRequested(false);
			}
		}
	}

	_cIdReverseLookUpMutex.unlock();
}

void Lightweight::publish(IcnId &rCid, enigma_t &enigma,
		sk_t &sessionKey, list<NodeId> &nodeIds, uint8_t *data,
		uint16_t &dataSize)
{
	ltp_hdr_ctrl_we_t ltpHeaderCtrlWe;
	list<NodeId>::iterator nodeIdsIt;
	ltpHeaderCtrlWe.enigma = enigma;
	ltpHeaderCtrlWe.sessionKey = sessionKey;

	if(nodeIds.empty())
	{
		LOG4CXX_WARN(logger, "List of NIDs is empty! Must not happen ... ")
	}

	// check if a previous LTP session is still active for rCID > enigma > SK
	while (_ltpSessionActivityCheck(rCid, enigma, sessionKey))
	{
		// just loop until the OK comes to go ahead
	}

	// set the state and start publishing
	_ltpSessionActivitySet(rCid, enigma, sessionKey, true);
	_publishData(rCid, ltpHeaderCtrlWe, nodeIds, data, dataSize);
	_publishWindowEnd(rCid, nodeIds, ltpHeaderCtrlWe);
	// check for CTRL-WED received
	//map<Session
	map<uint16_t, bool>::iterator sessionKeyIt;
	//map<NID,    map<Session
	map<uint32_t, map<uint16_t, bool>>::iterator nidIt;
	//map<Enigma,   map<NID,      map<Session
	map<uint32_t, map<uint32_t, map<uint16_t, bool>>>::iterator enigmaIt;
	// Now start WE timer
	uint8_t attempts = ENIGMA;
	uint16_t rtt = _rtt();
	boost::posix_time::ptime startStartTime =
					boost::posix_time::microsec_clock::local_time();

	while (attempts != 0)
	{
		// check boolean continuously for 2 * RRT (timeout)
		boost::posix_time::ptime startTime =
				boost::posix_time::microsec_clock::local_time();
		boost::posix_time::ptime currentTime = startTime;
		boost::posix_time::time_duration timeWaited;
		timeWaited = currentTime - startTime;

		// check for received WED until _timeout * RTT has been reached
		while (timeWaited.total_milliseconds() < (_rttMultiplier * rtt))
		{
			currentTime = boost::posix_time::microsec_clock::local_time();
			timeWaited = currentTime - startTime;
			_windowEndedResponsesMutex.lock();
			_windowEndedResponsesIt = _windowEndedResponses.find(rCid.uint());

			if (_windowEndedResponsesIt == _windowEndedResponses.end())
			{
				LOG4CXX_WARN(logger, "rCID " << rCid.print() << " does not "
						"exist in CTRL-WED map");
				_windowEndedResponsesMutex.unlock();
				return;
			}

			enigmaIt = _windowEndedResponsesIt->second.find(enigma);
			uint16_t numberOfConfirmedWeds = 0;

			for (nodeIdsIt = nodeIds.begin(); nodeIdsIt != nodeIds.end();
					nodeIdsIt++)
			{
				nidIt = enigmaIt->second.find(nodeIdsIt->uint());

				// NID found
				if (nidIt != enigmaIt->second.end())
				{
					sessionKeyIt = nidIt->second.find(
							ltpHeaderCtrlWe.sessionKey);

					// session key found
					if (sessionKeyIt != nidIt->second.end())
					{
						// WED received
						if (sessionKeyIt->second)
						{
							numberOfConfirmedWeds++;
						}
					}
				}
			}

			// Now check if the number of confirmed NIDs equals the CMC size
			if (numberOfConfirmedWeds == nodeIds.size())
			{
				timeWaited = currentTime - startStartTime;
				LOG4CXX_TRACE(logger, "All CTRL-WEDs received for rCID CMC "
						<< rCid.print() << " after waiting "
						<< timeWaited.total_milliseconds() << "ms");
				_rtt(timeWaited.total_milliseconds());
				_windowEndedResponsesMutex.unlock();
				return;
			}

			_windowEndedResponsesMutex.unlock();
		}

		LOG4CXX_TRACE(logger, "LTP CTRL-WED has not been received within given "
				"timeout of " << timeWaited.total_milliseconds() << "ms for "
				"rCID "	<< rCid.print());
		_publishWindowEnd(rCid, nodeIds, ltpHeaderCtrlWe);
		attempts--;

		if (attempts == 0)
		{
			LOG4CXX_DEBUG(logger, "Subscriber to rCID " << rCid.print()
					<< " has not responded to LTP CTRL-WE message after "
					<< (int)ENIGMA << " attempts. Stopping here and clean up "
					"LTP stack");
			_deleteBufferedLtpPacket(rCid, enigma, sessionKey);
			_ltpSessionActivitySet(rCid, enigma, sessionKey, false);
		}
	}
}

void Lightweight::publish(IcnId &cId, IcnId &rCId, enigma_t &enigma,
		sk_t &sessionKey, uint8_t *data, uint16_t &dataSize)
{
	_addReverseLookUp(cId, rCId);
	seq_t sequenceNumber;
	// First publish the data
	sequenceNumber = _publishData(cId, rCId, enigma, sessionKey, data,
			dataSize);
	map<sk_t, bool>::iterator sessionKeyMapIt;
	map<enigma_t, map<sk_t, bool>>::iterator enigmaMapIt;
	// Now start WE timer
	uint8_t attempts = ENIGMA;
	uint16_t rtt = _rtt();
	boost::posix_time::ptime startStartTime =
					boost::posix_time::microsec_clock::local_time();

	while (attempts != 0)
	{
		// check boolean continuously for 2 * RRT (timeout)
		boost::posix_time::ptime startTime =
				boost::posix_time::microsec_clock::local_time();
		boost::posix_time::ptime currentTime = startTime;
		boost::posix_time::time_duration timeWaited;
		timeWaited = currentTime - startTime;

		// check for received WED until _timeout * RTT has been reached
		while (timeWaited.total_milliseconds() < (_rttMultiplier * rtt))
		{
			currentTime = boost::posix_time::microsec_clock::local_time();
			timeWaited = currentTime - startTime;
			_windowEndedRequestsMutex.lock();
			_windowEndedRequestsIt = _windowEndedRequests.find(rCId.uint());

			if (_windowEndedRequestsIt == _windowEndedRequests.end())
			{
				LOG4CXX_WARN(logger, "rCID " << rCId.print() << " does not "
						"exist in CTRL-WED map");
				_windowEndedRequestsMutex.unlock();
				return;
			}

			enigmaMapIt =	_windowEndedRequestsIt->second.find(enigma);
			sessionKeyMapIt = enigmaMapIt->second.find(sessionKey);

			// window ended received. stop here, update RTT and leave
			if (sessionKeyMapIt->second)
			{
				timeWaited = currentTime - startStartTime;
				LOG4CXX_TRACE(logger, "LTP CTRL-WED received for CID "
						<< cId.print() << " after waiting "
						<< timeWaited.total_milliseconds() << "ms");
				// Update RTT
				_rtt(timeWaited.total_milliseconds());
				_statistics.roundTripTime(cId, timeWaited.total_milliseconds());
				// Erase entry from WED request map
				enigmaMapIt->second.erase(sessionKeyMapIt);
				LOG4CXX_TRACE(logger, "SK (FD) " << sessionKey << " removed "
						"from CTRL-WED map for rCID " << rCId.print()
						<< " > enigma " << enigma);

				// Delete entire enigma key if empty
				if (enigmaMapIt->second.size() == 0)
				{
					_windowEndedRequestsIt->second.erase(enigmaMapIt);
					LOG4CXX_TRACE(logger, "Entire CTRL-WED enigma entry "
							<< enigma << " in _windowEndedRequests map"
							" deleted for rCID " << rCId.print());
				}

				// Delete the entire rCID key if values are empty
				if (_windowEndedRequestsIt->second.size() == 0)
				{
					_windowEndedRequests.erase(_windowEndedRequestsIt);
					LOG4CXX_TRACE(logger, "Entire rCID " << rCId.print()
							<< " entry in _windowEndedRequests map "
									"deleted");
				}

				_windowEndedRequestsMutex.unlock();
				return;
			}

			_windowEndedRequestsMutex.unlock();
		}

		LOG4CXX_TRACE(logger, "WED CTRL has not been received within given "
				"timeout of " << _rttMultiplier * rtt << "ms for CID "
				<< cId.print());
		_publishWindowEnd(cId, rCId, enigma, sessionKey, sequenceNumber);
		attempts--;

		if (attempts == 0)
		{
			LOG4CXX_DEBUG(logger, "Subscriber to CID " << cId.print() << " has "
					"not responded to WE CTRL message after " << (int)ENIGMA
					<< " attempts. Stopping here");
		}
	}
}

void Lightweight::publishEndOfSession(IcnId &rCid, enigma_t &enigma,
		sk_t &sessionKey)
{
	ltp_hdr_ctrl_se_t ltpHeaderCtrlSe;
	list<NodeId> nodeIds;
	list<NodeId>::iterator nodeIdsIt;
	ltpHeaderCtrlSe.enigma = enigma;
	ltpHeaderCtrlSe.sessionKey = sessionKey;
	_cmcGroupsMutex->lock();
	_cmcGroupsIt = _cmcGroups->find(rCid.uint());

	if (_cmcGroupsIt == _cmcGroups->end())
	{
		LOG4CXX_TRACE(logger, "CMC group for rCID " << rCid.print()
				<< " already closed");
		_cmcGroupsMutex->unlock();
		return;
	}

	map<enigma_t, map<sk_t, list<NodeId>>>::iterator cmcenigmaIt;
	cmcenigmaIt = _cmcGroupsIt->second.find(enigma);

	if (cmcenigmaIt == _cmcGroupsIt->second.end())
	{
		LOG4CXX_TRACE(logger, "CMC group for rCID " << rCid.print()
				<< " > enigma " << enigma << " already closed");
		_cmcGroupsMutex->unlock();
		return;
	}

	map<sk_t, list<NodeId>>::iterator cmcSkIt;
	cmcSkIt = cmcenigmaIt->second.find(sessionKey);

	if (cmcSkIt == cmcenigmaIt->second.end())
	{
		LOG4CXX_TRACE(logger, "CMC group for rCID " << rCid.print()
				<< " > enigma " << enigma << " > SK " << sessionKey
				<< " already closed");
		_cmcGroupsMutex->unlock();
		return;
	}

	nodeIds = cmcSkIt->second;
	_cmcGroupsMutex->unlock();

	if(nodeIds.empty())
	{
		LOG4CXX_TRACE(logger, "List of NIDs is empty. End of LTP session "
				"cannot be sent to cNAPs for rCID " << rCid.print() << " > "
				"enigma " << enigma << " > SK " << sessionKey);
		return;
	}

	_addSessionEnd(rCid, ltpHeaderCtrlSe, nodeIds);
	_publishSessionEnd(rCid, nodeIds, ltpHeaderCtrlSe);
	// check for CTRL-SED received
	if (!_ltpCtrlSedReceived(rCid, nodeIds, ltpHeaderCtrlSe.enigma,
			ltpHeaderCtrlSe.sessionKey))
	{
		ostringstream nodeIdsOss;

		for (auto it = nodeIds.begin(); it != nodeIds.end(); it++)
		{
			nodeIdsOss << it->uint() << " ";
		}

		LOG4CXX_DEBUG(logger, "LTP CTRL SED for rCID " << rCid.print()
				<< " > NIDs " << nodeIdsOss.str() << "> enigma "
				<< ltpHeaderCtrlSe.enigma << " > SK "
				<< ltpHeaderCtrlSe.sessionKey << " not received. Assuming one "
				"of the nodes dissappeared");
	}

	// now delete the all LTP packet buffers
	Lightweight::cleanUpBuffers(rCid, ltpHeaderCtrlSe.enigma,
			ltpHeaderCtrlSe.sessionKey);
}

void Lightweight::publishEndOfSession(IcnId cid, IcnId rCid, sk_t sessionKey)
{
	uint8_t attempts = ENIGMA;
	_addSessionEnd(rCid, sessionKey);
	_publishSessionEnd(cid, rCid, sessionKey);

	// start CTRL SED check (sNAP to confirm SE)
	while (attempts > 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(_rtt()));

		if (!_ltpCtrlSedReceived(rCid, sessionKey))
		{
			LOG4CXX_TRACE(logger, "LTP CTRL-SED confirmation not received yet "
					"for rCID "	<< rCid.print() << " > SK " << sessionKey
					<< ". " << (int)attempts << " attempts left");
			attempts--;

			if (attempts == 0)
			{
				LOG4CXX_TRACE(logger, "LTP CTRL-SED confirmation not received "
						"after checking " << (int)ENIGMA << " times. Giving "
						"up");
			}
		}
		else
		{
			LOG4CXX_TRACE(logger, "LTP CTRL-SED confirmation received for rCID "
					<< rCid.print() << " > SK " << sessionKey);
			attempts = 0;
		}
	}

	_deleteSessionEnd(rCid, sessionKey);
}

void Lightweight::publishFromBuffer(IcnId &cId, IcnId &rCId,
		enigma_t &enigma, sk_t &sessionKey, uint8_t *data,
		uint16_t &dataSize)
{
	_addReverseLookUp(cId, rCId);
	ltp_hdr_data_t ltpHeader;
	// First publish the packet (and send WE)
	ltpHeader.sequenceNumber = _publishData(cId, rCId, enigma, sessionKey,
			data, dataSize);
	// Starting timer in a thread and go back to the ICN handler
	LightweightTimeout ltpTimeout(cId, rCId, enigma, sessionKey,
			ltpHeader.sequenceNumber, _icnCore, _icnCoreMutex, _rtt(),
			&_proxyPacketBuffer, &_proxyPacketBufferMutex,
			&_windowEndedRequests, &_windowEndedRequestsMutex);
	std::thread *ltpTimeoutThread = new std::thread(ltpTimeout);
	ltpTimeoutThread->detach();
	delete ltpTimeoutThread;
}

void Lightweight::publishReset(IcnId &cid, IcnId &rCid)
{
	// First check that there's no other LTP RST session running for this CID >
	// rCID
	_resettedMutex.lock();
	_resettedIt = _resetted.find(rCid.uint());

	if (_resettedIt == _resetted.end())
	{
		_resetted.insert(pair<cid_t, bool>(rCid.uint(), false));
		_resettedMutex.unlock();
		LOG4CXX_TRACE(logger, "New rCID added to LTP RSTED map to await the "
				"sNAP's response before switching");
		_publishReset(cid, rCid);
		// new timeout thread must be created
		LightweightTimeout lightweighttimeout(cid, rCid,
				_icnCore, _icnCoreMutex, _rtt(), (void *)&_resetted,
				&_resettedMutex);
		_resettedThreads.push_back(std::thread(lightweighttimeout));
	}
	else
	{
		LOG4CXX_TRACE(logger, "Another RSTED check thread is running already "
				"for CID " << cid.print() << ". Nothing to do");
	}

	_resettedMutex.unlock();
	// to not flood the local ICN core in case of many active LTP sessions,
	// sleep according to the network latency
	std::this_thread::sleep_for(std::chrono::microseconds(_rtt()));
}

/*void Lightweight::sessionCleanUp(IcnId &cid, IcnId &rCid)
{
	// First clean up reverse CID > rCID mapping
	_cIdReverseLookUpMutex.lock();
	_cIdReverseLookUpIt = _cIdReverseLookUp.find(rCid.uint());

	if (_cIdReverseLookUpIt != _cIdReverseLookUp.end())
	{
		cid = _cIdReverseLookUpIt->second;
		_cIdReverseLookUp.erase(_cIdReverseLookUpIt);
		LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " -> CID "
				<< cid.print() << " reverse lookup erased");
	}
	else
	{
		LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " does not "
				"exist as key in CID look-up table (anymore)");
	}

	_cIdReverseLookUpMutex.unlock();
}*/

tp_states_t Lightweight::handle(IcnId &rCid, uint8_t *packet,
		enigma_t &enigma, sk_t &sessionKey)
{
	tp_states_t ltpState;
	// First get the LTP message type (data || control)
	ltp_hdr_ctrl_t header;
	// [1] message type
	memcpy(&header.messageType, packet, sizeof(header.messageType));

	switch (header.messageType)
	{
	case LTP_DATA:
		_handleData(rCid, packet);
		ltpState = TP_STATE_NO_ACTION_REQUIRED;
		break;
	case LTP_CONTROL:
		ltpState = _handleControl(rCid, packet, enigma, sessionKey);
		break;
	default:
		LOG4CXX_WARN(logger, "Unknown LTP message type " << header.messageType
				<< " received under rCID " << rCid.print());
		ltpState = TP_STATE_NO_ACTION_REQUIRED;
	}

	return ltpState;
}

tp_states_t Lightweight::handle(IcnId &cId, IcnId &rCId, string &nodeIdStr,
		uint8_t *packet, enigma_t &enigma, sk_t &sessionKey)
{
	tp_states_t ltpState;
	NodeId nodeId = nodeIdStr;
	_addNodeId(nodeId);
	// First get the LTP message type (data || control)
	ltp_hdr_ctrl_t ltpHeader;
	// [1] message type
	memcpy(&ltpHeader.messageType, packet, sizeof(ltpHeader.messageType));

	switch (ltpHeader.messageType)
	{
	case LTP_DATA:
		_handleData(cId, rCId, nodeId, packet);
		ltpState = TP_STATE_NO_ACTION_REQUIRED;
		break;
	case LTP_CONTROL:
		ltpState = _handleControl(cId, rCId, nodeId, packet, enigma,
				sessionKey);
		break;
	default:
		ltpState = TP_STATE_NO_ACTION_REQUIRED;
	}

	return ltpState;
}

bool Lightweight::retrieveIcnPacket(IcnId &rCId, enigma_t &enigma,
		string &nodeIdStr, sk_t &sessionKey, uint8_t *packet,
		uint16_t &packetSize)
{
	NodeId nodeId = nodeIdStr;
	// map<SeqNum,PACKET>>>
	map<seq_t, pair<uint8_t*, uint16_t>>::iterator sequenceMapIt;
	// map<SK,    map<SeqNum,   PACKET>>
	map<sk_t, map<seq_t, pair<uint8_t *, uint16_t>>>::iterator
			sessionKeyMapIt;
	// map<NID,map<SK,   map<SeqNum,   PACKET>>>
	map<nid_t, map<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>>::iterator
			nIdMapIt;
	//map<Enigma, map<NID,   map<SK,   map<SeqNum,   PACKET>>>
	map<enigma_t, map<nid_t, map<sk_t, map<seq_t, pair<uint8_t*,
			uint16_t>>>>>::iterator enigmaMapIt;
	packetSize = 0;
	uint16_t offset = 0;
	_icnPacketBufferMutex.lock();
	_icnPacketBufferIt = _icnPacketBuffer.find(rCId.uint());

	if (_icnPacketBufferIt == _icnPacketBuffer.end())
	{
		LOG4CXX_DEBUG(logger, "rCID " << rCId.print() << " does not exist in "
				"ICN packet buffer. Nothing to retrieve");
		_icnPacketBufferMutex.unlock();
		return false;
	}

	enigmaMapIt = _icnPacketBufferIt->second.find(enigma);

	if (enigmaMapIt == _icnPacketBufferIt->second.end())
	{
		LOG4CXX_DEBUG(logger, "enigma " << enigma << " for known rCID "
				<< rCId.print() << " does not exist in ICN packet buffer. "
						"Nothing to retrieve");
		_icnPacketBufferMutex.unlock();
		return false;
	}

	nIdMapIt = enigmaMapIt->second.find(nodeId.uint());

	if (nIdMapIt == enigmaMapIt->second.end())
	{
		LOG4CXX_DEBUG(logger, "NID " << nodeId.str() << " for known enigma "
				<< enigma << " and rCID " << rCId.print() << " does not "
						"exist in ICN packet buffer. Nothing to retrieve");
		_icnPacketBufferMutex.unlock();
		return false;
	}

	sessionKeyMapIt = nIdMapIt->second.find(sessionKey);

	if (sessionKeyMapIt == nIdMapIt->second.end())
	{
		LOG4CXX_DEBUG(logger, "SK " << sessionKey << " for rCID "
				<< rCId.print() << " > enigma " << enigma << " > NID "
				<< nodeId.uint() << " could not be found");
		_icnPacketBufferMutex.unlock();
		return false;
	}

	// now retrieve all fragments and make packet
	packetSize = 0;
	offset = 0;

	for (sequenceMapIt = sessionKeyMapIt->second.begin();
			sequenceMapIt != sessionKeyMapIt->second.end(); sequenceMapIt++)
	{
		packetSize += sequenceMapIt->second.second;
		memcpy(packet + offset, sequenceMapIt->second.first,
				sequenceMapIt->second.second);
		offset += sequenceMapIt->second.second;// add size of current fragment
	}

	LOG4CXX_TRACE(logger, "Packet of length " << packetSize << " retrieved from"
			" ICN packet buffer");
	_icnPacketBufferMutex.unlock();
	_deleteBufferedIcnPacket(rCId, enigma, nodeId, sessionKey);
	return true;
}

void Lightweight::_addNackNodeId(IcnId &rCid,
		ltp_hdr_ctrl_nack_t &ltpHeaderNack, NodeId &nodeId)
{

	_nackGroupsIt = _nackGroups.find(rCid.uint());
	// rCID does not exist yet
	if (_nackGroupsIt == _nackGroups.end())
	{
		// map<Enigma,map<SK,   struct<nackGroup
		map<enigma_t, map<sk_t, nack_group_t>> enigmaMap;
		// map<SK,struct<nackGroup
		map<sk_t, nack_group_t> skMap;
		nack_group_t nackGroup;
		nackGroup.nodeIds.push_back(nodeId);
		nackGroup.startSequence = ltpHeaderNack.start;
		nackGroup.endSequence = ltpHeaderNack.end;
		skMap.insert(pair<sk_t, nack_group_t>(ltpHeaderNack.sessionKey,
				nackGroup));
		enigmaMap.insert(pair<enigma_t, map<sk_t, nack_group_t>>(
				ltpHeaderNack.enigma, skMap));
		_nackGroups.insert(pair<cid_t, map<enigma_t, map<sk_t, nack_group_t>>>
				(rCid.uint(), enigmaMap));
		LOG4CXX_TRACE(logger, "New NACK group created for rCID " << rCid.print()
				<< " > enigma " << ltpHeaderNack.enigma << " > SK "
				<< ltpHeaderNack.sessionKey << " with segment range "
				<< ltpHeaderNack.start << " - " << ltpHeaderNack.end
				<< " and NID " << nodeId.uint());
		return;
	}

	// rCID exists
	map<enigma_t, map<sk_t, nack_group_t>>::iterator enigmaMapIt;
	enigmaMapIt = _nackGroupsIt->second.find(ltpHeaderNack.enigma);

	// enigma does not exist
	if (enigmaMapIt == _nackGroupsIt->second.end())
	{
		map<sk_t, nack_group_t> skMap;
		nack_group_t nackGroup;
		nackGroup.nodeIds.push_back(nodeId);
		nackGroup.startSequence = ltpHeaderNack.start;
		nackGroup.endSequence = ltpHeaderNack.end;
		skMap.insert(pair<sk_t, nack_group_t>(ltpHeaderNack.sessionKey,
				nackGroup));
		_nackGroupsIt->second.insert(pair<enigma_t, map<sk_t, nack_group_t>>
				(ltpHeaderNack.enigma, skMap));
		LOG4CXX_TRACE(logger, "New NACK group created for known rCID "
				<< rCid.print()	<< " but new enigma " << ltpHeaderNack.enigma
				<< " > SK " << ltpHeaderNack.sessionKey << " with segment "
				"range " << ltpHeaderNack.start << " - " << ltpHeaderNack.end
				<< " and NID " << nodeId.uint());
		return;
	}

	// enigma exists
	map<sk_t, nack_group_t>::iterator skMapIt;
	skMapIt = enigmaMapIt->second.find(ltpHeaderNack.sessionKey);

	// SK does not exist
	if (skMapIt == enigmaMapIt->second.end())
	{
		nack_group_t nackGroup;
		nackGroup.nodeIds.push_back(nodeId);
		nackGroup.startSequence = ltpHeaderNack.start;
		enigmaMapIt->second.insert(pair<sk_t, nack_group_t>(
				ltpHeaderNack.sessionKey, nackGroup));
		LOG4CXX_TRACE(logger, "New NACK group created for known rCID "
				<< rCid.print()	<< " > enigma " << ltpHeaderNack.enigma
				<< " but new SK " << ltpHeaderNack.sessionKey << " with segment"
				" range " << ltpHeaderNack.start << " - " << ltpHeaderNack.end
				<< " and NID " << nodeId.uint());
		return;
	}

	// SK exists. Add NID, if it's unknown
	list<NodeId>::iterator nodeIdsIt;
	bool nidFound = false;

	for (nodeIdsIt = skMapIt->second.nodeIds.begin();
			nodeIdsIt != skMapIt->second.nodeIds.end(); nodeIdsIt++)
	{
		if (nodeIdsIt->uint() == nodeId.uint())
		{
			nidFound = true;
			LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " already known "
					"in the NACK group. This NID won't be added then");
			break;
		}
	}

	// add NID if it doesn't exist yet
	if (!nidFound)
	{
		skMapIt->second.nodeIds.push_back(nodeId);
		LOG4CXX_TRACE(logger, "New NID " << nodeId.uint() << " added to rCID "
				<< rCid.print() << " > enigma " << ltpHeaderNack.enigma
				<< " > SK " << ltpHeaderNack.sessionKey);
	}

	// decrease the start segment number if required
	if (ltpHeaderNack.start < skMapIt->second.startSequence)
	{
		skMapIt->second.startSequence = ltpHeaderNack.start;
		LOG4CXX_TRACE(logger, "Start segment decreased to "
				<< skMapIt->second.startSequence << " for rCID " << rCid.print()
				<< " > enigma " << ltpHeaderNack.enigma << " > SK "
				<< ltpHeaderNack.sessionKey);
	}

	// increase the end segment number if required
	if (ltpHeaderNack.end > skMapIt->second.endSequence)
	{
		skMapIt->second.endSequence = ltpHeaderNack.end;
		LOG4CXX_TRACE(logger, "Start segment increased to "
				<< skMapIt->second.endSequence << " for rCID " << rCid.print()
				<< " > enigma " << ltpHeaderNack.enigma << " > SK "
				<< ltpHeaderNack.sessionKey);
	}
}

void Lightweight::_addNodeId(NodeId &nodeId)
{
	_knownNIdsMutex->lock();
	_knownNIdsIt = _knownNIds->find(nodeId.uint());
	if (_knownNIdsIt == _knownNIds->end())
	{
		_knownNIds->insert(pair<uint16_t, bool>(nodeId.uint(), false));
		LOG4CXX_DEBUG(logger, "New NID " << nodeId.uint() << " added to the "
				"list of known remote NAPs (forwarding set to false)");
	}
	_knownNIdsMutex->unlock();
}

void Lightweight::_addNodeIdToWindowUpdate(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey, NodeId &nodeId)
{
	//map<NID, WUD has not been received
	map<nid_t, bool> nidMap;
	map<nid_t, bool>::iterator nidIt;
	map<sk_t, map<nid_t, bool>> skMap;
	map<sk_t, map<nid_t, bool>>::iterator skIt;
	map<enigma_t, map<sk_t, map<nid_t, bool>>> enigmaMap;
	map<enigma_t, map<sk_t, map<nid_t, bool>>>::iterator enigmaIt;
	_windowUpdateMutex.lock();
	_windowUpdateIt = _windowUpdate.find(rCid.uint());

	// rCID not found
	if (_windowUpdateIt == _windowUpdate.end())
	{
		nidMap.insert(pair<nid_t, bool>(nodeId.uint(), false));
		skMap.insert(pair<sk_t, map<nid_t, bool>>(sessionKey, nidMap));
		enigmaMap.insert(pair<enigma_t, map<sk_t, map<nid_t, bool>>>(enigma,
				skMap));
		_windowUpdate.insert(pair<cid_t, map<enigma_t, map<sk_t, map<nid_t,
				bool>>>>(rCid.uint(), enigmaMap));
		LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " added to new rCID "
				<< rCid.print() << " > enigma " << enigma << " > SK "
				<< sessionKey);
		_windowUpdateMutex.unlock();
		return;
	}

	// rCID found
	enigmaIt = _windowUpdateIt->second.find(enigma);

	// enigma does not exist
	if (enigmaIt == _windowUpdateIt->second.end())
	{
		nidMap.insert(pair<nid_t, bool>(nodeId.uint(), false));
		skMap.insert(pair<sk_t, map<nid_t, bool>>(sessionKey, nidMap));
		_windowUpdateIt->second.insert(pair<enigma_t, map<sk_t,
				map<nid_t, bool>>>(enigma, skMap));
		LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " added to existing "
				"rCID "	<< rCid.print() << " but new enigma " << enigma
				<< " > SK " << sessionKey);
		_windowUpdateMutex.unlock();
		return;
	}

	// enigma found
	skIt = enigmaIt->second.find(sessionKey);

	// SK does not exist
	if (skIt == enigmaIt->second.end())
	{
		nidMap.insert(pair<nid_t, bool>(nodeId.uint(), false));
		enigmaIt->second.insert(pair<sk_t, map<nid_t, bool>>(sessionKey, nidMap));
		LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " added to existing "
				"rCID "	<< rCid.print() << " > enigma " << enigma << " but "
				<< "new SK " << sessionKey);
		_windowUpdateMutex.unlock();
		return;
	}

	nidIt = skIt->second.find(nodeId.uint());

	if (nidIt != skIt->second.end())
	{
		_windowUpdateMutex.unlock();
		return;
	}

	skIt->second.insert(pair<nid_t, bool>(nodeId.uint(), false));
	LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " added to existing rCID "
			<< rCid.print() << " > enigma " << enigma << " > SK "
			<< sessionKey << ". " << skIt->second.size() << " NID(s) in this "
			"list");
	_windowUpdateMutex.unlock();
}


void Lightweight::_addReverseLookUp(IcnId &cid, IcnId &rCid)
{
	_cIdReverseLookUpMutex.lock();
	_cIdReverseLookUpIt = _cIdReverseLookUp.find(rCid.uint());

	if (_cIdReverseLookUpIt == _cIdReverseLookUp.end())
	{
		_cIdReverseLookUp.insert(pair<cid_t, IcnId>(rCid.uint(), cid));
		LOG4CXX_TRACE(logger, "Reverse rCID to CID lookup entry created: "
				<< rCid.print() << " -> " << cid.print());
	}

	_cIdReverseLookUpMutex.unlock();
}

void Lightweight::_addSessionEnd(IcnId &rCid, sk_t &sessionKey)
{
	unordered_map<sk_t, bool>::iterator skIt;
	unordered_map<cid_t, unordered_map<sk_t, bool>>::iterator cidIt;
	_sessionEndedResponsesUnicastMutex.lock();
	cidIt = _sessionEndedResponsesUnicast.find(rCid.uint());

	// New CID and SK
	if (cidIt == _sessionEndedResponsesUnicast.end())
	{
		unordered_map<sk_t, bool> skMap;
		skMap.insert(pair<sk_t, bool>(sessionKey, false));
		_sessionEndedResponsesUnicast.insert(pair<cid_t, unordered_map<sk_t,
				bool>>(rCid.uint(), skMap));
		LOG4CXX_TRACE(logger, "New LTP CTRL-SED boolean added for new rCID "
				<< rCid.print() << " > SK " << sessionKey);
		_sessionEndedResponsesUnicastMutex.unlock();
		return;
	}

	skIt = cidIt->second.find(sessionKey);

	// New SK under known rCID
	if (skIt == cidIt->second.end())
	{
		cidIt->second.insert(pair<sk_t, bool>(sessionKey, false));
		LOG4CXX_TRACE(logger, "New LTP CTRL-SED boolean added for known rCID "
				<< rCid.print() << " but new SK " << sessionKey);
		_sessionEndedResponsesUnicastMutex.unlock();
		return;
	}

	skIt->second = false;
	LOG4CXX_TRACE(logger, "LTP CTRL-SED boolean reset to false for known rCID "
			<< rCid.print() << " > SK " << sessionKey << ". A bit odd though "
			"that rCID > SK is already there");
	_sessionEndedResponsesUnicastMutex.unlock();
}

void Lightweight::_addSessionEnd(IcnId &rCid, ltp_hdr_ctrl_se_t &ltpHdrCtrlSe,
			list<NodeId> nodeIds)
{
	// 		u_map<SK, SED received
	unordered_map<sk_t, bool>::iterator skIt;
	unordered_map<nid_t, unordered_map<sk_t, bool>>::iterator nidIt;
	unordered_map<enigma_t, unordered_map<nid_t, unordered_map<sk_t,
			bool>>>::iterator enigmaIt;
	_sessionEndedResponsesMutex.lock();
	_sessionEndedResponsesIt = _sessionEndedResponses.find(rCid.uint());

	// rCID does not exist
	if (_sessionEndedResponsesIt == _sessionEndedResponses.end())
	{
		unordered_map<sk_t, bool> skMap;
		unordered_map<nid_t, unordered_map<sk_t, bool>> nidMap;
		unordered_map<enigma_t, unordered_map<nid_t, unordered_map<sk_t,
				bool>>> enigmaMap;
		skMap.insert(pair<sk_t, bool>(ltpHdrCtrlSe.sessionKey, false));

		for (list<NodeId>::iterator it = nodeIds.begin(); it != nodeIds.end();
				it++)
		{
			nidMap.insert(pair<nid_t, unordered_map<sk_t, bool>>
					(it->uint(), skMap));
		}

		enigmaMap.insert(pair<enigma_t, unordered_map<nid_t,
				unordered_map<sk_t, bool>>>(ltpHdrCtrlSe.enigma,
						nidMap));
		_sessionEndedResponses.insert(pair<cid_t, unordered_map<enigma_t,
				unordered_map<nid_t, unordered_map<sk_t, bool>>>>
				(rCid.uint(), enigmaMap));
		LOG4CXX_TRACE(logger, "New CTRL-SED boolean added for rCid "
				<< rCid.print() << " > enigma " << ltpHdrCtrlSe.enigma
				<< " > " << nodeIds.size() << " NID(s) > SK "
				<< ltpHdrCtrlSe.sessionKey);
		_sessionEndedResponsesMutex.unlock();
		return;
	}

	enigmaIt = _sessionEndedResponsesIt->second.find(ltpHdrCtrlSe.enigma);

	// enigma does not exist
	if (enigmaIt == _sessionEndedResponsesIt->second.end())
	{
		unordered_map<sk_t, bool> skMap;
		unordered_map<nid_t, unordered_map<sk_t, bool>> nidMap;
		skMap.insert(pair<sk_t, bool>(ltpHdrCtrlSe.sessionKey, false));

		for (list<NodeId>::iterator it = nodeIds.begin(); it != nodeIds.end();
				it++)
		{
			nidMap.insert(pair<nid_t, unordered_map<sk_t, bool>>
					(it->uint(), skMap));
		}

		_sessionEndedResponsesIt->second.insert(pair<enigma_t,
				unordered_map<nid_t, unordered_map<sk_t, bool>>>
				(ltpHdrCtrlSe.enigma, nidMap));
		LOG4CXX_TRACE(logger, "New CTRL-SED boolean added for known rCid "
				<< rCid.print() << " but new enigma " << ltpHdrCtrlSe.enigma
				<< " > " << nodeIds.size() << " NID(s) > SK "
				<< ltpHdrCtrlSe.sessionKey);
		_sessionEndedResponsesMutex.unlock();
		return;
	}

	// iterate over list of NIDs and set boolean to false
	for (list<NodeId>::iterator it = nodeIds.begin(); it != nodeIds.end(); it++)
	{
		nidIt = enigmaIt->second.find(it->uint());

		// NID does not exist
		if (nidIt == enigmaIt->second.end())
		{
			unordered_map<sk_t, bool> skMap;
			skMap.insert(pair<sk_t, bool>(ltpHdrCtrlSe.sessionKey, false));
			enigmaIt->second.insert(pair<nid_t, unordered_map<sk_t, bool>>
					(it->uint(), skMap));
			LOG4CXX_TRACE(logger, "New CTRL-SED boolean added for known rCid "
					<< rCid.print() << " > enigma " << ltpHdrCtrlSe.enigma
					<< " but new " << nodeIds.size() << " NID(s) > SK "
					<< ltpHdrCtrlSe.sessionKey);
		}
		// NID exists
		else
		{
			skIt = nidIt->second.find(ltpHdrCtrlSe.sessionKey);

			// SK does not exist
			if (skIt == nidIt->second.end())
			{
				nidIt->second.insert(pair<sk_t, bool>(
						ltpHdrCtrlSe.sessionKey, false));
				LOG4CXX_TRACE(logger, "New CTRL-SED boolean added for known"
						"rCid " << rCid.print() << " > enigma "
						<< ltpHdrCtrlSe.enigma << " > NID " << it->uint()
						<< " but new SK " << ltpHdrCtrlSe.sessionKey);
			}
			// SK exists
			else
			{
				skIt->second = false;
				LOG4CXX_TRACE(logger, "CTRL-SED boolean reset to false for"
						"known rCid " << rCid.print() << " > enigma "
						<< ltpHdrCtrlSe.enigma << " > NID " << it->uint()
						<< " > SK " << ltpHdrCtrlSe.sessionKey);
			}
		}
	}

	_sessionEndedResponsesMutex.unlock();
}

void Lightweight::_addWindowEnd(IcnId &rCid, ltp_hdr_ctrl_we_t &ltpHeaderCtrlWe)
{
	// Inform endpoint that all data has been sent
	_windowEndedRequestsMutex.lock();
	_windowEndedRequestsIt = _windowEndedRequests.find(rCid.uint());

	// rCID does not exist in WED map
	if (_windowEndedRequestsIt == _windowEndedRequests.end())
	{
		map<sk_t, bool> sessionKeyMap;
		map<enigma_t, map<sk_t, bool>> enigmaMap;
		sessionKeyMap.insert(pair<sk_t, bool>(ltpHeaderCtrlWe.sessionKey,
				false));
		enigmaMap.insert(pair<enigma_t, map<sk_t, bool>>(
				ltpHeaderCtrlWe.enigma, sessionKeyMap));
		_windowEndedRequests.insert(pair<cid_t, map<enigma_t, map<sk_t,
				bool>>>(rCid.uint(), enigmaMap));
		LOG4CXX_TRACE(logger, "New CTRL-WED boolean added for rCID "
				<< rCid.print()	<< " > enigma " << ltpHeaderCtrlWe.enigma
				<< " > SK " << ltpHeaderCtrlWe.sessionKey);
	}
	// rCID exists
	else
	{
		map<enigma_t, map<sk_t, bool>>::iterator enigmaMapIt;
		enigmaMapIt = _windowEndedRequestsIt->second.find(
				ltpHeaderCtrlWe.enigma);

		// enigma does not exist
		if (enigmaMapIt == _windowEndedRequestsIt->second.end())
		{
			map<sk_t, bool> sessionKeyMap;
			sessionKeyMap.insert(pair<sk_t, bool>(
					ltpHeaderCtrlWe.sessionKey, false));
			_windowEndedRequestsIt->second.insert(pair<enigma_t, map<sk_t,
					bool>>(ltpHeaderCtrlWe.enigma, sessionKeyMap));
			LOG4CXX_TRACE(logger, "New CTRL-WED boolean added for existing "
					"rCID " << rCid.print() << " but new enigma "
					<< ltpHeaderCtrlWe.enigma << " > SK "
					<< ltpHeaderCtrlWe.sessionKey);
		}
		// enigma exists. Simply set it to false (even though this should never be
		// the case
		else
		{
			map<sk_t, bool>::iterator sessionKeyMapIt;
			sessionKeyMapIt = enigmaMapIt->second.find(
					ltpHeaderCtrlWe.sessionKey);

			// Session key doesn't exist
			if (sessionKeyMapIt == enigmaMapIt->second.end())
			{
				enigmaMapIt->second.insert(pair<sk_t, bool>(
						ltpHeaderCtrlWe.sessionKey,	false));
				LOG4CXX_TRACE(logger, "New CTRL-WED boolean added for existing "
						"rCID "	<< rCid.print() << " > enigma "
						<< ltpHeaderCtrlWe.enigma << " but new session "
								"key " << ltpHeaderCtrlWe.sessionKey);
			}
			sessionKeyMapIt->second = false;
			LOG4CXX_TRACE(logger, "CTRL-WED boolean reset to false for rCID "
					<< rCid.print() << " > enigma " << ltpHeaderCtrlWe.enigma
					<< " > SK " << ltpHeaderCtrlWe.sessionKey);
		}
	}

	_windowEndedRequestsMutex.unlock();
}

void Lightweight::_addWindowEnd(IcnId &rCid, list<NodeId> &nodeIds,
		ltp_hdr_ctrl_we_t &ltpHeaderCtrlWe)
{
	ostringstream oss;
	// Inform endpoint that all data has been sent
	_windowEndedResponsesMutex.lock();
	_windowEndedResponsesIt = _windowEndedResponses.find(rCid.uint());

	// rCID does not exist in WED map
	if (_windowEndedResponsesIt == _windowEndedResponses.end())
	{
		map<sk_t, bool> sessionKeyMap;
		map<nid_t, map<sk_t, bool>> nidMap;
		list<NodeId>::iterator nidsIt;
		map<enigma_t, map<nid_t, map<sk_t, bool>>> enigmaMap;
		sessionKeyMap.insert(pair<sk_t, bool>(ltpHeaderCtrlWe.sessionKey,
				false));
		for (nidsIt = nodeIds.begin(); nidsIt != nodeIds.end(); nidsIt++)
		{
			nidMap.insert(pair<nid_t, map<sk_t, bool>>(
					nidsIt->uint(), sessionKeyMap));
			oss << nidsIt->uint() << " ";
		}
		enigmaMap.insert(pair<enigma_t, map<nid_t, map<sk_t, bool>>>(
				ltpHeaderCtrlWe.enigma, nidMap));
		_windowEndedResponses.insert(pair<cid_t, map<enigma_t, map<nid_t,
				map<sk_t, bool>>>>(rCid.uint(), enigmaMap));
		LOG4CXX_TRACE(logger, "New CTRL-WED boolean added for rCID "
				<< rCid.print()	<< " > enigma " << ltpHeaderCtrlWe.enigma
				<< " > NIDs " << oss.str() << "> SK "
				<< ltpHeaderCtrlWe.sessionKey);
		_windowEndedResponsesMutex.unlock();
		return;
	}

	// rCID exists
	map<enigma_t, map<nid_t, map<sk_t, bool>>>::iterator enigmaMapIt;
	enigmaMapIt = _windowEndedResponsesIt->second.find(
			ltpHeaderCtrlWe.enigma);

	// enigma does not exist
	if (enigmaMapIt == _windowEndedResponsesIt->second.end())
	{
		map<sk_t, bool> sessionKeyMap;
		map<nid_t, map<sk_t, bool>> nidMap;
		list<NodeId>::iterator nidsIt;
		map<enigma_t, map<nid_t, map<sk_t, bool>>> enigmaMap;
		sessionKeyMap.insert(pair<sk_t, bool>(ltpHeaderCtrlWe.sessionKey,
				false));
		for (nidsIt = nodeIds.begin(); nidsIt != nodeIds.end(); nidsIt++)
		{
			nidMap.insert(pair<nid_t, map<sk_t, bool>>(
					nidsIt->uint(), sessionKeyMap));
			oss << nidsIt->uint() << " ";
		}

		_windowEndedResponsesIt->second.insert(pair<enigma_t, map<nid_t,
				map<uint16_t, bool>>>(ltpHeaderCtrlWe.enigma, nidMap));
		LOG4CXX_TRACE(logger, "New CTRL-WED boolean added for existing "
				"rCID " << rCid.print() << " but new enigma "
				<< ltpHeaderCtrlWe.enigma << " > NIDs " << oss.str()
				<< " > SK " << ltpHeaderCtrlWe.sessionKey);
		_windowEndedResponsesMutex.unlock();
		return;
	}

	// enigma exists
	map<nid_t, map<sk_t, bool>>::iterator nidMapIt;
	list<NodeId>::iterator nidsIt;

	for (nidsIt = nodeIds.begin(); nidsIt != nodeIds.end(); nidsIt++)
	{
		nidMapIt = enigmaMapIt->second.find(nidsIt->uint());

		// NID does not exist
		if (nidMapIt == enigmaMapIt->second.end())
		{	//map<SK
			map<sk_t, bool> sessionKeyMap;
			sessionKeyMap.insert(pair<sk_t, bool>(
					ltpHeaderCtrlWe.sessionKey, false));
			enigmaMapIt->second.insert(pair<nid_t, map<sk_t, bool>>(
					nidsIt->uint(), sessionKeyMap));
			LOG4CXX_TRACE(logger, "New CTRL-WED boolean added for existing "
					"rCID " << rCid.print() << " > enigma "
					<< ltpHeaderCtrlWe.enigma << " but new NID "
					<< nidsIt->uint() << " > SK "
					<< ltpHeaderCtrlWe.sessionKey);
		}
		//NID does exist
		else
		{	//map<SK
			map<sk_t, bool>::iterator sessionKeyMapIt;
			sessionKeyMapIt = nidMapIt->second.find(ltpHeaderCtrlWe.sessionKey);

			// Session key does not exist
			if (sessionKeyMapIt == nidMapIt->second.end())
			{
				nidMapIt->second.insert(pair<sk_t, bool>(
						ltpHeaderCtrlWe.sessionKey, false));
				LOG4CXX_TRACE(logger, "New CTRL-WED boolean added for existing "
						"rCID " << rCid.print() << " > enigma "
						<< ltpHeaderCtrlWe.enigma << " > NID "
						<< nidsIt->uint() << " but new SK "
						<< ltpHeaderCtrlWe.sessionKey);
			}
			// Session key does exist. Simply reset it to false
			else
			{
				sessionKeyMapIt->second = false;
				LOG4CXX_TRACE(logger, "Existing CTRL-WED boolean reset to "
						"false for existing rCID " << rCid.print() << " > enigma "
						<< ltpHeaderCtrlWe.enigma << " > NID "
						<< nidsIt->uint() << " > SK "
						<< ltpHeaderCtrlWe.sessionKey);
			}
		}
	}

	_windowEndedResponsesMutex.unlock();
}

void Lightweight::_bufferIcnPacket(IcnId &rCId, NodeId &nodeId,
		ltp_hdr_data_t &ltpHeader, uint8_t *packet)
{
	//         Packet
	map<seq_t, pair<uint8_t*, uint16_t>> sequenceMap;
	map<seq_t, pair<uint8_t*, uint16_t>>::iterator sequenceMapIt;
	map<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>> sessionKeyMap;
	map<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>::iterator
			sessionKeyMapIt;
	map<nid_t, map<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>> nodeIdMap;
	map<nid_t, map<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>>::iterator
			nodeIdMapIt;
	map<enigma_t, map<nid_t, map<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>>>
			enigmaMap;
	map<enigma_t, map<nid_t, map<sk_t, map<seq_t, pair<uint8_t*,
			uint16_t>>>>>::iterator enigmaMapIt;
	pair<uint8_t *, uint16_t> packetDescriptor;
	packetDescriptor.first = (uint8_t *)malloc(ltpHeader.payloadLength);

	if (packetDescriptor.first == NULL)
	{
		LOG4CXX_ERROR(logger, "Allocation of " << ltpHeader.payloadLength
				<< " bytes failed to buffer received ICN packet")
	}
	else
	{
		memcpy(packetDescriptor.first, packet, ltpHeader.payloadLength);
		packetDescriptor.second = ltpHeader.payloadLength;
	}

	_icnPacketBufferMutex.lock();
	_icnPacketBufferIt = _icnPacketBuffer.find(rCId.uint());

	// First packet received for rCId
	if (_icnPacketBufferIt == _icnPacketBuffer.end())
	{
		sequenceMap.insert(pair<seq_t, pair<uint8_t*, uint16_t>>(
				ltpHeader.sequenceNumber, packetDescriptor));
		sessionKeyMap.insert(pair<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>
				(ltpHeader.sessionKey, sequenceMap));
		nodeIdMap.insert(pair<nid_t, map<sk_t, map<seq_t, pair<uint8_t*,
				uint16_t>>>>(nodeId.uint(), sessionKeyMap));
		enigmaMap.insert(pair<enigma_t, map<nid_t, map<sk_t, map<seq_t,
				pair<uint8_t*, uint16_t>>>>>(ltpHeader.enigma, nodeIdMap));
		_icnPacketBuffer.insert(pair<cid_t, map<enigma_t, map<nid_t, map<sk_t,
				map<seq_t, pair<uint8_t*, uint16_t>>>>>>(rCId.uint(), enigmaMap));
		LOG4CXX_TRACE(logger, "Packet of length " << ltpHeader.payloadLength
				<< " for new rCID " << rCId.print() << " added to LTP packet "
				"buffer for received ICN packets (enigma "
				<< ltpHeader.enigma  << " > NID " << nodeId.uint()
				<< " > SK " << ltpHeader.sessionKey << " > Sequence "
				<< ltpHeader.sequenceNumber << ")");
		_icnPacketBufferMutex.unlock();
		return;
	}

	// rCId is known
	enigmaMapIt = _icnPacketBufferIt->second.find(ltpHeader.enigma);

	// enigma entry does not exist. Create it
	if (enigmaMapIt == _icnPacketBufferIt->second.end())
	{
		sequenceMap.insert(pair<seq_t, pair<uint8_t*, uint16_t>>(
				ltpHeader.sequenceNumber, packetDescriptor));
		sessionKeyMap.insert(pair<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>
				(ltpHeader.sessionKey, sequenceMap));
		nodeIdMap.insert(pair<nid_t, map<sk_t, map<seq_t, pair<uint8_t*,
				uint16_t>>>> (nodeId.uint(), sessionKeyMap));
		_icnPacketBufferIt->second.insert(pair<enigma_t, map<nid_t, map<sk_t,
				map<seq_t, pair<uint8_t*, uint16_t>>>>>(ltpHeader.enigma,
						nodeIdMap));
		LOG4CXX_TRACE(logger, "Packet of length " << ltpHeader.payloadLength
				<< " for existing rCID " << rCId.print() << " but new enigma "
				<< ltpHeader.enigma << " added to ICN buffer (NID "
				<< nodeId.uint() << " > SK "<< ltpHeader.sessionKey
				<< " > Sequence " << ltpHeader.sequenceNumber << ")");
		_icnPacketBufferMutex.unlock();
		return;
	}

	// enigma does exist
	nodeIdMapIt = enigmaMapIt->second.find(nodeId.uint());

	// This NID does not exist
	if (nodeIdMapIt == enigmaMapIt->second.end())
	{
		sequenceMap.insert(pair<seq_t, pair<uint8_t*, uint16_t>>(
				ltpHeader.sequenceNumber, packetDescriptor));
		sessionKeyMap.insert(pair<sk_t, map<seq_t, pair<uint8_t*,
				uint16_t>>>(ltpHeader.sessionKey, sequenceMap));
		// map<NID, sequenceMap>
		enigmaMapIt->second.insert(pair<nid_t, map<sk_t, map<seq_t, pair<uint8_t*,
				uint16_t>>>>(nodeId.uint(),	sessionKeyMap));
		LOG4CXX_TRACE(logger, "Packet of length "
				<< ltpHeader.payloadLength << " for existing rCID "
				<< rCId.print() << " > enigma " << ltpHeader.enigma
				<< " but new NID " << nodeId.uint() << " > SK "
				<< ltpHeader.sessionKey << " > Sequence "
				<< ltpHeader.sequenceNumber << " added to ICN buffer");
		_icnPacketBufferMutex.unlock();
		return;
	}

	// NID exists
	sessionKeyMapIt = nodeIdMapIt->second.find(ltpHeader.sessionKey);

	// Session key does not exist
	if (sessionKeyMapIt == nodeIdMapIt->second.end())
	{
		sequenceMap.insert(pair<seq_t, pair<uint8_t*, uint16_t>>(
				ltpHeader.sequenceNumber, packetDescriptor));
		nodeIdMapIt->second.insert(pair<sk_t, map<seq_t, pair<uint8_t*,
				uint16_t>>>(ltpHeader.sessionKey, sequenceMap));
		LOG4CXX_TRACE(logger, "Packet of length " << ltpHeader.payloadLength
				<< " for existing rCID " << rCId.print() << " > enigma "
				<< ltpHeader.enigma << " > NID " << nodeId.uint()
				<< " but new SK " << ltpHeader.sessionKey	<< " > "
				"Sequence " << ltpHeader.sequenceNumber << " added to ICN "
				"packet buffer");
		_icnPacketBufferMutex.unlock();
		return;
	}

	// Session key does exist
	sequenceMapIt = sessionKeyMapIt->second.find(ltpHeader.sequenceNumber);

	// Session key exists -> check packet length is identical and leave
	if (sequenceMapIt != sessionKeyMapIt->second.end())
	{
		// check if length is identical
		if (sequenceMapIt->second.second == ltpHeader.payloadLength)
		{
			LOG4CXX_TRACE(logger, "Sequence " << ltpHeader.sequenceNumber
					<< " already exists for rCID " << rCId.print() << " > enigma "
					<< ltpHeader.enigma << " > NID " << nodeId.uint()
					<< " > Session " << ltpHeader.sessionKey);
			_icnPacketBufferMutex.unlock();
			return;
		}
		else
		{// not identical -> delete old packet and continue
			LOG4CXX_TRACE(logger, "Sequence " << ltpHeader.sequenceNumber
					<< " already exists for rCID " << rCId.print() << " > enigma "
					<< ltpHeader.enigma << " > NID " << nodeId.uint()
					<< " > Session " << ltpHeader.sessionKey << ". But packet "
					"size is different ... overwriting packet");
			free(sequenceMapIt->second.first);
			sessionKeyMapIt->second.erase(sequenceMapIt);
		}
	}

	// Adding new packet
	sessionKeyMapIt->second.insert(pair<seq_t, pair<uint8_t*,uint16_t>>
			(ltpHeader.sequenceNumber, packetDescriptor));
	_icnPacketBufferMutex.unlock();
	LOG4CXX_TRACE(logger, "Packet of length " << ltpHeader.payloadLength
			<< " and Sequence " << ltpHeader.sequenceNumber << " added to ICN "
			"buffer for existing rCID " << rCId.print() << " > enigma "
			<< ltpHeader.enigma << ", NID " << nodeId.uint() << " Session "
			"key " << ltpHeader.sessionKey);
}

void Lightweight::_bufferLtpPacket(IcnId &rCid, ltp_hdr_data_t &ltpHeaderData,
		uint8_t *packet, uint16_t packetSize)
{
	map<cid_t, map<enigma_t, map<sk_t, map<seq_t, packet_t>>>>::iterator rCidIt;
	map<enigma_t, map<sk_t, map<seq_t, packet_t>>>::iterator enigmaIt;
	map<sk_t, map<seq_t, packet_t>>::iterator skIt;
	map<seq_t, packet_t>::iterator snIt;
	packet_t packetDescription;
	// Note, ltpHeaderData.payloadLength only has the HTTP msg. paketSize is
	// padded to be a multiple of 4!
	packetDescription.packet = (uint8_t *)malloc(packetSize);
	memcpy(packetDescription.packet, packet, packetSize);
	packetDescription.packetSize = packetSize;
	packetDescription.timestamp =
			boost::posix_time::microsec_clock::local_time();
	_ltpPacketBufferMutex.lock();
	rCidIt = _ltpPacketBuffer.find(rCid.uint());

	// rCID not found
	if (rCidIt == _ltpPacketBuffer.end())
	{
		map<seq_t, packet_t> snMap;
		snMap.insert(pair<seq_t, packet_t>(ltpHeaderData.sequenceNumber,
				packetDescription));
		map<sk_t, map<seq_t, packet_t>> skMap;
		skMap.insert(pair<sk_t, map<seq_t, packet_t>>(ltpHeaderData.sessionKey,
				snMap));
		map<enigma_t, map<sk_t, map<seq_t, packet_t>>> enigmaMap;
		enigmaMap.insert(pair<enigma_t, map<sk_t, map<seq_t, packet_t>>>
				(ltpHeaderData.enigma, skMap));
		_ltpPacketBuffer.insert(pair<cid_t, map<enigma_t, map<sk_t, map<seq_t,
				packet_t>>>>(rCid.uint(), enigmaMap));
		_ltpPacketBufferMutex.unlock();
		LOG4CXX_TRACE(logger, "LTP packet buffered of length "
				<< ltpHeaderData.payloadLength << " and Sequence "
				<< ltpHeaderData.sequenceNumber << " under new rCID "
				<< rCid.print() << " > enigma " << ltpHeaderData.enigma
				<< " > SK " << ltpHeaderData.sessionKey);
		return;
	}

	enigmaIt = rCidIt->second.find(ltpHeaderData.enigma);

	// enigma not found
	if (enigmaIt == rCidIt->second.end())
	{
		map<seq_t, packet_t> snMap;
		snMap.insert(pair<seq_t, packet_t>(ltpHeaderData.sequenceNumber,
				packetDescription));
		map<sk_t, map<seq_t, packet_t>> skMap;
		skMap.insert(pair<sk_t, map<seq_t, packet_t>>(ltpHeaderData.sessionKey,
				snMap));
		rCidIt->second.insert(pair<enigma_t, map<sk_t, map<seq_t, packet_t>>>
				(ltpHeaderData.enigma, skMap));
		_ltpPacketBufferMutex.unlock();
		LOG4CXX_TRACE(logger, "LTP packet buffered of length "
				<< ltpHeaderData.payloadLength << " and Sequence "
				<< ltpHeaderData.sequenceNumber << " under known rCID "
				<< rCid.print() << " but new enigma " << ltpHeaderData.enigma
				<< " > SK " << ltpHeaderData.sessionKey);
		return;
	}

	skIt = enigmaIt->second.find(ltpHeaderData.sessionKey);

	// SK not found
	if (skIt == enigmaIt->second.end())
	{
		// Sequence
		map<seq_t, packet_t> snMap;
		snMap.insert(pair<seq_t, packet_t>(ltpHeaderData.sequenceNumber,
				packetDescription));
		enigmaIt->second.insert(pair<sk_t, map<seq_t, packet_t>>
				(ltpHeaderData.sessionKey, snMap));
		_ltpPacketBufferMutex.unlock();
		LOG4CXX_TRACE(logger, "LTP packet buffred of length "
				<< ltpHeaderData.payloadLength << " and Sequence "
				<< ltpHeaderData.sequenceNumber << " under known rCID "
				<< rCid.print() << " > enigma " << ltpHeaderData.enigma
				<< " but new SK " << ltpHeaderData.sessionKey);
		return;
	}

	snIt = skIt->second.find(ltpHeaderData.sequenceNumber);

	if (snIt != skIt->second.end())
	{
		_ltpPacketBufferMutex.unlock();
		LOG4CXX_WARN(logger, "Sequence number " << ltpHeaderData.sequenceNumber
				<< " already exists in LTP buffer for rCID " << rCid.print()
				<< " > enigma " << ltpHeaderData.enigma << " > SK "
				<< ltpHeaderData.sessionKey);
		return;
	}

	skIt->second.insert(pair<seq_t, packet_t>(ltpHeaderData.sequenceNumber,
			packetDescription));
	_ltpPacketBufferMutex.unlock();
	LOG4CXX_TRACE(logger, "LTP packet buffered of length "
			<< ltpHeaderData.payloadLength << " and Sequence "
			<< ltpHeaderData.sequenceNumber << " under known rCID "
			<< rCid.print() << " > enigma " << ltpHeaderData.enigma
			<< " > SK " << ltpHeaderData.sessionKey);

}

void Lightweight::_bufferProxyPacket(IcnId &cId, IcnId &rCId,
		ltp_hdr_data_t &ltpHeaderData, uint8_t *packet,
		uint16_t &packetSize)
{
	pair<uint8_t *, uint16_t> packetDescriptor;
	_proxyPacketBufferMutex.lock();
	_proxyPacketBufferIt = _proxyPacketBuffer.find(rCId.uint());

	// rCID does not exist, create new entry
	if (_proxyPacketBufferIt == _proxyPacketBuffer.end())
	{
		/* First create the map with the sequence number as the key and the
		 * packet (pointer/size) as the map's value
		 */
		map<seq_t, pair<uint8_t *, uint16_t>> sequenceMap;
		map<sk_t, map<seq_t, pair<uint8_t *, uint16_t>>> sessionKeyMap;
		map<enigma_t, map<sk_t, map<seq_t, pair<uint8_t *, uint16_t>>>> enigmaMap;
		packetDescriptor.first = (uint8_t *)malloc(packetSize);
		memcpy(packetDescriptor.first, packet, packetSize);
		packetDescriptor.second = packetSize;
		sequenceMap.insert(pair<uint16_t, pair<uint8_t *, uint16_t>>(
				ltpHeaderData.sequenceNumber, packetDescriptor));
		// Secondly, create a new session key map
		sessionKeyMap.insert(pair<sk_t, map<seq_t, pair<uint8_t *, uint16_t>>>
				(ltpHeaderData.sessionKey, sequenceMap));
		// Thirdly, create a new enigma map with the session key Map above as its
		// value
		enigmaMap.insert(pair<enigma_t, map<sk_t, map<seq_t, pair<uint8_t *,
				uint16_t>>>>(ltpHeaderData.enigma,	sessionKeyMap));
		/* Fourthly, create new rCID key entry in _proxyPacketBuffer map with
		 * the enigmaMap as its value
		 */
		_proxyPacketBuffer.insert(pair<cid_t, map<enigma_t, map<sk_t, map<seq_t,
				pair<uint8_t *, uint16_t>>>>>(rCId.uint(), enigmaMap));
		LOG4CXX_TRACE(logger, "Proxy packet buffered of length " << packetSize
				<< " and CID" << cId.print() << " ("	<< cId.printFqdn()
				<< ") for new rCID " << rCId.print() << " > enigma "
				<< ltpHeaderData.enigma << " > Session "
				<< ltpHeaderData.sessionKey << " > Sequence "
				<< ltpHeaderData.sequenceNumber);
		_proxyPacketBufferMutex.unlock();
		return;
	}

	// rCID exists
	map<enigma_t, map<sk_t, map<seq_t, pair<uint8_t *, uint16_t>>>>::iterator enigmaIt;
	enigmaIt = _proxyPacketBufferIt->second.find(ltpHeaderData.enigma);

	// enigma does not exist
	if (enigmaIt == _proxyPacketBufferIt->second.end())
	{
		map<seq_t, pair<uint8_t *, uint16_t>> sequenceMap;
		map<sk_t, map<seq_t, pair<uint8_t *, uint16_t>>> sessionKeyMap;
		packetDescriptor.first = (uint8_t *)malloc(packetSize);
		memcpy(packetDescriptor.first, packet, packetSize);
		packetDescriptor.second = packetSize;
		sequenceMap.insert(pair<seq_t, pair<uint8_t*, uint16_t>>(
				ltpHeaderData.sequenceNumber, packetDescriptor));
		sessionKeyMap.insert(pair<sk_t, map<seq_t, pair<uint8_t *, uint16_t>>>
				(ltpHeaderData.sessionKey, sequenceMap));
		_proxyPacketBufferIt->second.insert(pair<enigma_t, map<sk_t, map<seq_t,
				pair<uint8_t *, uint16_t>>>>(ltpHeaderData.enigma,
						sessionKeyMap));
		LOG4CXX_TRACE(logger, "Proxy packet buffered of length " << packetSize
				<< " and CID " << cId.print() << " ("	<< cId.printFqdn()
				<< ") for existing rCID " << rCId.print()
				<< " but new enigma " << ltpHeaderData.enigma << " > SK "
				<< ltpHeaderData.sessionKey << " > SN "
				<< ltpHeaderData.sequenceNumber);
		_proxyPacketBufferMutex.unlock();
		return;
	}

	// enigma exists
	map<sk_t, map<seq_t, pair<uint8_t *, uint16_t>>>::iterator skIt;
	skIt = enigmaIt->second.find(ltpHeaderData.sessionKey);

	// SK does not exist
	if (skIt == enigmaIt->second.end())
	{
		map<seq_t, pair<uint8_t *, uint16_t>> sequenceMap;
		packetDescriptor.first = (uint8_t *)malloc(packetSize);
		memcpy(packetDescriptor.first, packet, packetSize);
		packetDescriptor.second = packetSize;
		sequenceMap.insert(pair<seq_t, pair<uint8_t*, uint16_t>>(
				ltpHeaderData.sequenceNumber, packetDescriptor));
		enigmaIt->second.insert(pair<sk_t, map<uint16_t, pair<uint8_t *,
				uint16_t>>>(ltpHeaderData.sessionKey, sequenceMap));
		LOG4CXX_TRACE(logger, "Proxy packet buffer of length " << packetSize
				<< " and CID " << cId.print() << " ("	<< cId.printFqdn()
				<< ") for existing rCID " << rCId.print()
				<< " > enigma " << ltpHeaderData.enigma << " but new SK "
				<< ltpHeaderData.sessionKey << " > SN "
				<< ltpHeaderData.sequenceNumber);
		_proxyPacketBufferMutex.unlock();
		return;
	}

	// SK exists
	map<seq_t, pair<uint8_t *,	uint16_t>>::iterator snIt;
	snIt = skIt->second.find(ltpHeaderData.sequenceNumber);

	// Sequence also exists. Update it (DNSlocal scenario)
	if (snIt != skIt->second.end())
	{
		LOG4CXX_TRACE(logger, "Sequence " << ltpHeaderData.sequenceNumber
				<< " already exists in proxy packet buffer for rCID "
				<< rCId.print() << " > enigma " << ltpHeaderData.enigma
				<< " > SK "	<< ltpHeaderData.sessionKey << ". Overwriting it");

		if (snIt->second.second != packetSize)
		{
			snIt->second.first = (uint8_t *)realloc(snIt->second.first,
					packetSize);
			// reallocation of memory failed
			if (snIt->second.first == NULL)
			{
				LOG4CXX_WARN(logger, "Memory could not be reallocated in LTP "
						"proxy packet buffer for rCID " << rCId.print() << " > "
						"enigma " << ltpHeaderData.enigma << " > SK "
						<< ltpHeaderData.sessionKey);
				_proxyPacketBufferMutex.unlock();
				return;
			}
		}

		memcpy(snIt->second.first, packet, packetSize);
		snIt->second.second = packetSize;
		_proxyPacketBufferMutex.unlock();
		return;
	}

	packetDescriptor.first = (uint8_t *)malloc(packetSize);
	memcpy(packetDescriptor.first, packet, packetSize);
	packetDescriptor.second = packetSize;
	skIt->second.insert(pair<uint16_t, pair<uint8_t *, uint16_t>>(
			ltpHeaderData.sequenceNumber, packetDescriptor));
	LOG4CXX_TRACE(logger, "proxy packet bufferd with SN "
			<< ltpHeaderData.sequenceNumber << ", length " << packetSize
			<< " and CID " << cId.print() << " (" << cId.printFqdn()
			<< ") added to LTP proxy packet buffer for existing rCID "
			<< rCId.print() << " > enigma " << ltpHeaderData.enigma
			<< " > SK " << ltpHeaderData.sessionKey);
	_proxyPacketBufferMutex.unlock();
}

bool Lightweight::_cmcGroupEmpty(IcnId &rCid, enigma_t &enigma,
		sk_t &sessionKey)
{
	map<enigma_t, map<sk_t, list<NodeId>>>::iterator cmcenigmaIt;
	map<sk_t, list<NodeId>>::iterator cmcSkIt;
	_cmcGroupsMutex->lock();
	// Find rCID
	_cmcGroupsIt = _cmcGroups->find(rCid.uint());

	if (_cmcGroupsIt == _cmcGroups->end())
	{
		_cmcGroupsMutex->unlock();
		return true;
	}

	// Find enigma
	cmcenigmaIt = _cmcGroupsIt->second.find(enigma);

	if (cmcenigmaIt == _cmcGroupsIt->second.end())
	{
		_cmcGroupsMutex->unlock();
		return true;
	}

	// find SK
	cmcSkIt = cmcenigmaIt->second.find(sessionKey);

	if (cmcSkIt == cmcenigmaIt->second.end())
	{
		_cmcGroupsMutex->unlock();
		return true;
	}

	// Check if NID list is empty
	if (cmcSkIt->second.empty())
	{
		_cmcGroupsMutex->unlock();
		return true;
	}

	_cmcGroupsMutex->unlock();
	return false;
}

void Lightweight::_deleteLtpBufferEntry(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey)
{
	map<enigma_t, map<sk_t, map<seq_t, packet_t>>>::iterator enigmaIt;
	map<sk_t, map<seq_t, packet_t>>::iterator skIt;
	ltp_packet_buffer_t::iterator ltpPacketBufferIt;
	_ltpPacketBufferMutex.lock();
	ltpPacketBufferIt = _ltpPacketBuffer.find(rCid.uint());

	// rCID not found
	if (ltpPacketBufferIt == _ltpPacketBuffer.end())
	{
		_ltpPacketBufferMutex.unlock();
		return;
	}

	enigmaIt = ltpPacketBufferIt->second.find(enigma);

	// enigma not found
	if (!enigmaIt->second.empty())
	{
		_ltpPacketBufferMutex.unlock();
		return;
	}

	skIt = enigmaIt->second.find(sessionKey);

	// SK not found
	if (skIt == enigmaIt->second.end())
	{
		_ltpPacketBufferMutex.unlock();
		return;
	}

	// SK values list not empty
	if (!skIt->second.empty())
	{
		skIt->second.clear();
		LOG4CXX_TRACE(logger, "Cleared entire LTP packet buffer for rCID "
				<< rCid.print() << " > enigma " << enigma << " > SK "
				<< sessionKey);
	}

	// deleting SK
	enigmaIt->second.erase(skIt);
	LOG4CXX_DEBUG(logger, "SK " << sessionKey << " deleted as key from LTP "
			"packet buffer");

	// enigma still has more SKs
	if (!enigmaIt->second.empty())
	{
		_ltpPacketBufferMutex.unlock();
		return;
	}

	ltpPacketBufferIt->second.erase(enigmaIt);
	LOG4CXX_TRACE(logger, "enigma " << enigma << " deleted as key from LTP "
			"packet buffer");
	// rCID hase more enigmas
	if (!ltpPacketBufferIt->second.empty())
	{
		_ltpPacketBufferMutex.unlock();
		return;
	}

	_ltpPacketBuffer.erase(ltpPacketBufferIt);
	LOG4CXX_DEBUG(logger, "rCID " << rCid.print() << " deleted as map key from "
			"LTP packet buffer");
	_ltpPacketBufferMutex.unlock();
}

void Lightweight::_deleteBufferedIcnPacket(IcnId &rCid, enigma_t &enigma,
		NodeId &nid, sk_t &sk)
{
	map<enigma_t, map<nid_t, map<sk_t, map<seq_t, pair<uint8_t*,
			uint16_t>>>>>::iterator enigmaIt;
	map<nid_t, map<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>>::iterator nidIt;
	map<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>::iterator skIt;
	map<seq_t, pair<uint8_t*, uint16_t>>::iterator seqIt;
	_icnPacketBufferMutex.lock();
	_icnPacketBufferIt = _icnPacketBuffer.find(rCid.uint());

	if (_icnPacketBufferIt == _icnPacketBuffer.end())
	{
		_icnPacketBufferMutex.unlock();
		LOG4CXX_TRACE(logger, "ICN packet could not be deleted from buffer. "
				"rCID " << rCid.print() << " does not exist");
		return;
	}

	enigmaIt = _icnPacketBufferIt->second.find(enigma);

	if (enigmaIt == _icnPacketBufferIt->second.end())
	{
		_icnPacketBufferMutex.unlock();
		LOG4CXX_TRACE(logger, "ICN packet could not be deleted from buffer. "
				"rCID " << rCid.print() << " > enigma " << enigma << " does not "
				"exist");
		return;
	}

	nidIt = enigmaIt->second.find(nid.uint());

	if (nidIt == enigmaIt->second.end())
	{
		_icnPacketBufferMutex.unlock();
		LOG4CXX_TRACE(logger, "ICN packet could not be deleted from buffer. "
				"rCID " << rCid.print() << " > enigma " << enigma << " > NID "
				<< nid.uint() << " does not exist");
		return;
	}

	skIt = nidIt->second.find(sk);

	if (skIt == nidIt->second.end())
	{
		_icnPacketBufferMutex.unlock();
		LOG4CXX_TRACE(logger, "ICN packet could not be deleted from buffer. "
				"rCID " << rCid.print() << " > enigma " << enigma << " > NID "
				<< nid.uint() << " > SK " << sk << " does not exist");
		return;
	}

	// Packet(s) found. Freeing up memory
	for (seqIt = skIt->second.begin(); seqIt != skIt->second.end(); seqIt++)
	{
		free(seqIt->second.first);
	}

	// now delete the entire SK map key
	nidIt->second.erase(skIt);
	LOG4CXX_TRACE(logger, "Deleted packet(s) from ICN buffer for rCID "
			<< rCid.print() << " > enigma " << enigma << " > NID " << nid.uint()
			<< " > SK " << sk);
	// now check if parent map keys can be deleted too
	// more SKs are stored for rCID > enigma > NID
	if (!nidIt->second.empty())
	{
		_icnPacketBufferMutex.unlock();
		return;
	}

	enigmaIt->second.erase(nidIt);
	LOG4CXX_TRACE(logger, "No other SK in ICN buffer map for rCID "
			<< rCid.print() << " > enigma " << enigma << " > NID " << nid.uint()
			<< ". Deleted NID ");

	// more NIDs for rCID > enigma
	if (!enigmaIt->second.empty())
	{
		_icnPacketBufferMutex.unlock();
		return;
	}

	_icnPacketBufferIt->second.erase(enigmaIt);
	LOG4CXX_TRACE(logger, "No other NID in ICN buffer map for rCID "
				<< rCid.print() << " > enigma " << enigma << ". Deleted enigma ");

	// more enigmas stored for rCID
	if (!_icnPacketBufferIt->second.empty())
	{
		_icnPacketBufferMutex.unlock();
		return;
	}

	_icnPacketBuffer.erase(_icnPacketBufferIt);
	LOG4CXX_TRACE(logger, "No other enigma in ICN buffer map for rCID "
					<< rCid.print() << ". Deleted rCID ");
	_icnPacketBufferMutex.unlock();
}

void Lightweight::_deleteBufferedLtpPacket(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey)
{
	map<cid_t, map<enigma_t, map<sk_t, map<seq_t, packet_t>>>>::iterator rCidIt;
	_ltpPacketBufferMutex.lock();
	rCidIt = _ltpPacketBuffer.find(rCid.uint());

	if (rCidIt == _ltpPacketBuffer.end())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " not found in LTP "
				"buffer");
		_ltpPacketBufferMutex.unlock();
		return;
	}

	map<enigma_t, map<sk_t, map<seq_t, packet_t>>>::iterator enigmaIt;
	enigmaIt = rCidIt->second.find(enigma);

	if (enigmaIt == rCidIt->second.end())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " > enigma "
				<< enigma << " not found in LTP buffer");
		_ltpPacketBufferMutex.unlock();
		return;
	}

	map<sk_t, map<seq_t, packet_t>>::iterator skIt;
	skIt = enigmaIt->second.find(sessionKey);

	if (skIt == enigmaIt->second.end())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " > enigma "
				<< enigma << " > SK " << sessionKey << " not found in LTP "
						"buffer");
		_ltpPacketBufferMutex.unlock();
		return;
	}

	// free up all allocated memory
	map<seq_t, packet_t>::iterator seqIt;

	for (seqIt = skIt->second.begin(); seqIt != skIt->second.end(); seqIt++)
	{
		free(seqIt->second.packet);

	}

	skIt->second.clear();
	LOG4CXX_TRACE(logger, "Packet(s) deleted from LTP packet buffer for rCID "
			<< rCid.print() << " > enigma " << enigma << " > SK "
			<< sessionKey);
	_ltpPacketBufferMutex.unlock();
}

void Lightweight::_deleteNackGroup(IcnId &rCid,
		ltp_hdr_ctrl_nack_t &ltpHeaderNack)
{
	_nackGroupsIt = _nackGroups.find(rCid.uint());

	// rCID not found
	if (_nackGroupsIt == _nackGroups.end())
	{
		LOG4CXX_DEBUG(logger, "No NACK group does exist for rCID "
				<< rCid.print());
		return;
	}

	map<enigma_t, map<sk_t, nack_group_t>>::iterator enigmaIt;
	enigmaIt = _nackGroupsIt->second.find(ltpHeaderNack.enigma);

	// enigma not found
	if (enigmaIt == _nackGroupsIt->second.end())
	{
		LOG4CXX_DEBUG(logger, "No NACK group does exist for rCID "
				<< rCid.print() << " > enigma " << ltpHeaderNack.enigma);
		return;
	}

	map<sk_t, nack_group_t>::iterator skIt;
	skIt = enigmaIt->second.find(ltpHeaderNack.sessionKey);

	// SK not found
	if (skIt == enigmaIt->second.end())
	{
		LOG4CXX_DEBUG(logger, "No NACK group does exist for rCID "
				<< rCid.print() << " > enigma " << ltpHeaderNack.enigma
				<< " > SK " << ltpHeaderNack.sessionKey);
		return;
	}

	enigmaIt->second.erase(skIt);
	LOG4CXX_TRACE(logger, "NACK group for rCID " << rCid.print() << " > enigma "
			<< ltpHeaderNack.enigma << " > SK "
			<< ltpHeaderNack.sessionKey << " deleted");

	if (enigmaIt->second.empty())
	{
		_nackGroupsIt->second.erase(enigmaIt);
		LOG4CXX_TRACE(logger, "This was the last SK for rCID " << rCid.print()
				<< " > enigma " << ltpHeaderNack.enigma << ". enigma deleted");
		if (_nackGroupsIt->second.empty())
		{
			_nackGroups.erase(_nackGroupsIt);
			LOG4CXX_TRACE(logger, "This was the last enigma for rCID "
					<< rCid.print() << ". rCID deleted");
		}
	}
}

void Lightweight::_deleteProxyPacket(IcnId &rCId, enigma_t enigma,
		sk_t sessionKey)
{
	map<seq_t, pair<uint8_t *, uint16_t>>::iterator sequenceMapIt;
	map<sk_t, map<seq_t, pair<uint8_t *, uint16_t>>>::iterator sessionKeyMapIt;
	map<enigma_t, map<sk_t, map<seq_t, pair<uint8_t *, uint16_t>>>>::iterator
			enigmaMapIt;
	_proxyPacketBufferMutex.lock();
	_proxyPacketBufferIt = _proxyPacketBuffer.find(rCId.uint());

	// rCID does not exist
	if (_proxyPacketBufferIt == _proxyPacketBuffer.end())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCId.print() << " cannot be found "
				"in proxy packet buffer map anymore");
		_proxyPacketBufferMutex.unlock();
		return;
	}

	enigmaMapIt = _proxyPacketBufferIt->second.find(enigma);

	// enigma does not exist
	if (enigmaMapIt == _proxyPacketBufferIt->second.end())
	{
		LOG4CXX_TRACE(logger, "enigma " << enigma << " could "
				"not be found in proxy packet buffer map anymore for rCID "
				<< rCId.print());
		_proxyPacketBufferMutex.unlock();
		return;
	}

	sessionKeyMapIt = enigmaMapIt->second.find(sessionKey);

	// SK does not exist
	if (sessionKeyMapIt == enigmaMapIt->second.end())
	{
		LOG4CXX_TRACE(logger, "SK " << sessionKey << " could"
				" not be found in proxy packet buffer map anymore for rCID "
				<< rCId.print() << " > enigma " << enigma);
		_proxyPacketBufferMutex.unlock();
		return;
	}

	// deleting packet
	for (sequenceMapIt = sessionKeyMapIt->second.begin();
			sequenceMapIt != sessionKeyMapIt->second.end(); sequenceMapIt++)
	{
		free(sequenceMapIt->second.first);
	}

	enigmaMapIt->second.erase(sessionKeyMapIt);
	_proxyPacketBufferMutex.unlock();
	LOG4CXX_TRACE(logger, "Packet deleted from proxy packet buffer for rCID "
			<< rCId.print() << " > enigma " << enigma << " > SK "
			<< sessionKey);
}

void Lightweight::_deleteSessionEnd(IcnId &rCid, sk_t &sessionKey)
{
	unordered_map<sk_t, bool>::iterator skIt;
	unordered_map<cid_t, unordered_map<sk_t, bool>>::iterator cidIt;
	_sessionEndedResponsesUnicastMutex.lock();
	cidIt = _sessionEndedResponsesUnicast.find(rCid.uint());

	// rCID not found
	if (cidIt == _sessionEndedResponsesUnicast.end())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " not found in LTP "
				"CTRL-SED map");
		_sessionEndedResponsesUnicastMutex.unlock();
		return;
	}

	skIt = cidIt->second.find(sessionKey);

	// SK not found
	if (skIt == cidIt->second.end())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " > SK " << sessionKey
				<< " not found in LTP CTRL-SED map");
		_sessionEndedResponsesUnicastMutex.unlock();
		return;
	}

	cidIt->second.erase(skIt);
	LOG4CXX_TRACE(logger, "SK " << sessionKey << " deleted from LTP CTRL-SED "
			"map under rCID " << rCid.print());

	// delete rCID if SK map is empty
	if (cidIt->second.empty())
	{
		_sessionEndedResponsesUnicast.erase(cidIt);
		LOG4CXX_TRACE(logger, "No SK left for rCID " << rCid.print() << " in "
				"LTP CTRL-SED map. rCID deleted");
	}

	_sessionEndedResponsesUnicastMutex.unlock();
}

void Lightweight::_deleteSessionEnd(IcnId &rCid,
		ltp_hdr_ctrl_se_t &ltpHdrCtrlSe, list<NodeId> &nodeIds)
{
	unordered_map<sk_t, bool>::iterator skIt;
	unordered_map<nid_t, unordered_map<sk_t, bool>>::iterator nidIt;
	unordered_map<enigma_t, unordered_map<nid_t, unordered_map<sk_t,
			bool>>>::iterator enigmaIt;
	_sessionEndedResponsesMutex.lock();
	_sessionEndedResponsesIt = _sessionEndedResponses.find(rCid.uint());

	// rCID does not exist
	if (_sessionEndedResponsesIt == _sessionEndedResponses.end())
	{
		LOG4CXX_DEBUG(logger, "rCID " << rCid.print() << " does not exist in "
				"SED responses map");
		_sessionEndedResponsesMutex.unlock();
		return;
	}

	enigmaIt = _sessionEndedResponsesIt->second.find(ltpHdrCtrlSe.enigma);

	// enigma does not exist
	if (enigmaIt == _sessionEndedResponsesIt->second.end())
	{
		LOG4CXX_DEBUG(logger, "rCID " << rCid.print() << " > enigma "
				<< ltpHdrCtrlSe.enigma << " does not exist in SED"
						" responses map");
		_sessionEndedResponsesMutex.unlock();
		return;
	}

	// iterate over NIDs
	list<NodeId> nodeIdsToBeDeleted;

	for (auto it = nodeIds.begin(); it != nodeIds.end(); it++)
	{
		nidIt = enigmaIt->second.find(it->uint());

		// NID found
		if (nidIt != enigmaIt->second.end())
		{
			skIt = nidIt->second.find(ltpHdrCtrlSe.sessionKey);

			// SK found, delete it
			if (skIt != nidIt->second.end())
			{
				nidIt->second.erase(skIt);
				LOG4CXX_TRACE(logger, "SK " << ltpHdrCtrlSe.sessionKey
						<< " deleted for rCID " << rCid.print() << " > enigma "
						<< ltpHdrCtrlSe.enigma << " NID "
						<< it->uint() << " from CTRL-SED responses map");

				// if this was the last SK for this NID, delete it
				if (nidIt->second.empty())
				{
					LOG4CXX_TRACE(logger, "NID " << it->uint() << " deleted for"
							" rCID " << rCid.print() << " > enigma "
							<< ltpHdrCtrlSe.enigma << " from CTRL-SED"
									" responses map");
					enigmaIt->second.erase(nidIt);
				}
			}
		}
	}

	// Now check for empty enigma and rCID values and erase them if necessary
	// No NIDs left
	if (enigmaIt->second.empty())
	{
		LOG4CXX_TRACE(logger, "enigma " << ltpHdrCtrlSe.enigma << " deleted "
				"for rCID " << rCid.print() << " from CTRL-SED responses map");
		_sessionEndedResponsesIt->second.erase(enigmaIt);
	}

	// No enigmas left
	if (_sessionEndedResponsesIt->second.empty())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " deleted from "
				"CTRL-SED responses map");
		_sessionEndedResponses.erase(_sessionEndedResponsesIt);
	}

	_sessionEndedResponsesMutex.unlock();
}

void Lightweight::_enableNIdInCmcGroup(IcnId &rCId, enigma_t &enigma,
			NodeId &nodeId)
{
	_potentialCmcGroupsMutex->lock();
	_potentialCmcGroupsIt = _potentialCmcGroups->find(rCId.uint());
	if (_potentialCmcGroupsIt == _potentialCmcGroups->end())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCId.print() << " could not be found "
				"in potential CMC group map");
		_potentialCmcGroupsMutex->unlock();
		return;
	}
	map<enigma_t, map<nid_t, bool>>::iterator enigmaMapIt;
	enigmaMapIt = _potentialCmcGroupsIt->second.find(enigma);

	if (enigmaMapIt == _potentialCmcGroupsIt->second.end())
	{
		LOG4CXX_TRACE(logger, "enigma " << enigma << " could not be found "
				"for rCID " << rCId.print() << " in potential CMC group map");
		_potentialCmcGroupsMutex->unlock();
		return;
	}

	map<nid_t, bool>::iterator nodeIdMapIt;
	nodeIdMapIt = enigmaMapIt->second.find(nodeId.uint());

	if (nodeIdMapIt == enigmaMapIt->second.end())
	{
		LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " could not be found "
				" for enigma " << enigma << " and " << " rCID "
				<< rCId.print() << " in potential CMC group map");
		_potentialCmcGroupsMutex->unlock();
		return;
	}

	nodeIdMapIt->second = true;
	LOG4CXX_DEBUG(logger, "NID " << nodeId.uint() << " enabled in potential CMC"
			" group rCID " << rCId.print() << " > enigma " << enigma);
	_potentialCmcGroupsMutex->unlock();
}

uint32_t Lightweight::_enabledNIdsInPotentialCmcGroup(IcnId &rCId,
		enigma_t &enigma)
{
	uint32_t enabledNids = 0;
	_potentialCmcGroupsMutex->lock();
	_potentialCmcGroupsIt = _potentialCmcGroups->find(rCId.uint());

	if (_potentialCmcGroupsIt == _potentialCmcGroups->end())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCId.print() << " could not be found "
				"in potential CMC group map");
		_potentialCmcGroupsMutex->unlock();
		return enabledNids;
	}

	map<enigma_t, map<nid_t, bool>>::iterator enigmaMapIt;
	enigmaMapIt = _potentialCmcGroupsIt->second.find(enigma);

	if (enigmaMapIt == _potentialCmcGroupsIt->second.end())
	{
		LOG4CXX_TRACE(logger, "enigma " << enigma << " could not be found "
				"for rCID " << rCId.print() << " in potential CMC group map");
		_potentialCmcGroupsMutex->unlock();
		return enabledNids;
	}

	map<nid_t, bool>::iterator nodeIdMapIt;

	for (nodeIdMapIt = enigmaMapIt->second.begin();
			nodeIdMapIt != enigmaMapIt->second.end(); nodeIdMapIt++)
	{
		if (nodeIdMapIt->second)
		{
			enabledNids++;
		}
	}

	_potentialCmcGroupsMutex->unlock();
	return enabledNids;
}

bool Lightweight::_forwardingEnabled(NodeId &nodeId)
{
	bool state;
	_knownNIdsMutex->lock();
	_knownNIdsIt = _knownNIds->find(nodeId.uint());

	if (_knownNIdsIt == _knownNIds->end())
	{
		LOG4CXX_DEBUG(logger, "NID " << nodeId.uint() << " doesn't exist in "
				"list of known NIDs");
		_knownNIdsMutex->unlock();
		return false;
	}

	state = _knownNIdsIt->second;
	_knownNIdsMutex->unlock();
	return state;
}

bool Lightweight::_getCmcGroup(IcnId &rCid, enigma_t &enigma,
		sk_t &sessionKey, list<NodeId> &cmcGroup)
{
	map<enigma_t, map<sk_t, list<NodeId>>>::iterator cmcenigmaIt;
	map<sk_t, list<NodeId>>::iterator cmcSkIt;
	_cmcGroupsIt = _cmcGroups->find(rCid.uint());

	// rCID does not exist
	if (_cmcGroupsIt == _cmcGroups->end())
	{
		LOG4CXX_TRACE(logger, "CMC group with rCID "
				<< rCid.print() << " does not exist (anymore)");
		return false;
	}

	cmcenigmaIt = _cmcGroupsIt->second.find(enigma);

	// enigma does not exist
	if (cmcenigmaIt == _cmcGroupsIt->second.end())
	{
		LOG4CXX_TRACE(logger, "CMC group cannot be found  (anymore) for rCID "
				<< rCid.print() << " > enigma " << enigma);
		return false;
	}

	//enigma exists
	cmcSkIt = cmcenigmaIt->second.find(sessionKey);

	// SK does not exist
	if (cmcSkIt == cmcenigmaIt->second.end())
	{
		LOG4CXX_TRACE(logger, "CMC group cannot be found  (anymore) for rCID "
				<< rCid.print() << " > enigma " << enigma << " > SK "
				<< sessionKey);
		return false;
	}

	cmcGroup = cmcSkIt->second;
	_statistics.cmcGroupSize(cmcGroup.size());
	return true;
}

tp_states_t Lightweight::_handleControl(IcnId &rCId, uint8_t *packet,
		enigma_t &enigma, sk_t &sessionKey)
{// LTP CTRL from sNAPs
	ltp_hdr_ctrl_t ltpHeaderCtrl;
	// [1] message type
	packet += sizeof(ltpHeaderCtrl.messageType);
	// [2] control type
	memcpy(&ltpHeaderCtrl.controlType, packet,
			sizeof(ltpHeaderCtrl.controlType));
	packet += sizeof(ltpHeaderCtrl.controlType);

	switch (ltpHeaderCtrl.controlType)
	{
	case LTP_CONTROL_NACK:
	{// HTTP request wasn't fully received by sNAP
		ltp_hdr_ctrl_nack_t ltpHeaderNack;
		// [3] enigma
		memcpy(&ltpHeaderNack.enigma, packet,
				sizeof(ltpHeaderNack.enigma));
		packet += sizeof(ltpHeaderNack.enigma);
		// [4] SK
		memcpy(&ltpHeaderNack.sessionKey, packet,
				sizeof(ltpHeaderNack.sessionKey));
		packet += sizeof(ltpHeaderNack.sessionKey);
		// [5] Start
		memcpy(&ltpHeaderNack.start, packet,
				sizeof(ltpHeaderNack.start));
		packet += sizeof(ltpHeaderNack.start);
		// [6] End
		memcpy(&ltpHeaderNack.end, packet, sizeof(ltpHeaderNack.end));
		LOG4CXX_TRACE(logger, "CTRL-NACK for Segments " << ltpHeaderNack.start
				<< " - " << ltpHeaderNack.end << " received under rCID "
				<< rCId.print() << " > enigma " << ltpHeaderNack.enigma
				<< " > SK " << ltpHeaderNack.sessionKey);
		_publishDataRange(rCId, ltpHeaderNack);
		break;
	}
	case LTP_CONTROL_RESETTED:
	{
		LOG4CXX_TRACE(logger, "LTP CTRL-RSTED received for rCID "
				<< rCId.print());
		_resettedMutex.lock();
		_resettedIt = _resetted.find(rCId.uint());

		// if rCID found, set received flag to true
		if (_resettedIt != _resetted.end())
		{
			_resettedIt->second = true;
			LOG4CXX_TRACE(logger, "LTP CTRL-RSTED 'received' flag set to true");
			// check if all LTP RST has been received
		}

		_resettedIt = _resetted.begin();

		while (_resettedIt != _resetted.end())
		{
			// oustanding LTP RSTED found. Exit here
			if (!_resettedIt->second)
			{
				LOG4CXX_DEBUG(logger, "Outstanding LTP RSTED found. Still not "
						"switching");
				_resettedMutex.unlock();
				return TP_STATE_NO_ACTION_REQUIRED;
			}
			_resettedIt++;
		}

		_resettedMutex.unlock();
		// get the CID for the received rCID
		IcnId cid;
		_cIdReverseLookUpMutex.lock();
		_cIdReverseLookUpIt = _cIdReverseLookUp.find(rCId.uint());

		// CID could not be found. Exit
		if (_cIdReverseLookUpIt == _cIdReverseLookUp.end())
		{
			_cIdReverseLookUpMutex.unlock();
			LOG4CXX_DEBUG(logger, "CID (FQDN) for rCID " << rCId.print()
					<< " could not be found to request a new FID")
			break;
		}

		// check if FID has been already requested. If so, stop here
		if (_cIdReverseLookUpIt->second.fidRequested())
		{
			LOG4CXX_TRACE(logger, "FID has been already requested for CID "
					<< _cIdReverseLookUpIt->second.print() << ". Info item will"
					" not be re-advertised");
			_cIdReverseLookUpMutex.unlock();
			break;
		}
		// if forwarding == true and FID == false, DNSlocal complete. Stop too
		// This could be the case if the old sNAP has received multiple LTP RST
		// for the same CID
		else if (_cIdReverseLookUpIt->second.forwarding() &&
				!_cIdReverseLookUpIt->second.fidRequested())
		{
			LOG4CXX_DEBUG(logger, "Switch to new surrogate has been already "
					"completed for CID " << _cIdReverseLookUpIt->second.print()
					<< ". Nothing to be done for the received LTP CTRL-RSTED "
							"message")
		}
		else
		{// setting FID state
			_cIdReverseLookUpIt->second.fidRequested(true);
		}

		cid = _cIdReverseLookUpIt->second;
		_cIdReverseLookUpMutex.unlock();
		// now un-publish and immediately publish again the availability of
		// information for the found CID
		_icnCoreMutex.lock();
		_icnCore->unpublish_info(cid.binId(), cid.binPrefixId(), DOMAIN_LOCAL,
				NULL, 0);
		LOG4CXX_TRACE(logger, "Availability of information removed from RV for "
				"CID " << cid.print());
		_icnCore->publish_info(cid.binId(), cid.binPrefixId(), DOMAIN_LOCAL,
				NULL, 0);
		LOG4CXX_TRACE(logger, "Availability of information for CID "
				<< cid.print() << " published again to RV");
		_icnCoreMutex.unlock();
		break;
	}
	case LTP_CONTROL_SESSION_END:
	{
		IcnId cid;
		// [3] enigma
		memcpy(&enigma, packet, sizeof(enigma));
		packet += sizeof(enigma);
		// [4] SK
		memcpy(&sessionKey, packet,	sizeof(sessionKey));
		LOG4CXX_TRACE(logger, "LTP CTRL-SE received for rCID " << rCId.print()
				<< " > enigma " << enigma << " > SK " << sessionKey);

		if (!cidLookup(rCId, cid))
		{
			LOG4CXX_DEBUG(logger, "CID cannot be retrieved for rCID "
					<< rCId.print() << ". LTP CTRL-SED cannot be sent for enigma "
					<< enigma << " > SK " << sessionKey);
			return TP_STATE_SESSION_ENDED;
		}

		_publishSessionEnded(cid, rCId, enigma, sessionKey);
		return TP_STATE_SESSION_ENDED;
	}
	case LTP_CONTROL_SESSION_ENDED:
	{
		// [3] enigma
		packet += sizeof(enigma);
		// [4] SK
		memcpy(&sessionKey, packet,	sizeof(sessionKey));
		LOG4CXX_TRACE(logger, "LTP CTRL-SED received for rCID " << rCId.print()
				<< " > enigma 0 > SK " << sessionKey);
		_setSessionEnded(rCId, sessionKey, true);
		//clean up LTP buffers
		Lightweight::cleanUpBuffers(rCId, enigma, sessionKey);
		break;
	}
	case LTP_CONTROL_WINDOW_END:
	{
		ltp_hdr_ctrl_we_t ltpHeaderCtrlWe;
		map<seq_t, pair<uint8_t*, uint16_t>>::iterator sequenceMapIt;
		map<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>::iterator
				sessionKeyMapIt;
		map<nid_t, map<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>>::iterator
				nodeIdMapIt;
		map<enigma_t, map<nid_t, map<sk_t, map<seq_t, pair<uint8_t*,
				uint16_t>>>>>::iterator enigmaMapIt;
		// [3] enigma
		memcpy(&ltpHeaderCtrlWe.enigma, packet,
				sizeof(ltpHeaderCtrlWe.enigma));
		packet += sizeof(ltpHeaderCtrlWe.enigma);
		// [4] Session key
		memcpy(&ltpHeaderCtrlWe.sessionKey, packet,
				sizeof(ltpHeaderCtrlWe.sessionKey));
		packet += sizeof(ltpHeaderCtrlWe.sessionKey);
		// [5] Sequence number
		memcpy(&ltpHeaderCtrlWe.sequenceNumber, packet,
				sizeof(ltpHeaderCtrlWe.sequenceNumber));
		LOG4CXX_TRACE(logger, "LTP CTRL-WE received for rCID " << rCId.print()
				<< " > enigma " << ltpHeaderCtrlWe.enigma << " > Session "
				"key " << ltpHeaderCtrlWe.sessionKey << " > Seq "
				<< ltpHeaderCtrlWe.sequenceNumber);
		/* Check in ICN packet buffer that all sequences starting from one up to
		 * the one received here have been received
		 */
		_icnPacketBufferMutex.lock();
		// Find rCID
		_icnPacketBufferIt = _icnPacketBuffer.find(rCId.uint());

		if (_icnPacketBufferIt == _icnPacketBuffer.end())
		{
			LOG4CXX_DEBUG(logger, "rCID " << rCId.print() << " unknown. "
					"Cannot check if CTRL-WED should be sent");
			_icnPacketBufferMutex.unlock();
			break;
		}

		// Find enigma
		enigmaMapIt = _icnPacketBufferIt->second.find(
				ltpHeaderCtrlWe.enigma);

		if (enigmaMapIt == _icnPacketBufferIt->second.end())
		{
			LOG4CXX_DEBUG(logger, "enigma " << ltpHeaderCtrlWe.enigma
					<< " unknown. Cannot check if CTRL-WED should be sent");
			_icnPacketBufferMutex.unlock();
			break;
		}

		// Find NID (for packet sent through CMC NID is always the NID of this
		// NAP)
		nodeIdMapIt = enigmaMapIt->second.find(_configuration.nodeId().uint());

		// NID does not exist
		if (nodeIdMapIt == enigmaMapIt->second.end())
		{
			LOG4CXX_TRACE(logger, "NID " << _configuration.nodeId().uint()
					<< " not known (anymore) in LTP packet store. Cannot check "
					"if WED CTRL should be sent ... sending one anyway");
			_icnPacketBufferMutex.unlock();
			IcnId cid;
			_cIdReverseLookUpMutex.lock();
			_cIdReverseLookUpIt = _cIdReverseLookUp.find(rCId.uint());

			if (_cIdReverseLookUpIt == _cIdReverseLookUp.end())
			{
				LOG4CXX_TRACE(logger, "rCID " << rCId.print() << " does not "
						"exist as key in CID reverse look-up table (anymore)");
				_cIdReverseLookUpMutex.unlock();

				return TP_STATE_NO_ACTION_REQUIRED;
			}

			cid = _cIdReverseLookUpIt->second;
			_cIdReverseLookUpMutex.unlock();
			ltp_hdr_ctrl_wed_t ltpHeaderCtrlWed;
			ltpHeaderCtrlWed.enigma = ltpHeaderCtrlWe.enigma;
			ltpHeaderCtrlWed.sessionKey = ltpHeaderCtrlWe.sessionKey;
			_publishWindowEnded(cid, rCId, ltpHeaderCtrlWed);
			break;
		}

		// Find session
		sessionKeyMapIt = nodeIdMapIt->second.find(ltpHeaderCtrlWe.sessionKey);

		if (sessionKeyMapIt == nodeIdMapIt->second.end())
		{
			LOG4CXX_DEBUG(logger, "SK " << ltpHeaderCtrlWe.sessionKey
					<< " unknown. Cannot check if WED CTRL should be sent");
			_icnPacketBufferMutex.unlock();
			break;
		}

		// Check that consecutive sequence numbers exist
		bool allFragmentsReceived = true;
		seq_t firstMissingSequence = 0;
		seq_t lastMissingSequence = 0;
		seq_t previousSequence = 0;

		for (sequenceMapIt = sessionKeyMapIt->second.begin();
				sequenceMapIt != sessionKeyMapIt->second.end(); sequenceMapIt++)
		{
			// First fragment is missing
			if (previousSequence == 0 && sequenceMapIt->first != 1)
			{
				firstMissingSequence = 1;
				lastMissingSequence = 1;
				allFragmentsReceived = false;
			}
			// Not first, but any other sequence is missing
			else if ((previousSequence + 1) < sequenceMapIt->first)
			{
				if (firstMissingSequence == 0)
				{
					firstMissingSequence = previousSequence + 1;
				}

				lastMissingSequence = sequenceMapIt->first - 1;
				allFragmentsReceived = false;
			}

			previousSequence = sequenceMapIt->first;

		}

		// check if sequence indicated in CTRL-WE was actually received
		map<seq_t, pair<uint8_t*, uint16_t>>::reverse_iterator sequenceMapRIt;
		sequenceMapRIt = sessionKeyMapIt->second.rbegin();

		if (sequenceMapRIt->first < ltpHeaderCtrlWe.sequenceNumber)
		{
			if (firstMissingSequence == 0)
			{
				firstMissingSequence = ltpHeaderCtrlWe.sequenceNumber;
				LOG4CXX_TRACE(logger, "First missing sequence changed to "
						<< firstMissingSequence);
			}

			lastMissingSequence = ltpHeaderCtrlWe.sequenceNumber;
			LOG4CXX_TRACE(logger, "Last missing sequence changed to "
					<< ltpHeaderCtrlWe.sequenceNumber);
			allFragmentsReceived = false;
		}

		_icnPacketBufferMutex.unlock();
		IcnId cid;
		_cIdReverseLookUpMutex.lock();
		_cIdReverseLookUpIt = _cIdReverseLookUp.find(rCId.uint());

		if (_cIdReverseLookUpIt == _cIdReverseLookUp.end())
		{
			LOG4CXX_TRACE(logger, "rCID " << rCId.print() << " does not exist "
					"as key in CID look-up table (anymore)");
			_cIdReverseLookUpMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		cid = _cIdReverseLookUpIt->second;
		_cIdReverseLookUpMutex.unlock();

		if (allFragmentsReceived)
		{
			ltp_hdr_ctrl_wed_t ltpHeaderCtrlWed;
			ltpHeaderCtrlWed.enigma = ltpHeaderCtrlWe.enigma;
			ltpHeaderCtrlWed.sessionKey = ltpHeaderCtrlWe.sessionKey;
			_publishWindowEnded(cid, rCId, ltpHeaderCtrlWed);
			enigma = ltpHeaderCtrlWe.enigma;
			sessionKey = ltpHeaderCtrlWe.sessionKey;
			return TP_STATE_ALL_FRAGMENTS_RECEIVED;
		}
		// Sequence was missing. Send off NACK message
		else
		{
			LOG4CXX_TRACE(logger, "Missing packets for rCID " << rCId.print()
					<< " > enigma " << ltpHeaderCtrlWe.enigma << " > SK "
					<< ltpHeaderCtrlWe.sessionKey << ". First: "
					<< firstMissingSequence << ", Last: "
					<< lastMissingSequence);
			_publishNegativeAcknowledgement(cid, rCId,
					ltpHeaderCtrlWe.enigma, ltpHeaderCtrlWe.sessionKey,
					firstMissingSequence, lastMissingSequence);
		}

		break;
	}
	case LTP_CONTROL_WINDOW_ENDED:
	{
		map<sk_t, bool>::iterator sessionKeyMapIt;
		map<enigma_t, map<sk_t, bool>>::iterator enigmaMapIt;
		ltp_hdr_ctrl_wed_t ltpHeader;
		// [3] enigma
		memcpy(&ltpHeader.enigma, packet, sizeof(ltpHeader.enigma));
		packet += sizeof(ltpHeader.enigma);
		// [4] Session key
		memcpy(&ltpHeader.sessionKey, packet, sizeof(ltpHeader.sessionKey));
		LOG4CXX_TRACE(logger, "LTP CTRL-WED received for rCID " << rCId.print()
				<< " > enigma " << ltpHeader.enigma << " > SK "
				<< ltpHeader.sessionKey);
		_windowEndedRequestsMutex.lock();
		// Check for CID
		_windowEndedRequestsIt = _windowEndedRequests.find(rCId.uint());

		if (_windowEndedRequestsIt == _windowEndedRequests.end())
		{
			LOG4CXX_DEBUG(logger, "rCID " << rCId.print() << " could not be "
					"found in _windowEndedRequests map");
			_windowEndedRequestsMutex.unlock();
			break;
		}

		// Check for enigma
		enigmaMapIt = _windowEndedRequestsIt->second.find(ltpHeader.enigma);

		if (enigmaMapIt == _windowEndedRequestsIt->second.end())
		{
			LOG4CXX_DEBUG(logger, "enigma " << ltpHeader.enigma << " does "
					"not exist in _windowEndedRequests map for rCID "
					<< rCId.print());
			_windowEndedRequestsMutex.unlock();
			break;
		}

		// Check SK
		sessionKeyMapIt = enigmaMapIt->second.find(ltpHeader.sessionKey);

		if (sessionKeyMapIt == enigmaMapIt->second.end())
		{
			LOG4CXX_DEBUG(logger, "SK " << ltpHeader.sessionKey
					<< " could not be found in _windowEndedRequests map for "
					"rCID " << rCId.print()	<< " > enigma "
					<< ltpHeader.enigma);
			_windowEndedRequestsMutex.unlock();
			break;
		}

		sessionKeyMapIt->second = true;
		LOG4CXX_TRACE(logger, "CTRL-WED flag set to true in _windowEndedRequest"
				"s map for rCID " << rCId.print() << " > enigma "
				<< ltpHeader.enigma << " > SK " << ltpHeader.sessionKey);
		_windowEndedRequestsMutex.unlock();
		//TODO delete LTP packet buffer (_proxyPacketBuffer)
		break;
	}
	case LTP_CONTROL_WINDOW_UPDATE:
	{
		// [3] enigma
		memcpy(&ltpHeaderCtrl.enigma, packet,
				sizeof(ltpHeaderCtrl.enigma));
		packet += sizeof(ltpHeaderCtrl.enigma);
		// [4] SK
		memcpy(&ltpHeaderCtrl.sessionKey, packet,
				sizeof(ltpHeaderCtrl.sessionKey));
		LOG4CXX_TRACE(logger, "LTP CTRL-WU received for rCID " << rCId.print()
				<< " > enigma " << ltpHeaderCtrl.enigma << " > SK "
				<< ltpHeaderCtrl.sessionKey)
		IcnId cid;
		ltp_hdr_ctrl_wud_t ltpHeaderCtrlWud;
		ltpHeaderCtrlWud.enigma = ltpHeaderCtrl.enigma;
		ltpHeaderCtrlWud.sessionKey = ltpHeaderCtrl.sessionKey;
		_cIdReverseLookUpMutex.lock();
		_cIdReverseLookUpIt = _cIdReverseLookUp.find(rCId.uint());

		if (_cIdReverseLookUpIt == _cIdReverseLookUp.end())
		{
			LOG4CXX_WARN(logger, "rCID " << rCId.print() << " does not "
					"exist as key in rCID > CID look-up table to check for LTP "
					"CTRL-WU");
			_cIdReverseLookUpMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		cid = _cIdReverseLookUpIt->second;
		_cIdReverseLookUpMutex.unlock();
		_publishWindowUpdated(cid, rCId, ltpHeaderCtrlWud);
		return TP_STATE_NO_ACTION_REQUIRED;
	}
	case LTP_CONTROL_WINDOW_UPDATED:
		// [3] enigma
		memcpy(&ltpHeaderCtrl.enigma, packet,
				sizeof(ltpHeaderCtrl.enigma));
		packet += sizeof(ltpHeaderCtrl.enigma);
		LOG4CXX_TRACE(logger, "LTP CTRL-WUD for enigma "
				<< ltpHeaderCtrl.enigma << " received under CID "
				<< rCId.print());
		break;
	default:
		LOG4CXX_TRACE(logger, "Unknown LTP CTRL type received for rCID "
				<< rCId.print() << ": " << ltpHeaderCtrl.controlType);
	}

	return TP_STATE_NO_ACTION_REQUIRED;
}

tp_states_t Lightweight::_handleControl(IcnId &cId, IcnId &rCId, NodeId &nodeId,
		uint8_t *packet, enigma_t &enigma, sk_t &sessionKey)
{// Handling LTP CTRL from cNAP
	ltp_hdr_ctrl_t ltpHeader;
	uint16_t offset = 0;
	offset += sizeof(ltpHeader.messageType);
	// First get the LTP control type
	// [2] control type
	memcpy(&ltpHeader.controlType, packet + offset,
			sizeof(ltpHeader.controlType));
	offset += sizeof(ltpHeader.controlType);

	switch (ltpHeader.controlType)
	{
	case LTP_CONTROL_NACK:
	{
		ltp_hdr_ctrl_nack_t ltpHeaderNack;
		// [3] enigma
		memcpy(&ltpHeaderNack.enigma, packet + offset,
				sizeof(ltpHeaderNack.enigma));
		offset += sizeof(ltpHeaderNack.enigma);
		// [4] SK
		memcpy(&ltpHeaderNack.sessionKey, packet + offset,
				sizeof(ltpHeaderNack.sessionKey));
		offset += sizeof(ltpHeaderNack.sessionKey);
		// [5] Start
		memcpy(&ltpHeaderNack.start, packet + offset,
				sizeof(ltpHeaderNack.start));
		offset += sizeof(ltpHeaderNack.start);
		// [6] End
		memcpy(&ltpHeaderNack.end, packet + offset, sizeof(ltpHeaderNack.end));
		LOG4CXX_TRACE(logger, "CTRL-NACK for Segments " << ltpHeaderNack.start
				<< " - " << ltpHeaderNack.end << " received under CID "
				<< cId.print() << " for rCID " << rCId.print() << " > enigma "
				<< ltpHeaderNack.enigma << " > SK "
				<< ltpHeaderNack.sessionKey);
		// first add the NID and the start/end
		_nackGroupsMutex.lock();
		_addNackNodeId(rCId, ltpHeaderNack, nodeId);
		// re-publish the requested range of segments
		// first check if a response from all nodes has been received. Find all
		// NID in the CMC group and check if this NID is the last one the sNAP
		// was waiting for
		list<NodeId> cmcGroup;
		_cmcGroupsMutex->lock();

		if (!_getCmcGroup(rCId, ltpHeaderNack.enigma,
				ltpHeaderNack.sessionKey, cmcGroup))
		{
			LOG4CXX_DEBUG(logger, "CMC group could not be obtained to resend "
					"NACK'd packets for rCID " << rCId.print() << " > enigma "
					<< ltpHeaderNack.enigma << " > SK "
					<< ltpHeaderNack.sessionKey);
			_cmcGroupsMutex->unlock();
			_nackGroupsMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		// SK exists
		// now get the iterator to the WED response map to check for the NIDs
		// 			WED received
		map<sk_t, bool>::iterator wedSkIt;
		map<nid_t, map<sk_t, bool>>::iterator wedNidIt;
		map<enigma_t, map<nid_t, map<sk_t, bool>>>::iterator wedenigmaIt;
		_windowEndedResponsesMutex.lock();
		_windowEndedResponsesIt = _windowEndedResponses.find(rCId.uint());

		// rCID does not exist in WED map
		if (_windowEndedResponsesIt == _windowEndedResponses.end())
		{
			LOG4CXX_TRACE(logger, "rCID " << rCId.print() << " does not exist "
					"in sent CTRL-WEs map");
			_nackGroupsMutex.unlock();
			_windowEndedResponsesMutex.unlock();
			_cmcGroupsMutex->unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		wedenigmaIt = _windowEndedResponsesIt->second.find(
				ltpHeaderNack.enigma);

		// enigma does not exist in WED map
		if (wedenigmaIt == _windowEndedResponsesIt->second.end())
		{
			LOG4CXX_TRACE(logger, "enigma " << ltpHeaderNack.enigma
					<< " could not be found in sent CTRL-WEs map");
			_nackGroupsMutex.unlock();
			_windowEndedResponsesMutex.unlock();
			_cmcGroupsMutex->unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		// find a particular node and check if WED was received
		list<NodeId>::iterator cmcGroupIt;
		uint16_t confirmedWeds = 0;

		for (cmcGroupIt = cmcGroup.begin();	cmcGroupIt != cmcGroup.end();
				cmcGroupIt++)
		{
			// Find CMC NID in WED NID map
			wedNidIt = wedenigmaIt->second.find(cmcGroupIt->uint());

			if (wedNidIt != wedenigmaIt->second.end())
			{
				wedSkIt = wedNidIt->second.find(ltpHeaderNack.sessionKey);

				// SK exists in WED map
				if (wedSkIt != wedNidIt->second.end())
				{
					// check if WED has been received for this NID
					if (wedSkIt->second)
					{
						confirmedWeds++;
					}
				}
			}
		}

		// If sum of received WEDs + NACKs == CMC group size, send range of NACK
		// fragments
		nack_group_t nackGroup = _nackGroup(rCId, ltpHeaderNack);

		if (cmcGroup.size() > (confirmedWeds + nackGroup.nodeIds.size()))
		{
			_nackGroupsMutex.unlock();
			_windowEndedResponsesMutex.unlock();
			_cmcGroupsMutex->unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		//all nodes have replied to the WE msg with either WED or NACK. Publish
		//range of segments
		_windowEndedResponsesMutex.unlock();
		_cmcGroupsMutex->unlock();
		_publishDataRange(rCId, ltpHeaderNack.enigma,
				ltpHeaderNack.sessionKey, nackGroup);
		// delete NACK group
		_deleteNackGroup(rCId, ltpHeaderNack);
		_nackGroupsMutex.unlock();
		break;
	}
	case LTP_CONTROL_RESET:
	{
		list<pair<enigma_t, sk_t>> enigmaSkPairs;
		list<pair<enigma_t, sk_t>>::iterator enigmaSkPairsIt;
		LOG4CXX_TRACE(logger, "CTRL-RST received under CID " << cId.print()
				<< " > rCID " << rCId.print());
		// first delete NID from CMC group(s) for rCID
		_removeNidFromCmcGroups(rCId, nodeId, &enigmaSkPairs);
		_publishResetted(rCId, nodeId);

		// delete packet from LTP buffer if there's no NID left in CMC group
		enigmaSkPairsIt = enigmaSkPairs.begin();

		while (enigmaSkPairsIt != enigmaSkPairs.end())
		{
			// check if group exists and still has members
			if (_cmcGroupEmpty(rCId, enigmaSkPairsIt->first, enigmaSkPairsIt->second))
			{
				_deleteBufferedLtpPacket(rCId, enigmaSkPairsIt->first,
						enigmaSkPairsIt->second);
			}

			enigmaSkPairsIt++;
		}

		return TP_STATE_NO_ACTION_REQUIRED;
	}
	case LTP_CONTROL_RESETTED:
	{
		LOG4CXX_TRACE(logger, "CTRL-RSTED received under CID " << cId.print()
				<< " > rCID " << rCId.print() << ". That's WRONG in the sNAP");
		return TP_STATE_NO_ACTION_REQUIRED;
	}
	case LTP_CONTROL_SESSION_END:
	{
		// [3] enigma
		//memcpy(&enigma, packet + offset, sizeof(enigma));
		offset += sizeof(enigma);
		// [4] SK
		memcpy(&sessionKey, packet + offset, sizeof(sessionKey));
		LOG4CXX_TRACE(logger, "CTRL-SE received from cNAP for CID "
				<< cId.print() << " rCID " << rCId.print() << " > enigma "
				<< enigma << " > SK " << sessionKey);
		_publishSessionEnded(rCId, nodeId, sessionKey);
		// shutdown as write. TCP client class (read()) will receive close state
		LOG4CXX_TRACE(logger, "Shutting down socket FD " << sessionKey);
		shutdown(sessionKey, SHUT_WR);
		return TP_STATE_NO_ACTION_REQUIRED;
	}
	case LTP_CONTROL_SESSION_ENDED:
	{
		unordered_map<sk_t, bool>::iterator skIt;
		unordered_map<nid_t, unordered_map<sk_t, bool>>::iterator nidIt;
		unordered_map<enigma_t, unordered_map<nid_t, unordered_map<sk_t,
				bool>>>::iterator enigmaIt;
		// [3] enigma
		memcpy(&enigma, packet + offset, sizeof(enigma));
		offset += sizeof(enigma);
		// [4] SK
		memcpy(&sessionKey, packet + offset, sizeof(sessionKey));
		LOG4CXX_TRACE(logger, "CTRL-SED received from cNAP for CID "
				<< cId.print() << " rCID " << rCId.print() << " > enigma "
				<< enigma << " > SK " << sessionKey);
		_sessionEndedResponsesMutex.lock();
		_sessionEndedResponsesIt = _sessionEndedResponses.find(rCId.uint());

		// rCID does not exist
		if (_sessionEndedResponsesIt == _sessionEndedResponses.end())
		{
			LOG4CXX_DEBUG(logger, "rCID " << rCId.print() << " does not exist "
					"in LTP CTRL SED map (anymore). Dropping packet");
			_sessionEndedResponsesMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		enigmaIt = _sessionEndedResponsesIt->second.find(enigma);

		// enigma does not exist
		if (enigmaIt == _sessionEndedResponsesIt->second.end())
		{
			LOG4CXX_DEBUG(logger, "rCID " << rCId.print() << " > enigma "
					<< enigma << " does not exist in LTP CTRL SED map"
							"(anymore). Dropping packet");
			_sessionEndedResponsesMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		nidIt = enigmaIt->second.find(nodeId.uint());

		// NID does not exist
		if (nidIt == enigmaIt->second.end())
		{
			LOG4CXX_DEBUG(logger, "rCID " << rCId.print() << " > enigma "
					<< enigma << " > NID " << nodeId.uint() << " does not "
					"exist in LTP CTRL SED map (anymore). Dropping packet");
			_sessionEndedResponsesMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		skIt = nidIt->second.find(sessionKey);

		if (skIt == nidIt->second.end())
		{
			LOG4CXX_DEBUG(logger, "rCID " << rCId.print() << " > enigma "
					<< enigma << " > NID " << nodeId.uint() << " > SK "
					<< sessionKey << " does not exist in LTP CTRL SED map "
							"(anymore). Dropping packet");
			_sessionEndedResponsesMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		skIt->second = true;
		LOG4CXX_TRACE(logger, "SED received flag set to true for rCID "
				<< rCId.print() << " > enigma " << enigma << " > NID "
				<< nodeId.uint() << " > SK "<< sessionKey);
		_sessionEndedResponsesMutex.unlock();
		break;
	}
	case LTP_CONTROL_WINDOW_END:
	{
		ltp_hdr_ctrl_we_t ltpHeaderControlWe;
		map<seq_t, pair<uint8_t*, uint16_t>>::iterator sequenceMapIt;
		map<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>::iterator
				sessionKeyMapIt;
		map<nid_t, map<sk_t, map<seq_t, pair<uint8_t*, uint16_t>>>>::iterator
				nodeIdMapIt;
		map<enigma_t, map<nid_t, map<sk_t, map<seq_t, pair<uint8_t*,
				uint16_t>>>>>::iterator enigmaMapIt;
		// [3] enigma
		memcpy(&ltpHeaderControlWe.enigma, packet + offset,
				sizeof(ltpHeaderControlWe.enigma));
		offset += sizeof(ltpHeaderControlWe.enigma);
		// [4] Session key
		memcpy(&ltpHeaderControlWe.sessionKey, packet + offset,
				sizeof(ltpHeaderControlWe.sessionKey));
		offset += sizeof(ltpHeaderControlWe.sessionKey);
		// [5] Sequence number
		memcpy(&ltpHeaderControlWe.sequenceNumber, packet + offset,
				sizeof(ltpHeaderControlWe.sequenceNumber));
		LOG4CXX_TRACE(logger, "LTP CTRL-WE (enigma "
				<< ltpHeaderControlWe.enigma << ", SK "
				<< ltpHeaderControlWe.sessionKey << ", Seq "
				<< ltpHeaderControlWe.sequenceNumber << ") received for CID "
				<< cId.print() << ", rCID " << rCId.print());
		// Update CMC group that this NID is ready to receive response
		_enableNIdInCmcGroup(rCId, ltpHeaderControlWe.enigma, nodeId);
		/* Check in ICN packet buffer that all sequences starting from one up to
		 * the one received here have been received
		 */
		_icnPacketBufferMutex.lock();
		// Find rCID
		_icnPacketBufferIt = _icnPacketBuffer.find(rCId.uint());

		if (_icnPacketBufferIt == _icnPacketBuffer.end())
		{
			LOG4CXX_DEBUG(logger, "rCID " << rCId.print() << " unknown. "
					"Cannot check if CTRL-WED should be sent (START_PUBLISH_iSU"
					"B hasn't been received when CTRL-WE came");
			_icnPacketBufferMutex.unlock();
			break;
		}

		// Find enigma
		enigmaMapIt = _icnPacketBufferIt->second.find(ltpHeaderControlWe.enigma);

		if (enigmaMapIt == _icnPacketBufferIt->second.end())
		{
			LOG4CXX_TRACE(logger, "Enigma " << ltpHeaderControlWe.enigma
					<< " unknown. Cannot check if CTRL-WED should be sent "
							"(original WED got potentially lost");
			_icnPacketBufferMutex.unlock();
			_publishWindowEnded(rCId, nodeId, ltpHeaderControlWe.enigma,
					ltpHeaderControlWe.sessionKey);
			break;
		}

		// Find NID
		nodeIdMapIt = enigmaMapIt->second.find(nodeId.uint());

		// NID does not exist
		if (nodeIdMapIt == enigmaMapIt->second.end())
		{
			LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " unknown. Cannot "
					"check if WED CTRL should be sent. Simply re-publish a WED "
					"again");
			_icnPacketBufferMutex.unlock();
			_publishWindowEnded(rCId, nodeId, ltpHeaderControlWe.enigma,
					ltpHeaderControlWe.sessionKey);
			break;
		}

		// Find session
		sessionKeyMapIt = nodeIdMapIt->second.find(ltpHeaderControlWe.sessionKey);

		if (sessionKeyMapIt == nodeIdMapIt->second.end())
		{
			LOG4CXX_TRACE(logger, "SK " << ltpHeaderControlWe.sessionKey
					<< " unknown. Cannot check if WED CTRL should be sent. "
							"Simply re-publish a CTRL-WED again");
			_icnPacketBufferMutex.unlock();
			_publishWindowEnded(rCId, nodeId, ltpHeaderControlWe.enigma,
					ltpHeaderControlWe.sessionKey);
			break;
		}

		// Check that consecutive sequence numbers exist
		bool allFragmentsReceived = true;
		seq_t firstMissingSequence = 0;
		seq_t lastMissingSequence = 0;
		seq_t previousSequence = 0;

		for (sequenceMapIt = sessionKeyMapIt->second.begin();
				sequenceMapIt != sessionKeyMapIt->second.end(); sequenceMapIt++)
		{
			// First fragment is missing
			if (previousSequence == 0 && sequenceMapIt->first != 1)
			{
				firstMissingSequence = 1;
				lastMissingSequence = 1;
				cout << "both set to 1\n";
				allFragmentsReceived = false;
			}
			// Not first, but any other sequence is missing
			else if ((previousSequence + 1) < sequenceMapIt->first)
			{
				if (firstMissingSequence == 0)
				{
					firstMissingSequence = previousSequence + 1;
					cout << "firstMissingSequence set to "
							<< firstMissingSequence << endl;
				}

				lastMissingSequence = sequenceMapIt->first - 1;
				cout << "lastMissingSequence set to " << lastMissingSequence
						<< endl;
				allFragmentsReceived = false;
			}

			previousSequence = sequenceMapIt->first;

		}
		// check if sequence indicated in CTRL-WE was actually received
		map<seq_t, pair<uint8_t*, uint16_t>>::reverse_iterator sequenceMapRIt;
		sequenceMapRIt = sessionKeyMapIt->second.rbegin();

		if (sequenceMapRIt->first < ltpHeaderControlWe.sequenceNumber)
		{
			if (firstMissingSequence == 0)
			{
				firstMissingSequence = ltpHeaderControlWe.sequenceNumber;
			}

			lastMissingSequence = ltpHeaderControlWe.sequenceNumber;
			allFragmentsReceived = false;
		}

		_icnPacketBufferMutex.unlock();

		// send WED if all segments have been received
		if (allFragmentsReceived)
		{
			_publishWindowEnded(rCId, nodeId, ltpHeaderControlWe.enigma,
					ltpHeaderControlWe.sessionKey);
			enigma = ltpHeaderControlWe.enigma;
			sessionKey = ltpHeaderControlWe.sessionKey;
			//retrieve the packet right afterwards in Icn class
			return TP_STATE_ALL_FRAGMENTS_RECEIVED;
		}
		// Sequence was missing. Send off NACK message
		else
		{
			LOG4CXX_TRACE(logger, "Missing packets for CID " << cId.print()
					<< " rCID " << rCId.print()	<< " > enigma "
					<< ltpHeaderControlWe.enigma << " > SK "
					<< ltpHeaderControlWe.sessionKey << ". First: "
					<< firstMissingSequence << ", Last: "
					<< lastMissingSequence);
			_publishNegativeAcknowledgement(rCId, nodeId,
					ltpHeaderControlWe.enigma,
					ltpHeaderControlWe.sessionKey, firstMissingSequence,
					lastMissingSequence);
		}
		break;
	}
	case LTP_CONTROL_WINDOW_ENDED:
	{
		ltp_hdr_ctrl_wed_t ltpHeaderControlWed;
		// map<Session, WED received
		map<sk_t, bool>::iterator sessionMapIt;
		map<nid_t, map<sk_t, bool>>::iterator nidMapIt;
		map<enigma_t, map<nid_t, map<sk_t, bool>>>::iterator enigmaMapIt;
		// [3] enigma
		memcpy(&ltpHeaderControlWed.enigma, packet + offset,
				sizeof(ltpHeaderControlWed.enigma));
		offset += sizeof(ltpHeaderControlWed.enigma);
		// [4] Session key
		memcpy(&ltpHeaderControlWed.sessionKey, packet + offset,
				sizeof(ltpHeaderControlWed.sessionKey));
		LOG4CXX_TRACE(logger, "LTP CTRL-WED received for rCID " << rCId.print()
				<< " > enigma " << ltpHeaderControlWed.enigma << " > NID "
				<< nodeId.uint() << " > SK "
				<< ltpHeaderControlWed.sessionKey);
		_windowEndedResponsesMutex.lock();
		_windowEndedResponsesIt = _windowEndedResponses.find(rCId.uint());

		if (_windowEndedResponsesIt == _windowEndedResponses.end())
		{
			LOG4CXX_TRACE(logger, "rCID " << rCId.print() << " does not exist "
					"in sent CTRL-WEs map");
			_windowEndedResponsesMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		enigmaMapIt = _windowEndedResponsesIt->second.find(
				ltpHeaderControlWed.enigma);

		if (enigmaMapIt == _windowEndedResponsesIt->second.end())
		{
			LOG4CXX_TRACE(logger, "enigma " << ltpHeaderControlWed.enigma
					<< " could not be found in sent CTRL-WEs map");
			_windowEndedResponsesMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		nidMapIt = enigmaMapIt->second.find(nodeId.uint());

		if (nidMapIt == enigmaMapIt->second.end())
		{
			LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " does not exist "
					"in sent CTRL-WEs map");
			_windowEndedResponsesMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		sessionMapIt = nidMapIt->second.find(ltpHeaderControlWed.sessionKey);

		if (sessionMapIt == nidMapIt->second.end())
		{
			LOG4CXX_TRACE(logger, "SK " << ltpHeaderControlWed.sessionKey
					<< " does not exist in sent CTRL-WEs map");
			_windowEndedResponsesMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		// Only update if WED received boolean is set to false
		if (!sessionMapIt->second)
		{
			sessionMapIt->second = true;
			LOG4CXX_TRACE(logger, "WED received boolean set to true for rCID "
					<< rCId.print() << " > enigma "
					<< ltpHeaderControlWed.enigma << " > NID "
					<< nodeId.uint() << " > SK "
					<< ltpHeaderControlWed.sessionKey);
		}

		// check if entire packet in LTP buffer can be deleted. Conditions: all
		// WEDs from all members of this CMC group were received
		list<NodeId> cmcGroup;
		_cmcGroupsMutex->lock();

		if (!_getCmcGroup(rCId, ltpHeaderControlWed.enigma,
				ltpHeaderControlWed.sessionKey, cmcGroup))
		{
			LOG4CXX_TRACE(logger, "No CMC group exists (anymore) for rCID "
					<< rCId.print() << " > enigma "
					<< ltpHeaderControlWed.enigma << " > SK "
					<< ltpHeaderControlWed.enigma);
			_windowEndedResponsesMutex.unlock();
			_cmcGroupsMutex->unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		for (list<NodeId>::iterator cmcGroupIt = cmcGroup.begin();
				cmcGroupIt != cmcGroup.end(); cmcGroupIt++)
		{
			nidMapIt = enigmaMapIt->second.find(cmcGroupIt->uint());

			// NID found
			if (nidMapIt != enigmaMapIt->second.end())
			{
				sessionMapIt = nidMapIt->second.find(
						ltpHeaderControlWed.sessionKey);

				// SK found
				if (sessionMapIt != nidMapIt->second.end())
				{
					// WED still missing. exit here
					if (!sessionMapIt->second)
					{
						_cmcGroupsMutex->unlock();
						_windowEndedResponsesMutex.unlock();
						return TP_STATE_NO_ACTION_REQUIRED;
					}
				}
			}
		}

		_cmcGroupsMutex->unlock();
		_windowEndedResponsesMutex.unlock();
		// now go off and delete the packet in the LTP buffer to allow the next
		// packet from the server to be added to the buffer and published to the
		// UE(s)
		Lightweight::_deleteBufferedLtpPacket(rCId,
						ltpHeaderControlWed.enigma,
						ltpHeaderControlWed.sessionKey);
		Lightweight::_ltpSessionActivitySet(rCId,
				ltpHeaderControlWed.enigma, ltpHeaderControlWed.sessionKey,
				false);
		break;
	}
	case LTP_CONTROL_WINDOW_UPDATE:
	{
		ltp_hdr_ctrl_wu_t ltpHeaderControlWu;
		// [3] enigma
		memcpy(&ltpHeaderControlWu.enigma, packet + offset,
				sizeof(ltpHeaderControlWu.enigma));
		offset += sizeof(ltpHeaderControlWu.enigma);
		// [4] Session key
		memcpy(&ltpHeaderControlWu.sessionKey, packet + offset,
				sizeof(ltpHeaderControlWu.sessionKey));
		LOG4CXX_TRACE(logger, "LTP CTRL-WU for enigma "
				<< ltpHeaderControlWu.enigma << " received for CID "
				<< cId.print() << " with rCID " << rCId.print());
		break;
	}
	case LTP_CONTROL_WINDOW_UPDATED:
	{
		ltp_hdr_ctrl_wu_t ltpHeaderControlWud;
		list<uint32_t> nidsIt;
		map<nid_t, bool>::iterator nidIt;
		map<sk_t, map<nid_t, bool>>::iterator skIt;
		map<enigma_t, map<sk_t, map<nid_t, bool>>>::iterator enigmaIt;
		// [3] enigma
		memcpy(&ltpHeaderControlWud.enigma, packet + offset,
				sizeof(ltpHeaderControlWud.enigma));
		offset += sizeof(ltpHeaderControlWud.enigma);
		// [4] Session key
		memcpy(&ltpHeaderControlWud.sessionKey, packet + offset,
				sizeof(ltpHeaderControlWud.sessionKey));
		LOG4CXX_TRACE(logger, "LTP CTRL-WUD received for rCID " << rCId.print()
				<< " > enigma " << ltpHeaderControlWud.enigma << " > NID "
				<< nodeId.uint() << " > SK " << ltpHeaderControlWud.sessionKey);
		_windowUpdateMutex.lock();
		_windowUpdateIt = _windowUpdate.find(rCId.uint());

		if (_windowUpdateIt == _windowUpdate.end())
		{
			LOG4CXX_DEBUG(logger, "rCID " << rCId.print() << " does not exist "
					"in WU map");
			_windowUpdateMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		enigmaIt = _windowUpdateIt->second.find(ltpHeaderControlWud.enigma);

		if (enigmaIt == _windowUpdateIt->second.end())
		{
			LOG4CXX_DEBUG(logger, "rCID " << rCId.print() << " > enigma "
					<< ltpHeaderControlWud.enigma << " does not exist in "
							"WU map");
			_windowUpdateMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		skIt = enigmaIt->second.find(ltpHeaderControlWud.sessionKey);

		if (skIt == enigmaIt->second.end())
		{
			LOG4CXX_DEBUG(logger, "rCID " << rCId.print() << " > enigma "
					<< ltpHeaderControlWud.enigma << " > SK "
					<< ltpHeaderControlWud.sessionKey << " does not exist in WU"
							" map");
			_windowUpdateMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		nidIt = skIt->second.find(nodeId.uint());

		if (nidIt == skIt->second.end())
		{
			LOG4CXX_DEBUG(logger, "rCID " << rCId.print() << " > enigma "
					<< ltpHeaderControlWud.enigma << " > SK "
					<< ltpHeaderControlWud.sessionKey << " > NID "
					<< nodeId.uint() << " does not exist in WU map");
			_windowUpdateMutex.unlock();
			return TP_STATE_NO_ACTION_REQUIRED;
		}

		nidIt->second = true;
		LOG4CXX_TRACE(logger, "WUD state for NID " << nodeId.uint() << " set to"
				" 'received' for rCID " << rCId.print() << " > enigma "
				<< ltpHeaderControlWud.enigma << " > SK "
				<< ltpHeaderControlWud.sessionKey);
		_windowUpdateMutex.unlock();
		return TP_STATE_NO_ACTION_REQUIRED;
	}
	default:
		LOG4CXX_DEBUG(logger, "Unknown LTP CTRL type of value "
				<< ltpHeader.controlType << " received: "
				<< ltpHeader.controlType);
	}
	return TP_STATE_FRAGMENTS_OUTSTANDING;
}

void Lightweight::_handleData(IcnId &rCid, uint8_t *packet)
{
	ltp_hdr_data_t ltpHeader;
	uint8_t offset = 0;
	// [1] Message type
	offset += sizeof(ltpHeader.messageType);
	// [2] enigma
	memcpy(&ltpHeader.enigma, packet + offset,
			sizeof(ltpHeader.enigma));
	offset += sizeof(ltpHeader.enigma);
	// [3] Session key
	memcpy(&ltpHeader.sessionKey, packet + offset,
			sizeof(ltpHeader.sessionKey));
	offset += sizeof(ltpHeader.sessionKey);
	// [4] Sequence num
	memcpy(&ltpHeader.sequenceNumber, packet + offset,
			sizeof(ltpHeader.sequenceNumber));
	offset += sizeof(ltpHeader.sequenceNumber);
	// [5] Payload length
	memcpy(&ltpHeader.payloadLength, packet + offset,
			sizeof(ltpHeader.payloadLength));
	offset += sizeof(ltpHeader.payloadLength);
	packet += offset;
	//add packet to LTP buffer
	_bufferIcnPacket(rCid, _configuration.nodeId(), ltpHeader, packet);
}

void Lightweight::_handleData(IcnId cId, IcnId &rCId, NodeId &nodeId,
		uint8_t *packet)
{
	ltp_hdr_data_t ltpHeader;
	uint8_t offset = 0;
	// [1] Message type
	offset += sizeof(ltpHeader.messageType);
	// [2] enigma
	memcpy(&ltpHeader.enigma, packet + offset,
			sizeof(ltpHeader.enigma));
	offset += sizeof(ltpHeader.enigma);
	// [3] Session key
	memcpy(&ltpHeader.sessionKey, packet + offset,
			sizeof(ltpHeader.sessionKey));
	offset += sizeof(ltpHeader.sessionKey);
	// [4] Sequence num
	memcpy(&ltpHeader.sequenceNumber, packet + offset,
			sizeof(ltpHeader.sequenceNumber));
	offset += sizeof(ltpHeader.sequenceNumber);
	// [5] Payload length
	memcpy(&ltpHeader.payloadLength, packet + offset,
			sizeof(ltpHeader.payloadLength));
	offset += sizeof(ltpHeader.payloadLength);
	packet += offset;
	// add packet to LTP ICN buffer
	_bufferIcnPacket(rCId, nodeId, ltpHeader, packet);
	// Note, the _potentialCmcGroups* map and mutex are only pointers. They have
	// been declared in the HTTP handler and were passed when the LTP class was
	// initialised
	_potentialCmcGroupsMutex->lock();
	// check if rCID is known
	_potentialCmcGroupsIt = _potentialCmcGroups->find(rCId.uint());
	// rCID unknown --> new CMC group
	if (_potentialCmcGroupsIt == _potentialCmcGroups->end())
	{
		//map<NID,    entire HTTP request received
		map<nid_t, bool> nodeIdMap;
		map<enigma_t, map<nid_t, bool>> enigmaMap;
		nodeIdMap.insert(pair<nid_t, bool>(nodeId.uint(), false));
		enigmaMap.insert(pair<enigma_t, map<nid_t, bool>>(
				ltpHeader.enigma, nodeIdMap));
		_potentialCmcGroups->insert(pair<cid_t, map<enigma_t, map<nid_t, bool>>>
				(rCId.uint(), enigmaMap));
		LOG4CXX_TRACE(logger, "New potential CMC group created for rCID "
				<< rCId.print() << " and enigma " << ltpHeader.enigma
				<< " with NID " << nodeId.uint());
		_potentialCmcGroupsMutex->unlock();
		return;
	}

	// rCID known. Check if enigma is known
	map<enigma_t, map<nid_t, bool>>::iterator enigmaIt;
	enigmaIt = _potentialCmcGroupsIt->second.find(ltpHeader.enigma);

	// enigma unknown
	if (enigmaIt == _potentialCmcGroupsIt->second.end())
	{
		//map<NID,    entire HTTP request received
		map<nid_t, bool> nodeIdMap;
		nodeIdMap.insert(pair<nid_t, bool>(nodeId.uint(), false));
		_potentialCmcGroupsIt->second.insert(pair<enigma_t, map<nid_t, bool>>
				(ltpHeader.enigma, nodeIdMap));
		LOG4CXX_TRACE(logger, "New potential CMC group created for known rCID "
				<< rCId.print() << " but new enigma " << ltpHeader.enigma
				<< " with NID " << nodeId.uint());
		_potentialCmcGroupsMutex->unlock();
		return;
	}

	// enigma known. Check if NID is known
	map<enigma_t, bool>::iterator nidIt;
	nidIt = enigmaIt->second.find(nodeId.uint());

	// NID unknown
	if (nidIt == enigmaIt->second.end())
	{
		enigmaIt->second.insert(pair<nid_t, bool>(nodeId.uint(), false));
		LOG4CXX_TRACE(logger, "Existing potential CMC group for rCID "
				<< rCId.print() << " > enigma " << ltpHeader.enigma
				<< " updated with NID " << nodeId.uint());
		_potentialCmcGroupsMutex->unlock();
		return;
	}

	LOG4CXX_TRACE(logger, "NID already exists in potential CMC group for rCID "
			<< rCId.print() << " > enigma " << ltpHeader.enigma);
	_potentialCmcGroupsMutex->unlock();
}

bool Lightweight::_ltpCtrlSedReceived(IcnId &rCid, sk_t &sessionKey)
{
	bool state;
	unordered_map<sk_t, bool>::iterator skIt;
	unordered_map<cid_t, unordered_map<sk_t, bool>>::iterator cidIt;

	_sessionEndedResponsesUnicastMutex.lock();
	cidIt = _sessionEndedResponsesUnicast.find(rCid.uint());

	// rCID not found
	if (cidIt == _sessionEndedResponsesUnicast.end())
	{
		LOG4CXX_TRACE(logger, "Cannot check if LTP CTRL-SED has been received. "
				"rCID " << rCid.print() << " not found in map");
		_sessionEndedResponsesUnicastMutex.unlock();
		return false;
	}

	skIt = cidIt->second.find(sessionKey);

	// SK not found
	if (skIt == cidIt->second.end())
	{
		LOG4CXX_TRACE(logger, "Cannot check if LTP CTRL-SED has been received. "
				"rCID " << rCid.print() << " > SK " << sessionKey << " not found"
				" in LTP CTRL-SED map");
		_sessionEndedResponsesUnicastMutex.unlock();
		return false;
	}

	state = skIt->second;
	_sessionEndedResponsesUnicastMutex.unlock();
	return state;
}

bool Lightweight::_ltpCtrlSedReceived(IcnId rCid, list<NodeId> nodeIds,
		enigma_t enigma,  sk_t sessionKey)
{
	ltp_hdr_ctrl_se_t ltpHeaderCtrlSe;
	ltpHeaderCtrlSe.enigma = enigma;
	ltpHeaderCtrlSe.sessionKey = sessionKey;
	unordered_map<sk_t, bool>::iterator skIt;
	unordered_map<nid_t, unordered_map<sk_t, bool>>::iterator nidIt;
	unordered_map<enigma_t, unordered_map<nid_t, unordered_map<sk_t,
			bool>>>::iterator enigmaIt;
	// Now start SE timer
	uint8_t attempts = ENIGMA;
	uint16_t rtt = _rtt();
	boost::posix_time::ptime startStartTime =
					boost::posix_time::microsec_clock::local_time();

	while (attempts != 0)
	{
		// check boolean continuously for 2 * RRT (timeout)
		boost::posix_time::ptime startTime =
				boost::posix_time::microsec_clock::local_time();
		boost::posix_time::ptime currentTime = startTime;
		boost::posix_time::time_duration timeWaited;
		timeWaited = currentTime - startTime;

		// check for received WED until _timeout * RTT has been reached
		while (timeWaited.total_milliseconds() < (_rttMultiplier * rtt))
		{
			currentTime = boost::posix_time::microsec_clock::local_time();
			timeWaited = currentTime - startTime;
			_sessionEndedResponsesMutex.lock();
			_sessionEndedResponsesIt = _sessionEndedResponses.find(rCid.uint());

			if (_sessionEndedResponsesIt == _sessionEndedResponses.end())
			{
				LOG4CXX_WARN(logger, "rCID " << rCid.print() << " does not "
						"exist in CTRL-SED map");
				_sessionEndedResponsesMutex.unlock();
				return false;
			}

			enigmaIt = _sessionEndedResponsesIt->second.find(enigma);
			uint16_t numberOfConfirmedSeds = 0;

			for (auto nodeIdsIt = nodeIds.begin(); nodeIdsIt != nodeIds.end();
					nodeIdsIt++)
			{
				nidIt = enigmaIt->second.find(nodeIdsIt->uint());

				// NID found
				if (nidIt != enigmaIt->second.end())
				{
					skIt = nidIt->second.find(ltpHeaderCtrlSe.sessionKey);

					// SK found
					if (skIt != nidIt->second.end())
					{
						// SED received
						if (skIt->second)
						{
							numberOfConfirmedSeds++;
						}
					}
				}
			}

			// Now check if the number of confirmed NIDs equals the CMC size
			if (numberOfConfirmedSeds == nodeIds.size())
			{
				timeWaited = currentTime - startStartTime;
				LOG4CXX_TRACE(logger, "All CTRL-SEDs received for rCID CMC "
						<< rCid.print() << " after waiting "
						<< timeWaited.total_milliseconds() << "ms");
				_rtt(timeWaited.total_milliseconds());
				_sessionEndedResponsesMutex.unlock();
				// cleaning up
				_deleteSessionEnd(rCid, ltpHeaderCtrlSe, nodeIds);
				return true;
			}

			_sessionEndedResponsesMutex.unlock();
		}

		LOG4CXX_TRACE(logger, "LTP CTRL-SED has not been received within given "
				"timeout of " << timeWaited.total_milliseconds() << "ms for "
				"rCID "	<< rCid.print() << " > enigma " << enigma
				<< " > SK " << sessionKey);
		_publishSessionEnd(rCid, nodeIds, ltpHeaderCtrlSe);
		attempts--;
	}

	LOG4CXX_DEBUG(logger, "Subscriber to rCID " << rCid.print()
			<< " has not responded to LTP CTRL-SE message after "
			<< (int)ENIGMA << " attempts. Stopping here");
	return false;
}

bool Lightweight::_ltpSessionActivityCheck(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey)
{
	unordered_map<enigma_t, unordered_map<sk_t, bool>>::iterator enigmaIt;
	unordered_map<sk_t, bool>::iterator skIt;
	bool status = false;
	ltp_session_activity_t::iterator it;
	_ltpSessionActivityMutex.lock();
	it = _ltpSessionActivity.find(rCid.uint());

	// rCID does not exist
	if (it == _ltpSessionActivity.end())
	{
		_ltpSessionActivityMutex.unlock();
		return status;
	}

	enigmaIt = it->second.find(enigma);

	// enigma does not exist
	if (enigmaIt == it->second.end())
	{
		_ltpSessionActivityMutex.unlock();
		return status;
	}

	skIt = enigmaIt->second.find(sessionKey);

	// SK does not exist
	if (skIt == enigmaIt->second.end())
	{
		_ltpSessionActivityMutex.unlock();
		return status;
	}

	status = skIt->second;
	_ltpSessionActivityMutex.unlock();
	return status;
}

void Lightweight::_ltpSessionActivitySet(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey, bool status)
{
	unordered_map<enigma_t, unordered_map<sk_t, bool>>::iterator enigmaIt;
	unordered_map<sk_t, bool>::iterator skIt;
	ltp_session_activity_t::iterator it;
	bool statusOld;
	_ltpSessionActivityMutex.lock();
	it = _ltpSessionActivity.find(rCid.uint());

	// rCID does not exist
	if (it == _ltpSessionActivity.end())
	{
		unordered_map<sk_t, bool> skMap;
		unordered_map<enigma_t, unordered_map<sk_t, bool>> enigmaMap;
		skMap.insert(pair<sk_t, bool>(sessionKey, status));
		enigmaMap.insert(pair<enigma_t, unordered_map<sk_t, bool>>(enigma,
				skMap));
		_ltpSessionActivity.insert(pair<cid_t, unordered_map<enigma_t,
				unordered_map<sk_t, bool>>>(rCid.uint(), enigmaMap));
		_ltpSessionActivityMutex.unlock();
		LOG4CXX_TRACE(logger, "LTP session activity for new rCID "
				<< rCid.print() << " > enigma " << enigma << " > SK "
				<< sessionKey << " set to " << status);
		return;
	}

	enigmaIt = it->second.find(enigma);

	// enigma does not exist
	if (enigmaIt == it->second.end())
	{
		unordered_map<sk_t, bool> skMap;
		skMap.insert(pair<sk_t, bool>(sessionKey, status));
		it->second.insert(pair<enigma_t, unordered_map<sk_t, bool>>(enigma,
				skMap));
		_ltpSessionActivityMutex.unlock();
		LOG4CXX_TRACE(logger, "LTP session activity for known rCID "
				<< rCid.print() << " but new enigma " << enigma << " > SK "
				<< sessionKey << " set to " << status);
		return;
	}

	skIt = enigmaIt->second.find(sessionKey);

	// SK does not exist
	if (skIt == enigmaIt->second.end())
	{
		enigmaIt->second.insert(pair<sk_t, bool>(sessionKey, status));
		_ltpSessionActivityMutex.unlock();
		LOG4CXX_TRACE(logger, "LTP session activity for known rCID "
				<< rCid.print() << " > enigma " << enigma << " but new SK "
				<< sessionKey << " set to " << status);
		return;
	}

	statusOld = skIt->second;
	skIt->second = status;
	_ltpSessionActivityMutex.unlock();
	LOG4CXX_TRACE(logger, "LTP session activity for known rCID "
			<< rCid.print() << " > enigma " << enigma << "> SK "
			<< sessionKey << " change from " << statusOld << " to " << status);
}

nack_group_t Lightweight::_nackGroup(IcnId &rCid,
		ltp_hdr_ctrl_nack_t &ltpHeaderNack)
{
	nack_group_t nackGroup;
	_nackGroupsIt = _nackGroups.find(rCid.uint());

	// rCID not found
	if (_nackGroupsIt == _nackGroups.end())
	{
		LOG4CXX_WARN(logger, "rCID " << rCid.print() << " does not exist in "
				"NACK group");
		return nackGroup;
	}

	map<enigma_t, map<sk_t, nack_group_t>>::iterator enigmaIt;
	enigmaIt = _nackGroupsIt->second.find(ltpHeaderNack.enigma);

	// enigma not found
	if (enigmaIt == _nackGroupsIt->second.end())
	{
		LOG4CXX_WARN(logger, "rCID " << rCid.print() << " > enigma "
				<< ltpHeaderNack.enigma << " does not exist in NACK "
						"group");
		return nackGroup;
	}

	map<sk_t, nack_group_t>::iterator skIt;
	skIt = enigmaIt->second.find(ltpHeaderNack.sessionKey);

	// SK not found
	if (skIt == enigmaIt->second.end())
	{
		LOG4CXX_WARN(logger, "rCID " << rCid.print() << " > enigma "
				<< ltpHeaderNack.enigma << " > SK "
				<< ltpHeaderNack.sessionKey << " does not exist in NACK group");
		return nackGroup;
	}

	return skIt->second;
}

void Lightweight::_publishData(IcnId &rCId, ltp_hdr_ctrl_we_t &ltpHeaderCtrlWe,
		list<NodeId> &nodeIds, uint8_t *data, uint16_t &dataSize)
{
	uint8_t attempts;
	ltp_hdr_data_t ltpHeaderData;
	uint32_t sentBytes = 0;
	uint16_t fragmentSize = 0;
	uint8_t *packet;
	uint16_t packetSize;
	uint8_t pad = 0;
	list<string> nodeIdsStr;
	list<NodeId>::iterator nodeIdsIt;
	ostringstream nodeIdsOss;
	uint16_t credit = _configuration.ltpInitialCredit();
	map<nid_t, bool>::iterator nidIt;
	map<sk_t, map<nid_t, bool>>::iterator skIt;
	map<enigma_t, map<sk_t, map<nid_t, bool>>>::iterator enigmaIt;
	// calculate the payload the LTP packet can carry
	uint32_t mitu = _configuration.mitu() - rCId.length() -
			_configuration.icnHeaderLength() - sizeof(ltp_hdr_data_t) - 20;
	mitu = mitu - (mitu % 8);//make it a multiple of 8
	ltpHeaderData.messageType = LTP_DATA;
	ltpHeaderData.enigma = ltpHeaderCtrlWe.enigma;
	ltpHeaderData.sessionKey = ltpHeaderCtrlWe.sessionKey;
	ltpHeaderData.sequenceNumber = 0;

	for (nodeIdsIt = nodeIds.begin(); nodeIdsIt != nodeIds.end(); nodeIdsIt++)
	{
		nodeIdsStr.push_back(nodeIdsIt->str());
		nodeIdsOss << nodeIdsIt->uint() << " ";
	}

	LOG4CXX_TRACE(logger, "CMC group size = " << nodeIds.size() << ": "
			<< nodeIdsOss.str());

	while (sentBytes < dataSize)
	{
		// Check if enough credit is left
		if (credit == 0)
		{
			attempts = ENIGMA;
			uint16_t rtt = _rtt();
			LOG4CXX_TRACE(logger, "Run out of credit for rCID " << rCId.print()
					<< " > enigma " << ltpHeaderData.enigma << " > SK "
					<< ltpHeaderData.sessionKey);
			list<NodeId> unconfirmedNids = nodeIds;
			_publishWindowUpdate(rCId, ltpHeaderData.enigma,
					ltpHeaderData.sessionKey, unconfirmedNids);

			while (!unconfirmedNids.empty())
			{
				boost::posix_time::ptime startTime =
						boost::posix_time::microsec_clock::local_time();
				boost::posix_time::ptime currentTime = startTime;
				boost::posix_time::time_duration timeWaited;
				timeWaited = currentTime - startTime;

				// continuously check if all WEDs have been received
				while (timeWaited.total_milliseconds() < (_rttMultiplier * rtt))
				{
					_windowUpdateMutex.lock();
					unconfirmedNids = _wudsNotReceived(rCId,
							ltpHeaderData.enigma,
							ltpHeaderData.sessionKey);

					if (unconfirmedNids.empty())
					{
						_windowUpdateMutex.unlock();
						break;
					}

					_windowUpdateMutex.unlock();
					currentTime =
							boost::posix_time::microsec_clock::local_time();
					timeWaited = currentTime - startTime;
				}

				// Timeout or all WUDs received. Simply check and resend if
				// necessary
				if (!unconfirmedNids.empty())
				{
					LOG4CXX_TRACE(logger, unconfirmedNids.size() << " NIDs did "
							"not reply with CTRL-WED for rCID " << rCId.print()
							<< " > enigma " << ltpHeaderData.enigma
							<< " > SK " << ltpHeaderData.sessionKey << " within"
							" " << timeWaited.total_milliseconds() << "ms");

					// try as long as necessary. If NID disappeared an LTP RST
					// is required from the outside
					if (attempts == 0)
					{
						ostringstream nids;
						list<NodeId>::iterator it;

						for (it = unconfirmedNids.begin();
								it != unconfirmedNids.end(); it++)
						{
							nids << it->uint() << " ";
						}

						LOG4CXX_DEBUG(logger, "NIDs " << nids.str() << " going "
								"to be removed from CMC group after "
								<< (int)ENIGMA << " attempts to request a "
										"WUD");
						_removeNidsFromCmcGroup(rCId, ltpHeaderData.enigma,
								ltpHeaderData.sessionKey, unconfirmedNids);
						unconfirmedNids.clear();
					}
					else
					{
						_publishWindowUpdate(rCId, ltpHeaderData.enigma,
								ltpHeaderData.sessionKey, unconfirmedNids);
						attempts--;
					}
				}
			}

			_removeNidsFromWindowUpdate(rCId, ltpHeaderData.enigma,
					ltpHeaderData.sessionKey, nodeIds);
			credit = _configuration.ltpInitialCredit();
		}

		ltpHeaderData.sequenceNumber++;

		// another fragment needed
		if ((dataSize - sentBytes) > mitu)
		{
			fragmentSize = mitu;
			ltpHeaderData.payloadLength = fragmentSize;
		}
		// last (or single) fragment
		else
		{
			fragmentSize = dataSize - sentBytes;
			ltpHeaderData.payloadLength = fragmentSize;
			// add padding bits, if necessary
			pad = (fragmentSize % 8);

			if (pad != 0)
			{
				pad = 8 - (fragmentSize % 8);
				fragmentSize += pad;
				LOG4CXX_TRACE(logger, (int)pad << " padding bits added to make "
						"fragment a multiple of 8 (" << fragmentSize << ")");
			}
		}

		packetSize = sizeof(ltp_hdr_data_t) + fragmentSize;
		uint8_t offsetPacket = 0;
		// Make the packet
		packet = (uint8_t *)malloc(packetSize);
		// [1] messageType;
		memcpy(packet, &ltpHeaderData.messageType,
				sizeof(ltpHeaderData.messageType));
		offsetPacket += sizeof(ltpHeaderData.messageType);
		// [2] enigma
		memcpy(packet + offsetPacket, &ltpHeaderData.enigma,
				sizeof(ltpHeaderData.enigma));
		offsetPacket += sizeof(ltpHeaderData.enigma);
		// [3] Session key
		memcpy(packet + offsetPacket,	&ltpHeaderData.sessionKey,
				sizeof(ltpHeaderData.sessionKey));
		offsetPacket += sizeof(ltpHeaderData.sessionKey);
		// [4] Sequence number
		memcpy(packet + offsetPacket, &ltpHeaderData.sequenceNumber,
				sizeof(ltpHeaderData.sequenceNumber));
		offsetPacket += sizeof(ltpHeaderData.sequenceNumber);
		// [5] Payload length;
		memcpy(packet + offsetPacket, &ltpHeaderData.payloadLength,
				sizeof(ltpHeaderData.payloadLength));
		offsetPacket += sizeof(ltpHeaderData.payloadLength);
		// now the actual packet
		memcpy(packet + offsetPacket, data + sentBytes, fragmentSize);
		// This blocks until the buffer for rCID > enigma > SK is empty again
		_bufferLtpPacket(rCId, ltpHeaderData, packet, packetSize);
#ifdef TRAFFIC_CONTROL
		// Check if TC drop rate should be applied
		if (!TrafficControl::handle())
		{
#endif
			_icnCoreMutex.lock();
			_icnCore->publish_data(rCId.binIcnId(), DOMAIN_LOCAL, NULL, 0,
					nodeIdsStr, packet, packetSize);
			_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
		}
#endif

		sentBytes += (fragmentSize - pad);// remove padding bits
		LOG4CXX_TRACE(logger, packetSize << " bytes published to "
				<< nodeIdsStr.size() << " cNAP(s) under rCID " << rCId.print()
				<< ", enigma " << ltpHeaderData.enigma << ", SK "
				<< ltpHeaderData.sessionKey << ", Sequence "
				<< ltpHeaderData.sequenceNumber << ", Credit " << credit << " ("
				<< sentBytes << "/" << dataSize << " sent)");
		credit--;// decrement credit by one
		free(packet);
	}

	ltpHeaderCtrlWe.sessionKey = ltpHeaderData.sessionKey;
	ltpHeaderCtrlWe.sequenceNumber = ltpHeaderData.sequenceNumber;
}

uint16_t Lightweight::_publishData(IcnId &cId, IcnId &rCId, enigma_t &enigma,
		sk_t &sessionKey, uint8_t *data, uint16_t &dataSize)
{
	seq_t sequenceNumber = 0;
	uint32_t sentBytes = 0;
	uint16_t fragmentSize = 0;
	uint8_t *packet;
	uint16_t packetSize;
	uint8_t pad = 0;
	uint16_t credit = _configuration.ltpInitialCredit();
	// calculate the payload the LTP packet can carry
	uint32_t mtu = _configuration.mtu() - cId.length() - rCId.length() -
			_configuration.icnHeaderLength() - sizeof(ltp_hdr_data_t) - 20;
	mtu = mtu - (mtu % 8);//make it a multiple of 8
	ltp_hdr_data_t ltpHeaderData;
	ltpHeaderData.messageType = LTP_DATA;
	ltpHeaderData.enigma = enigma;
	ltpHeaderData.sessionKey = sessionKey;// socket FD

	while (sentBytes < dataSize)
	{
		if (credit == 0)
		{
			LOG4CXX_INFO(logger, "WU + WUD has not been implemented for HTTP"
					"POST messages as this would block the code. Simply "
					"sleeping " << _rtt() << "ms");
			credit = _configuration.ltpInitialCredit();
//FIXME initialise credit properly (WU+WUD) before continue: blocking code!
			std::this_thread::sleep_for(std::chrono::milliseconds(_rtt()));
		}

		sequenceNumber++;

		// another fragment needed
		if ((dataSize - sentBytes) > mtu)
		{
			fragmentSize = mtu;
			ltpHeaderData.payloadLength = fragmentSize;
		}
		// last (or single) fragment
		else
		{
			fragmentSize = dataSize - sentBytes;
			ltpHeaderData.payloadLength = fragmentSize;
			// add padding bits, if necessary
			pad = (fragmentSize % 8);

			if (pad != 0)
			{
				pad = 8 - (fragmentSize % 8);
				fragmentSize += pad;
				LOG4CXX_TRACE(logger, (int)pad << " padding bits added to make "
						"fragment a multiple of 8 (" << fragmentSize << ")");
			}
		}

		ltpHeaderData.sequenceNumber = sequenceNumber;
		packetSize = sizeof(ltp_hdr_data_t) + fragmentSize;
		uint8_t offsetPacket = 0;
		// Make the packet
		packet = (uint8_t *)malloc(packetSize);
		// [1] messageType;
		memcpy(packet, &ltpHeaderData.messageType,
				sizeof(ltpHeaderData.messageType));
		offsetPacket += sizeof(ltpHeaderData.messageType);
		// [2] enigma
		memcpy(packet + offsetPacket, &ltpHeaderData.enigma,
				sizeof(ltpHeaderData.enigma));
		offsetPacket += sizeof(ltpHeaderData.enigma);
		// [3] Session key
		memcpy(packet + offsetPacket,	&ltpHeaderData.sessionKey,
				sizeof(ltpHeaderData.sessionKey));
		offsetPacket += sizeof(ltpHeaderData.sessionKey);
		// [4] Sequence number
		memcpy(packet + offsetPacket, &ltpHeaderData.sequenceNumber,
				sizeof(ltpHeaderData.sequenceNumber));
		offsetPacket += sizeof(ltpHeaderData.sequenceNumber);
		// [5] Payload length;
		memcpy(packet + offsetPacket, &ltpHeaderData.payloadLength,
				sizeof(ltpHeaderData.payloadLength));
		offsetPacket += sizeof(ltpHeaderData.payloadLength);
		// now the actual packet
		memcpy(packet + offsetPacket, data + sentBytes, fragmentSize);
#ifdef TRAFFIC_CONTROL
		// Check if TC drop rate should be applied
		if (!TrafficControl::handle())
		{
#endif
			_icnCoreMutex.lock();
			_icnCore->publish_data_isub(cId.binIcnId(), DOMAIN_LOCAL, NULL, 0,
					rCId.binIcnId(), packet, packetSize);
			_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
		}
#endif

		sentBytes += (fragmentSize - pad);// remove padding bits
		credit--;// decrement credit by one
		LOG4CXX_TRACE(logger, packetSize << " bytes published under CID "
				<< cId.print() << ", rCID " << rCId.print() << ", enigma "
				<< ltpHeaderData.enigma << ", SK "
				<< ltpHeaderData.sessionKey << ", Sequence "
				<< ltpHeaderData.sequenceNumber << ", Credit " << credit
				<< " ("	<< sentBytes << "/" << dataSize << " sent)");
		_bufferProxyPacket(cId, rCId, ltpHeaderData, packet,
				packetSize);
		free(packet);
	}

	_publishWindowEnd(cId, rCId, enigma, sessionKey, sequenceNumber);
	return sequenceNumber;
}

void Lightweight::_publishDataRange(IcnId &rCid,
		ltp_hdr_ctrl_nack_t &ltpCtrlNack)
{
	// First get the CID for the given rCID
	IcnId cid;
	_cIdReverseLookUpMutex.lock();
	_cIdReverseLookUpIt = _cIdReverseLookUp.find(rCid.uint());

	if (_cIdReverseLookUpIt == _cIdReverseLookUp.end())
	{
		LOG4CXX_ERROR(logger, "CID for rCID " << rCid.print() << " could not "
				"be found in reverse look-up map");
		_cIdReverseLookUpMutex.unlock();
		return;
	}

	cid = _cIdReverseLookUpIt->second;
	_cIdReverseLookUpMutex.unlock();
	_proxyPacketBufferMutex.lock();
	_proxyPacketBufferIt = _proxyPacketBuffer.find(rCid.uint());

	//rCID not found
	if (_proxyPacketBufferIt == _proxyPacketBuffer.end())
	{
		LOG4CXX_WARN(logger, "rCID " << rCid.print() << " does not exist in LTP"
				" proxy buffer");
		_proxyPacketBufferMutex.unlock();
		return;
	}

	map<enigma_t, map<sk_t, map<seq_t, pair<uint8_t *, uint16_t>>>>::iterator
			enigmaIt;
	enigmaIt = _proxyPacketBufferIt->second.find(ltpCtrlNack.enigma);

	// enigma not found
	if (enigmaIt == _proxyPacketBufferIt->second.end())
	{
		LOG4CXX_WARN(logger, "enigma " << ltpCtrlNack.enigma << " does not "
				"exist in LTP proxy buffer for rCID " << rCid.print());
		_proxyPacketBufferMutex.unlock();
		return;
	}

	map<sk_t, map<seq_t, pair<uint8_t *, uint16_t>>>::iterator skIt;
	skIt = enigmaIt->second.find(ltpCtrlNack.sessionKey);

	if (skIt == enigmaIt->second.end())
	{
		LOG4CXX_WARN(logger, "SK " << ltpCtrlNack.sessionKey << " does not "
				"exist in LTP proxy buffer for rCID " << rCid.print()
				<< " > enigma " << ltpCtrlNack.enigma);
		_proxyPacketBufferMutex.unlock();
		return;
	}

	seq_t sequence = 0;
	list<string> nodeIds;
	nodeIds.push_back(_configuration.nodeId().str());
	map<seq_t, pair<uint8_t *, uint16_t>>::iterator snIt;

	// iterate over the LTP buffer and re-publish the range of fragments
	for (sequence = ltpCtrlNack.start; sequence <= ltpCtrlNack.end; sequence++)
	{
		snIt = skIt->second.find(sequence);

		if (snIt == skIt->second.end())
		{
			LOG4CXX_ERROR(logger, "Sequence number " << sequence << " is "
					"missing in LTP buffer for rCID " << rCid.print()
					<< " > enigma " << ltpCtrlNack.enigma << " > SK "
					<< ltpCtrlNack.sessionKey);
			break;
		}
#ifdef TRAFFIC_CONTROL
		if (!TrafficControl::handle())
		{
#endif
			_icnCoreMutex.lock();
			_icnCore->publish_data(rCid.binIcnId(), DOMAIN_LOCAL, NULL, 0,
					nodeIds, snIt->second.first, snIt->second.second);
			_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
		}
#endif
		LOG4CXX_TRACE(logger, "Packet of total length " << snIt->second.second
				<< " with Sequence " << sequence << " re-published under rCID "
				<< rCid.print() << " > enigma " << ltpCtrlNack.enigma
				<< " > SK " << ltpCtrlNack.sessionKey);
	}

	_proxyPacketBufferMutex.unlock();
}

void Lightweight::_publishDataRange(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey, nack_group_t &nackGroup)
{
	map<cid_t, map<enigma_t, map<sk_t, map<seq_t, packet_t>>>>::iterator rCidIt;
	map<enigma_t, map<sk_t, map<seq_t, packet_t>>>::iterator enigmaIt;
	map<sk_t, map<seq_t, packet_t>>::iterator skIt;
	map<seq_t, packet_t>::iterator snIt;
	_ltpPacketBufferMutex.lock();
	rCidIt = _ltpPacketBuffer.find(rCid.uint());

	// rCID not found
	if (rCidIt == _ltpPacketBuffer.end())
	{
		LOG4CXX_ERROR(logger, "rCID " << rCid.print() << " not found in LTP "
				"packet buffer");
		_ltpPacketBufferMutex.unlock();
		return;
	}

	enigmaIt = rCidIt->second.find(enigma);

	// enigma not found
	if (enigmaIt == rCidIt->second.end())
	{
		LOG4CXX_ERROR(logger, "rCID " << rCid.print() << " > enigma "
				<< enigma << " not found in LTP packet buffer");
		_ltpPacketBufferMutex.unlock();
		return;
	}

	skIt = enigmaIt->second.find(sessionKey);

	// SK not found
	if (skIt == enigmaIt->second.end())
	{
		LOG4CXX_ERROR(logger, "rCID " << rCid.print() << " > enigma "
				<< enigma << " > SK " << sessionKey << " not found in LTP "
				"packet buffer");
		_ltpPacketBufferMutex.unlock();
		return;
	}

	// convert list<NodeId> into list<string>
	list<string> nodeIds;
	ostringstream nodeIdsOss;

	for (list<NodeId>::iterator it = nackGroup.nodeIds.begin();
			it != nackGroup.nodeIds.end(); it++)
	{
		nodeIds.push_back(it->str());
		nodeIdsOss << it->uint() << " ";
	}

	// Iterate over the range of sequence number for rCID > enigma > SK and
	// re-publish the packets
	for (seq_t sequence = nackGroup.startSequence;
			sequence <= nackGroup.endSequence; sequence++)
	{
		snIt = skIt->second.find(sequence);

		if (snIt == skIt->second.end())
		{
			LOG4CXX_ERROR(logger, "Sequence number " << sequence << " is "
					"missing in LTP buffer for rCID " << rCid.print()
					<< " > enigma " << enigma << " > SK " << sessionKey
					<< ". Total number of packets: " << skIt->second.size());
			break;
		}

#ifdef TRAFFIC_CONTROL
		if (!TrafficControl::handle())
		{
#endif
			_icnCoreMutex.lock();
			_icnCore->publish_data(rCid.binIcnId(), DOMAIN_LOCAL, NULL, 0,
					nodeIds, snIt->second.packet, snIt->second.packetSize);
			_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
		}
#endif

		LOG4CXX_TRACE(logger, "Packet of total length "
				<< snIt->second.packetSize << " with Sequence " << sequence
				<< " re-published under rCID " << rCid.print() << " > enigma "
				<< enigma << " > SK " << sessionKey << " to "
				<< nodeIds.size() << " NIDs: " << nodeIdsOss.str());
	}

	_ltpPacketBufferMutex.unlock();
}

void Lightweight::_publishNegativeAcknowledgement(IcnId &rCid, NodeId &nodeId,
		enigma_t &enigma, sk_t &sessionKey, seq_t &firstMissingSequence,
		seq_t &lastMissingSequence)
{
	ltp_hdr_ctrl_nack_t ltpHeaderControlNack;
	ltpHeaderControlNack.messageType = LTP_CONTROL;
	ltpHeaderControlNack.controlType = LTP_CONTROL_NACK;
	ltpHeaderControlNack.enigma = enigma;
	ltpHeaderControlNack.sessionKey = sessionKey;
	ltpHeaderControlNack.start = firstMissingSequence;
	ltpHeaderControlNack.end = lastMissingSequence;
	uint8_t *packet = (uint8_t *)malloc(sizeof(ltp_hdr_ctrl_nack_t));
	uint8_t offset = 0;
	// [1] Message type
	memcpy(packet, &ltpHeaderControlNack.messageType,
			sizeof(ltpHeaderControlNack.messageType));
	offset += sizeof(ltpHeaderControlNack.messageType);
	// [2] Control type
	memcpy(packet + offset, &ltpHeaderControlNack.controlType,
			sizeof(ltpHeaderControlNack.controlType));
	offset += sizeof(ltpHeaderControlNack.controlType);
	// [3] enigma
	memcpy(packet + offset, &ltpHeaderControlNack.enigma,
			sizeof(ltpHeaderControlNack.enigma));
	offset += sizeof(ltpHeaderControlNack.enigma);
	// [4] SK
	memcpy(packet + offset, &ltpHeaderControlNack.sessionKey,
			sizeof(ltpHeaderControlNack.sessionKey));
	offset += sizeof(ltpHeaderControlNack.sessionKey);
	// [5] Start sequence number
	memcpy(packet + offset, &ltpHeaderControlNack.start,
			sizeof(ltpHeaderControlNack.start));
	offset += sizeof(ltpHeaderControlNack.start);
	// [6] End sequence number
	memcpy(packet + offset, &ltpHeaderControlNack.end,
			sizeof(ltpHeaderControlNack.end));
	list<string> nodeIds;
	nodeIds.push_back(nodeId.str());

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data(rCid.binIcnId(), DOMAIN_LOCAL, NULL, 0, nodeIds,
				packet, sizeof(ltp_hdr_ctrl_nack_t));
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "CTRL-NACK for Sequence range "
			<< firstMissingSequence << " - " << lastMissingSequence
			<< " published to rCID " << rCid.print() << " > NID "
			<< nodeId.uint() << " > enigma " << enigma << " > SK "
			<< sessionKey);
	free(packet);
}

void Lightweight::_publishNegativeAcknowledgement(IcnId &cid, IcnId &rCid,
		enigma_t &enigma, sk_t &sessionKey, seq_t &firstMissingSequence,
		seq_t &lastMissingSequence)
{
	ltp_hdr_ctrl_nack_t ltpHeaderControlNack;
	ltpHeaderControlNack.messageType = LTP_CONTROL;
	ltpHeaderControlNack.controlType = LTP_CONTROL_NACK;
	ltpHeaderControlNack.enigma = enigma;
	ltpHeaderControlNack.sessionKey = sessionKey;
	ltpHeaderControlNack.start = firstMissingSequence;
	ltpHeaderControlNack.end = lastMissingSequence;
	uint8_t *packet = (uint8_t *)malloc(sizeof(ltp_hdr_ctrl_nack_t));
	uint8_t offset = 0;
	// [1] Message type
	memcpy(packet, &ltpHeaderControlNack.messageType,
			sizeof(ltpHeaderControlNack.messageType));
	offset += sizeof(ltpHeaderControlNack.messageType);
	// [2] Control type
	memcpy(packet + offset, &ltpHeaderControlNack.controlType,
			sizeof(ltpHeaderControlNack.controlType));
	offset += sizeof(ltpHeaderControlNack.controlType);
	// [3] enigma
	memcpy(packet + offset, &ltpHeaderControlNack.enigma,
			sizeof(ltpHeaderControlNack.enigma));
	offset += sizeof(ltpHeaderControlNack.enigma);
	// [4] SK
	memcpy(packet + offset, &ltpHeaderControlNack.sessionKey,
			sizeof(ltpHeaderControlNack.sessionKey));
	offset += sizeof(ltpHeaderControlNack.sessionKey);
	// [5] Start sequence number
	memcpy(packet + offset, &ltpHeaderControlNack.start,
			sizeof(ltpHeaderControlNack.start));
	offset += sizeof(ltpHeaderControlNack.start);
	// [6] End sequence number
	memcpy(packet + offset, &ltpHeaderControlNack.end,
			sizeof(ltpHeaderControlNack.end));

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data_isub(cid.binIcnId(), DOMAIN_LOCAL, NULL, 0,
					rCid.binIcnId(), packet, sizeof(ltp_hdr_ctrl_nack_t));
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "CTRL-NACK for Sequence range "
			<< firstMissingSequence << " - " << lastMissingSequence
			<< " published to CID " << cid.print() << " for rCID "
			<< rCid.print() << " > enigma " << enigma << " > SK "
			<< sessionKey);
	free(packet);
}

void Lightweight::_publishReset(IcnId &cid, IcnId &rCid)
{
	ltp_hdr_ctrl_rst_t ltpHeaderCtrlRst;
	uint16_t packetSize = sizeof(ltp_hdr_ctrl_rst_t);
	uint8_t *packet = (uint8_t *)malloc(packetSize);
	uint8_t offset = 0;
	// [1] Message type
	memcpy(packet, &ltpHeaderCtrlRst.messageType,
			sizeof(ltpHeaderCtrlRst.messageType));
	offset += sizeof(ltpHeaderCtrlRst.messageType);
	// [2] Control type
	memcpy(packet + offset, &ltpHeaderCtrlRst.controlType,
			sizeof(ltpHeaderCtrlRst.controlType));

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data_isub(cid.binIcnId(), DOMAIN_LOCAL, NULL, 0,
				rCid.binIcnId(), packet, packetSize);
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "CTRL-RST (" << ltpHeaderCtrlRst.messageType << ", "
			<< ltpHeaderCtrlRst.controlType << ") published under "
			<< cid.print() << " for rCID " << rCid.print());
	free(packet);
}

void Lightweight::_publishResetted(IcnId &rCid, NodeId &nodeId)
{
	list<string> nodeIdList;
	nodeIdList.push_back(nodeId.str());
	ltp_hdr_ctrl_rsted_t ltpHeaderCtrlRsted;
	uint16_t packetSize = sizeof(ltp_hdr_ctrl_rsted_t);
	uint8_t *packet = (uint8_t *)malloc(packetSize);
	uint8_t offset = 0;
	// [1] Message type
	memcpy(packet, &ltpHeaderCtrlRsted.messageType,
			sizeof(ltpHeaderCtrlRsted.messageType));
	offset += sizeof(ltpHeaderCtrlRsted.messageType);
	// [2] Control type
	memcpy(packet + offset, &ltpHeaderCtrlRsted.controlType,
			sizeof(ltpHeaderCtrlRsted.controlType));

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data(rCid.binIcnId(), DOMAIN_LOCAL, NULL, 0,
				nodeIdList, packet, packetSize);
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "CTRL-RSTED published under " << rCid.print() << " > "
			"NID " << nodeId.uint());
	free(packet);
}

void Lightweight::_publishSessionEnd(IcnId &rCid, list<NodeId> &nodeIds,
		ltp_hdr_ctrl_se_t &ltpHeaderCtrlSe)
{
	list<string> nodeIdsStr;
	ostringstream nodeIdsOss;
	uint16_t packetSize = sizeof(ltp_hdr_ctrl_se_t);
	uint8_t *packet = (uint8_t *)malloc(packetSize);
	uint8_t offset = 0;
	//ltpHeaderCtrlSe.messageType = LTP_CONTROL;
	//ltpHeaderCtrlSe.controlType = LTP_CONTROL_SESSION_END;
	// [1] Message type
	memcpy(packet, &ltpHeaderCtrlSe.messageType,
			sizeof(ltpHeaderCtrlSe.messageType));
	offset += sizeof(ltpHeaderCtrlSe.messageType);
	// [2] Control type
	memcpy(packet + offset, &ltpHeaderCtrlSe.controlType,
			sizeof(ltpHeaderCtrlSe.controlType));
	offset += sizeof(ltpHeaderCtrlSe.controlType);
	// [3] enigma
	memcpy(packet + offset, &ltpHeaderCtrlSe.enigma,
			sizeof(ltpHeaderCtrlSe.enigma));
	offset += sizeof(ltpHeaderCtrlSe.enigma);
	// [4] SK
	memcpy(packet + offset, &ltpHeaderCtrlSe.sessionKey,
			sizeof(ltpHeaderCtrlSe.sessionKey));

	// convert list of NIDs into strings
	for (list<NodeId>::iterator it = nodeIds.begin(); it != nodeIds.end(); it++)
	{
		nodeIdsStr.push_back(it->str());
		nodeIdsOss << it->uint() << " ";
	}

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
	_icnCoreMutex.lock();
	_icnCore->publish_data(rCid.binIcnId(), DOMAIN_LOCAL, NULL, 0, nodeIdsStr,
			packet, packetSize);
	_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "CTRL-SE published to " << nodeIds.size() << " NIDs "
			"under rCID " << rCid.print() << " > enigma "
			<< ltpHeaderCtrlSe.enigma << " > SK "
			<< ltpHeaderCtrlSe.sessionKey << ": " << nodeIdsOss.str());
	free(packet);
}

void Lightweight::_publishSessionEnd(IcnId cid, IcnId rCid, sk_t sessionKey)
{
	ltp_hdr_ctrl_se_t ltpHeaderCtrlSe;
	ltpHeaderCtrlSe.enigma = 0;
	ltpHeaderCtrlSe.sessionKey = sessionKey;
	uint16_t packetSize = sizeof(ltp_hdr_ctrl_se_t);
	uint8_t *packet = (uint8_t *)malloc(packetSize);
	uint8_t offset = 0;
	// [1] Message type
	memcpy(packet, &ltpHeaderCtrlSe.messageType,
			sizeof(ltpHeaderCtrlSe.messageType));
	offset += sizeof(ltpHeaderCtrlSe.messageType);
	// [2] Control type
	memcpy(packet + offset, &ltpHeaderCtrlSe.controlType,
			sizeof(ltpHeaderCtrlSe.controlType));
	offset += sizeof(ltpHeaderCtrlSe.controlType);
	// [3] enigma
	memcpy(packet + offset, &ltpHeaderCtrlSe.enigma,
			sizeof(ltpHeaderCtrlSe.enigma));
	offset += sizeof(ltpHeaderCtrlSe.enigma);
	// [4] SK
	memcpy(packet + offset, &ltpHeaderCtrlSe.sessionKey,
			sizeof(ltpHeaderCtrlSe.sessionKey));

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data_isub(cid.binIcnId(), DOMAIN_LOCAL, NULL, 0,
				rCid.binIcnId(), packet, packetSize);
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "CTRL-SE published under " << cid.print() << " / rCID"
			" " << rCid.print() << " > enigma " << ltpHeaderCtrlSe.enigma
			<< " > SK " << ltpHeaderCtrlSe.sessionKey);
	free(packet);
}

void Lightweight::_publishSessionEnded(IcnId &rCid, NodeId &nodeId,
		sk_t &sessionKey)
{
	ltp_hdr_ctrl_sed_t ltpHdrCtrlSed;
	uint8_t *data;
	uint8_t offset = 0;
	list<string> nodeIds;
	nodeIds.push_back(nodeId.str());
	uint16_t dataSize = sizeof(ltpHdrCtrlSed);
	ltpHdrCtrlSed.enigma = 0;
	ltpHdrCtrlSed.sessionKey = sessionKey;
	data = (uint8_t *)malloc(dataSize);
	// [1] Message type
	memcpy(data, &ltpHdrCtrlSed.messageType, sizeof(ltpHdrCtrlSed.messageType));
	offset += sizeof(ltpHdrCtrlSed.messageType);
	// [2] Control type
	memcpy(data + offset, &ltpHdrCtrlSed.controlType,
			sizeof(ltpHdrCtrlSed.controlType));
	offset += sizeof(ltpHdrCtrlSed.controlType);
	// [3] enigma
	memcpy(data + offset, &ltpHdrCtrlSed.enigma,
			sizeof(ltpHdrCtrlSed.enigma));
	offset += sizeof(ltpHdrCtrlSed.enigma);
	// [4] SK
	memcpy(data + offset, &ltpHdrCtrlSed.sessionKey,
			sizeof(ltpHdrCtrlSed.sessionKey));

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data(rCid.binIcnId(), DOMAIN_LOCAL, NULL, 0, nodeIds,
				data, dataSize);
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "CTRL-SED published to cNAP with NID "
			<< nodeId.uint() << " under rCID " << rCid.print() << " > enigma "
			<< ltpHdrCtrlSed.enigma << " > SK "
			<< ltpHdrCtrlSed.sessionKey);
	free(data);
}

void Lightweight::_publishSessionEnded(IcnId &cid, IcnId &rCid,
		enigma_t &enigma, sk_t &sessionKey)
{
	ltp_hdr_ctrl_sed_t ltpHdrCtrlSed;
	uint8_t *data;
	uint8_t offset = 0;
	uint16_t dataSize = sizeof(ltpHdrCtrlSed);
	//ltpHdrCtrlSed.messageType = LTP_CONTROL;
	//ltpHdrCtrlSed.controlType = LTP_CONTROL_SESSION_ENDED;
	ltpHdrCtrlSed.enigma = enigma;
	ltpHdrCtrlSed.sessionKey = sessionKey;
	data = (uint8_t *)malloc(dataSize);
	// [1] Message type
	memcpy(data, &ltpHdrCtrlSed.messageType, sizeof(ltpHdrCtrlSed.messageType));
	offset += sizeof(ltpHdrCtrlSed.messageType);
	// [2] Control type
	memcpy(data + offset, &ltpHdrCtrlSed.controlType,
			sizeof(ltpHdrCtrlSed.controlType));
	offset += sizeof(ltpHdrCtrlSed.controlType);
	// [3] enigma
	memcpy(data + offset, &ltpHdrCtrlSed.enigma,
			sizeof(ltpHdrCtrlSed.enigma));
	offset += sizeof(ltpHdrCtrlSed.enigma);
	// [4] SK
	memcpy(data + offset, &ltpHdrCtrlSed.sessionKey,
			sizeof(ltpHdrCtrlSed.sessionKey));

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data_isub(cid.binIcnId(), DOMAIN_LOCAL, NULL, 0,
					rCid.binIcnId(), data, dataSize);
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "CTRL-SED published to CID " << cid.print() << " for "
			"rCID "	<< rCid.print() << " > enigma " << enigma << " > SK "
			<< sessionKey);
	free(data);
}

void Lightweight::_publishWindowEnd(IcnId &rCId, list<NodeId> &nodeIds,
			ltp_hdr_ctrl_we_t &ltpHeader)
{
	list<string> nodeIdsStr;
	list<NodeId>::iterator nodeIdsIt;
	uint8_t *packet;
	uint16_t packetSize = sizeof(ltp_hdr_ctrl_we_t);
	for (nodeIdsIt = nodeIds.begin(); nodeIdsIt != nodeIds.end(); nodeIdsIt++)
	{
		nodeIdsStr.push_back(nodeIdsIt->str());
	}
	// Fill up the LTP CTRL header
	ltpHeader.messageType = LTP_CONTROL;
	ltpHeader.controlType = LTP_CONTROL_WINDOW_END;
	// first create WED boolean so that there's no potential raise condition
	_addWindowEnd(rCId, nodeIds, ltpHeader);
	// make packet
	packet = (uint8_t *)malloc(packetSize);
	// [1] messageType
	memcpy(packet, &ltpHeader.messageType, sizeof(ltpHeader.messageType));
	// [2] controlType
	memcpy(packet + sizeof(ltpHeader.messageType), &ltpHeader.controlType,
			sizeof(ltpHeader.controlType));
	// [3] enigma
	memcpy(packet + sizeof(ltpHeader.messageType)
			+ sizeof(ltpHeader.controlType), &ltpHeader.enigma,
			sizeof(ltpHeader.enigma));
	// [4] session key
	memcpy(packet + sizeof(ltpHeader.messageType)
			+ sizeof(ltpHeader.controlType) + sizeof(ltpHeader.enigma),
			&ltpHeader.sessionKey,	sizeof(ltpHeader.sessionKey));
	// [5] sequenceNumber
	memcpy(packet + sizeof(ltpHeader.messageType)
				+ sizeof(ltpHeader.controlType) + sizeof(ltpHeader.enigma)
				+ sizeof(ltpHeader.sessionKey), &ltpHeader.sequenceNumber,
				sizeof(ltpHeader.sequenceNumber));

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data(rCId.binIcnId(), DOMAIN_LOCAL, NULL, 0,
				nodeIdsStr, packet, packetSize);
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "LTP CTRL-WE published under " << rCId.print()
			<< " > enigma " << ltpHeader.enigma << " > SK "
			<< ltpHeader.sessionKey << " > Sequence "
			<< ltpHeader.sequenceNumber);

	free(packet);
}

void Lightweight::_publishWindowEnd(IcnId &cId, IcnId &rCId,
		enigma_t &enigma, sk_t &sessionKey, seq_t &sequenceNumber)
{
	ltp_hdr_ctrl_we_t ltpHeader;
	uint8_t *packet;
	uint16_t packetSize = sizeof(ltp_hdr_ctrl_we_t);
	// Fill up the LTP CTRL header
	ltpHeader.messageType = LTP_CONTROL;
	ltpHeader.controlType = LTP_CONTROL_WINDOW_END;
	ltpHeader.enigma = enigma;
	ltpHeader.sessionKey = sessionKey;
	ltpHeader.sequenceNumber = sequenceNumber;
	// make packet
	packet = (uint8_t *)malloc(packetSize);
	// [1] messageType
	memcpy(packet, &ltpHeader.messageType, sizeof(ltpHeader.messageType));
	// [2] controlType
	memcpy(packet + sizeof(ltpHeader.messageType), &ltpHeader.controlType,
			sizeof(ltpHeader.controlType));
	// [3] enigma
	memcpy(packet + sizeof(ltpHeader.messageType)
			+ sizeof(ltpHeader.controlType), &ltpHeader.enigma,
			sizeof(ltpHeader.enigma));
	// [4] session key
	memcpy(packet + sizeof(ltpHeader.messageType)
			+ sizeof(ltpHeader.controlType) + sizeof(ltpHeader.enigma),
			&ltpHeader.sessionKey,	sizeof(ltpHeader.sessionKey));
	// [5] sequenceNumber
	memcpy(packet + sizeof(ltpHeader.messageType)
				+ sizeof(ltpHeader.controlType) + sizeof(ltpHeader.enigma)
				+ sizeof(ltpHeader.sessionKey), &ltpHeader.sequenceNumber,
				sizeof(ltpHeader.sequenceNumber));

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data_isub(cId.binIcnId(), DOMAIN_LOCAL, NULL, 0,
				rCId.binIcnId(), packet, packetSize);
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "LTP CTRL-WE published under " << cId.print()
			<< " > enigma " << enigma << " > SK "
			<< ltpHeader.sessionKey << " > Sequence " << sequenceNumber);
	_addWindowEnd(rCId, ltpHeader);
	free(packet);
}

void Lightweight::_publishWindowEnded(IcnId &cid, IcnId &rCid,
			ltp_hdr_ctrl_wed_t &ltpHeaderCtrlWed)
{
	uint8_t *packet = (uint8_t *)malloc(sizeof(ltp_hdr_ctrl_wed_t));
	uint8_t offset = 0;
	ltpHeaderCtrlWed.messageType = LTP_CONTROL;
	ltpHeaderCtrlWed.controlType = LTP_CONTROL_WINDOW_ENDED;
	// [1] message type
	memcpy(packet, &ltpHeaderCtrlWed.messageType,
			sizeof(ltpHeaderCtrlWed.messageType));
	offset += sizeof(ltpHeaderCtrlWed.messageType);
	// [2] control type
	memcpy(packet + offset, &ltpHeaderCtrlWed.controlType,
			sizeof(ltpHeaderCtrlWed.controlType));
	offset += sizeof(ltpHeaderCtrlWed.controlType);
	// [3] enigma
	memcpy(packet + offset, &ltpHeaderCtrlWed.enigma,
			sizeof(ltpHeaderCtrlWed.enigma));
	offset += sizeof(ltpHeaderCtrlWed.enigma);
	// [4] Session key
	memcpy(packet + offset, &ltpHeaderCtrlWed.sessionKey,
			sizeof(ltpHeaderCtrlWed.sessionKey));

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data_isub(cid.binIcnId(), DOMAIN_LOCAL, NULL, 0,
				rCid.binIcnId(), packet, sizeof(ltp_hdr_ctrl_wed_t));
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "LTP CTRL-WED published under CID "
			<< cid.print() << " with rCID " << rCid.print() << " > enigma "
			<< ltpHeaderCtrlWed.enigma << " > SK "
			<< ltpHeaderCtrlWed.sessionKey);
	free(packet);
}

void Lightweight::_publishWindowEnded(IcnId &rCId, NodeId &nodeId,
		enigma_t &enigma, sk_t &sessionKey)
{
	if (!_forwardingEnabled(nodeId))
	{
		LOG4CXX_DEBUG(logger, "WED message could not be published. Forwarding "
				"state for NID " << nodeId.uint() << " is still disabled");
		return;
	}

	ltp_hdr_ctrl_wed_t ltpHeader;
	list<string> nodeIdList;
	uint8_t *packet = (uint8_t *)malloc(sizeof(ltp_hdr_ctrl_wed_t));
	ltpHeader.messageType = LTP_CONTROL;
	ltpHeader.controlType = LTP_CONTROL_WINDOW_ENDED;
	ltpHeader.enigma = enigma;
	ltpHeader.sessionKey = sessionKey;
	// [1] message type
	memcpy(packet, &ltpHeader.messageType, sizeof(ltpHeader.messageType));
	// [2] control type
	memcpy(packet + sizeof(ltpHeader.messageType), &ltpHeader.controlType,
			sizeof(ltpHeader.controlType));
	// [3] enigma
	memcpy(packet + sizeof(ltpHeader.messageType)
			+ sizeof(ltpHeader.controlType), &ltpHeader.enigma,
			sizeof(ltpHeader.enigma));
	// [4] Session key
	memcpy(packet + sizeof(ltpHeader.messageType)
			+ sizeof(ltpHeader.controlType) + sizeof(ltpHeader.enigma),
			&ltpHeader.sessionKey, sizeof(ltpHeader.sessionKey));
	nodeIdList.push_back(nodeId.str());

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data(rCId.binIcnId(), DOMAIN_LOCAL, NULL, 0,
				nodeIdList, packet, sizeof(ltp_hdr_ctrl_wed_t));
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif
	LOG4CXX_TRACE(logger, "LTP CTRL-WED published to NID " << nodeId.uint()
			<< " under rCID " << rCId.print() << " > enigma "
			<< ltpHeader.enigma << " > SK " << ltpHeader.sessionKey);
	free(packet);
}

void Lightweight::_publishWindowUpdate(IcnId &rCid, enigma_t &enigma,
		sk_t &sessionKey, list<NodeId> &nodeIds)
{
	list<string> nodeIdsStr;
	list<NodeId>::iterator nodeIdsIt;
	ostringstream oss;

	for (nodeIdsIt = nodeIds.begin(); nodeIdsIt != nodeIds.end(); nodeIdsIt++)
	{
		if (!_forwardingEnabled(*nodeIdsIt))
		{
			LOG4CXX_DEBUG(logger, "Forwarding state of NID "
					<< nodeIdsIt->uint() << " is currently disabled.");
		}
		else
		{
			_addNodeIdToWindowUpdate(rCid, enigma, sessionKey, *nodeIdsIt);
			nodeIdsStr.push_back(nodeIdsIt->str());
			oss << nodeIdsIt->uint() << " ";
		}
	}

	if (nodeIdsStr.empty())
	{
		LOG4CXX_WARN(logger, "All NIDs are not reachable");
		return;
	}

	ltp_hdr_ctrl_wu_t ltpHeader;
	uint8_t *packet = (uint8_t *)malloc(sizeof(ltp_hdr_ctrl_wu_t));
	ltpHeader.messageType = LTP_CONTROL;
	ltpHeader.controlType = LTP_CONTROL_WINDOW_UPDATE;
	ltpHeader.enigma = enigma;
	ltpHeader.sessionKey = sessionKey;
	// [1] message type
	memcpy(packet, &ltpHeader.messageType, sizeof(ltpHeader.messageType));
	// [2] control type
	memcpy(packet + sizeof(ltpHeader.messageType), &ltpHeader.controlType,
			sizeof(ltpHeader.controlType));
	// [3] enigma
	memcpy(packet + sizeof(ltpHeader.messageType)
			+ sizeof(ltpHeader.controlType), &ltpHeader.enigma,
			sizeof(ltpHeader.enigma));
	// [4] Session key
	memcpy(packet + sizeof(ltpHeader.messageType)
			+ sizeof(ltpHeader.controlType) + sizeof(ltpHeader.enigma),
			&ltpHeader.sessionKey, sizeof(ltpHeader.sessionKey));

#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data(rCid.binIcnId(), DOMAIN_LOCAL, NULL, 0,
				nodeIdsStr, packet, sizeof(ltp_hdr_ctrl_wu_t));
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "LTP CTRL-WU published under rCID "
			<< rCid.print() << " > enigma " << enigma << " > SK "
			<< sessionKey << " to " << nodeIds.size() << " NID(s): "
			<< oss.str());
	free(packet);
}

void Lightweight::_publishWindowUpdated(IcnId &cid, IcnId &rCid,
		ltp_hdr_ctrl_wud_t &ltpHeaderControlWud)
{
	uint8_t *packet = (uint8_t *)malloc(sizeof(ltp_hdr_ctrl_wud_t));
	uint8_t offset = 0;
	// [1] message type
	memcpy(packet, &ltpHeaderControlWud.messageType,
			sizeof(ltpHeaderControlWud.messageType));
	offset += sizeof(ltpHeaderControlWud.messageType);
	// [2] control type
	memcpy(packet + offset, &ltpHeaderControlWud.controlType,
			sizeof(ltpHeaderControlWud.controlType));
	offset += sizeof(ltpHeaderControlWud.controlType);
	// [3] enigma
	memcpy(packet + offset, &ltpHeaderControlWud.enigma,
			sizeof(ltpHeaderControlWud.enigma));
	offset += sizeof(ltpHeaderControlWud.enigma);
	// [4] Session key
	memcpy(packet + offset, &ltpHeaderControlWud.sessionKey,
			sizeof(ltpHeaderControlWud.sessionKey));
#ifdef TRAFFIC_CONTROL
	if (!TrafficControl::handle())
	{
#endif
		_icnCoreMutex.lock();
		_icnCore->publish_data_isub(cid.binIcnId(), DOMAIN_LOCAL, NULL, 0,
				rCid.binIcnId(), packet, sizeof(ltp_hdr_ctrl_wud_t));
		_icnCoreMutex.unlock();
#ifdef TRAFFIC_CONTROL
	}
#endif

	LOG4CXX_TRACE(logger, "LTP CTRL-WUD published under CID "
			<< cid.print() << " with rCID " << rCid.print() << " > enigma "
			<< ltpHeaderControlWud.enigma << " > SK "
			<< ltpHeaderControlWud.sessionKey);
	free(packet);
}

void Lightweight::_setSessionEnded(IcnId &rCid, sk_t &sessionKey, bool state)
{
	unordered_map<sk_t, bool>::iterator skIt;
	unordered_map<cid_t, unordered_map<sk_t, bool>>::iterator cidIt;
	_sessionEndedResponsesUnicastMutex.lock();
	cidIt = _sessionEndedResponsesUnicast.find(rCid.uint());

	// rCID not found
	if (cidIt == _sessionEndedResponsesUnicast.end())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " not found in LTP "
				"CTRL-SED map");
		_sessionEndedResponsesUnicastMutex.unlock();
		return;
	}

	skIt = cidIt->second.find(sessionKey);

	// SK not found
	if (skIt == cidIt->second.end())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " > SK " << sessionKey
				<< " not found in LTP CTRL-SED map");
		_sessionEndedResponsesUnicastMutex.unlock();
		return;
	}

	skIt->second = state;
	_sessionEndedResponsesUnicastMutex.unlock();
	LOG4CXX_TRACE(logger, "LTP CTRL-SED boolean set to " << state << " for "
			"rCID "	<< rCid.print() << " > SK " << sessionKey);
}

void Lightweight::_removeNidFromCmcGroups(IcnId &rCid, NodeId &nodeId,
		list< pair<enigma_t,sk_t> > *enigmaSkPairs)
{
	_potentialCmcGroupsMutex->lock();
	_potentialCmcGroupsIt = _potentialCmcGroups->find(rCid.uint());

	if (_potentialCmcGroupsIt != _potentialCmcGroups->end())
	{
		auto enigmaIt = _potentialCmcGroupsIt->second.begin();

		while (enigmaIt != _potentialCmcGroupsIt->second.end())
		{
			auto nidIt = enigmaIt->second.find(nodeId.uint());

			// Check if NID is known for this particular potential CMC group
			if (nidIt != enigmaIt->second.end())
			{
				enigmaIt->second.erase(nidIt);
				LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " removed "
						"again from potential CMC groups for rCID "
						<< rCid.print() << " > enigma " << enigmaIt->first);
			}

			// check if this was the last NID in pot CMC group
			if (enigmaIt->second.empty())
			{
				LOG4CXX_TRACE(logger, "No NID left for potential CMC group rCID"
						" " << rCid.print() << " > enigma " << enigmaIt->first
						<< ". Deleting enigma under rCID");
				_potentialCmcGroupsIt->second.erase(enigmaIt);
				enigmaIt = _potentialCmcGroupsIt->second.begin();
			}
			else
			{
				enigmaIt++;
			}
		}

		// check if there's no enigma with no NID left for this rCID
		if (_potentialCmcGroupsIt->second.empty())
		{
			LOG4CXX_TRACE(logger, "No enigma left for rCID " << rCid.print()
					<< ". Deleting potential CMC group entry");
			_potentialCmcGroups->erase(_potentialCmcGroupsIt);
		}
	}

	_potentialCmcGroupsMutex->unlock();
	_cmcGroupsMutex->lock();
	_cmcGroupsIt = _cmcGroups->find(rCid.uint());

	// Locked (active) CMC group found
	if (_cmcGroupsIt == _cmcGroups->end())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " does not exist in "
				"locked CMC groups");
		_cmcGroupsMutex->unlock();
		return;
	}

	// Iterate over all rCID > enigmas
	pair<enigma_t, sk_t> enigmaSkPair;
	auto enigmaIt = _cmcGroupsIt->second.begin();

	while (enigmaIt != _cmcGroupsIt->second.end())
	{
		auto skIt = enigmaIt->second.begin();

		// Iterate over all rCID > enigma > SKs
		while (skIt != enigmaIt->second.end())
		{
			auto nidIt = skIt->second.begin();

			// iterate over rCID > enigma > SK > NIDs and remove NID if found
			while (nidIt != skIt->second.end())
			{
				// NID found
				if (nidIt->uint() == nodeId.uint())
				{

					enigmaSkPair.first = enigmaIt->first;
					enigmaSkPair.second = skIt->first;
					enigmaSkPairs->push_back(pair<enigma_t, sk_t>(enigmaSkPair));
					skIt->second.erase(nidIt);
					LOG4CXX_DEBUG(logger, "NID " << nodeId.uint() << " removed "
							"from CMC group rCID " << rCid.print() << " > enigma "
							<< enigmaIt->first << " > SK " << skIt->first);
					break;
				}

				nidIt++;
			}

			// check if SK map has no NID values left. If so, delete it
			if (skIt->second.empty())
			{
				LOG4CXX_TRACE(logger, "No NID left in CMC group for rCID "
						<< rCid.print() << " > enigma " << enigmaIt->first
						<< " > SK " << skIt->first << ". Deleting SK");
				enigmaIt->second.erase(skIt);
				skIt = enigmaIt->second.begin();
			}
			else
			{
				skIt++;
			}
		}

		// Check if this enigma has no SK entry left
		if (enigmaIt->second.empty())
		{
			LOG4CXX_TRACE(logger, "No SK left in CMC group for rCID "
					<< rCid.print() << " > enigma " << enigmaIt->first
					<< ". Deleting enigma");
			_cmcGroupsIt->second.erase(enigmaIt);
			enigmaIt = _cmcGroupsIt->second.begin();
		}
		else
		{
			enigmaIt++;
		}
	}

	// If rCID has no enigma left, delete entire CMC group
	if (_cmcGroupsIt->second.empty())
	{
		LOG4CXX_TRACE(logger, "No enigma left in CMC group for rCID "
				<< rCid.print() << ". Deleting entire group");
		_cmcGroups->erase(_cmcGroupsIt);
	}

	_cmcGroupsMutex->unlock();
}

void Lightweight::_removeNidsFromCmcGroup(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey, list<NodeId> &nodeIds)
{
	map<enigma_t, map<sk_t, list<NodeId>>>::iterator cmcenigmaIt;
	map<sk_t, list<NodeId>>::iterator cmcSkIt;
	_cmcGroupsMutex->lock();
	_cmcGroupsIt = _cmcGroups->find(rCid.uint());

	if (_cmcGroupsIt == _cmcGroups->end())
	{
		_cmcGroupsMutex->unlock();
		return;
	}

	cmcenigmaIt = _cmcGroupsIt->second.find(enigma);

	if (cmcenigmaIt == _cmcGroupsIt->second.end())
	{
		_cmcGroupsMutex->unlock();
		return;
	}

	cmcSkIt = cmcenigmaIt->second.find(sessionKey);

	if (cmcSkIt == cmcenigmaIt->second.end())
	{
		_cmcGroupsMutex->unlock();
		return;
	}

	list<NodeId>::iterator cmcNidsIt;
	list<NodeId>::iterator unconfirmedNidsIt;
	// Iterate over unconfirmed NIDs
	unconfirmedNidsIt = nodeIds.begin();

	while (unconfirmedNidsIt != nodeIds.end())
	{
		cmcNidsIt = cmcSkIt->second.begin();
		// Find this NID in CMC group
		while (cmcNidsIt != cmcSkIt->second.end())
		{
			// NID found
			if (unconfirmedNidsIt->uint() == cmcNidsIt->uint())
			{
				cmcSkIt->second.erase(cmcNidsIt);
				LOG4CXX_TRACE(logger, "NID " << unconfirmedNidsIt->uint()
						<< " removed from CMC group rCID " << rCid.print()
						<< " > enigma " << enigma << " > SK " << sessionKey);
				break;
			}

			cmcNidsIt++;
		}

		unconfirmedNidsIt++;
	}

	_cmcGroupsMutex->unlock();
}

void Lightweight::_removeNidsFromWindowUpdate(IcnId &rCid,
		enigma_t &enigma, sk_t &sessionKey, list<NodeId> &nodeIds)
{
	list<NodeId>::iterator nidsIt;
	map<nid_t, bool>::iterator nidIt;
	map<sk_t, map<nid_t, bool>>::iterator skIt;
	map<enigma_t, map<sk_t, map<nid_t, bool>>>::iterator enigmaIt;
	_windowUpdateMutex.lock();
	_windowUpdateIt = _windowUpdate.find(rCid.uint());

	// rCID does not exist
	if (_windowUpdateIt == _windowUpdate.end())
	{
		LOG4CXX_WARN(logger, "rCID " << rCid.print() << " does not exist in "
				"list of awaited CTRL-WEDs");
		_windowUpdateMutex.unlock();
		return;
	}

	enigmaIt = _windowUpdateIt->second.find(enigma);

	// enigma does not exist
	if (enigmaIt == _windowUpdateIt->second.end())
	{
		LOG4CXX_WARN(logger, "rCID " << rCid.print() << " > enigma "
				<< enigma << " does not exist in list of awaited "
						"CTRL-WEDs");
		_windowUpdateMutex.unlock();
		return;
	}

	skIt = enigmaIt->second.find(sessionKey);

	// SK does not exist
	if (skIt == enigmaIt->second.end())
	{
		LOG4CXX_WARN(logger, "rCID " << rCid.print() << " > enigma "
				<< enigma << " > SK " << sessionKey << " does not exist in"
				" list of awaited CTRL-WEDs");
		_windowUpdateMutex.unlock();
		return;
	}

	// Iterate over list of NIDs and remove them if they exist
	for (nidsIt = nodeIds.begin(); nidsIt != nodeIds.end(); nidsIt++)
	{
		nidIt = skIt->second.find(nidsIt->uint());

		// NID found
		if (nidIt != skIt->second.end())
		{
			skIt->second.erase(nidIt);
			LOG4CXX_TRACE(logger, "NID " << nidsIt->uint() << " removed from "
					"awaited WED list for rCID " << rCid.print() << " > enigma "
					<< enigma << " > SK " << sessionKey << ". "
					<< skIt->second.size() << " NIDs left in list");
		}
	}

	_windowUpdateMutex.unlock();
}

list<NodeId> Lightweight::_wudsNotReceived(IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey)
{
	list<NodeId> unconfirmedNids;
	map<nid_t, bool>::iterator nidIt;
	map<sk_t, map<nid_t, bool>>::iterator skIt;
	map<enigma_t, map<sk_t, map<nid_t, bool>>>::iterator enigmaIt;
	_windowUpdateIt = _windowUpdate.find(rCid.uint());

	if (_windowUpdateIt == _windowUpdate.end())
	{
		LOG4CXX_WARN(logger, "rCID " << rCid.print() << " not found in WU map");
		return unconfirmedNids;
	}

	enigmaIt = _windowUpdateIt->second.find(enigma);

	if (enigmaIt == _windowUpdateIt->second.end())
	{
		LOG4CXX_WARN(logger, "rCID " << rCid.print() << " > enigma "
				<< enigma << " not found in WU map");
		return unconfirmedNids;
	}

	skIt = enigmaIt->second.find(sessionKey);

	if (skIt == enigmaIt->second.end())
	{
		LOG4CXX_WARN(logger, "rCID " << rCid.print() << " > enigma "
				<< enigma << " > SK " << sessionKey << " not found in WU "
						"map");
		return unconfirmedNids;
	}

	for (nidIt = skIt->second.begin(); nidIt != skIt->second.end(); nidIt++)
	{
		if (!nidIt->second)
		{
			NodeId nodeId(nidIt->first);
			unconfirmedNids.push_back(nodeId);
		}
	}

	return unconfirmedNids;
}

uint16_t Lightweight::_rtt()
{
	uint32_t rtt = 0;
	forward_list<uint16_t>::iterator it;
	_rttsMutex.lock();
	it = _rtts.begin();

	// calculate sum
	while (it != _rtts.end())
	{
		rtt = rtt + (uint32_t)*it;
		it++;
	}

	_rttsMutex.unlock();
	// calculate mean
	rtt = ceil(rtt / _configuration.ltpRttListSize());
	LOG4CXX_TRACE(logger, "RTT mean is " << rtt << "ms");
	return (uint16_t)rtt;
}

void Lightweight::_rtt(uint16_t rtt)
{
	if (rtt == 0)
	{
		rtt = 1;//[ms]
		LOG4CXX_TRACE(logger, "RTT was given as 0. Assuming 1ms instead");
	}
	_rttsMutex.lock();
	// delete oldest entry first if list size has reached its max size
	_rtts.resize(_configuration.ltpRttListSize());
	_rtts.push_front(rtt);
	_rttsMutex.unlock();
}
