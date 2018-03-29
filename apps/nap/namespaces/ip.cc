/*
 * ip.cc
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

#include <thread>

#include "ip.hh"

using namespace namespaces::ip;
using namespace log4cxx;

LoggerPtr Ip::logger(Logger::getLogger("namespaces.ip"));

Ip::Ip(Blackadder *icnCore, Configuration &configuration, Transport &transport,
		Statistics &statistics, bool *run)
	: _icnCore(icnCore),
	  _configuration(configuration),
	  _transport(transport),
	  _statistics(statistics),
	  _run(run)
{
	_routingPrefixes = NULL;
	// Initialising IP buffer cleaner with this ugly void caster
	IpBufferCleaner ipBufferCleaner((void *)&_packetBuffer,
			(void *)&_mutexBuffer, _configuration, _statistics, _run);
	std::thread *ipBufferThread = new std::thread(ipBufferCleaner);
	ipBufferThread->detach();
	delete ipBufferThread;
}

Ip::~Ip(){}

void Ip::handle(IpAddress &destinationIpAddress, uint8_t *packet,
		uint16_t &packetSize)
{
	RoutingPrefix routingPrefix;

	_statistics.rxIpBytes(&packetSize);

	if (!_routingPrefix(destinationIpAddress, routingPrefix))
	{
		LOG4CXX_TRACE(logger, "Routing prefix for destination IP "
				<< destinationIpAddress.str() << " unknown. Dropping packet");
		return;
	}

	LOG4CXX_TRACE(logger, "Routing prefix for " << destinationIpAddress.str()
			<< " is " << routingPrefix.str());
	IcnId cid(routingPrefix, destinationIpAddress);
	_mutexIcnIds.lock();
	_icnIdsIt = _icnIds.find(cid.uint());

	// ICN ID unknown
	if (_icnIdsIt == _icnIds.end())
	{
		_icnIds.insert(pair<uint32_t, IcnId> (cid.uint(), cid));
		_mutexIcnIds.unlock();
		_bufferPacket(cid, packet, packetSize);
		// Advertise IP address information item under scope path
		// / NAMEPSPACE_IP / HASH_ROUTING_PREFIX
		LOG4CXX_DEBUG(logger, "Advertising information item " << cid.id()
				<< " under father scope " << cid.printPrefixId());
		_icnCore->publish_info(cid.binId(), cid.binPrefixId(), DOMAIN_LOCAL,
				NULL, 0);
		return;
	}

	// If known, check if the FID has been received
	if (!(*_icnIdsIt).second.forwarding())
	{
		//forwarding disabled - check that a FID has been requested
		if (_icnIdsIt->second.fidRequested())
		{
			_mutexIcnIds.unlock();
			LOG4CXX_TRACE(logger, "Info item " << cid.print() << " has been "
					"already advertised. Publish_info() is not called again");
			return;
		}

		_mutexIcnIds.unlock();
		_bufferPacket(cid, packet, packetSize);
		LOG4CXX_DEBUG(logger, "Forwarding state of CID " << cid.print()
				<< " disabled. Re-advertising information item " << cid.id()
				<< " under father scope " << cid.printPrefixId());
		_icnCore->publish_info(cid.binId(), cid.binPrefixId(), DOMAIN_LOCAL,
				NULL, 0);
		return;
	}

	_mutexIcnIds.unlock();
	_transport.Unreliable::publish(cid, packet, packetSize);
}

void Ip::subscribeInfo(IcnId &icnId)
{
	_mutexIcnIds.lock();
	_icnId(icnId);
	_icnCore->subscribe_info(icnId.binId(), icnId.binPrefixId(), DOMAIN_LOCAL,
			NULL, 0);
	LOG4CXX_DEBUG(logger, "Subscription to CID " << icnId.print() << " issued");
	_mutexIcnIds.unlock();
}

void Ip::forwarding(IcnId &cid, bool state)
{
	_mutexIcnIds.lock();
	_icnIdsIt = _icnIds.find(cid.uint());

	// Does not exist
	if (_icnIdsIt == _icnIds.end())
	{
		_mutexIcnIds.unlock();
		LOG4CXX_ERROR(logger, "State for CID " << cid.print() << " cannot be "
				"changed. CID does not exist");
		return;
	}

	_icnIdsIt->second.forwarding(state);

	// Forward enabled
	if (state)
	{
		LOG4CXX_DEBUG(logger, "Forwarding state for CID " << cid.print()
				<< " has been enabled");
		LOG4CXX_DEBUG(logger, "'FID requested' state for CID " << cid.print()
						<< " disabled");
		_icnIdsIt->second.fidRequested(false);
	}
	// Forwarding disabled
	else
	{
		LOG4CXX_DEBUG(logger, "Forwarding state for CID " << cid.print()
				<< " disabled");
	}

	_mutexIcnIds.unlock();
}

void Ip::initialise()
{
	IcnId icnId;
	_routingPrefixes = _configuration.hostRoutingPrefixes();
	// Host-based
	if (_configuration.hostBasedNap())
	{
		IcnId ii(_configuration.hostRoutingPrefix(),
				_configuration.endpointIpAddress());
		icnId = ii;
	}
	// Prefix-based (database.hostRoutingPrefix has been already written)
	else
	{
		IcnId ii(_configuration.hostRoutingPrefix());
		icnId = ii;
	}
	// Publishing root scope to RV
	_icnCore->publish_scope(icnId.binRootScopeId(), icnId.binEmpty(),
			DOMAIN_LOCAL, NULL, 0);
	LOG4CXX_DEBUG(logger, "Root scope ID " << icnId.rootScopeId()
			<< " published to RV");
	// Publishing prefix under root scope
	_icnCore->publish_scope(icnId.binScopeId(2), icnId.binScopePath(1),
			DOMAIN_LOCAL, NULL, 0);
	LOG4CXX_DEBUG(logger, "Routing prefix scope ID " << icnId.scopeId(2)
			<< " published under father scope " << icnId.scopePath(1));
	// If host-based NAP subscribe to II /ip/prefix/address
	if (_configuration.hostBasedNap())
	{
		_icnCore->subscribe_info(icnId.binId(), icnId.binPrefixId(),
				DOMAIN_LOCAL, NULL, 0);
		LOG4CXX_DEBUG(logger, "Subscribed to info item " << icnId.print());
	}
	// if prefix-based NAP subscribe to scope /ip/prefix
	else
	{
		_icnCore->subscribe_scope(icnId.binId(), icnId.binPrefixId(), DOMAIN_LOCAL,
			NULL, 0);
		LOG4CXX_DEBUG(logger, "Subscribed to scope  " << icnId.print());
	}
	_mutexIcnIds.lock();
	_icnIds.insert(pair<uint32_t, IcnId>(icnId.uint(), icnId));
	_mutexIcnIds.unlock();
	LOG4CXX_INFO(logger, "IP namespace successfully initialised");
}

void Ip::publishFromBuffer(IcnId &cId)
{
	_mutexBuffer.lock();
	_bufferIt = _packetBuffer.find(cId.uint());
	if (_bufferIt == _packetBuffer.end())
	{
		LOG4CXX_TRACE(logger, "IP buffer has no packet for CID "
				<< cId.print());
		_mutexBuffer.unlock();
		return;
	}
	_transport.Unreliable::publish(cId, (*_bufferIt).second.second.packet,
			(*_bufferIt).second.second.packetSize);
	_deleteBufferedPacket(cId);
	_mutexBuffer.unlock();
}

void Ip::subscribeScope(IcnId &icnId)
{
	_mutexIcnIds.lock();
	_icnId(icnId);
	_icnCore->subscribe_scope(icnId.binId(), icnId.binPrefixId(), DOMAIN_LOCAL,
			NULL, 0);
	LOG4CXX_DEBUG(logger, "Subscription to CID " << icnId.print() << " issued");
	_mutexIcnIds.unlock();
}

void Ip::uninitialise()
{
	if (_configuration.hostBasedNap())
	{
		// Creating CID for /IP/RoutingPrefix/IpAddress
		IcnId icnId(_configuration.hostRoutingPrefix(),
				_configuration.endpointIpAddress());
		_icnCore->unsubscribe_info(icnId.binId(), icnId.binPrefixId(),
				DOMAIN_LOCAL, NULL, 0);
		LOG4CXX_DEBUG(logger, "Unsubscribed from ID " << icnId.id()
				<< " published under father scope " << icnId.printPrefixId());
	}
	else
	{
		IcnId icnId(_configuration.hostRoutingPrefix());
		_icnCore->unsubscribe_scope(icnId.binId(), icnId.binPrefixId(),
				DOMAIN_LOCAL, NULL, 0);
		LOG4CXX_DEBUG(logger, "Unsubscribed from scope " << icnId.id()
				<< " under father scope " << icnId.printPrefixId());
	}

	LOG4CXX_INFO(logger, "IP namespace uninitialised");
}

void Ip::_bufferPacket(IcnId &icnId, uint8_t *packet, uint16_t &packetSize)
{
	_mutexBuffer.lock();
	_bufferIt = _packetBuffer.find(icnId.uint());

	// CID unknown
	if (_bufferIt == _packetBuffer.end())
	{
		pair<IcnId, packet_t> value;
		value.first = icnId;
		value.second.packet = (uint8_t *)malloc(packetSize);
		memcpy(value.second.packet, packet, packetSize);
		value.second.packetSize = packetSize;
		value.second.timestamp =
				boost::posix_time::microsec_clock::local_time();
		_packetBuffer.insert(pair <uint32_t, pair<IcnId, packet_t>>(
				icnId.uint(), value));
		LOG4CXX_TRACE(logger, "Packet of size " << packetSize << " and CID "
				<< icnId.str() << " added to buffer");
	}
	else
	{
		free((*_bufferIt).second.second.packet); // Deleting memory in heap
		(*_bufferIt).second.second.packet = (uint8_t *)malloc(packetSize);
		memcpy((*_bufferIt).second.second.packet, packet, packetSize);
		(*_bufferIt).second.second.packetSize = packetSize;
		(*_bufferIt).second.second.timestamp =
				boost::posix_time::microsec_clock::local_time();
		LOG4CXX_TRACE(logger, "Packet buffer for CID "
						<< icnId.str() << " updated with a packet of size "
						<< packetSize);
	}

	_mutexBuffer.unlock();
}

void Ip::_deleteBufferedPacket(IcnId &cid)
{
	_bufferIt = _packetBuffer.find(cid.uint());

	if (_bufferIt == _packetBuffer.end())
	{
		LOG4CXX_DEBUG(logger, "IP buffer has no packet for CID "
				<< cid.print());
		_mutexBuffer.unlock();
		return;
	}

	_packetBuffer.erase(_bufferIt);
	LOG4CXX_TRACE(logger, "IP packet for CID " << cid.print() << " deleted from"
			" IP packet buffer");
}

void Ip::_icnId(IcnId &icnId)
{
	_icnIdsIt = _icnIds.find(icnId.uint());

	if (_icnIdsIt != _icnIds.end())
	{
		LOG4CXX_DEBUG(logger, "CID " << icnId.print() << " cannot be added. It "
				"already exists");
		return;
	}

	_icnIds.insert(pair<uint32_t, IcnId>(icnId.uint(), icnId));
	LOG4CXX_DEBUG(logger, "New CID " << icnId.print() << " has been added");
}

bool Ip::_routingPrefix(IpAddress &ipAddress, RoutingPrefix &routingPrefix)
{
	_mutexRoutingPrefixes.lock();

	for (_routingPrefixesRevIt = _routingPrefixes->rbegin();
			_routingPrefixesRevIt != _routingPrefixes->rend();
			_routingPrefixesRevIt++)
	{
		if ((ipAddress.uint() & _routingPrefixesRevIt->second.netmask().uint())
				== _routingPrefixesRevIt->second.uint())
		{
			routingPrefix = _routingPrefixesRevIt->second;
			_mutexRoutingPrefixes.unlock();
			return true;
		}
	}

	_mutexRoutingPrefixes.unlock();
	return false;
}
