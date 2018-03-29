/*
 * httpbuffercleaner.cc
 *
 *  Created on: 7 Jul 2016
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

#include "httpbuffercleaner.hh"

using namespace cleaners::httpbuffer;

LoggerPtr HttpBufferCleaner::logger(Logger::getLogger("cleaners.httpbuffer"));

HttpBufferCleaner::HttpBufferCleaner(Configuration &configuration,
		Statistics &statistics, packet_buffer_requests_t &bufferRequests,
		std::mutex &mutexRequests,
		bool *run)
	: _configuration(configuration),
	  _statistics(statistics),
	  _bufferRequests(bufferRequests),
	  _mutexRequests(mutexRequests),
	  _run(run)
{}

HttpBufferCleaner::~HttpBufferCleaner() {}

void HttpBufferCleaner::operator()()
{
	LOG4CXX_DEBUG(logger, "Starting HTTP buffer cleaner with interval of "
			<< _configuration.bufferCleanerInterval() << "s");

	while (*_run)
	{
		_cleanUpRequests();
	//	_cleanUpResponses();
		sleep(_configuration.bufferCleanerInterval());
	}

	LOG4CXX_DEBUG(logger, "HTTP buffer cleaner stopped");
}

void HttpBufferCleaner::_cleanUpRequests()
{
	map<uint32_t, request_packet_t>::iterator packetBufferiSubIdIt;
	boost::posix_time::ptime currentTime;
	boost::posix_time::time_duration packetAge;
	IcnId iSubCId;
	uint32_t bufferSize = 0;//number of packets
	_mutexRequests.lock();
	currentTime = boost::posix_time::microsec_clock::local_time();

	// Iterate over the CIDs
	for (_bufferRequestsIt = _bufferRequests.begin();
			_bufferRequestsIt != _bufferRequests.end(); _bufferRequestsIt++)
	{
		//now iterate over the iSub CIDs to find buffered packets
		packetBufferiSubIdIt = _bufferRequestsIt->second.begin();

		while (packetBufferiSubIdIt != _bufferRequestsIt->second.end())
		{
			packetAge = currentTime -
					(*packetBufferiSubIdIt).second.timestamp;
			bufferSize++;

			// packet timed out
			if ((uint32_t)packetAge.seconds() > (ENIGMA * ENIGMA))
			{
				LOG4CXX_DEBUG(logger, "Packet of length "
						<< packetBufferiSubIdIt->second.packetSize
						<< " to be published under "
						<< packetBufferiSubIdIt->second.cId.print()
						<< " with iSub CID "
						<< packetBufferiSubIdIt->second.rCId.print()
						<< " deleted from HTTP buffer");
				iSubCId = packetBufferiSubIdIt->second.rCId;
				free(packetBufferiSubIdIt->second.packet);
				_bufferRequestsIt->second.erase(packetBufferiSubIdIt);
				// Reset iSub map iterator
				packetBufferiSubIdIt = _bufferRequestsIt->second.begin();
			}
			else
			{
				LOG4CXX_TRACE(logger, "Packet of length "
						<< packetBufferiSubIdIt->second.packetSize
						<< " to be published under "
						<< packetBufferiSubIdIt->second.cId.print()
						<< " with rCID "
						<< packetBufferiSubIdIt->second.rCId.print()
						<< " is only " << packetAge.seconds() << "s old. "
						"Thus, it doesn't have to be deleted");
				packetBufferiSubIdIt++;
			}
		}

		// iSub CID map is empty -> delete
		if (_bufferRequestsIt->second.size() == 0)
		{
			LOG4CXX_DEBUG(logger, "Deleted entire map for rCID "
					<< _bufferRequestsIt->first << " too (sorry, only rCID hash"
					" available)");
			_bufferRequests.erase(_bufferRequestsIt);
		}
	}

	_mutexRequests.unlock();
	_statistics.bufferSizeHttpHandlerRequests(bufferSize);
}
