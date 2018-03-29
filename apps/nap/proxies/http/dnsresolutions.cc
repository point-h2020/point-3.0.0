/*
 * dnsresolutions.cc
 *
 *  Created on: 27 Apr 2017
 *      Author: user
 */

#include "dnsresolutions.hh"

using namespace proxies::http::dnsresolutions;

LoggerPtr DnsResolutions::logger(
		Logger::getLogger("proxies.http.dnsresolutions"));

void dnsResolutionCleaner(void *dnsResolutionsPointer, void *mutexPointer,
		bool *run)
{
	uint32_t scheduledWakeUpTime = 30;
	unordered_map<string, dns_t>::iterator it;
	unordered_map<string, dns_t> *dnsResolutions =
			(unordered_map<string, dns_t> *)dnsResolutionsPointer;
	mutex *mut = (mutex *)mutexPointer;
	time_t currentTime;

	while (*run)
	{
		time(&currentTime);
		mut->lock();
		it = dnsResolutions->begin();

		// iterate over all known DNS resolutions
		while(it != dnsResolutions->end())
		{
			if (difftime(currentTime, it->second.timeResolved)
					> it->second.timeToLive)
			{
				dnsResolutions->erase(it);
				mut->unlock();//give the opportunity for a pending insertion/lookup
				mut->lock();
				it = dnsResolutions->begin();
			}
			else
			{
				it++;
			}
		}

		mut->unlock();
		std::this_thread::sleep_for(std::chrono::seconds(scheduledWakeUpTime));
	}
}

DnsResolutions::DnsResolutions(bool *run)
	: _run(run)
{
	_mutex = new mutex;
	_dnsResolutionCleaner = new thread(dnsResolutionCleaner, &_dnsResolutions,
			_mutex, _run);
	LOG4CXX_DEBUG(logger, "DNS resolutions cleaner thread started");
}

DnsResolutions::~DnsResolutions(){}

bool DnsResolutions::checkDns(string fqdn, IpAddress &ipAddress)
{
	_mutex->lock();
	auto it = _dnsResolutions.find(fqdn);

	// not found
	if (it == _dnsResolutions.end())
	{
		LOG4CXX_TRACE(logger, "FQDN " << fqdn << " unknown. Resolve it via "
				"DNS");
		_mutex->unlock();
		return false;
	}

	ipAddress = it->second.ipAddress;
	_mutex->unlock();
	LOG4CXX_TRACE(logger, "IP address for " << fqdn << " is "
			<< ipAddress.str());
	return true;
}

bool DnsResolutions::resolve(string fqdn, IpAddress &ipAddress)
{
	struct hostent *dnsResponse;
	uint32_t dnsTimeToLive = 1200;//[s]
	dnsResponse = gethostbyname(fqdn.c_str());

	if (dnsResponse == NULL)
	{
		LOG4CXX_DEBUG(logger, "IP address could not be resolved for "
				"FQDN " << fqdn);

		switch (h_errno)
		{
		case HOST_NOT_FOUND:
			LOG4CXX_DEBUG(logger, "FQDN " << fqdn << " is unknown to "
					"DNS");
			break;
		default:
			LOG4CXX_DEBUG(logger, "IP address for FQDN " << fqdn
					<< " could not be found. Error code: " << h_errno);
		}

		return false;
	}

	if (dnsResponse->h_addr_list[0] == NULL)
	{
		LOG4CXX_DEBUG(logger, "IP address field for FQDN " << fqdn
				<< " is empty");
		return false;
	}

	ipAddress =	IpAddress((struct in_addr*)dnsResponse->h_addr_list[0]);
	_addDns(fqdn, ipAddress, dnsTimeToLive);
	return true;
}

void DnsResolutions::_addDns(string fqdn, IpAddress ipAddress,
		uint32_t timeToLive)
{
	_mutex->lock();
	auto it = _dnsResolutions.find(fqdn);

	// found. Throw a msg and update the times
	if (it != _dnsResolutions.end())
	{
		time(&it->second.timeResolved);
		it->second.timeToLive = timeToLive;
		_mutex->unlock();
		LOG4CXX_DEBUG(logger, "DNS entry already exists for FQDN " << fqdn
				<< ". Updated times only");
		return;
	}

	dns_t dnsEntry;
	dnsEntry.ipAddress = ipAddress;
	time(&dnsEntry.timeResolved);
	dnsEntry.timeToLive = timeToLive;
	_dnsResolutions.insert(pair<string, dns_t>(fqdn, dnsEntry));
	LOG4CXX_DEBUG(logger, "New DNS entry added: " << fqdn << " <> "
			<< ipAddress.str());
	_mutex->unlock();
}
