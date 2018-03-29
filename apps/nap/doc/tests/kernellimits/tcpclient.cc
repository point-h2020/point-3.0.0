/*
 * tcpclient.cc
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

#include "../kernellimits/tcpclient.h"

TcpClient::TcpClient(mutex *tcpMutex, int *numOfSockets, bool *start)
	: _mutex(tcpMutex),
	  _numOfSockets(numOfSockets),
	  _start(start)
{
	*_numOfSockets = 0;
}

TcpClient::~TcpClient() {

}

void TcpClient::operator()(int i)
{
	_mutex->lock();
	(*_numOfSockets)++;

	while (!(*_start))
	{
		_mutex->unlock();
		std::this_thread::sleep_for(std::chrono::seconds(1));
		_mutex->lock();
	}

	_mutex->unlock();

	 _mutex->lock();
	 std::this_thread::sleep_for(std::chrono::milliseconds(1));
	 _mutex->unlock();

	 _mutex->lock();
	 std::this_thread::sleep_for(std::chrono::milliseconds(1));
	 _mutex->unlock();
	_mutex->lock();
	 std::this_thread::sleep_for(std::chrono::milliseconds(1));
	 _mutex->unlock();

	_mutex->lock();
	(*_numOfSockets)--;
	_mutex->unlock();
}

int TcpClient::numOfSockets()
{
	int i;
	_mutex->lock();
	i = *_numOfSockets;
	_mutex->unlock();
	return i;
}
void TcpClient::start()
{
	_mutex->lock();
	*_start = true;
	_mutex->unlock();
}
