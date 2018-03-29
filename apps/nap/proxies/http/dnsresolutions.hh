/*
 * dnsresolutions.hh
 *
 *  Created on: 27 Apr 2017
 *      Author: user
 */

#ifndef NAP_PROXIES_HTTP_DNSRESOLUTIONS_HH_
#define NAP_PROXIES_HTTP_DNSRESOLUTIONS_HH_

#include <chrono>
#include <log4cxx/logger.h>
#include <mutex>
#include <netdb.h>
#include <thread>
#include <time.h>
#include <unordered_map>

#include <types/ipaddress.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace log4cxx;
using namespace std;

struct dns_t
{
	IpAddress ipAddress;/*!< The IP address for the resolved FQDN*/
	time_t timeResolved;/*!< The timestamp of when the FQDN was resolved*/
	uint32_t timeToLive;/*!< The time the resolved IP address is valid*/
};

namespace proxies {

namespace http {

namespace dnsresolutions {

/*!
 * \brief Class to hold and clean up resolved FQDNs and their IP addresses
 */
class DnsResolutions
{
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	DnsResolutions(bool *run);
	/*!
	 * \brief Destructor
	 */
	~DnsResolutions();
protected:
	/*!
	 * \brief Check if IP address is known for given FQDN
	 *
	 * \param fqdn The FQDN which should be checked
	 * \param ipAddress If FQDN found this reference holds the known IP address
	 *
	 * \return Indicates whether or not an IP address is known for the given
	 * FQDN
	 */
	bool checkDns(string fqdn, IpAddress &ipAddress);
	/*!
	 * \brief Resolve the given FQDN
	 *
	 * \param fqdn The FQDN which should be resolved
	 * \param ipAddress The resolved IP address
	 *
	 * \return Boolean indicating if DNS was successful or not
	 */
	bool resolve(string fqdn, IpAddress &ipAddress);
private:
	thread *_dnsResolutionCleaner;
	mutex *_mutex;
	unordered_map<string, dns_t> _dnsResolutions;
	bool *_run;
	/*!
	 * \brief Add newly resolved DNS
	 *
	 * \param fqdn The FQDN which was resolved
	 * \param ipAddress The IP address for the FQDN
	 * \param timeToLive The TTL given in the response
	 */
	void _addDns(string fqdn, IpAddress ipAddress, uint32_t timeToLive);
};

} /* namespace dnsresolutios */

} /* namespace http */

} /* namespace proxies */

#endif /* NAP_PROXIES_HTTP_DNSRESOLUTIONS_HH_ */
