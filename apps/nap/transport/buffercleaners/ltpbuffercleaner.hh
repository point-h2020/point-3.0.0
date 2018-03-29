/*
 * ltpbuffercleaner.hh
 *
 *  Created on: 30 Jun 2017
 *      Author: user
 */

#ifndef NAP_TRANSPORT_BUFFERCLEANERS_LTPBUFFERCLEANER_HH_
#define NAP_TRANSPORT_BUFFERCLEANERS_LTPBUFFERCLEANER_HH_

#include <log4cxx/logger.h>
#include <mutex>
#include <thread>
#include <time.h>

#include <configuration.hh>
#include <transport/lightweighttypedef.hh>
#include <monitoring/statistics.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace log4cxx;
using namespace monitoring::statistics;

namespace cleaners
{

namespace ltpbuffer
{

/*!
 * \brief LTP buffer cleaner for cNAP and sNAP buffers
 *
 * Based on timeout configured in nap.cfg
 */
class LtpBufferCleaner
{
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	LtpBufferCleaner(Configuration &configuration, Statistics &statistics,
			ltp_packet_buffer_t *ltpPacketBuffer,
			std::mutex *ltpPacketBufferMutex, bool *run);
	/*!
	 * \brief Destructor
	 */
	~LtpBufferCleaner();
	/*!
	 * \brief Functor to place the buffer operations into a dedicated thread
	 */
	void operator()();
private:
	Configuration &_configuration;/*!< Reference to configuration */
	Statistics &_statistics;/*!< Reference to statistics clasds */
	ltp_packet_buffer_t *_ltpPacketBuffer;
	std::mutex *_ltpBufferMutex;/*!< mutex to ensure transaction safe operations on
	_ltpPacketBuffer*/
	bool *_run; /*!< Pointer to run boolean from main thread indication SIG*
	signals*/
	/*!
	 * \brief Clean LTP buffer
	 */
	void _cleanLtpPacketBuffer();
};

};/**/

};/**/
#endif /* NAP_TRANSPORT_BUFFERCLEANERS_LTPBUFFERCLEANER_HH_ */
