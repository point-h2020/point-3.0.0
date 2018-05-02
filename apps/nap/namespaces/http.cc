/*
 * http.cc
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

#include "http.hh"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace cleaners::httpbuffer;
using namespace namespaces::http;

LoggerPtr Http::logger(Logger::getLogger("namespaces.http"));

Http::Http(Blackadder *icnCore, Configuration &configuration,
		Transport &transport, Statistics &statistics, bool *run)
	: _icnCore(icnCore),
	  _configuration(configuration),
	  _transport(transport),
	  _statistics(statistics),
	  _run(run)
{
	// Initialising HTTP buffer cleaner
	HttpBufferCleaner httpBufferCleaner(_configuration, _statistics,
			_packetBufferRequests, _packetBufferRequestsMutex,
			//_packetBufferResponses, _packetBufferResponsesMutex,
			_run);
	std::thread *httpBufferThread = new std::thread(httpBufferCleaner);
	httpBufferThread->detach();
	delete httpBufferThread;
	_potentialCmcGroupsOpen = true;
}

Http::~Http(){}

void Http::addReversNidTorCIdLookUp(string &nodeId, IcnId &rCId)
{
	NodeId nid(nodeId);
	_reverseNidTorCIdLookUpMutex.lock();
	_reverseNidTorCIdLookUpIt = _reverseNidTorCIdLookUp.find(nid.uint());
	// NID does not exist, add it
	if (_reverseNidTorCIdLookUpIt == _reverseNidTorCIdLookUp.end())
	{
		list<uint32_t> rCIdsList;
		rCIdsList.push_back(rCId.uint());
		_reverseNidTorCIdLookUp.insert(pair<uint32_t, list<uint32_t>>(
				nid.uint(),	rCIdsList));
		LOG4CXX_TRACE(logger, "New NID " << nid.uint() << " added to reverse "
				"rCID look up map with rCID " << rCId.print());
		_reverseNidTorCIdLookUpMutex.unlock();
		return;
	}
	// NID does exist
	list<uint32_t>::iterator it;
	it = _reverseNidTorCIdLookUpIt->second.begin();
	while (it != _reverseNidTorCIdLookUpIt->second.end())
	{
		// rCID already in list
		if (*it == rCId.uint())
		{
			_reverseNidTorCIdLookUpMutex.unlock();
			return;
		}
		it++;
	}
	// rCID not in list -> add it now
	_reverseNidTorCIdLookUpIt->second.push_back(rCId.uint());
	LOG4CXX_TRACE(logger, "rCID " << rCId.print() << " added to reverse rCID "
			"look up map for NID " << nid.uint());
	_reverseNidTorCIdLookUpMutex.unlock();
}

void Http::deleteSessionKey(sk_t &sessionKey)
{
	map<enigma_t, list<socket_fd_t>>::iterator enigmaIt;
	list<socket_fd_t>::iterator socketFdIt;
	_ipEndpointSessionsMutex.lock();
	_ipEndpointSessionsIt = _ipEndpointSessions.begin();

	// Iterator over rCIDs
	while (_ipEndpointSessionsIt != _ipEndpointSessions.end())
	{
		enigmaIt = _ipEndpointSessionsIt->second.begin();

		// Iterate over Engimas
		while (enigmaIt != _ipEndpointSessionsIt->second.end())
		{
			socketFdIt = enigmaIt->second.begin();

			// Iterate over Session keys
			while (socketFdIt != enigmaIt->second.end())
			{
				// Session key found
				if (*socketFdIt == sessionKey)
				{
					enigmaIt->second.erase(socketFdIt);
					LOG4CXX_TRACE(logger, "SK " << sessionKey
							<< " deleted from IP endpoint session list for "
							"hashed rCID "  << _ipEndpointSessionsIt->first
							<< " > Engima " << enigmaIt->first);
					break;//TODO continue searching in other rCID > Engima lists
				}
				// if socket is -1, delete
				else if (*socketFdIt == -1)
				{
					enigmaIt->second.erase(socketFdIt);
					LOG4CXX_TRACE(logger, "Obsolete socket FD " << sessionKey
							<< " deleted from IP endpoint session list for "
							"hashed rCID "  << _ipEndpointSessionsIt->first
							<< " > Engima " << enigmaIt->first);
				}
				// check next session key
				else
				{
					socketFdIt++;
				}
			}

			enigmaIt++;
		}

		_ipEndpointSessionsIt++;
	}

	// delete all empty Engima maps
	_ipEndpointSessionsIt = _ipEndpointSessions.begin();

	while (_ipEndpointSessionsIt != _ipEndpointSessions.end())
	{
		enigmaIt = _ipEndpointSessionsIt->second.begin();

		// Iterate over Engimas
		while (enigmaIt != _ipEndpointSessionsIt->second.end())
		{
			if (enigmaIt->second.empty())
			{
				LOG4CXX_TRACE(logger, "List of SKs/FDs is empty for hashed rCID"
						<< " " << _ipEndpointSessionsIt->first << " > Engima "
						<< enigmaIt->first << ". Deleting it");
				_ipEndpointSessionsIt->second.erase(enigmaIt);

				// reset iterator and continue searching for this rCID > Engima
				if (!_ipEndpointSessionsIt->second.empty())
				{
					enigmaIt = _ipEndpointSessionsIt->second.begin();
				}
				else
				{
					break;
				}
			}

			enigmaIt++;
		}

		_ipEndpointSessionsIt++;
	}

	// delete all empty rCID maps
	_ipEndpointSessionsIt = _ipEndpointSessions.begin();

	while (_ipEndpointSessionsIt != _ipEndpointSessions.end())
	{
		if (_ipEndpointSessionsIt->second.empty())
		{
			LOG4CXX_TRACE(logger, "List of Engimas empty for hashed rCID "
					<< _ipEndpointSessionsIt->first << ". Deleting it");
			_ipEndpointSessions.erase(_ipEndpointSessionsIt);

			if (!_ipEndpointSessions.empty())
			{
				_ipEndpointSessionsIt = _ipEndpointSessions.begin();
			}
			else
			{
				break;
			}
		}

		_ipEndpointSessionsIt++;
	}

	_ipEndpointSessionsMutex.unlock();
}

void Http::closeCmcGroup(IcnId rCid, enigma_t enigma, sk_t sessionKey)
{
	//signal socket closure down to all cNAPs (this blocks until all NIDs in CMC
	// group have replied
	_transport.Lightweight::publishEndOfSession(rCid, enigma,
				sessionKey);

	// now close CMC group (i.e. deleting it)
	_cmcGroupsMutex.lock();
	_cmcGroupsIt = _cmcGroups.find(rCid.uint());

	if (_cmcGroupsIt == _cmcGroups.end())
	{
		LOG4CXX_TRACE(logger, "CMC group for rCID " << rCid.print()
				<< " already closed");
		_cmcGroupsMutex.unlock();
		return;
	}

	map<enigma_t, map<sk_t, list<NodeId>>>::iterator enigmaIt;
	enigmaIt = _cmcGroupsIt->second.find(enigma);

	if (enigmaIt == _cmcGroupsIt->second.end())
	{
		LOG4CXX_TRACE(logger, "CMC group for rCID " << rCid.print()
				<< " > Engima " << enigma << " already closed");
		_cmcGroupsMutex.unlock();
		return;
	}

	map<sk_t, list<NodeId>>::iterator skIt;
	skIt = enigmaIt->second.find(sessionKey);

	if (skIt == enigmaIt->second.end())
	{
		LOG4CXX_TRACE(logger, "CMC group for rCID " << rCid.print()
				<< " > Engima " << enigma << " > SK " << sessionKey
				<< " already closed");
		_cmcGroupsMutex.unlock();
		return;
	}

	// Delete entire list<NIDs> from rCID > Engima > SK
	LOG4CXX_TRACE(logger, "CMC group of " << skIt->second.size() <<" deleted "
			"for rCID " << rCid.print() << " > Engima " << enigma << " > SK "
			<< sessionKey);
	enigmaIt->second.erase(skIt);

	// Check if this was the last SK for rCID > Engima
	if (enigmaIt->second.empty())
	{
		_cmcGroupsIt->second.erase(enigmaIt);
		LOG4CXX_TRACE(logger, "Last SK for rCID " << rCid.print() << " > Engima"
				<< enigma << ". Removed Engima");
	}

	// Check if that was the last Engima for this rCID
	if (_cmcGroupsIt->second.empty())
	{
		_cmcGroups.erase(_cmcGroupsIt);
		LOG4CXX_TRACE(logger, "Last Engima for rCID " << rCid.print()
				<< ". Removed rCID too");
	}

	LOG4CXX_TRACE(logger, _cmcGroups.size() << " CMC group(s) still active");
	_cmcGroupsMutex.unlock();
	// now clean HTTP response packet buffer

}

void Http::handleRequest(string &fqdn, string &resource,
		http_methods_t httpMethod, uint8_t *packet, uint16_t &packetSize,
		socket_fd_t socketFd)
{
	enigma_t enigma;
	_statistics.httpRequestsPerFqdn(fqdn, 1);
	sk_t sessionKey = (sk_t)socketFd;//convert SFD to SK
	// Create corresponding CIDs
	IcnId cId(fqdn);
	IcnId rCId(fqdn, resource);
	enigma = 23;
	// Add rCID and session to known list of IP endpoint sessions
	_addIpEndpointSession(rCId, enigma, socketFd);
	// Buffer packet in any case (incl. DNSlocal)
	_bufferRequest(cId, rCId, enigma, sessionKey, httpMethod, packet,
			packetSize);

	// Create scopes in RV if necessary
	_mutexIcnIds.lock();
	_cIdsIt = _cIds.find(cId.uint());

	if (_cIdsIt == _cIds.end())
	{
		_cIds.insert(pair<cid_t, IcnId>(cId.uint(), cId));
		_mutexIcnIds.unlock();
		LOG4CXX_DEBUG(logger, "New CID " << cId.print() << " added to list of "
				"known CIDs");

		if (pausing(cId))
		{
			LOG4CXX_DEBUG(logger, "Pausing enabled for CID " << cId.print()
					<< ". Scopes are not being published to RV");
			return;
		}

		_icnCore->publish_scope(cId.binRootScopeId(),
				cId.binEmpty(), DOMAIN_LOCAL, NULL, 0);
		LOG4CXX_DEBUG(logger, "Root scope " << cId.rootScopeId()
				<< " published to domain local RV");

		/* iterate over the number of scope levels and publish them to the RV.
		 * Note, the root scope and the information item are excluded!
		 */
		for (size_t i = 2; i < cId.length() / 16; i++)
		{
			_icnCore->publish_scope(cId.binScopeId(i),
					cId.binScopePath(i - 1), DOMAIN_LOCAL, NULL, 0);
			LOG4CXX_DEBUG(logger, "Scope ID " << cId.scopeId(i)
					<< " published under scope path "
					<< cId.printScopePath(i - 1) << " to domain local "
							"RV");
		}

		LOG4CXX_DEBUG(logger, "Advertised " << cId.id() << " under scope path "
				<< cId.printPrefixId());
		_icnCore->publish_info(cId.binId(), cId.binPrefixId(), DOMAIN_LOCAL,
				NULL, 0);
		return;
	}

	// Advertise availability if forwarding rule set to disabled
	if (!(*_cIdsIt).second.forwarding())
	{
		if (_cIdsIt->second.fidRequested())
		{
			LOG4CXX_TRACE(logger, "FID has been already requested for CID "
					<< _cIdsIt->second.print() << ". Not calling publish_info()"
							" again. Awaiting START_PUBLISH");
			_mutexIcnIds.unlock();
			return;
		}

		_mutexIcnIds.unlock();

		if (pausing(cId))
		{
			LOG4CXX_DEBUG(logger, "Pausing enabled for CID " << cId.print()
					<< ". Availability of FQDN info item will not be "
							"advertised");
			return;
		}

		_icnCore->publish_info(cId.binId(), cId.binPrefixId(), DOMAIN_LOCAL,
				NULL, 0);
		LOG4CXX_DEBUG(logger, "Advertised " << cId.id() << " again "
				"under scope path " << cId.printPrefixId());
		return;
	}

	_mutexIcnIds.unlock();
	// Publish packet (blocking method to adjust TCP server socket to the NAP's
	// speed
	_transport.Lightweight::publish(cId, rCId, enigma, sessionKey, packet,
			packetSize);
}

