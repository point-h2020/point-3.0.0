/*
 * namespaces.cc
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

#include "namespaces.hh"

Namespaces::Namespaces(Blackadder *icnCore, std::mutex &icnCoreMutex,
		Configuration &configuration, Transport &transport,
		Statistics &statistics, bool *run)
	: Ip(icnCore, configuration, transport, statistics, run),
	  Http(icnCore, configuration, transport, statistics, run),
	  Management(icnCore, icnCoreMutex, configuration),
	  _configuration(configuration)
{}

void Namespaces::endOfSession(IcnId &cid, enigma_t &enigma,
		uint16_t &sessionKey)
{
	switch(cid.rootNamespace())
	{
	case NAMESPACE_IP:
		// not supported at the moment
		break;
	case NAMESPACE_HTTP:
		Http::endSession(cid, enigma, sessionKey);
		break;
	case NAMESPACE_COAP:
		// not supported at the moment
		break;
	}
}

void Namespaces::forwarding(IcnId &cId, bool state)
{
	switch (cId.rootNamespace())
	{
	case NAMESPACE_IP:
		Ip::forwarding(cId, state);
		break;
	case NAMESPACE_HTTP:
		Http::forwarding(cId, state);
		break;
	case NAMESPACE_MANAGEMENT:
		Management::forwarding(cId, state);
		break;
	}
}

void Namespaces::forwarding(NodeId &nodeId, bool state)
{
	Http::forwarding(nodeId, state);
}

void Namespaces::handlePublishedData(IcnId &cid, void *data, uint32_t &dataSize)
{
	switch(atoi(cid.scopeId(2).c_str()))
	{
	case MANAGEMENT_II_DNS_LOCAL:
		uint32_t hashedFqdn;
		memcpy(&hashedFqdn, data, sizeof(dataSize));
		Http::handleDnsLocal(hashedFqdn);
		break;
	}
}

void Namespaces::initialise()
{
	Ip::initialise();
	Http::initialise();
}

void Namespaces::publishFromBuffer(IcnId &cid)
{
	switch (cid.rootNamespace())
	{
	case NAMESPACE_IP:
		Ip::publishFromBuffer(cid);
		break;
	case NAMESPACE_HTTP:
		Http::publishFromBuffer(cid);
		break;
	}
}

/*void Namespaces::publishFromBuffer(NodeId &nodeId)
{
	Http::publishFromBuffer(nodeId);
}*/

void Namespaces::sendToEndpoint(IcnId &rCid, enigma_t &enigma,
		uint8_t *packet, uint16_t &packetSize)
{
	switch (rCid.rootNamespace())
	{
	case NAMESPACE_HTTP:
		Http::sendToEndpoint(rCid, enigma, packet, packetSize);
		break;
	}
}

void Namespaces::subscribeScope(IcnId &icnId)
{
	switch (icnId.rootNamespace())
	{
	case NAMESPACE_IP:
		Ip::subscribeScope(icnId);
		break;
	}
}

void Namespaces::surrogacy(root_namespaces_t rootNamespace, uint32_t hashedFqdn,
		uint32_t ipAddress, uint16_t port, nap_sa_commands_t command)
{
	IcnId cid(hashedFqdn);
	Management::DnsLocal::announce(cid.uintId());

	switch (rootNamespace)
	{
	case NAMESPACE_HTTP:
		if (command == NAP_SA_ACTIVATE)
		{
			IpAddress ipAddr(ipAddress);
			// add FQDN + IP address
			_configuration.addFqdn(cid, ipAddr, port);
			// subscribe to CID
			Http::subscribeToFqdn(cid);
		}
		else if (command == NAP_SA_DEACTIVATE)
		{
			Http::lockPotentialCmcGroupMembership();
			Http::closePotentialCmcGroups(cid);

			while (Http::activeCmcGroups(cid) > 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			// unsubscribe from CID
			Http::unsubscribeFromFqdn(cid);
			Http::unlockPotentialCmcGroupMembership();
		}
		break;
	default:
		return;
	}
}

void Namespaces::uninitialise()
{
	Ip::uninitialise();
	Http::uninitialise();
}
