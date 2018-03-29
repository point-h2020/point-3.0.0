/*
 * messagestack.cc
 *
 *  Created on: 29 Dec 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the ICN application MOnitOring SErver (MOOSE) which
 * comes with Blackadder.
 *
 * MOOSE is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOOSE is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOOSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include "messagestack.hh"

MessageStack::MessageStack(log4cxx::LoggerPtr logger,
		moose::Statistics &statistics)
	: _logger(logger),
	  _statistics(statistics)
{}

MessageStack::~MessageStack()
{
	LOG4CXX_DEBUG(_logger, "Deleting entire stack of size "
			<< _messageStack.size());
	// cleaning up memory
	_mutex.lock();
	while(!_messageStack.empty())
	{
		// freeing the memory
		free(_messageStack.top().second.first);
		_messageStack.pop();
	}
	_mutex.unlock();
}

void MessageStack::write(IcnId cid, uint8_t *data, uint16_t dataLength)
{
	_icnMessage.first = cid;
	_icnMessage.second.first = (uint8_t*)malloc(dataLength);
	memcpy(_icnMessage.second.first, data, dataLength);
	_icnMessage.second.second = dataLength;
	_mutex.lock();
	_messageStack.push(_icnMessage);
	LOG4CXX_TRACE(_logger, "Data of length " << dataLength << " added to "
			"message stack of size " << _messageStack.size() << " for CID "
			<< cid.print());
	_mutex.unlock();
}

bool MessageStack::readNext(IcnId &cid, uint8_t *data, uint16_t &dataLength)
{
	// check first that stack has a message. If not, sleep for 100ms to not
	// overload the CPU
	_mutex.lock();

	while(_messageStack.empty())
	{
		_mutex.unlock();
		boost::this_thread::sleep(boost::posix_time::milliseconds(100));
		_mutex.lock();
	}

	cid = _messageStack.top().first;
	dataLength = _messageStack.top().second.second;
	memcpy(data, _messageStack.top().second.first, dataLength);
	//remove message from stack
	free(_messageStack.top().second.first);
	_messageStack.pop();
	LOG4CXX_TRACE(_logger, "Message of length " << dataLength << " retrieved "
			"from stack for CID " << cid.print());
	_statistics.messageStackSize(_messageStack.size());
	_mutex.unlock();

	// check that pointer is not NULL
	if (data == NULL)
	{
		LOG4CXX_WARN(_logger, "Message could not be obtained from stack. Data "
				"pointer was NULL");
		return false;
	}

	return true;
}
