/*
 * messagestack.hh
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

#ifndef APPS_MONITORING_SERVER_MESSAGESTACK_HH_
#define APPS_MONITORING_SERVER_MESSAGESTACK_HH_

#include <bampers/bampers.hh>
#include <boost/thread.hpp>
#include <log4cxx/logger.h>
#include <stack>

#include "../monitoring/statistics.hh"

using namespace std;

/*!
 * \brief Implementation of the message stack which allows the ICN handler to
 * write incoming messages and the MySQL connector (client) to read it. The
 * class allows parallel read and write operations using mutex
 */
class MessageStack {
public:
	/*!
	 * \brief Constructor
	 */
	MessageStack(log4cxx::LoggerPtr logger, moose::Statistics &statistics);
	/*!
	 * \brief Destructor
	 */
	~MessageStack();
	/*!
	 * \brief Write an ICN message into the stack
	 *
	 * \param cid The CID of the data to be written to the stack
	 * \param data Pointer to the packet to be written to the stack
	 * \param dataLength Length of the data pointer 'data'
	 */
	void write(IcnId cid, uint8_t *data, uint16_t dataLength);
	/*!
	 * \brief Read the next ICN message from the stack and delete it immediately
	 *
	 * \param cid The CID for under which the data was received
	 * \param data Pointer to the data retrieved from stack. Note, this pointer
	 * must already point to allocated memory
	 * \param dataLength The length of the data retrieved from stack
	 *
	 * \return Boolean indicating if read was successful
	 */
	bool readNext(IcnId &cid, uint8_t *data, uint16_t &dataLength);
private:
	log4cxx::LoggerPtr _logger;
	moose::Statistics &_statistics; /*!< Reference to statistics class*/
	boost::mutex _mutex;
	stack<pair<IcnId, pair<uint8_t*, uint16_t>>> _messageStack;/*!< The message
	stack*/
	pair<IcnId, pair<uint8_t*, uint16_t>> _icnMessage;
};

#endif /* APPS_MONITORING_SERVER_MESSAGESTACK_HH_ */
