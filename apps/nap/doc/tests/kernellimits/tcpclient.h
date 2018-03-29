/*
 * tcpclient.h
 *
 *  Created on: 17 Jul 2017
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

#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <mutex>
#include <thread>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;

class TcpClient {
public:
	TcpClient(mutex *tcpMutex, int *numOfSockets, bool *start);
	~TcpClient();
	void operator() (int i);
	int numOfSockets();
	void start();
private:
	mutex *_mutex;
	int *_numOfSockets;
	bool *_start;
};

#endif /* TCPCLIENT_H_ */