bool Http::handleResponse(IcnId rCId, enigma_t enigma,	sk_t &sessionKey,
		bool &firstPacket, uint8_t *packet,	uint16_t &packetSize)
{
	uint8_t attempts = ENIGMA;
	list<NodeId> cmcGroup;

	LOG4CXX_ASSERT(logger, rCId.str().length() == 0, "rCID is empty. CMC group "
			"cannot be identified");

	while (cmcGroup.empty() && rCId.str().length() != 0)
	{
		cmcGroup.clear();

		// ICN core doesn't have the FID for any NID which awaits the response.
		// Note, _getCmcGroup returns true if packet must be buffered!
		if (_getCmcGroup(rCId, enigma, sessionKey, firstPacket, cmcGroup))
		{
			LOG4CXX_TRACE(logger, "Trying to form CMC group in 200ms again for "
					<< "rCID " << rCId.print() << " > Engima " << enigma
					<< " > SK " << sessionKey);
			// Sleep 200ms (default RTT)
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		if (attempts == 0)
		{
			LOG4CXX_DEBUG(logger, "Giving up trying to get a CMC group for rCID"
					<< " " << rCId.print() << " > Engima " << enigma
					<< " > SK " << sessionKey);
			return false;
		}

		attempts--;
	}

	/*Old code below which adds the packet to the buffer. Issue is that when
	 * eventually publishing the packet the method gets called from Icn class
	 * which must not go into a blocking method. But as HTTP responses are
	 * significantly larger then HTTP requests, flow control (waiting for WUDs)
	 * will block the entire NAP and incoming WUD cannot be read. Placing the
	 * WUD reader into a thread is feasible but going back to where the
	 * publication of the HTTP response got interrupted is quite tricky. That is
	 * why the HTTP response gets published via LTP if _getCmcGroup above
	 * returns true which includes the reception of START_PUBLISH_iSUB for at
	 * least one NID in the potential CMC group
	if(_getCmcGroup(rCId, enigma, sessionKey, firstPacket, cmcGroup))
	{
		_bufferResponse(rCId, enigma, sessionKey, packet, packetSize);
	}
	// doesn't exist or not ready (all NIDs in potential CMC group may still
	// wait for receive the entire HTTP request
	if (cmcGroup.empty())
	{
		LOG4CXX_TRACE(logger, "No START_PUBLISH_iSUB has been received for any "
				"NID in the potential CMC group map. Packet will be buffered");
		return;
	}*/
	_transport.Lightweight::publish(rCId, enigma, sessionKey, cmcGroup,
			packet, packetSize);
	return true;
}

uint16_t Http::activeCmcGroups(IcnId &cid)
{//FIXME check all rCIDs for the given CID (backwards mapping required)
	uint16_t cmcGroups;
	_cmcGroupsMutex.lock();
	cmcGroups = _cmcGroups.size();
	_cmcGroupsMutex.unlock();
	LOG4CXX_TRACE(logger, "Current number of active CMC groups = "
			<< cmcGroups);
	return cmcGroups;
}

void Http::closePotentialCmcGroups(IcnId &cid)
{// FIXME delete CMC groups for a particular FQDN only
	_potentialCmcGroupsMutex.lock();
	_potentialCmcGroups.clear();
	_potentialCmcGroupsMutex.unlock();
	LOG4CXX_DEBUG(logger, "All potential CMC groups were deleted");
}

void Http::endSession(IcnId &rCid, enigma_t &enigma, sk_t &sessionKey)
{
	forward_list<sk_t> obsoleteSessionKeys;
	forward_list<sk_t>::iterator obsoleteSessionKeysIt;
	list<socket_fd_t>::iterator socketFdIt;
	map<enigma_t, list<socket_fd_t>>::iterator enigmaIt;
	IcnId cid;

	// delete HTTP request packets from HTTP packet buffer if CID is still known
	if (_transport.Lightweight::cidLookup(rCid, cid))
	{
		_deleteBufferedRequest(cid, rCid, enigma, sessionKey);
	}

	// First get the socket FD for this IP endpoints (multiple possible!)
	_ipEndpointSessionsMutex.lock();
	_ipEndpointSessionsIt = _ipEndpointSessions.find(rCid.uint());

	if (_ipEndpointSessionsIt == _ipEndpointSessions.end())
	{
		LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " does not exist in "
				"IP endpoint sessions map (anymore). Socket already closed")
		_ipEndpointSessionsMutex.unlock();
		return;
	}

	enigmaIt = _ipEndpointSessionsIt->second.find(enigma);

	if (enigmaIt == _ipEndpointSessionsIt->second.end())
	{
		LOG4CXX_TRACE(logger, "Engima " << enigma << " does not exist in "
				"IP endpoint session map (anymore) under rCID "
				<< rCid.print());
		_ipEndpointSessionsMutex.unlock();
		return;
	}

	// now iterate over all IP endpoints and shutdown their sockets
	fd_set writeSet;

	for (socketFdIt = enigmaIt->second.begin();
			socketFdIt != enigmaIt->second.end(); socketFdIt++)
	{
		FD_ZERO(&writeSet);
		FD_SET(*socketFdIt, &writeSet);

		if (FD_ISSET(*socketFdIt, &writeSet))
		{
			LOG4CXX_DEBUG(logger, "Shutting down socket FD " << *socketFdIt
					<< " for rCID " << rCid.print() << " > Engima "
					<< enigma);
			if (shutdown(*socketFdIt, SHUT_WR) == -1)
			{//FIXME RW shutdown must be aware of slow LTP session with NACKs
				LOG4CXX_DEBUG(logger, "Shutdown (WR) of socket FD "
						<< *socketFdIt << " failed: " << strerror(errno));
			}
		}
		else
		{
			LOG4CXX_TRACE(logger, "Socket FD " << *socketFdIt << " not "
					"readable anymore");
			break;
		}

		// Store SKs to clean up packet buffers in the end
		sk_t skTmp = (sk_t)*socketFdIt;//convert int to sk_t
		obsoleteSessionKeys.push_front(skTmp);
	}

	obsoleteSessionKeysIt = obsoleteSessionKeys.begin();

	while (obsoleteSessionKeysIt != obsoleteSessionKeys.end())
	{
		for (socketFdIt = enigmaIt->second.begin();
							socketFdIt != enigmaIt->second.end(); socketFdIt++)
		{
			// Delete socket FD
			if (*socketFdIt == (int)*obsoleteSessionKeysIt)
			{
				LOG4CXX_TRACE(logger, "Socket FD " << *obsoleteSessionKeysIt
						<< " deleted from list of IP endpoints for rCID "
						<< rCid.print() << " > Engima " << enigma);
				enigmaIt->second.erase(socketFdIt);
				break;
			}
		}

		obsoleteSessionKeysIt++;
	}

	_ipEndpointSessionsMutex.unlock();

	// now clean up the HTTP packet buffer for all stored SKs
	while (!obsoleteSessionKeys.empty())
	{
		auto skIt = obsoleteSessionKeys.begin();
		_deleteBufferedRequest(cid, rCid, enigma, *skIt);
		obsoleteSessionKeys.pop_front();
	}
}

