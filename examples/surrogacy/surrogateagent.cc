/*
 * surrogateagent.cc
 *
 *  Created on: 27 July 2016
 *      Author: Hongfei Du
 *      		Sebastian Robitzsch
 *
 * This file is part of Blackadder.
 *
 * Blackadder is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
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

#include <bampers/bampers.hh>
#include <blackadder_enums.hpp>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "enumerations.hh"

/*!
 * \brief Hash a string identically to how the NAP is doing it
 *
 * \param stringChar The string to be hashed
 *
 * \return Integer representation of the hashed string
 */
uint32_t hashString(const char *string)
{
	uint32_t stringHashed = 0;
	while( *string != 0 )
	{
		stringHashed = stringHashed * 31 + *string++;
	}
	return stringHashed;
}

int main(int argc, char* argv[]) {
	if (argc != 5)
	{
		cout << "surrogateagent <FQDN> <IP_SERVER> <PORT> 0||1\n\n"
				"Four arguments are required: FQDN, IP address and port of "
				"server and numeric value (0 or 1) where 0 represents "
				"'deactivate' and 1 'activate'" << endl;
		return EXIT_FAILURE;
	}
    uint32_t fqdn;
    uint32_t ipAddress;
	uint16_t port;
    uint32_t argvCommand;
	// Read FQDN
	fqdn = hashString(argv[1]);
	cout << "FQDN entered: " << argv[1] << ", hash = " << fqdn << endl;
	// Read the IP of the endpoint serving FQDN
    struct in_addr address;
    inet_aton(argv[2], &address);
    ipAddress = address.s_addr;
    cout << "IP entered: " << argv[2] << ", binary = " << ipAddress << endl;
    // Read port
    port = atoi(argv[3]);
    cout << "Port entered: " << port << endl;
    // Read activation/deactivation input
    argvCommand = atoi(argv[4]);

    if (argvCommand != 1 && argvCommand != 0)
    {
    	cout << "Fourth argument must be 1 (activate) or 0 (deactivate)\n";
    	return EXIT_FAILURE;
    }

    // Create netlink socket and send off the command following the NAP-SA
    // specification
	/* Netlink socket SA-NAP sender Start */
	struct sockaddr_nl src_addr, dest_addr;
	struct nlmsghdr *nlh = NULL;
	struct iovec iov;
	struct msghdr msg;
	int socket_SA_NAP, return_socket_SA_NAP;
	/* create socket */
	socket_SA_NAP = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);

	if(socket_SA_NAP < 0)
	{
		cout << strerror(errno) << endl;
	}
	else
	{
		cout << "Socket created"  << endl;
	}

	memset(&msg, 0, sizeof(msg));
	memset(&src_addr, 0, sizeof(src_addr));
	// SRC Address
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid();
	src_addr.nl_groups = 0;  /* not in mcast groups */
	src_addr.nl_pad = 0;
	/* bind socket */
	return_socket_SA_NAP = bind(socket_SA_NAP, (struct sockaddr*) &src_addr,
			sizeof(src_addr));

	if(return_socket_SA_NAP < 0)
	{
		cout << "Cannot bind to socket: " << strerror(errno) << endl;
	}
	else
	{
		cout << "Bound to socket"  << endl;
	}

	// DST address
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = PID_NAP_SA_LISTENER;
	dest_addr.nl_groups = 0; /* unicast */
	dest_addr.nl_pad = 0;
	/* Fill in the netlink message payload */
	nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(sizeof(fqdn) +
			sizeof(ipAddress) + sizeof(port)));
	/* Fill the netlink message header */
	nlh->nlmsg_len = NLMSG_SPACE(sizeof(fqdn) + sizeof(ipAddress) +
			sizeof(port));
	nlh->nlmsg_pid = src_addr.nl_pid;
	nlh->nlmsg_flags = NLM_F_REQUEST;

	if(argvCommand == 1)
	{
		nlh->nlmsg_type = NAP_SA_ACTIVATE;
	}
	else if(argvCommand == 0)
	{
		nlh->nlmsg_type = NAP_SA_DEACTIVATE;
	}

	nlh->nlmsg_seq = rand();
	cout << "Netlink port information: SRC port = " << src_addr.nl_pid << ", "
			"DST port = " << dest_addr.nl_pid << endl;
	// [1] hashed FQDN
	memcpy(NLMSG_DATA(nlh), &fqdn, sizeof(fqdn));
	// [2] IP address
	memcpy((uint8_t *)NLMSG_DATA(nlh) + sizeof(fqdn), &ipAddress,
			sizeof(ipAddress));
	// [3] port
	memcpy((uint8_t *)NLMSG_DATA(nlh) + sizeof(fqdn) + sizeof(ipAddress), &port,
			sizeof(port));
	// Header
	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	// Creating message
	msg.msg_name = (void *)&dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	/* Send the message */
	return_socket_SA_NAP = sendmsg(socket_SA_NAP, &msg, 0);

	if(return_socket_SA_NAP < 0)
	{
		cout << "Message could not be sent to NAP: " << strerror(errno) << endl;
	}
	else
	{
		cout << "Message of length " << return_socket_SA_NAP << " successfully "
				"sent to NAP" << endl;
	}

	cout << "Closing socket" << endl;
	close(socket_SA_NAP);
	free(nlh);
    return 0;
}
