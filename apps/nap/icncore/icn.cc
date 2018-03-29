/*
 * icn.cc
 *
 *  Created on: 25 Apr 2016
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

#include <icncore/icn.hh>
#include <proxies/http/tcpclient.hh>

using namespace icn;
using namespace log4cxx;
using namespace namespaces::ip;
using namespace namespaces::http;
using namespace proxies::http::tcpclient;
using namespace std;
using namespace transport::lightweight;
using namespace transport::unreliable;

LoggerPtr Icn::logger(Logger::getLogger("icn"));

Icn::Icn(Blackadder *icnCore, Configuration &configuration,
		Namespaces &namespaces, Transport &transport, Statistics &statistics,
		bool *run)
	: _icnCore(icnCore),
	  _configuration(configuration),
	  _namespaces(namespaces),
	  _transport(transport),
	  _statistics(statistics),
	  _run(run)
{}

Icn::~Icn(){}

void Icn::operator ()()
{
	IcnId icnId;
	IcnId rCId;
	string icnIdStr;
	string rCIdStr;
	tp_states_t tpState;
	enigma_t enigma = 23;
	uint16_t dataLength = 0;
	uint8_t *retrievedPacket = (uint8_t *)malloc(65535);
	uint16_t retrievedPacketSize = 0;
	std::mutex tcpClientMutex;
	std::thread *tcpClientThread;
	reverse_lookup_t tcpClientReverseLookup;
	socket_fds_t tcpClientSocketFds;
	socket_state_t tcpClientSocketState;
	TcpClient tcpClient(_configuration, _namespaces, _statistics,
			&tcpClientReverseLookup, &tcpClientSocketFds, &tcpClientSocketState,
			tcpClientMutex, _run);
	LOG4CXX_DEBUG(logger, "ICN listener thread started");

	while (*_run)
	{
		Event event;
		_icnCore->getEvent(event);
		icnIdStr = chararray_to_hex(event.id);
		icnId = icnIdStr;

		switch (event.type)
		{
		/*
		 * Scope published
		 */
		case SCOPE_PUBLISHED:
			LOG4CXX_DEBUG(logger, "SCOPE_PUBLISHED received for ICN ID "
					<< icnId.print());
			_namespaces.subscribeScope(icnId);
			break;
		/*
		 * Scope unpublished
		 */
		case SCOPE_UNPUBLISHED:
			LOG4CXX_DEBUG(logger, "SCOPE_UNPUBLISHED received for ICN ID "
					<< icnId.print());
			break;
		/*
		 * Start publish
		 */
		case START_PUBLISH:
		{
			LOG4CXX_DEBUG(logger, "START_PUBLISH received for CID "
					<< icnId.print());
			_namespaces.forwarding(icnId, true);
			_namespaces.publishFromBuffer(icnId);
			break;
		}
		/*
		 * Start publish iSub
		 */
		case START_PUBLISH_iSUB:
		{
			NodeId nodeId = event.id;
			LOG4CXX_DEBUG(logger, "START_PUBLISH_iSUB received for NID "
					<< nodeId.uint());
			_namespaces.forwarding(nodeId, true);
			//_namespaces.publishFromBuffer(nodeId);//should be always empty
			break;
		}
		/*
		 * Stop publish
		 */
		case STOP_PUBLISH:
		{
			LOG4CXX_DEBUG(logger, "STOP_PUBLISH received for CID "
					<< icnId.print());
			_namespaces.forwarding(icnId, false);
			break;
		}
		//FIXME No STOP_PUBLISH_iSUB???
		/*
		 * Pause publish
		 */
		case PAUSE_PUBLISH:
			_namespaces.forwarding(icnId, false);
			break;
		/*
		 * Published data
		 */
		case PUBLISHED_DATA:
		{
			dataLength = event.data_len;
			uint32_t enigma = 0;
			uint16_t sessionKey = 0;
			LOG4CXX_TRACE(logger, "PUBLISHED_DATA of length " << event.data_len
					<< " received under (r)CID " << icnId.print());
			tp_states_t tpState = _transport.handle(icnId, event.data,
					dataLength, enigma, sessionKey);

			if (tpState == TP_STATE_ALL_FRAGMENTS_RECEIVED)
			{
				bzero(retrievedPacket, 65535);
				string nodeId = _configuration.nodeId().str();

				// If packet could be retrieve, send it
				if (_transport.retrieveIcnPacket(icnId, enigma, nodeId,
						sessionKey, retrievedPacket, retrievedPacketSize))
				{
					_namespaces.sendToEndpoint(icnId, enigma,
							retrievedPacket, retrievedPacketSize);
				}
				else
				{
					LOG4CXX_WARN(logger, "Packet could not be retrieved from "
							"ICN buffer");
				}
			}
			else if (tpState == TP_STATE_SESSION_ENDED)
			{
				_namespaces.endOfSession(icnId, enigma, sessionKey);
			}
			else if (tpState == TP_STATE_NO_TRANSPORT_PROTOCOL_USED)
			{
				_namespaces.handlePublishedData(icnId, event.data,
						event.data_len);
			}
			break;
		}
		/*
		 * Published data iSub for HTTP-over-ICN namespace (requests)
		 */
		case PUBLISHED_DATA_iSUB:
		{
			rCIdStr = chararray_to_hex(event.isubID);
			rCId = rCIdStr;
			dataLength = event.data_len;
			uint16_t sessionKey;
			LOG4CXX_TRACE(logger, "PUBLISHED_DATA_iSUB received for CID "
						<< icnId.print() << " and rCID " << rCId.print()
						<< " of length " << event.data_len);
			// add NID > rCID look up to HTTP handler so when START_PUBLISH_iSUB
			// arrives the corresponding rCID can be looked up
			_namespaces.Http::addReversNidTorCIdLookUp(event.nodeId, rCId);
			// handle received publication
			tpState = _transport.handle(icnId, rCId, event.nodeId,
					event.data,	dataLength, enigma, sessionKey);

			if (tpState	== TP_STATE_ALL_FRAGMENTS_RECEIVED)
			{
				bzero(retrievedPacket, 65535);

				if (!_transport.retrieveIcnPacket(rCId, enigma, event.nodeId,
						sessionKey, retrievedPacket, retrievedPacketSize))
				{//packet retrieval failed. Break here
					LOG4CXX_TRACE(logger, "HTTP request packet retrieval failed"
							"for CID " << icnId.print() << ", rCID "
							<< rCId.print() << ", Enigma " << enigma
							<< " and NID " << event.nodeId);
					break;
				}

				// switch over root scope to know which proxy should be called
				switch (icnId.rootNamespace())
				{
				case NAMESPACE_HTTP:
				{
					tcpClientThread = new std::thread(tcpClient, icnId,
							rCId, enigma, sessionKey, event.nodeId,
							retrievedPacket, retrievedPacketSize);
					tcpClientThread->detach();
					delete tcpClientThread;
					break;
				}
				case NAMESPACE_COAP://example entry point for other proxies
					break;
				default:
					LOG4CXX_INFO(logger, "Unknown root namespace for CID "
							<< icnId.print() << ". Cannot send off retrieved "
							"packet from ICN buffer")
				}
			}

			break;
		}
		/*
		 * Re-publish
		 */
		case RE_PUBLISH:
			LOG4CXX_TRACE(logger, "RE_PUBLISH received for CID "
						<< icnId.print());
			LOG4CXX_DEBUG(logger, "Unpublishing info item for CID "
					<< icnId.print());
			_icnCore->unpublish_info(icnId.binId(), icnId.binPrefixId(),
					DOMAIN_LOCAL, 0, 0);
			_namespaces.forwarding(icnId, false);
			break;
		/*
		 * Resume publish
		 */
		case RESUME_PUBLISH:
			LOG4CXX_DEBUG(logger, "RESUME_PUBLISH received for CID "
						<< icnId.print());
			break;
		default:
			LOG4CXX_WARN(logger, "Unknown BA API event type received: "
					<< event.type);
		}
	}

	LOG4CXX_INFO(logger, "ICN handler thread stopped");
	free(retrievedPacket);//clean up memory
}
