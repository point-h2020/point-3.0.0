/*
 * agent.cc
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

#include "agent.hh"

void bootstrapProcesses(map<uint32_t, bool> *process,
		boost::mutex *mutex, int *bootstrapSocket)
{
#ifdef MONITORING_TRIGGER_BUGFIX
	bool monitoringServerUp = true;
#else
	bool monitoringServerUp = false;
#endif
	map<uint32_t, bool>::iterator it;
	struct sockaddr_nl agentSocketAddress;
	int ret;
	int bytesReceived;
	*bootstrapSocket = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);

	if (*bootstrapSocket < 0)
	{
		cout << "((MOLY)) Could not create bootstrapping listener socket\n";
		return;
	}

#ifdef MOLY_DEBUG
	cout << "((MOLY)) Created boostrapping listener socket (FD = "
			<< *bootstrapSocket << ")\n";
#endif
	memset(&agentSocketAddress, 0, sizeof(agentSocketAddress));
	agentSocketAddress.nl_family = AF_NETLINK;
	agentSocketAddress.nl_pid = PID_MOLY_BOOTSTRAP_LISTENER;
	agentSocketAddress.nl_pad = 0;
	agentSocketAddress.nl_groups = 0;
	cout << "((MOLY)) Binding to PID " << (int)PID_MOLY_BOOTSTRAP_LISTENER
			<< endl;
	ret = bind(*bootstrapSocket, (struct sockaddr*) &agentSocketAddress,
			sizeof(agentSocketAddress));

	if (ret == -1)
	{
		cout << "((MOLY)) Could not bind to bootstrapping listener socket with "
				"PID " << PID_MOLY_BOOTSTRAP_LISTENER;
		return;
	}

#ifdef MOLY_DEBUG
	cout << "((MOLY)) Bound to boostrapping listener socket using PID "
			<< PID_MOLY_BOOTSTRAP_LISTENER << " (FD = " << *bootstrapSocket
			<< ")\n";
#endif
	struct sockaddr_nl applicationSocketAddress;
	struct nlmsghdr *nlh = NULL;
	struct iovec iov;
	struct msghdr msg;
	memset(&applicationSocketAddress, 0, sizeof(applicationSocketAddress));
	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_MESSAGE_PAYLOAD));
	memset(nlh, 0, NLMSG_SPACE(MAX_MESSAGE_PAYLOAD));
	iov.iov_base = (void*)nlh;
	iov.iov_len = NLMSG_SPACE(MAX_MESSAGE_PAYLOAD);
	msg.msg_name = (void *)&applicationSocketAddress;
	msg.msg_namelen = sizeof(applicationSocketAddress);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	while (true)
	{
		bytesReceived = recvmsg(*bootstrapSocket, &msg, 0);

		if (bytesReceived == -1)
		{
#ifdef MOLY_DEBUG
			cout << "((MOLY)) Could not read incoming message\n";
#endif
		}
		else
		{
			// Switch based on incoming message type
			switch (nlh->nlmsg_type)
			{
			case BOOTSTRAP_MY_PID:
			{
				uint32_t pid = nlh->nlmsg_pid;
				mutex->lock();
				it = process->find(pid);

				// New applications
				if (it == process->end())
				{
#ifdef MOLY_DEBUG
					cout << "((MOLY)) New application announced its "
							"availability on PID " << pid << endl;
#endif
					process->insert(pair<uint32_t, bool>(pid, false));
				}
				// Application exists
				else
				{
#ifdef MOLY_DEBUG
					cout << "((MOLY)) Application with PID " << pid
							<< " already known\n";
#endif
				}

				// Respond to application with OK
				nlh->nlmsg_type = BOOTSTRAP_OK;

				if (sendmsg(*bootstrapSocket, &msg, 0) == -1)
				{
					cout << "((MOLY)) Application with PID " << pid << " could "
							"not be notified\n";
				}
				else
				{
#ifdef MOLY_DEBUG
					cout << "((MOLY)) Application was notified that it is "
							"listed to send trigger event\n";
#endif
				}

				// if trigger has been already received, send start reporting
				// too
				if (monitoringServerUp)
				{
					nlh->nlmsg_type = BOOTSTRAP_START_REPORTING;

					if (sendmsg(*bootstrapSocket, &msg, 0) == -1)
					{
						cout << "((MOLY)) Application with PID " << pid
								<< " could not be notified\n";
					}
					else
					{
#ifdef MOLY_DEBUG
						cout << "((MOLY)) Application listing on PID "
								<< nlh->nlmsg_pid << " was notified that it can"
										" start reporting" << endl;
#endif
						it = process->find(pid);
						it->second = true;
					}
				}

				mutex->unlock();
				break;
			}
			case BOOTSTRAP_SERVER_UP:
			{
#ifdef MOLY_DEBUG
				cout << "((MOLY)) 'Server is up' trigger received from BAMPERS"
						<< endl;
#endif
				if (monitoringServerUp)
				{
#ifdef MOLY_DEBUG
				cout << "((MOLY)) 'Server is up' trigger has been already "
						"received from BAMPERS?! SRC PID: " << nlh->nlmsg_pid
						<< endl;
#endif
				}
				mutex->lock();
				monitoringServerUp = true;

				for (it = process->begin(); it != process->end(); it++)
				{
					if (!it->second)
					{
						struct nlmsghdr *nlhSend = NULL;
						struct sockaddr_nl applicationAddress;
						applicationAddress.nl_family = AF_NETLINK;
						applicationAddress.nl_pid = it->first;
						applicationAddress.nl_groups = 0;
						applicationAddress.nl_pad = 0;
						/* Allocate the required memory (NLH + payload) */
						nlhSend = (struct nlmsghdr *)malloc(NLMSG_SPACE(0));

						if (nlhSend == NULL)
						{
							cout << "((MOLY)) Application with PID "
									<< nlh->nlmsg_pid << " could not be "
											"notified. Malloc failed!\n";
							break;
						}

						/* Fill the netlink message header */
						nlhSend->nlmsg_len = NLMSG_SPACE(0);
						nlhSend->nlmsg_pid = it->first;
						nlhSend->nlmsg_flags = NLM_F_REQUEST;
						nlhSend->nlmsg_type = BOOTSTRAP_START_REPORTING;
						nlhSend->nlmsg_seq = rand();
						iov.iov_base = (void *)nlhSend;
						iov.iov_len = nlhSend->nlmsg_len;
						// Creating message
						msg.msg_name = (void *)&applicationAddress;
						msg.msg_namelen = sizeof(applicationAddress);
						msg.msg_iov = &iov;
						msg.msg_iovlen = 1;

						if (sendmsg(*bootstrapSocket, &msg, 0) == -1)
						{
							cout << "((MOLY)) Application with PID "
									<< nlh->nlmsg_pid << " could not be "
											"notified\n";
						}
						else
						{
#ifdef MOLY_DEBUG
							cout << "((MOLY)) Application listing on PID "
									<< it->first << " was notified that it can "
											"start reporting" << endl;
#endif
							it->second = true;
						}
					}
				}
				mutex->unlock();
				break;
			}
			default:
				cout << "((MOLY)) Unknown bootstrap message type "
						<< nlh->nlmsg_type<< endl;
			}
		}
	}

	free(nlh);
}

