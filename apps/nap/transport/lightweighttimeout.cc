/*
 * lightweighttimeout.cc
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

#include "lightweighttimeout.hh"

using namespace transport::lightweight;

LoggerPtr LightweightTimeout::logger(Logger::getLogger("transport.lightweight"));

LightweightTimeout::LightweightTimeout(IcnId cId, IcnId rCId,
		enigma_t enigma, uint16_t sessionKey, uint16_t sequenceNumber,
		Blackadder *icnCore, std::mutex &icnCoreMutex, uint16_t rtt,
		proxy_packet_buffer_t *proxyPacketBuffer, std::mutex *proxyPacketBufferMutex,
		window_ended_requests_t *windowEnded, std::mutex *windowEndedMutex)
	: _cId(cId),
	  _rCId(rCId),
	  _icnCore(icnCore),
	  _icnCoreMutex(icnCoreMutex),
	  _rtt(rtt),
	  _proxyPacketBuffer(proxyPacketBuffer),
	  _proxyPacketBufferMutex(proxyPacketBufferMutex),
	  _windowEndedRequests(windowEnded),
	  _windowEndedMutex(windowEndedMutex)
{
	_ltpReset = false;
	_ltpHeaderData.enigma = enigma;
	_ltpHeaderData.sessionKey = sessionKey;
	_ltpHeaderData.sequenceNumber = sequenceNumber;
	_nodeId = NodeId(0);
	_resetted = NULL;
	_resettedMutex = new std::mutex;

	_windowEndedResponses = NULL;
}

LightweightTimeout::LightweightTimeout(IcnId cid, IcnId rCid,
		Blackadder *icnCore, std::mutex &icnCoreMutex, uint16_t rtt,
		void *resetted, std::mutex *resettedMutex)
	: _cId(cid),
	  _rCId(rCid),
	  _icnCore(icnCore),
	  _icnCoreMutex(icnCoreMutex),
	  _rtt(rtt),
	  _resettedMutex(resettedMutex)
{
	_ltpReset = true;
	_proxyPacketBuffer = NULL;
	_proxyPacketBufferMutex = new std::mutex;
	_resetted = (resetted_t *)resetted;
	_windowEndedMutex = new std::mutex;
	_windowEndedRequests = NULL;
	_windowEndedResponses = NULL;
}

LightweightTimeout::~LightweightTimeout() {}

void LightweightTimeout::operator()()
{
	map<sk_t, bool>::iterator skIt;
	map<enigma_t, map<sk_t, bool>>::iterator enigmaIt;

	// LTP_RST constructor called
	if (_ltpReset)
	{
		_checkForLtpRsted();
		_deleteObjects();
		return;
	}
	/*
	 * First it must be checked what type of WE map must be used (is this timer
	 * for a request or a response)
	 */
	uint8_t attempts = ENIGMA;

	// WE timer for Responses
	if (_nodeId.uint() != 0)
	{// HTTP response
		//TODO
		_deleteObjects();
		return;
	}

	// check boolean continuously for 2 * RRT (timeout)
	while (attempts != 0)
	{// HTTP request
		std::this_thread::sleep_for(std::chrono::milliseconds(2 * _rtt));
		_windowEndedMutex->lock();
		_windowEndedRequestsIt = _windowEndedRequests->find(_rCId.uint());

		// rCID does not exist (unlikely - just to avoid seg faults)
		if (_windowEndedRequestsIt == _windowEndedRequests->end())
		{
			LOG4CXX_TRACE(logger, "rCID " << _rCId.print()
					<< " does not exist (anymore) in list of sent LTP CTRL-WE "
							"messages");
			_windowEndedMutex->unlock();
			_deleteObjects();
			return;
		}

		enigmaIt =
				_windowEndedRequestsIt->second.find(_ltpHeaderData.enigma);

		if (enigmaIt == _windowEndedRequestsIt->second.end())
		{
			LOG4CXX_TRACE(logger, "Enigma " << _ltpHeaderData.enigma
					<< " for rCID "<< _rCId.print() << " does not "
					"exist (anymore) in list of sent LTP CTRL-WE messages");
			_windowEndedMutex->unlock();
			_deleteObjects();
			return;
		}

		skIt = enigmaIt->second.find(_ltpHeaderData.sessionKey);

		//SK does not exist
		if (skIt == enigmaIt->second.end())
		{
			LOG4CXX_TRACE(logger, "SK " << _ltpHeaderData.sessionKey
					<< " for rCID "<< _rCId.print() << " > Enigma "
					<< _ltpHeaderData.enigma << " does not exist (anymore)"
					" in list of sent LTP CTRL-WE messages");
			_windowEndedMutex->unlock();
			_deleteObjects();
			return;
		}

		// WED received. stop here
		if (skIt->second)
		{
			LOG4CXX_TRACE(logger, "LTP CTRL-WED received for rCID "
					<< _cId.print() << " > Enigma "
					<< _ltpHeaderData.enigma << " > SK "
					<< _ltpHeaderData.sessionKey);
			enigmaIt->second.erase(skIt);
			LOG4CXX_TRACE(logger, "SK " << _ltpHeaderData.sessionKey
					<< " removed from LTP CTRL-WED map for rCID "
					<< _rCId.print() << " > Enigma " << enigmaIt->first);

			// No SK left. Delete entire Enigma map
			if (enigmaIt->second.empty())
			{
				_windowEndedRequestsIt->second.erase(enigmaIt);
				LOG4CXX_TRACE(logger, "That was the last SK. Enigma "
						<< _ltpHeaderData.enigma << " removed from LTP "
						"CTRL-WED map for rCID " << _rCId.print());

				// No Enigma left
				if (_windowEndedRequestsIt->second.empty())
				{
					_windowEndedRequests->erase(_windowEndedRequestsIt);
					LOG4CXX_TRACE(logger, "That was the last Enigma.  rCID "
							<< _rCId.print() << " removed from LTP CTRL-WED "
							"map");
				}
			}

			_windowEndedMutex->unlock();
			_deleteObjects();
			return;
		}

		_windowEndedMutex->unlock();
		LOG4CXX_TRACE(logger, "LTP CTRL-WED has not been received within "
				"given timeout of " << 2 * _rtt << "ms for rCID "
				<< _rCId.print());
		_rePublishWindowEnd();
		attempts--;

		if (attempts == 0)
		{
			LOG4CXX_TRACE(logger, "LTP CTRL-WED has not been received after "
					<< (int)ENIGMA << " attempts. Giving up");
		}
	} /* ENIGMA attempts reached*/

	_deleteObjects();
}

