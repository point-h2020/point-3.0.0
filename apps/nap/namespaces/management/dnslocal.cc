/*
 * dnslocal.cc
 *
 *  Created on: 26 Aug 2016
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

#include <namespaces/management/dnslocal.hh>
#include <types/icnid.hh>

using namespace namespaces::management::dnslocal;

LoggerPtr DnsLocal::logger(Logger::getLogger("namespaces.management.dnslocal"));

DnsLocal::DnsLocal(Blackadder *icnCore, std::mutex &icnCoreMutex,
		Configuration &configuration)
	: _icnCore(icnCore),
	  _icnCoreMutex(icnCoreMutex),
	  _configuration(configuration)
{
	IcnId cid(NAMESPACE_MANAGEMENT, MANAGEMENT_II_DNS_LOCAL);
	_cid = cid;
	_icnCoreMutex.lock();
	_icnCore->publish_scope(cid.binRootScopeId(), cid.binEmpty(), DOMAIN_LOCAL,
			NULL, 0);
	LOG4CXX_DEBUG(logger, "Management root namespace scope ID "
			<< cid.rootNamespace() << " published");

	// As a cNAP subscribe to DNSlocal info item
	if (_configuration.cNap())
	{
		_icnCore->subscribe_info(cid.binId(), cid.binPrefixId(), DOMAIN_LOCAL,
				NULL, 0);
		LOG4CXX_DEBUG(logger, "Subscribed to DNS local information item "
				<< cid.print());
	}

	// If FQDN registered or surrogacy enabled advertise availability of
	// information
	else
	{
		_icnCore->publish_info(cid.binId(), cid.binPrefixId(), DOMAIN_LOCAL,
				NULL, 0);
		LOG4CXX_DEBUG(logger, "Advertised DNSlocal information item "
						<< cid.print());
		_cid.fidRequested(true);
	}

	_icnCoreMutex.unlock();
}

DnsLocal::~DnsLocal()
{

}

void DnsLocal::announce(uint32_t hashedFqdn)
{
	if (!_cid.forwarding())
	{
		LOG4CXX_DEBUG(logger, "Start publish hasn't been received for CID "
				<< _cid.print() << ". DNSlocal announcement for hashed FQDN "
				<< hashedFqdn << " could not be sent");
		return;
	}

	uint8_t *packet = (uint8_t *)malloc(sizeof(hashedFqdn));
	memcpy(packet, &hashedFqdn, sizeof(hashedFqdn));
	_icnCoreMutex.lock();
	_icnCore->publish_data(_cid.binIcnId(), DOMAIN_LOCAL, NULL, 0, packet,
			sizeof(hashedFqdn));
	_icnCoreMutex.unlock();
	LOG4CXX_TRACE(logger, "DNSlocal announcement published to " << _cid.print()
			<< " for hashed FQDN " << hashedFqdn);
	free(packet);
}

void DnsLocal::forwarding(bool state)
{
	_cid.forwarding(state);
	LOG4CXX_TRACE(logger, "Forwarding state for DNSlocal CID " << _cid.print()
			<< " set to " << state);
	_cid.fidRequested(false);
	LOG4CXX_TRACE(logger, "DNSlocal 'FID requested' state for CID "
			<< _cid.print() << " reset to false");
}
