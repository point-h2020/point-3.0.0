/*
 * ltpbuffercleaner.cc
 *
 *  Created on: 30 Jun 2017
 *      Author: user
 */

#include "ltpbuffercleaner.hh"

using namespace cleaners::ltpbuffer;

LoggerPtr LtpBufferCleaner::logger(Logger::getLogger("cleaners.ltpbuffer"));

LtpBufferCleaner::LtpBufferCleaner(Configuration &configuration,
		Statistics &statistics, ltp_packet_buffer_t *ltpPacketBuffer,
		std::mutex *ltpPacketBufferMutex, bool *run)
	: _configuration(configuration),
	  _statistics(statistics),
	  _ltpPacketBuffer(ltpPacketBuffer),
	  _ltpBufferMutex(ltpPacketBufferMutex),
	  _run(run)
{}

LtpBufferCleaner::~LtpBufferCleaner() {}

void LtpBufferCleaner::operator()()
{
	LOG4CXX_DEBUG(logger, "LTP packet buffer cleaner thread started");

	while (*_run)
	{
		LOG4CXX_TRACE(logger, "Check LTP packet buffer");
		_cleanLtpPacketBuffer();
		LOG4CXX_TRACE(logger, "Check LTP packet buffer again in "
				<< _configuration.bufferCleanerInterval() << " seconds");
		this_thread::sleep_for(
				chrono::seconds(_configuration.bufferCleanerInterval()));
	}

	LOG4CXX_DEBUG(logger, "LTP packet buffer cleaner thread ended");
}

void LtpBufferCleaner::_cleanLtpPacketBuffer()
{
	ltp_packet_buffer_t::iterator ltpBufferIt;
	bool deleteEntry = false;
	uint32_t bufferSize = 0; // number of packets
	_ltpBufferMutex->lock();
	ltpBufferIt = _ltpPacketBuffer->begin();
	boost::posix_time::ptime currentTime;
	boost::posix_time::time_duration packetAge;
	currentTime = boost::posix_time::microsec_clock::local_time();

	while (ltpBufferIt != _ltpPacketBuffer->end())
	{
		map<enigma_t, map<sk_t, map<seq_t, packet_t>>>::iterator enigmaIt;
		enigmaIt = ltpBufferIt->second.begin();

		while (enigmaIt != ltpBufferIt->second.end())
		{
			map<sk_t, map<seq_t, packet_t>>::iterator skIt;
			skIt = enigmaIt->second.begin();

			while (skIt != enigmaIt->second.end())
			{
				map<seq_t, packet_t>::iterator seqIt;
				seqIt = skIt->second.begin();
				deleteEntry = false;

				while (seqIt != skIt->second.end())
				{
					packetAge = currentTime - seqIt->second.timestamp;

					// at least one of the packets is above the timeout value
					// Now go off and delete all packets for rCID > Enigma > SK
					if (packetAge.seconds() > _configuration.timeout())
					{
						LOG4CXX_DEBUG(logger, "Start freeing up "
								<< skIt->second.size() << " packet(s) for "
								"hashed rCID " << ltpBufferIt->first
								<< " > Enigma " << ltpBufferIt->first << " > SK "
								<< skIt->first);

						// free up memory
						for (auto it = skIt->second.begin();
								it != skIt->second.end(); it++)
						{
							if (it->second.packet != NULL)
							{
								LOG4CXX_TRACE(logger, "Free up memory for rCID "
										<< ltpBufferIt->first << " > Enigma "
										<< enigmaIt->first << " > SK "
										<< skIt->first << " > SEQ "
										<< it->first);
								free((*it).second.packet);
							}
							else
							{
								LOG4CXX_TRACE(logger, "Memory already freed up "
										"for rCID " << ltpBufferIt->first
										<< " > Enigma " << enigmaIt->first
										<< " > SK " << skIt->first << " > SEQ "
										<< it->first);
							}
						}

						LOG4CXX_TRACE(logger, "Freed up all memory for packet "
								"LTP buffer rCID " << ltpBufferIt->first
								<< " > Enigma " << enigmaIt->first << " > SK "
								<< skIt->first);

						deleteEntry = true;
						seqIt = skIt->second.end();//and stop here
					}
					else
					{
						LOG4CXX_TRACE(logger, "Packet for hashed rCID "
								<< ltpBufferIt->first << " > Enigma "
								<< enigmaIt->first << " checked but only "
								<< packetAge.seconds() << "s old");
						seqIt++;
					}
				}

				// if packet(s) deleted, delete SK key too
				if (deleteEntry)
				{
					LOG4CXX_TRACE(logger, "SK " << skIt->first << " deleted "
							"under rCID " << ltpBufferIt->first << " > Enigma "
							<< enigmaIt->first);
					enigmaIt->second.erase(skIt);

					// No SK left for this rCID > Enigma. Delete Enigma
					if (enigmaIt->second.empty())
					{
						LOG4CXX_TRACE(logger, "Enigma " << enigmaIt->first
								<< " deleted under hashed rCID "
								<< ltpBufferIt->first);
						ltpBufferIt->second.erase(enigmaIt);
					}

					// give a chance to pending LTP operations
					_ltpBufferMutex->unlock();
					// and queue for exclusive access
					_ltpBufferMutex->lock();
					ltpBufferIt = _ltpPacketBuffer->begin();

					if (ltpBufferIt != _ltpPacketBuffer->end())
					{
						enigmaIt = ltpBufferIt->second.begin();

						if (enigmaIt != ltpBufferIt->second.end())
						{
							skIt = enigmaIt->second.begin();
						}
						else
						{//break while (skIt != enigmaIt->second.end())
							break;
						}
					}
					else
					{//break while (skIt != enigmaIt->second.end())
						break;
					}
				}
				else
				{
					bufferSize++;
					skIt++;
				}
			}

			if (!deleteEntry)
			{
				enigmaIt++;
			}
		}

		ltpBufferIt++;
	}

	_ltpBufferMutex->unlock();
	// now report the buffer size to the monitoring module
	_statistics.bufferSizeLtp(bufferSize);
}