void LightweightTimeout::_checkForLtpRsted()
{
	uint8_t attempts = ENIGMA;
	resetted_t::iterator resettedIt;

	while (attempts != 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(2 * _rtt));
		_resettedMutex->lock();
		resettedIt = _resetted->find(_rCId.uint());

		if (resettedIt == _resetted->end())
		{
			LOG4CXX_WARN(logger, "rCID " << _rCId.print() << " not found in"
					" resetted map anymore (attempts " << (int)attempts << ")");
			_resettedMutex->unlock();
			return;
		}

		//stop here if LTP RST if RSTED has been received
		if (resettedIt->second)
		{
			LOG4CXX_TRACE(logger, "LTP RST had been received for rCID "
					<< _rCId.print() << ". Stopping thread");
			_resettedMutex->unlock();
			return;
		}

		_resettedMutex->unlock();
		LOG4CXX_TRACE(logger, "LTP RSTED not received within " << 2 * _rtt
				<< "ms (attempts left: " << (int)attempts << ")");
		_rePublishReset();
		attempts--;
	}
}

void LightweightTimeout::_deleteObjects()
{
	if (_ltpReset)
	{
		delete _proxyPacketBufferMutex;
		delete _windowEndedMutex;
	}
	else
	{
		delete _resettedMutex;
	}
}

