/*
 * cpuUtilisation.cc
 *
 *  Created on: 16 Feburary 2017
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of Blackadder.
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
 * Blackadder. If not, see <http://www.gnu.org/licenses/>.
 *
 * IMPORTANT: You need to compile the NAP first and install the sysstat package
 * on the machine where the CPU utilisation will be read
 */

#include <libssh/libssh.h>
#include <moly/moly.hh>
#include <signal.h>

#include "../../apps/nap/types/ipaddress.hh"

using namespace std;
ssh_session sshSession;
ssh_channel sshChannel;

void signal(int sig);

int main(int argc, char **argv)
{
	Moly moly;
	uint16_t cpuUtilisation;
	IpAddress ipAddress;
	string ipAddressStr;
	int measurementInterval;
	ostringstream command;
	int rc;
	string username;
	string password;
	char buffer[256];
	int nbytes;

	if (argc != 5)
	{
		cout << "How to run: sudo ./cpuutilisation <IP_ADDRESS> <USERNAME> "
				"<PASSWORD> <INTERVAL [s]>" << endl;
		return EXIT_FAILURE;
	}

	ipAddressStr = argv[1];
	ipAddress = ipAddressStr;
	username = argv[2];
	password = argv[3];
	measurementInterval = atoi(argv[4]);
	sshSession = ssh_new();

	if (sshSession == NULL)
	{
		return EXIT_FAILURE;
	}

	ssh_options_set(sshSession, SSH_OPTIONS_HOST, ipAddress.str().c_str());
	ssh_options_set(sshSession, SSH_OPTIONS_USER, username.c_str());
	rc = ssh_connect(sshSession);

	if (rc != SSH_OK)
	{
		cout << "Connection towards " << ipAddress.str() << " could not be "
				"established: " << ssh_get_error(sshSession) << endl;
		ssh_free(sshSession);
		return EXIT_FAILURE;
	}

	cout << "Connection established towards " << ipAddress.str() << endl;
	rc = ssh_userauth_password(sshSession, NULL, password.c_str());

	if (rc != SSH_AUTH_SUCCESS)
	{
		cout << "Error authenticating with password: " << password << ": "
				<< ssh_get_error(sshSession) << endl;
		ssh_disconnect(sshSession);
		ssh_free(sshSession);
		return EXIT_FAILURE;
	}

	cout << "Authenticated to " << ipAddress.str() << " with password "
			<< password << endl;

	if (!moly.Process::startReporting())
	{
		cout << "Communication with monitoring agent failed ... exiting\n";
		return EXIT_FAILURE;
	}

	//Creating command
	command << "sar -u " << measurementInterval << " 1 | awk '{if ($0 ~ "
			"/Average/) print 100 - $8}'";
	signal(SIGINT, signal);
	signal(SIGABRT, signal);
	signal(SIGTERM, signal);
	signal(SIGQUIT, signal);

	while(true)
	{
		// Open SSH channel
		sshChannel = ssh_channel_new(sshSession);

		if (sshChannel == NULL)
		{
			cout << "SSH channel could not be created" << endl;
			ssh_disconnect(sshSession);
			ssh_free(sshSession);
			return SSH_ERROR;
		}

		rc = ssh_channel_open_session(sshChannel);

		if (rc != SSH_OK)
		{
			cout << "SSH channel could not be opened" << endl;
			ssh_channel_free(sshChannel);
			ssh_disconnect(sshSession);
			ssh_free(sshSession);
			return rc;
		}

		// Get CPU utilisation
		rc = ssh_channel_request_exec(sshChannel, command.str().c_str());

		if (rc != SSH_OK)
		{
			ssh_channel_close(sshChannel);
			ssh_channel_free(sshChannel);
			ssh_disconnect(sshSession);
			ssh_free(sshSession);
			return rc;
		}

		nbytes = ssh_channel_read(sshChannel, buffer, sizeof(buffer), 0);
		ssh_channel_send_eof(sshChannel);
		ssh_channel_close(sshChannel);
		ssh_channel_free(sshChannel);
		cpuUtilisation = atof(buffer) * 100;

		if (!moly.Process::cpuUtilisation(NODE_ROLE_SERVER,
				ipAddress.uint(), cpuUtilisation))
		{
			cout << "CPU utilisation was not reported\n";
		}
		else
		{
			cout << "CPU utilisation of " << cpuUtilisation / 100.0 << "% "
					"reported to MONA" << endl;
		}
	}
}

void signal(int sig)
{
	ssh_channel_send_eof(sshChannel);
	ssh_channel_close(sshChannel);
	ssh_channel_free(sshChannel);
	ssh_disconnect(sshSession);
	ssh_free(sshSession);

}