void Http::deleteSession(string fqdn, string resource, sk_t sessionKey)
{
	deleteSessionKey(sessionKey);
	IcnId cid(fqdn);
	IcnId rCid(fqdn, resource);
	_transport.Lightweight::publishEndOfSession(cid, rCid, sessionKey);
}

void Http::handleDnsLocal(uint32_t &hashedFqdn)
{
	IcnId cid(hashedFqdn);
	forwarding(cid, false);

	// HTTP packet buffer has all request for which HTTP responses are to be or
	// being received
	_packetBufferRequestsMutex.lock();
	auto cidIt = _packetBufferRequests.find(cid.uint());

	if (cidIt == _packetBufferRequests.end())
	{
		LOG4CXX_TRACE(logger, "CID could not be found in HTTP packet buffer");
		_packetBufferRequestsMutex.unlock();
		_icnCore->unpublish_info(cid.binId(), cid.binPrefixId(), DOMAIN_LOCAL,
				NULL, 0);
		LOG4CXX_DEBUG(logger,"Unpublished availablility of information for CID "
				<< cid.print());
		_icnCore->publish_info(cid.binId(), cid.binPrefixId(), DOMAIN_LOCAL,
				NULL, 0);
		LOG4CXX_DEBUG(logger,"Published availablility of information for CID "
						<< cid.print());
		return;
	}

	// if list of rCIDs for CID is not empty set even pause flag
	if (!cidIt->second.empty())
	{
		pausing(cid, true);
	}
	// if list is indeed empty straight away to new subscriber
	else
	{
		_icnCore->unpublish_info(cid.binId(), cid.binPrefixId(), DOMAIN_LOCAL,
				NULL, 0);
		LOG4CXX_DEBUG(logger,"Unpublished availablility of information for CID "
				<< cid.print());
		_icnCore->publish_info(cid.binId(), cid.binPrefixId(), DOMAIN_LOCAL,
				NULL, 0);
		LOG4CXX_DEBUG(logger,"Published availablility of information for CID "
						<< cid.print());
	}

	for (auto rCidIt = cidIt->second.begin(); rCidIt != cidIt->second.end();
			rCidIt++)
	{
		LOG4CXX_TRACE(logger, "Calling publishReset for CID " << cid.print()
				<< " > " << rCidIt->second.rCId.print());
		_transport.Lightweight::publishReset(rCidIt->second.cId,
				rCidIt->second.rCId);
	}

	_packetBufferRequestsMutex.unlock();
}