void LightweightTimeout::_deleteProxyPacket(IcnId &rCId,
		ltp_hdr_ctrl_wed_t &ltpHeader)
{//TODO delete ... legacy code for when surrogacy had not been implemented
	//map<Sequence, Packet
	map<uint16_t, pair<uint8_t *, uint16_t>>::iterator	sequenceKeyMapIt;
	//map<SK,     map<Sequence, Packet
	map<uint16_t, map<uint16_t, pair<uint8_t *, uint16_t>>>::iterator
			sessionKeyMapIt;
	//map<Enigma,   map<SK,       map<Sequence, Packet
	map<uint32_t, map<uint16_t, map<uint16_t, pair<uint8_t *,
			uint16_t>>>>::iterator enigmaMapIt;
	_proxyPacketBufferMutex->lock();
	_proxyPacketBufferIt = _proxyPacketBuffer->find(rCId.uint());

	// rCID does not exist
	if (_proxyPacketBufferIt == _proxyPacketBuffer->end())
	{
		LOG4CXX_WARN(logger, "rCID " << rCId.print() << " cannot be found "
				"in proxy packet buffer map");
		_proxyPacketBufferMutex->unlock();
		return;
	}

	enigmaMapIt = _proxyPacketBufferIt->second.find(ltpHeader.enigma);

	// Enigma does not exist
	if (enigmaMapIt == _proxyPacketBufferIt->second.end())
	{
		LOG4CXX_WARN(logger, "Enigma " << ltpHeader.enigma << " could "
				"not be found in proxy packet buffer map for rCID "
				<< rCId.print());
		_proxyPacketBufferMutex->unlock();
		return;
	}

	sessionKeyMapIt = enigmaMapIt->second.find(ltpHeader.sessionKey);

	// SK does not exist
	if (sessionKeyMapIt == enigmaMapIt->second.end())
	{
		LOG4CXX_WARN(logger, "SK " << ltpHeader.sessionKey << " could "
				"not be found in proxy packet buffer map for rCID "
				<< rCId.print() << " > Enigma " << ltpHeader.enigma);
		_proxyPacketBufferMutex->unlock();
		return;
	}

	// deleting packets
	for (sequenceKeyMapIt = sessionKeyMapIt->second.begin();
			sequenceKeyMapIt != sessionKeyMapIt->second.end();
			sequenceKeyMapIt++)
	{
		free(sequenceKeyMapIt->second.first);
	}

	// deleting entire session key
	enigmaMapIt->second.erase(sessionKeyMapIt);
	_proxyPacketBufferMutex->unlock();
	LOG4CXX_TRACE(logger, "Packet deleted from proxy packet buffer for rCID "
			<< rCId.print() << " > Enigma " << ltpHeader.enigma
			<< " > SK " << ltpHeader.sessionKey);
}

void LightweightTimeout::_rePublishWindowEnd()
{
	ltp_hdr_ctrl_we_t ltpHeader;
	uint8_t *packet;
	uint8_t offset = 0;
	uint16_t packetSize = sizeof(ltp_hdr_ctrl_we_t);
	// Fill up the LTP CTRL header
	ltpHeader.messageType = LTP_CONTROL;
	ltpHeader.controlType = LTP_CONTROL_WINDOW_END;
	ltpHeader.enigma = _ltpHeaderData.enigma;
	ltpHeader.sessionKey = _ltpHeaderData.sessionKey;
	ltpHeader.sequenceNumber = _ltpHeaderData.sequenceNumber;
	// make packet
	packet = (uint8_t *)malloc(packetSize);
	// [1] message type
	memcpy(packet, &ltpHeader.messageType, sizeof(ltpHeader.messageType));
	offset += sizeof(ltpHeader.messageType);
	// [2] control type
	memcpy(packet + offset, &ltpHeader.controlType,
			sizeof(ltpHeader.controlType));
	offset += sizeof(ltpHeader.controlType);
	// [3] proxy rule Id
	memcpy(packet + offset, &ltpHeader.enigma,
			sizeof(ltpHeader.enigma));
	offset += sizeof(ltpHeader.enigma);
	// [4] Session key
	memcpy(packet + offset, &ltpHeader.sessionKey,
			sizeof(ltpHeader.sessionKey));
	offset += sizeof(ltpHeader.sessionKey);
	// [5] Sequence number
	memcpy(packet + offset, &ltpHeader.sequenceNumber,
			sizeof(ltpHeader.sequenceNumber));
	_icnCoreMutex.lock();
	_icnCore->publish_data_isub(_cId.binIcnId(), DOMAIN_LOCAL, NULL, 0,
			_rCId.binIcnId(), packet, packetSize);
	_icnCoreMutex.unlock();
	LOG4CXX_TRACE(logger, "LTP Window End CTRL re-published under "
			<< _cId.print()	<< ", Enigma " << ltpHeader.enigma
			<< ", Sequence " << ltpHeader.sequenceNumber);
	free(packet);
}

void LightweightTimeout::_rePublishReset()
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
	_icnCoreMutex.lock();
	_icnCore->publish_data_isub(_cId.binIcnId(), DOMAIN_LOCAL, NULL, 0,
			_rCId.binIcnId(), packet, packetSize);
	_icnCoreMutex.unlock();
	LOG4CXX_TRACE(logger, "CTRL-RST re-published under " << _cId.print()
			<< " with rCID " << _rCId.print());
	free(packet);
}
