/*
 * ltpreversecidcleaner.cc
 *
 *  Created on: 3 Jul 2017
 *      Author: user
 */

#include "ltpreversecidcleaner.hh"

using namespace cleaners::ltpreversecid;

LoggerPtr LtpReverseCidCleaner::logger(
		Logger::getLogger("cleaners.ltpreversecid"));

LtpReverseCidCleaner::LtpReverseCidCleaner(Configuration &configuration,
		reverse_cid_lookup_t *reverseLookupMap,
		std::mutex *reverseLookupMapMutex)
	: _configuration(configuration),
	  _reverseLookupMap(reverseLookupMap),
	  _mutex(reverseLookupMapMutex)
{

}

LtpReverseCidCleaner::~LtpReverseCidCleaner() {}

void LtpReverseCidCleaner::operator ()()
{
	LOG4CXX_INFO(logger, "LTP reverse CID look-up cleaner thread started");
	reverse_cid_lookup_t::iterator it;

	while (true)
	{
		boost::posix_time::ptime currentTime;
		boost::posix_time::time_duration duration;
		LOG4CXX_TRACE(logger, "Check reverse rCID > CID map for unused entries");
		currentTime = boost::posix_time::microsec_clock::local_time();
		_mutex->lock();
		it = _reverseLookupMap->begin();

		while (it != _reverseLookupMap->end())
		{
			duration = currentTime - it->second.lastAccessed();

			if (duration.seconds() > _configuration.timeout())
			{
				LOG4CXX_TRACE(logger, "Hashed rCID " << it->first << " > CID "
						<< it->second.print() << " mapping deleted from reverse"
						" rCID > CID map");
				_reverseLookupMap->erase(it);
				// give normal operations a chance to acces the map
				_mutex->unlock();
				// and queue again to lock the map
				_mutex->lock();
				it = _reverseLookupMap->begin();
			}
			else
			{
				it++;
			}
		}

		_mutex->unlock();
		LOG4CXX_TRACE(logger, "Check reverse rCID > CID map for unused entries "
				<< "again in " << _configuration.bufferCleanerInterval()
				<< " seconds");
		std::this_thread::sleep_for(
				std::chrono::seconds(_configuration.bufferCleanerInterval()));
	}

	LOG4CXX_INFO(logger, "LTP reverse CID look-up cleaner thread ended");
}