void Http::forwarding(IcnId &cId, bool state)
{
	// first disable pausing for this CID in case the cNAP might switch to a new
	// surrogate server
	if (pausing(cId) && state)
	{
		pausing(cId, false);
	}

	_mutexIcnIds.lock();
	_cIdsIt = _cIds.find(cId.uint());

	// CID does not exist in icn maps
	if (_cIdsIt == _cIds.end())
	{
		_rCIdsIt = _rCIds.find(cId.uint());

		// CID does not exist in iSub icn map either
		if (_rCIdsIt == _rCIds.end())
		{
			LOG4CXX_WARN(logger, "CID " << cId.print() << " is unknown. "
				"Forwarding state cannot be changed");
			_mutexIcnIds.unlock();
			return;
		}
	}

	// CID is HTTP request
	if (_cIdsIt != _cIds.end())
	{
		_cIdsIt->second.forwarding(state);
		LOG4CXX_DEBUG(logger, "Forwarding for CID " << cId.print() << " set "
				<< "to " << state);

		if (state)
		{
			LOG4CXX_DEBUG(logger, "'FID requested' state for CID "
					<< cId.print() << " disabled");
			_cIdsIt->second.fidRequested(false);
			_transport.Lightweight::forwarding(cId, state);//DNSlocal scenarios
			//in cNAPs
		}
	}
	// CID is HTTP response
	else if (_rCIdsIt != _rCIds.end())
	{
		_rCIdsIt->second.forwarding(state);
		LOG4CXX_DEBUG(logger, "Forwarding for rCID " << cId.print()
				<< " set to " << state);

		if (state)
		{
			LOG4CXX_DEBUG(logger, "'FID requested' state for rCID "
					<< cId.print() << " disabled");
			_cIdsIt->second.fidRequested(false);
		}
	}
	// error case. should not occur, but just in case
	else
	{
		LOG4CXX_ERROR(logger, "CID " << cId.print() << " initially found but "
				"iterators say differently?!");
	}

	_mutexIcnIds.unlock();
}

void Http::forwarding(NodeId &nodeId, bool state)
{
	_nIdsMutex.lock();
	_nIdsIt = _nIds.find(nodeId.uint());

	// New NID
	if (_nIdsIt == _nIds.end())
	{
		LOG4CXX_WARN(logger, "NID " << nodeId.uint() << " is not known. "
				"Ignoring forwarding state change");
	}
	// known NID
	else
	{
		_nIdsIt->second = state;
		LOG4CXX_DEBUG(logger, "State for existing NID " << nodeId.uint()
				<< " updated to " << state);
	}

	_nIdsMutex.unlock();
}

void Http::initialise()
{
	if (!_configuration.httpHandler())
	{
		return;
	}

	list<pair<IcnId, pair<IpAddress, uint16_t>>> fqdns = _configuration.fqdns();
	list<pair<IcnId, pair<IpAddress, uint16_t>>>::iterator fqdnsIt;
	IcnId cid("video.point");//Dummy CID to publish root scope
	_icnCore->publish_scope(cid.binRootScopeId(), cid.binEmpty(), DOMAIN_LOCAL,
			NULL, 0);
	LOG4CXX_DEBUG(logger, "Publish HTTP root scope to domain local RV");

	for (fqdnsIt = fqdns.begin(); fqdnsIt != fqdns.end(); fqdnsIt++)
	{
		subscribeToFqdn(fqdnsIt->first);
		// reporting this server to the statistics module
		_statistics.ipEndpointAdd(fqdnsIt->second.first,
				fqdnsIt->first.printFqdn(), fqdnsIt->second.second);
	}

	LOG4CXX_INFO(logger, "HTTP namespace successfully initialised");
	// Initialise LTP with the pointers to the CMC and NID maps and mutexes
	_transport.Lightweight::initialise((void *)&_potentialCmcGroups,
			(void *)&_potentialCmcGroupsMutex, (void *)&_nIds,
			(void *)&_nIdsMutex, (void *)&_cmcGroups, (void *)&_cmcGroupsMutex);
}

void Http::lockPotentialCmcGroupMembership()
{
	_potentialCmcGroupsMutex.lock();
	_potentialCmcGroupsOpen = false;
	_potentialCmcGroupsMutex.unlock();
}

bool Http::pausing(IcnId &cid)
{
	bool state;
	_mutexIcnIds.lock();
	// check CIDs
	_cIdsIt = _cIds.find(cid.uint());

	if (_cIdsIt != _cIds.end())
	{
		state = _cIdsIt->second.pausing();
		_mutexIcnIds.unlock();
		return state;
	}

	//check rCIDs
	_rCIdsIt = _rCIds.find(cid.uint());

	if (_rCIdsIt != _rCIds.end())
	{
		state = _rCIdsIt->second.pausing();
		_mutexIcnIds.unlock();
		return state;
	}

	_mutexIcnIds.unlock();
	return false;
}

void Http::pausing(IcnId &cid, bool state)
{
	_mutexIcnIds.lock();
	_cIdsIt = _cIds.find(cid.uint());

	if (_cIdsIt != _cIds.end())
	{
		_cIdsIt->second.pausing(state);
		_mutexIcnIds.unlock();
		LOG4CXX_DEBUG(logger, "CID " << cid.print() << " state set to "
				<< state);
		return;
	}

	_rCIdsIt = _rCIds.find(cid.uint());

	if (_rCIdsIt != _rCIds.end())
	{
		_rCIdsIt->second.pausing(state);
		_mutexIcnIds.unlock();
		LOG4CXX_DEBUG(logger, "rCID " << cid.print() << " state set to "
				<< state);
		return;
	}

	_mutexIcnIds.unlock();
}

