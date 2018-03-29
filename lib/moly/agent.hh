/*
 * agent.hh
 *
 *  Created on: 20 Feb 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the MOnitoring LibrarY (MOLY).
 *
 * MOLY is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOLY is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOLY. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_MOLY_AGENT_HH_
#define LIB_MOLY_AGENT_HH_

#include <blackadder_enums.hpp>
#include <boost/thread.hpp>
#include "enum.hh"
#include <linux/netlink.h>
#include <iostream>
#include <sys/socket.h>
#include "typedef.hh"
#include <map>

using namespace std;

/*!
 * \brief Implementation of bootstrapping class for the monitoring agent
 */
class Agent {
public:
	/*!
	 * \brief Constructor
	 */
	Agent();
	/*!
	 * \brief Destructor
	 */
	~Agent();
	/*!
	 * \brief Initialisation a MOLY listener.
	 *
	 * In order to receive messages from applications, a monitoring agent must
	 * initialise the MOLY listener before calling Moly::receiveMessage().
	 *
	 * \return If the socket has been successfully initialised this method
	 * returns true. If not, false is returned.
	 */
	bool initialiseListener();
	/*!
	 * \brief Receive message from application
	 *
	 * \param messageType Reference to the message type received
	 * \param data Pointer to data packet
	 * \param dataSize Reference to length of the packet
	 */
	bool receive(uint8_t &messageType, uint8_t *data, uint32_t &dataSize);
	/*!
	 * \brief closing socket
	 */
	void shutdown();
private:
	int _listenerSocket;
	int _bootstrapSocket;
	boost::mutex _mutex;
	map<uint32_t, bool> _applications;
	struct nlmsghdr *_nlh;
};

#endif /* LIB_MOLY_AGENT_HH_ */

