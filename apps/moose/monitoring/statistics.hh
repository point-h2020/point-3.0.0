/*
 * statistics.hh
 *
 *  Created on: 12 Oct 2017
 *      Author: user
 */

#ifndef SERVER_MONITORING_STATISTICS_HH_
#define SERVER_MONITORING_STATISTICS_HH_

#include <log4cxx/logger.h>
#include <mutex>

namespace moose {

class Statistics {
public:
	/*!
	 * \brief Constructor
	 */
	Statistics(log4cxx::LoggerPtr logger);
	/*!
	 * \brief Destructor
	 */
	~Statistics();
	/*!
	 * \brief Obtain the stack size
	 */
	uint32_t messageStackSize();
	/*!
	 * \brief Update the mesage size stack counter
	 *
	 * This method is called by readNext()
	 */
	void messageStackSize(uint32_t stackSize);
private:
	log4cxx::LoggerPtr _logger;
	std::mutex _mutex;
	uint32_t _stackSize;
};

} /* namespace moose */

#endif /* SERVER_MONITORING_STATISTICS_HH_ */