void Http::publishFromBuffer(IcnId &cId)
{
	map<cid_t, request_packet_t>::iterator packetBufferIt;
	_packetBufferRequestsMutex.lock();
	_packetBufferRequestsIt = _packetBufferRequests.find(cId.uint());

	// HTTP request(s) to be published
	if (_packetBufferRequestsIt != _packetBufferRequests.end())
	{
		// Iterate over the entire iSub CID map which is the value to the key
		for (packetBufferIt = _packetBufferRequestsIt->second.begin();
				packetBufferIt != _packetBufferRequestsIt->second.end();
				packetBufferIt++)
		{
			// now publish the packet
			LOG4CXX_TRACE(logger, "Packet of length "
					<< packetBufferIt->second.packetSize << " retrieved from "
					"buffer and handed over to LTP for publication under CID "
					<< packetBufferIt->second.cId.print());
			_transport.Lightweight::publishFromBuffer(
					packetBufferIt->second.cId,
					packetBufferIt->second.rCId,
					packetBufferIt->second.enigma,
					packetBufferIt->second.sessionKey,
					packetBufferIt->second.packet,
					packetBufferIt->second.packetSize);

			/* free up allocated memory for packet TODO remove this obsolete code
			*free(packetBufferIt->second.packet);
			*/
		}

		/* now delete all packets that were buffered for this particular CID TODO
		 * remove this obsolete code
		*LOG4CXX_TRACE(logger, _packetBufferRequestsIt->second.size()
		*		<< " packet(s) deleted from buffer for CID " << cId.print());
		*_packetBufferRequests.erase(_packetBufferRequestsIt);
		*/
		_packetBufferRequestsMutex.unlock();
		return;
	}

	_packetBufferRequestsMutex.unlock();
	LOG4CXX_TRACE(logger, "No packet could be found in buffer for CID "
			<< cId.print());
}

void Http::sendToEndpoint(IcnId &rCid, uint32_t &enigma, uint8_t *packet,
		uint16_t packetSize)
{
	list<socket_fd_t>::reverse_iterator socketFdIt;
	map<enigma_t, list<socket_fd_t>>::iterator enigmaIt;
	ostringstream socketFds;
	uint8_t *p = (uint8_t *)malloc(packetSize);
	memcpy(p, packet, packetSize);
	// First get the socket FD for this IP endpoints (multiple possible!)
	// FIXME Make sure multiple TCP client were really in the same CMC group
	// (Engima!)
	_ipEndpointSessionsMutex.lock();
	_ipEndpointSessionsIt = _ipEndpointSessions.find(rCid.uint());

	if (_ipEndpointSessionsIt == _ipEndpointSessions.end())
	{
		LOG4CXX_DEBUG(logger, "rCID " << rCid.print() << " does not exist in "
				"IP endpoint sessions map with " << _ipEndpointSessions.size()
				<< " keys. Dropping packet");
		_ipEndpointSessionsMutex.unlock();
		return;
	}

	enigmaIt = _ipEndpointSessionsIt->second.find(enigma);

	if (enigmaIt == _ipEndpointSessionsIt->second.end())
	{
		LOG4CXX_DEBUG(logger, "Engima " << enigma << " does not exist in "
				"IP endpoint session map with " << _ipEndpointSessions.size()
				<< " keys under rCID " << rCid.print() << ". Dropping packet");
		_ipEndpointSessionsMutex.unlock();
		return;
	}

	for (auto it = enigmaIt->second.begin(); it != enigmaIt->second.end(); it++)
	{
		socketFds << *it << " ";
	}

	LOG4CXX_TRACE(logger, "IP packet for rCID " << rCid.print() << " > Engima "
			<< enigmaIt->first << " will be placed on " << enigmaIt->second.size()
			<< " socket FD(s) " << socketFds.str() << " (if more than one "
			"socket FD is listed the last one is only used -> Issue #2");
	// now iterate over all IP endpoints that await the HTTP response
	int bytesWritten;
	//uint8_t offset;
	fd_set writeSet;

	//FIXME Just take the last client for this rCID > Engima. Issue #2
	socketFdIt = enigmaIt->second.rbegin();
//	for (socketFdIt = enigmaIt->second.begin();
//			socketFdIt != enigmaIt->second.end(); socketFdIt++)
	if (socketFdIt != enigmaIt->second.rend())
	{
		bytesWritten = 0;
	//	offset = 0;

		// Write until all bytes have been sent off
		while(bytesWritten != packetSize)
		{
			FD_ZERO(&writeSet);
			FD_SET(*socketFdIt, &writeSet);

			if (FD_ISSET(*socketFdIt, &writeSet))
			{
				//bytesWritten = write(*socketFdIt, p + offset, packetSize
				//		- offset);
				//bytesWritten = write(*socketFdIt, p, packetSize);
				bytesWritten = send(*socketFdIt, p, packetSize, MSG_NOSIGNAL);
			}
			else
			{
				LOG4CXX_WARN(logger, "TCP Socket not writeable anymore for "
						"rCID " << rCid.print() << " > Engima " << enigmaIt->first);
				break;
			}

			// write again
			if (bytesWritten < 0 && errno == EINTR)
			{
				LOG4CXX_DEBUG(logger, "HTTP response of length "
						<< packetSize << " could not be sent to IP endpoint"
						<< " using FD " << *socketFdIt << ". Will try again");
				bytesWritten = 0;
			}
			else if (bytesWritten == -1)
			{
				LOG4CXX_WARN(logger, "HTTP response of length "
						<< packetSize << " could not be sent to IP endpoint"
						" using FD " << *socketFdIt << ": "
						<< strerror(errno));
				break;
			}
			/*else if(bytesWritten < (packetSize - offset))
			{
				offset += bytesWritten;
				LOG4CXX_TRACE(logger, "HTTP response of length "
						<< packetSize << " could not be fully sent to IP "
						"endpoint using FD " << *socketFdIt << ". Only "
						<< bytesWritten << " bytes were written");//. Sending "
						//"off the remaining bits");
			}*/
			else
			{
				LOG4CXX_TRACE(logger, "Entire HTTP response of length "
						<< bytesWritten	<< " sent off to IP endpoint using "
						"socket FD " << *socketFdIt);
				_statistics.txHttpBytes(&bytesWritten);
			}
		}
	}

	_ipEndpointSessionsMutex.unlock();
	free(p);
}

void Http::subscribeToFqdn(IcnId &cid)
{
	_icnCore->subscribe_info(cid.binId(), cid.binPrefixId(), DOMAIN_LOCAL, NULL,
			0);
	LOG4CXX_DEBUG(logger, "Subscribed to " << cid.print() << " ("
			<< cid.printFqdn() << ")");
}

void Http::uninitialise()
{
	if (!_configuration.httpHandler())
	{
		return;
	}

	//TODO _icnCore->unsubscribe_info()
	//TODO clean LTP buffers (deleting allocated memory for packet pointers)
	LOG4CXX_INFO(logger, "HTTP namespace uninitialised");
}

void Http::unlockPotentialCmcGroupMembership()
{
	_potentialCmcGroupsMutex.lock();
	_potentialCmcGroupsOpen = true;
	_potentialCmcGroupsMutex.unlock();
}

void Http::unsubscribeFromFqdn(IcnId &cid)
{
	_icnCore->unsubscribe_info(cid.binId(), cid.binPrefixId(), DOMAIN_LOCAL,
			NULL, 0);
	LOG4CXX_DEBUG(logger, "Unsubscribed from " << cid.print());
}

