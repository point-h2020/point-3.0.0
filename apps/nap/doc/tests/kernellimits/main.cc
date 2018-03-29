/*
 * main.cc
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

#include <iostream>
#include <mutex>
#include <signal.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <unistd.h>

#include "../kernellimits/tcpclient.h"

using namespace std;

int i;

void sigproc(int sig)
{
	switch (sig)
	{
	case SIGABRT:
		cout << "SIGABRT at thread # " << i << endl;
		break;
	case SIGTRAP:
		cout << "SIGTRAP at thread # " << i << endl;
		break;
	case SIGSEGV:
		cout << "SIGSEGV at thread # " << i << endl;
		break;
	default:
		cout << sig << "  at thread # " << i << endl;
	}

	exit(0);
}

int main(int argc,char *argv[])
{
	int socketCount;
	bool start = false;
	thread *tcpClientThread;
	std::mutex threadMutex;
	TcpClient tcpClient(&threadMutex, &socketCount, &start);

	if (argc != 2)
	{
		cout << "Usage: ./kernellimits <NUM_OF_THREADS>" << endl;
		return -1;
	}

	signal(SIGINT, sigproc);
	signal(SIGABRT, sigproc);
	signal(SIGTERM, sigproc);
	signal(SIGQUIT, sigproc);

	for (i = 0; i < atoi(argv[1]); i++)
	{
		tcpClientThread = new thread (tcpClient, i);
		tcpClientThread->detach();
		delete tcpClientThread;
	}

	cout << "Created " << i << " threads" << endl;
	tcpClient.start();
	cout << "Started thread execution\n";

	while (tcpClient.numOfSockets() > 0)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	cout << "All threads have finished. Bye\n";

	return EXIT_SUCCESS;
}