Agent::Agent()
{
	_nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_MESSAGE_PAYLOAD));
	_listenerSocket = -1;
	_bootstrapSocket = -1;
}

Agent::~Agent() {
	free(_nlh);

	if (_listenerSocket != -1)
	{
#ifdef MOLY_DEBUG
		cout << "((MOLY)) Closing monitoring listener socket (FD = "
				<< _listenerSocket	<< ")\n";
#endif
		close(_listenerSocket);
	}
	if (_bootstrapSocket != -1)
	{
#ifdef MOLY_DEBUG
		cout << "((MOLY)) Closing bootstrap listener socket (FD = "
				<< _bootstrapSocket	<< ")\n";
#endif
		close(_bootstrapSocket);
	}
}

bool Agent::initialiseListener()
{
	// Opening listener socket for reported monitoring data
	struct sockaddr_nl srcAddr;
	int ret;
	_listenerSocket = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);

	if (_listenerSocket < 0)
	{
		return false;
	}

#ifdef MOLY_DEBUG
	cout << "((MOLY)) Monitoring socket created (FD = " << _listenerSocket
			<< ")\n";
#endif
	memset(&srcAddr, 0, sizeof(srcAddr));
	srcAddr.nl_family = AF_NETLINK;
	srcAddr.nl_pid = PID_MOLY_LISTENER;
	srcAddr.nl_pad = 0;
	srcAddr.nl_groups = 0;
	cout << "((MOLY)) Binding to PID " << (int)PID_MOLY_LISTENER << endl;
	ret = bind(_listenerSocket, (struct sockaddr*) &srcAddr, sizeof(srcAddr));

	if (ret == -1)
	{
		cout << "((MOLY)) Cannot bind to monitoring socket using PID "
				<< (int)PID_MOLY_LISTENER << endl;
		close(_listenerSocket);
		return false;
	}

	// Start bootstrap thread so that processes can connect to the agent
	boost::thread boostrapProcessesThread(bootstrapProcesses,
			&_applications, &_mutex, &_bootstrapSocket);
	return true;
}

bool Agent::receive(uint8_t &messageType, uint8_t *data, uint32_t &dataSize)
{
	struct sockaddr_nl dstAddr;
	struct iovec iov;
	struct msghdr msg;
	memset(&dstAddr, 0, sizeof(dstAddr));
	memset(_nlh, 0, NLMSG_SPACE(MAX_MESSAGE_PAYLOAD));
	iov.iov_base = (void*)_nlh;
	iov.iov_len = NLMSG_SPACE(MAX_MESSAGE_PAYLOAD);
	msg.msg_name = (void *)&dstAddr;
	msg.msg_namelen = sizeof(dstAddr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	dataSize = recvmsg(_listenerSocket, &msg, 0);

	if (dataSize <= 0)
	{
#ifdef MOLY_DEBUG
		cout << "((MOLY)) Message received from process is malfunctioned"
				<< endl;
#endif
		return false;
	}

	messageType = _nlh->nlmsg_type;
	dataSize = _nlh->nlmsg_len - NLMSG_HDRLEN;
	memcpy(data, NLMSG_DATA(_nlh), dataSize);
#ifdef MOLY_DEBUG
	cout << "((MOLY)) Message of length " << _nlh->nlmsg_len << " (Payload "
			<< dataSize << ") bytes received from process with PID "
			<< _nlh->nlmsg_pid << endl;
#endif
	return true;
}