void Http::_addIpEndpointSession(IcnId &rCId, enigma_t &enigma,
			socket_fd_t socketFd)
{
	_ipEndpointSessionsMutex.lock();
	_ipEndpointSessionsIt = _ipEndpointSessions.find(rCId.uint());

	// New rCID
	if (_ipEndpointSessionsIt == _ipEndpointSessions.end())
	{
		list<socket_fd_t> socketFdList;
		map<enigma_t, list<socket_fd_t>> enigmaMap;
		socketFdList.push_back(socketFd);
		enigmaMap.insert(pair<enigma_t, list<socket_fd_t>>(enigma,
				socketFdList));
		_ipEndpointSessions.insert(pair<cid_t, map<enigma_t, list<socket_fd_t>>>
				(rCId.uint(), enigmaMap));
		LOG4CXX_TRACE(logger, "New SK " << socketFd << " added to new rCID "
				<< rCId.print() << " > Engima " << enigma << " map");
		_ipEndpointSessionsMutex.unlock();
		return;
	}

	// rCID exists
	map<enigma_t, list<socket_fd_t>>::iterator enigmaMapIt;
	enigmaMapIt = _ipEndpointSessionsIt->second.find(enigma);

	// new Engima
	if (enigmaMapIt == _ipEndpointSessionsIt->second.end())
	{
		list<socket_fd_t> socketFdList;
		socketFdList.push_back(socketFd);
		_ipEndpointSessionsIt->second.insert(pair<enigma_t, list<socket_fd_t>>
				(enigma, socketFdList));
		LOG4CXX_TRACE(logger, "New SK " << socketFd << " added to known rCID "
				<< rCId.print() << " but new Engima " << enigma << " map");
		_ipEndpointSessionsMutex.unlock();
		return;
	}

	// Engima exists. Iterate over list to find session key. If it does exist,
	// exit. If not, add it
	list<socket_fd_t>::iterator socketFdListIt;
	socketFdListIt = enigmaMapIt->second.begin();

	while (socketFdListIt != enigmaMapIt->second.end())
	{
		if (*socketFdListIt == socketFd)
		{
			_ipEndpointSessionsMutex.unlock();
			return;
		}

		socketFdListIt++;
	}

	// Session key does not exist on list. Add it now
	enigmaMapIt->second.push_back(socketFd);
	LOG4CXX_TRACE(logger, "New socket FD " << socketFd << " added to rCID "
			<< rCId.print() << " > Engima " << enigma << " map of known IP "
			"endpoint sessions");
	_ipEndpointSessionsMutex.unlock();
}

void Http::_bufferRequest(IcnId &cId, IcnId &rCId, enigma_t &enigma,
		sk_t &sessionKey, http_methods_t httpMethod, uint8_t *packet,
		uint16_t &packetSize)
{
	map<cid_t, request_packet_t>::iterator packetBufferIt;
	_packetBufferRequestsMutex.lock();
	_packetBufferRequestsIt = _packetBufferRequests.find(cId.uint());

	// ICN ID does not exist
	if (_packetBufferRequestsIt == _packetBufferRequests.end())
	{
		request_packet_t packetDescription;
		packetDescription.cId = cId;
		packetDescription.rCId = rCId;
		packetDescription.enigma = enigma;
		packetDescription.sessionKey = sessionKey;
		packetDescription.httpMethod = httpMethod;
		packetDescription.packet = (uint8_t *)malloc(packetSize);
		memcpy(packetDescription.packet, packet, packetSize);
		packetDescription.packetSize = packetSize;
		packetDescription.timestamp =
				boost::posix_time::second_clock::local_time();
		map<uint32_t, request_packet_t> rCIdMap;
		rCIdMap.insert(pair<uint32_t, request_packet_t>(rCId.uint(),
				packetDescription));
		_packetBufferRequests.insert(pair<uint32_t,map<uint32_t,
				request_packet_t>>(cId.uint(), rCIdMap));
		LOG4CXX_TRACE(logger, "Packet for new CID " << cId.print() << " (FQDN: "
				<< cId.printFqdn() << "), rCID " << rCId.print() << " and "
				"SK " << sessionKey <<" of length " << packetSize << " has been"
						" added to HTTP packet buffer");
	}
	// ICN ID does exist
	else
	{
		// check if rCID exist
		packetBufferIt = (*_packetBufferRequestsIt).second.find(rCId.uint());
		if (packetBufferIt == (*_packetBufferRequestsIt).second.end())
		{
			request_packet_t packetDescription;
			packetDescription.cId = cId;
			packetDescription.rCId = rCId;
			packetDescription.enigma = enigma;
			packetDescription.sessionKey = sessionKey;
			packetDescription.httpMethod = httpMethod;
			packetDescription.packet = (uint8_t *)malloc(packetSize);
			memcpy(packetDescription.packet, packet, packetSize);
			packetDescription.packetSize = packetSize;
			packetDescription.timestamp =
							boost::posix_time::second_clock::local_time();
			(*_packetBufferRequestsIt).second.insert(pair<uint32_t,
					request_packet_t>(rCId.uint(),packetDescription));
			LOG4CXX_TRACE(logger, "Packet for existing CID " << cId.print()
					<< " (FQDN: " << cId.printFqdn() << ") but new rCID "
					"of length " << packetSize << " has been added to HTTP "
					"packet buffer (Engima " << enigma << ", SK "
					<< sessionKey << ")");
		}
		// rCId exists. updating packet description
		else
		{
			packetBufferIt->second.cId = cId;
			packetBufferIt->second.rCId = rCId;
			packetBufferIt->second.enigma = enigma;
			packetBufferIt->second.sessionKey = sessionKey;
			packetBufferIt->second.httpMethod = httpMethod;
			free(packetBufferIt->second.packet);
			packetBufferIt->second.packet = (uint8_t *)malloc(packetSize);
			memcpy(packetBufferIt->second.packet, packet, packetSize);
			packetBufferIt->second.packetSize = packetSize;
			packetBufferIt->second.timestamp =
							boost::posix_time::second_clock::local_time();
			LOG4CXX_TRACE(logger, "Packet for existing rCID " << rCId.print()
					<< " (FQDN: " << cId.printFqdn() << ") "
					"of length " << packetSize << " has been added to HTTP "
					"packet buffer");
		}
	}
	_packetBufferRequestsMutex.unlock();
}

