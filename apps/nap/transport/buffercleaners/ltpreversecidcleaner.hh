/*
 * ltpreversecidcleaner.hh
 *
 *  Created on: 3 Jul 2017
 *      Author: user
 */

#ifndef NAP_TRANSPORT_BUFFERCLEANERS_LTPREVERSECIDCLEANER_HH_
#define NAP_TRANSPORT_BUFFERCLEANERS_LTPREVERSECIDCLEANER_HH_

#include <log4cxx/logger.h>
#include <mutex>
#include <thread>
#include <time.h>

#include <configuration.hh>
#include <transport/lightweighttypedef.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace log4cxx;

namespace cleaners {

namespace ltpreversecid {

/*
 * \brief Cleaning up old rCID > CID entries
 *
 * The assumption is that CIDs which have not been used in a predefined time can
 * be considered as obsolete
 *
 */
class LtpReverseCidCleaner
{
	static LoggerPtr logger;
public:
	/*!
	 * \brief Constructor
	 */
	LtpReverseCidCleaner(Configuration &configuration,
			reverse_cid_lookup_t *reverseLookupMap,
			std::mutex *reverseLookupMapMutex);
	/*!
	 * \brief Destructor
	 */
	~LtpReverseCidCleaner();
	/*!
	 * \brief Functor
	 */
	void operator()();
private:
	Configuration &_configuration;/*!< Reference to Configuration class */
	reverse_cid_lookup_t *_reverseLookupMap;/*!< Pointer to reverse look-up map
	*/
	std::mutex *_mutex;/*!< Pointer the mutex which allows transaction safe
	operations on _reverseLookupMap*/
};

};

};

#endif /* NAP_TRANSPORT_BUFFERCLEANERS_LTPREVERSECIDCLEANER_HH_ */
