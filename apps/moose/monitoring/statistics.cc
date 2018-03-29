/*
 * statistics.cc
 *
 *  Created on: 12 Oct 2017
 *      Author: user
 */

#include "statistics.hh"

namespace moose {

Statistics::Statistics(log4cxx::LoggerPtr logger)
	: _logger(logger)
{
	_stackSize = 0;
}

Statistics::~Statistics() {}

uint32_t Statistics::messageStackSize()
{
	uint32_t stackSize;

	_mutex.lock();
	stackSize = _stackSize;
	_stackSize = 0;
	_mutex.unlock();

	return stackSize;
}

void Statistics::messageStackSize(uint32_t stackSize)
{
	_mutex.lock();
	_stackSize = stackSize;
	_mutex.unlock();
}

} /* namespace moose */