void Http::_createCmcGroup(IcnId &rCid, uint32_t &enigma,
		uint16_t &sessionKey, list<NodeId> &nodeIds)
{
	//map<SK,     list<NID
	map<uint16_t, list<NodeId>> skMap;
	map<uint16_t, list<NodeId>>::iterator skIt;
	//map<Engima,   map<SK,       list<NID
	map<uint32_t, map<uint16_t, list<NodeId>>> enigmaMap;
	map<uint32_t, map<uint16_t, list<NodeId>>>::iterator enigmaIt;
	skMap.insert(pair<uint32_t, list<NodeId>>(sessionKey, nodeIds));
	enigmaMap.insert(pair<uint32_t, map<uint16_t, list<NodeId>>>(enigma,
			skMap));
	ostringstream nodeIdsOss;

	for (list<NodeId>::iterator it = nodeIds.begin(); it != nodeIds.end(); it++)
	{
		nodeIdsOss << it->uint() << " ";
	}

	_cmcGroupsMutex.lock();
	_cmcGroupsIt = _cmcGroups.find(rCid.uint());

	// rCID does not exist
	if (_cmcGroupsIt == _cmcGroups.end())
	{
		_cmcGroups.insert(pair<uint32_t, map<uint32_t, map<uint16_t,
				list<NodeId>>>>(rCid.uint(), enigmaMap));
		LOG4CXX_TRACE(logger, "CMC group created for rCID " << rCid.print()
				<< " > Engima " << enigma << " > SK " << sessionKey
				<< " with " << nodeIds.size() << " NID(s): " << nodeIdsOss.str());
		_cmcGroupsMutex.unlock();
		return;
	}

	enigmaIt = _cmcGroupsIt->second.find(enigma);
	// Engima does not exist
	if (enigmaIt == _cmcGroupsIt->second.end())
	{
		_cmcGroupsIt->second.insert(pair<uint32_t, map<uint16_t, list<NodeId>>>
				(enigma, skMap));
		LOG4CXX_DEBUG(logger, "CMC group created for existing rCID "
				<< rCid.print() << " but new Engima " << enigma
				<< " > SK " << sessionKey << " with " << nodeIds.size()
				<< " NID(s): " << nodeIdsOss.str());
		_cmcGroupsMutex.unlock();
		return;
	}
	skIt = enigmaIt->second.find(sessionKey);
	// SK does not exist
	if (skIt == enigmaIt->second.end())
	{
		enigmaIt->second.insert(pair<uint16_t, list<NodeId>>(sessionKey,
				nodeIds));
		LOG4CXX_TRACE(logger, "CMC group create for existing rCID "
				<< rCid.print() << " > Engima " << enigma << " but new SK "
				<< sessionKey << " with " << nodeIds.size() << " NID(s): "
				<< nodeIdsOss.str());
		_cmcGroupsMutex.unlock();
		return;
	}
	if (skIt->second.empty())
	{
		LOG4CXX_DEBUG(logger, "CMC group for rCID " << rCid.print()
				<< " > Engima " << enigma << " > SK " << sessionKey
				<< " exists but is empty. " << nodeIds.size() << " NIDs will be"
				" added: " << nodeIdsOss.str());
		skIt->second = nodeIds;
	}
	else
	{
		LOG4CXX_TRACE(logger, "CMC for rCID " << rCid.print() << " > Engima "
				<< enigma << " > SK " << sessionKey << " already exists");
	}
	_cmcGroupsMutex.unlock();
}

void Http::_deleteBufferedRequest(IcnId &cid, IcnId &rCid, enigma_t &enigma,
			sk_t &sessionKey)
{
	_packetBufferRequestsMutex.lock();
	auto cidIt = _packetBufferRequests.find(cid.uint());

	// CID not found
	if (cidIt == _packetBufferRequests.end())
	{
		_packetBufferRequestsMutex.unlock();
		LOG4CXX_TRACE(logger, "CID " << cid.print() << " was not be found in "
				"HTTP packet buffer");
		return;
	}

	auto rCidIt = cidIt->second.find(rCid.uint());

	// rCID not found
	if (rCidIt == cidIt->second.end())
	{
		_packetBufferRequestsMutex.unlock();
		LOG4CXX_TRACE(logger, "CID " << cid.print() << " -> rCID "
				<< rCid.print() << " was not found in HTTP packet buffer");
		return;
	}

	// sanity check that Engima and SK match. Even if they don't delete packet :P
	if (rCidIt->second.enigma != enigma ||
			rCidIt->second.sessionKey != sessionKey)
	{
		LOG4CXX_DEBUG(logger, " Engima " << enigma << " and/or SK "
				<< sessionKey << " did not match in HTTP packet buffer. "
				"Deleting packet anyway");
	}

	// free memory and erase packet
	free(rCidIt->second.packet);
	cidIt->second.erase(rCidIt);
	LOG4CXX_TRACE(logger, "HTTP request deleted from HTTP packet buffer for "
			"CID " << cid.print() << " > rCID " << rCid.print() << " > RRID "
			<< enigma << " > SK " << sessionKey);
	_packetBufferRequestsMutex.unlock();
}

void Http::_deleterCids(NodeId &nodeId)
{
	_reverseNidTorCIdLookUpMutex.lock();
	_reverseNidTorCIdLookUpIt = _reverseNidTorCIdLookUp.find(nodeId.uint());

	if (_reverseNidTorCIdLookUpIt != _reverseNidTorCIdLookUp.end())
	{
		_reverseNidTorCIdLookUp.erase(_reverseNidTorCIdLookUpIt);
		LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " erased from reverse "
				"NID > rCID look up table");
	}

	_reverseNidTorCIdLookUpMutex.unlock();
}

bool Http::_forwardingEnabled(uint32_t nodeId)
{
	_nIdsMutex.lock();
	_nIdsIt = _nIds.find(nodeId);
	if (_nIdsIt == _nIds.end())
	{
		LOG4CXX_WARN(logger, "NID " << nodeId << " does not exist in known NIDs"
				" table");
		_nIdsMutex.unlock();
		return false;
	}
	if (_nIdsIt->second)
	{
		_nIdsMutex.unlock();
		return true;
	}
	_nIdsMutex.unlock();
	return false;
}

bool Http::_getCmcGroup(IcnId &rCid, uint32_t &enigma,
		uint16_t &sessionKey, bool firstPacket, list<NodeId> &nodeIds)
{
	list<NodeId>::iterator nodeIdsIt;
	bool bufferPacket = false;
	// Check if an already created CMC group is awaiting more data
	_cmcGroupsMutex.lock();
	_cmcGroupsIt = _cmcGroups.find(rCid.uint());
	// rCID found
	if (_cmcGroupsIt != _cmcGroups.end())
	{
		//map<Engima,   map<SK,       list<NIDs
		map<uint32_t, map<uint16_t, list<NodeId>>>::iterator cmcPridIt;
		cmcPridIt = _cmcGroupsIt->second.find(enigma);
		// Engima found
		if (cmcPridIt != _cmcGroupsIt->second.end())
		{
			//map<SK,     list<NIDs
			map<uint16_t, list<NodeId>>::iterator cmcSkIt;
			cmcSkIt = cmcPridIt->second.find(sessionKey);
			// Session key found
			if (cmcSkIt != cmcPridIt->second.end())
			{
				//list of NIDs not empty
				if (!cmcSkIt->second.empty())
				{
					nodeIds = cmcSkIt->second;
					LOG4CXX_TRACE(logger, "CMC group of " << nodeIds.size()
							<< " NID(s) found for rCID " << rCid.print() << " >"
							" Engima " << enigma << " > SK " << sessionKey);
					_cmcGroupsMutex.unlock();
					return false;
				}
				else
				{
					LOG4CXX_TRACE(logger, "CMC group is empty for rCID "
							<< rCid.print() << " > Engima " << enigma
							<< " > SK "	<< sessionKey << ". START_PUBLISH_iSUB "
							"is probably missing for all of the CMC's members");
				}
			}
			else
			{
				LOG4CXX_TRACE(logger, "No existing CMC group known for rCID "
						<< rCid.print() << " > Engima " << enigma << " > SK "
						<< sessionKey);
			}
		}
		else
		{
			LOG4CXX_TRACE(logger, "No existing CMC group known for rCID "
					<< rCid.print() << " > Engima " << enigma);
		}
	}
	else
	{
		LOG4CXX_TRACE(logger, "No existing CMC group known for rCID "
				<< rCid.print() << ". Checking potential CMC groups");

	}

	_cmcGroupsMutex.unlock();

	if (!_potentialCmcGroupsOpen)
	{
		LOG4CXX_TRACE(logger, "Potential CMC group membership locked at the "
				"moment (DNSlocal procedures active)");
		return false;
	}

	if (!firstPacket)
	{
		LOG4CXX_TRACE(logger, "This is not the first time a packet needs to be "
				"sent to cNAPs. No existing CMC group could be found though "
				"for rCID " << rCid.print() << " > Engima " << enigma
				<< " > SK " << sessionKey << " (# of CMC groups = "
				<< _cmcGroups.size() << ")");
		return false;
	}
	/* If CMC group did not have any entry for rCID > Engima look up potential CMC
	 * groups map. Note, it is important to not add NIDs from the potential CMC
	 * groups map into an existing CMC group. So if this was the first packet
	 * from the server and no existing CMC group could be found, look up the
	 * potential list of NIDs and create a new one.
	 */
	//map<NID,    ready to receive HTTP response
	map<uint32_t, bool>::iterator pCmcNidIt;
	//map<Engima,   map<NID,      ready to receive HTTP response
	map<uint32_t, map<uint32_t, bool>>::iterator pCmcPridIt;
	// Check potential CMC groups map in case CMC group did not have an NID
	if (nodeIds.empty())
	{
		_potentialCmcGroupsMutex.lock();
		_potentialCmcGroupsIt = _potentialCmcGroups.find(rCid.uint());
		//rCID does not exist
		if (_potentialCmcGroupsIt == _potentialCmcGroups.end())
		{
			LOG4CXX_TRACE(logger, "rCID " << rCid.print() << " could not be "
					"found in potential CMC group map");
			_potentialCmcGroupsMutex.unlock();
			return false;
		}
		pCmcPridIt = _potentialCmcGroupsIt->second.find(enigma);
		if (pCmcPridIt == _potentialCmcGroupsIt->second.end())
		{
			LOG4CXX_WARN(logger, "Engima " << enigma << ", value of rCID "
					<< rCid.print() << " could not be found in potential CMC "
					"group map");
			_potentialCmcGroupsMutex.unlock();
			return false;
		}
		// iterate over NIDs
		for (pCmcNidIt = pCmcPridIt->second.begin();
				pCmcNidIt != pCmcPridIt->second.end(); pCmcNidIt++)
		{
			//NID has sent off the entire request and is awaiting the HTTP
			//response
			if (pCmcNidIt->second)
			{
				// FID is still missing
				if (!_forwardingEnabled(pCmcNidIt->first))
				{
					LOG4CXX_TRACE(logger, "START_PUBLISH_iSUB has not been "
							"received for NID " << pCmcNidIt->first);
					bufferPacket = true;
				}
				else
				{
					NodeId nodeId(pCmcNidIt->first);
					nodeIds.push_back(nodeId);
					LOG4CXX_TRACE(logger, "NID " << nodeId.uint() << " moved to"
							" CMC group of size " << nodeIds.size() << " for "
							"rCID " << rCid.print() << " > Engima "
							<< enigma << " > SK " << sessionKey);
				}
			}
			else
			{
				LOG4CXX_TRACE(logger, "NID " << pCmcNidIt->first << " hasn't "
						"completed delivery of entire HTTP request");
			}
		}
		// Add all selected NIDs to a new CMC group
		if (!nodeIds.empty())
		{
			_createCmcGroup(rCid, enigma, sessionKey, nodeIds);
		}
		else
		{
			LOG4CXX_TRACE(logger, "No NID was ready to be moved to CMC group");
			_potentialCmcGroupsMutex.unlock();
			return true;//bufferPacket = true
		}
		// Delete the NIDs which have been added to the CMC group
		for (nodeIdsIt = nodeIds.begin(); nodeIdsIt != nodeIds.end();
				nodeIdsIt++)
		{
			pCmcNidIt = pCmcPridIt->second.find(nodeIdsIt->uint());
			pCmcPridIt->second.erase(pCmcNidIt);
			LOG4CXX_TRACE(logger, "NID " << nodeIdsIt->uint()
					<< " removed from potential CMC group. FYI "
					<< pCmcPridIt->second.size() << " remaining NIDs in this "
							"list");
		}
		// If list of NIDs is empty, remove Engima
		if (pCmcPridIt->second.empty())
		{
			_potentialCmcGroupsIt->second.erase(pCmcPridIt);
			LOG4CXX_TRACE(logger, "List of NIDs is empty in potential CMC group"
					" for rCID " << rCid.print() << " > Engima " << enigma
					<< ". Engima entry deleted");
		}
		// If this was the last Engima for this rCID, delete rCID entry too
		if (_potentialCmcGroupsIt->second.empty())
		{
			_potentialCmcGroups.erase(_potentialCmcGroupsIt);
			LOG4CXX_TRACE(logger, "List of Engimas is empty in potential CMC "
					"group for rCID " << rCid.print()
					<< ". rCID entry deleted");
		}
		_potentialCmcGroupsMutex.unlock();
	}
	return bufferPacket;
}
list<uint32_t> Http::_getrCids(NodeId &nodeId)
{
	list<uint32_t> rCids;
	_reverseNidTorCIdLookUpMutex.lock();
	_reverseNidTorCIdLookUpIt = _reverseNidTorCIdLookUp.find(nodeId.uint());
	if (_reverseNidTorCIdLookUpIt == _reverseNidTorCIdLookUp.end())
	{
		LOG4CXX_WARN(logger, "NID " << nodeId.uint() << " does not exist in "
				"reverse NID to rCID look up map");
		_reverseNidTorCIdLookUpMutex.unlock();
		return rCids;
	}
	rCids = _reverseNidTorCIdLookUpIt->second;
	_reverseNidTorCIdLookUpMutex.unlock();
	return rCids;
}

uint8_t Http::_numberOfSessionKeys(IcnId &rCId, enigma_t &enigma)
{
	uint8_t numberOfSessionKeys = 0;
	_ipEndpointSessionsMutex.lock();
	_ipEndpointSessionsIt = _ipEndpointSessions.find(rCId.uint());

	// rCID found
	if (_ipEndpointSessionsIt != _ipEndpointSessions.end())
	{
		map<enigma_t, list<socket_fd_t>>::iterator enigmaMapIt;
		enigmaMapIt = _ipEndpointSessionsIt->second.find(enigma);

		// Engima found
		if (enigmaMapIt != _ipEndpointSessionsIt->second.end())
		{
			numberOfSessionKeys = enigmaMapIt->second.size();
		}
		else
		{
			LOG4CXX_WARN(logger, "Engima " << enigma << " could not be found"
					" for known rCID " << rCId.print() << " in map of active"
							"sessions");
		}
	}
	else
	{
		LOG4CXX_WARN(logger, "rCID " << rCId.print() << " could not be found in"
				" map of active sessions");
	}

	_ipEndpointSessionsMutex.unlock();
	return numberOfSessionKeys;
}
