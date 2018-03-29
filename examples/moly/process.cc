/*
 * process.cc
 *
 *  Created on: 13 Jan 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *      		Mays Al-Naday <mfhaln@essex.ac.uk>
 *
 * This file is part of the Monitoring LibrarY (MOLY).
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

#include <moly/moly.hh>
#include <iostream>
#include <string.h>
#include <stdio.h>

using namespace std;

/*!
 * \brief Main loop
 *
 * This is an example process application which communicates with MONA via MOLY
 */
int main()
{
	Moly moly;
	string msg = "Hit ENTER to send ";
	char input[255];

	// initialise communication with MONA and wait until data points can be
	// reported
	if (!moly.Process::startReporting())
	{
		cout << "Communication with monitoring agent failed ... exiting\n";
		return EXIT_FAILURE;
	}

	cout << "Trigger received. Start reporting\n";

	// Topology related values (static)
	// Node
	if (!moly.Process::addNode("TM", 1, NODE_ROLE_TM))
	{
		cout << "Sending ADD_NODE failed\n";
	}
	else
	{
		cout << "ADD_NODE(TM, 1, NODE_ROLE_TM) sent\n";
	}

	cout << msg << "NODE_STATE" << endl;
	fgets(input, sizeof input, stdin);

	if (!moly.Process::nodeState(NODE_ROLE_TM, 1, NODE_STATE_UP))
	{
		cout << "Seding NODE_STATE failed\n";
	}
	else
	{
		cout << "NODE_STATE(NODE_ROLE_TM, 1, NODE_STATE_UP) sent\n";
	}

	cout << msg << "ADD_LINK" << endl;
	fgets(input, sizeof input, stdin);

	// Link
	if (!moly.Process::addLink(2, 1, 2, 0, LINK_TYPE_SDN_802_3_Z, "1>2"))
	{
		cout << "Sending ADD_LINK failed\n";
	}
	else
	{
		cout << "ADD_LINK(2, 1, 2, 0, LINK_TYPE_SDN_802_3_Z, 1>2) sent\n";
	}

	cout << msg << "LINK_STATE" << endl;
	fgets(input, sizeof input, stdin);

	if (!moly.Process::linkState(2, 1, 2, 0, LINK_TYPE_SDN_802_3_Z,
			LINK_STATE_UP))
	{
		cout << "Sending LINK_STATE failed\n";
	}
	else
	{
		cout << "LINK_STATE(2, 1, 2, 0, LINK_TYPE_SDN_802_3_Z, LINK_STATE_UP) "
				"sent\n";
	}

	cout << msg << "ADD_PORT" << endl;
	fgets(input, sizeof input, stdin);

	// Port
	if (!moly.Process::addPort(1, 0, "enp0s3"))
	{
		cout << "Sending ADD_PORT failed\n";
	}
	else
	{
		cout << "ADD_PORT(1, 0, enp0s3) sent\n";
	}

	cout << msg << "PORT_STATE" << endl;
	fgets(input, sizeof input, stdin);

	if (!moly.Process::portState(1, 0, PORT_STATE_UP))
	{
		cout << "Sending PORT_STATE failed\n";
	}
	else
	{
		cout << "PORT_STATE(1, 0, PORT_STATE_UP) sent\n";
	}

	// Statistics
	cout << msg << "CHANNEL_AQUISITION_TIME" << endl;
	fgets(input, sizeof input, stdin);

	if (!moly.Process::channelAquisitionTime(101))
	{
		cout << "Sending CHANNEL_AQUISITION_TIME failed\n";
	}
	else
	{
		cout << "CHANNEL_AQUISITION_TIME(101) sent\n";
	}

	cout << msg << "CMC_GROUP_SIZE" << endl;
	fgets(input, sizeof input, stdin);

	if (!moly.Process::cmcGroupSize(5))
	{
		cout << "Sending CMC_GROUP_SIZE failed\n";
	}
	else
	{
		cout << "CMC_GROUP_SIZE(5) sent\n";
	}

	cout << msg << "TX_BYTES" << endl;
	fgets(input, sizeof input, stdin);

	if (!moly.Process::txBytesPort(1, 0, 0))
	{
		cout << "Sending TRANSMITTED_BYTES failed\n";
	}
	else
	{
		cout << "TX_BYTES(1, 0, 123456) sent\n";
	}

	cout << msg << "PACKET_DROP_RATE" << endl;
	fgets(input, sizeof input, stdin);
	int rate = 0;

	if (!moly.Process::packetDropRate(1, 0, rate))
	{
		cout << "Sending PACKET_DROP_RATE failed\n";
	}
	else
	{
		cout << "PACKET_DROP_RATE(1, 0, " << rate << ") sent\n";
	}

	cout << msg << "PACKET_ERROR_RATE" << endl;
	fgets(input, sizeof input, stdin);

	rate = 0;

	if (!moly.Process::packetErrorRate(1, 0, rate))
	{
		cout << "Sending PACKET_ERROR_RATE failed\n";
	}
	else
	{
		cout << "PACKET_ERROR_RATE(1, 0, " << rate << ") sent\n";
	}

	cout << msg << "RX_BYTES_HTTP" << endl;
	fgets(input, sizeof input, stdin);
	rate = 0;

	if (!moly.Process::rxBytesHttp(1, rate))
	{
		cout << "Sending RX_BYTES_HTTP failed\n";
	}
	else
	{
		cout << "RX_BYTES_HTTP(1, " << rate << ") sent\n";
	}

	cout << msg << endl;
	fgets(input, sizeof input, stdin);

	if (!moly.Process::rxBytesIpMulticast(1, 4, 3456))
	{
		cout << "Sending RX_BYTES_IP_MULTICAST failed\n";
	}
	else
	{
		cout << "RX_BYTES_IP_MULTICAST(1, 4, 3456) sent\n";
	}

	cout << msg << endl;
	fgets(input, sizeof input, stdin);

	if (!moly.Process::rxBytesPort(1, 0, 2301))
	{
		cout << "Sending RX_BYTES_PORT failed\n";
	}
	else
	{
		cout << "RX_BYTES_PORT(1, 0, 2301) sent\n";
	}

	cout << msg << "BUFFER_SIZES_M" << endl;
	fgets(input, sizeof input, stdin);

	// Buffer Sizes
	buffer_sizes_t bufferSizes;
	bufferSizes.push_back(pair<buffer_name_t, buffer_size_t>(
			BUFFER_NAP_HTTP_HANDLER_REQUESTS, 23));
	bufferSizes.push_back(pair<buffer_name_t, buffer_size_t>(
			BUFFER_NAP_HTTP_HANDLER_RESPONSES, 2301));
	bufferSizes.push_back(pair<buffer_name_t, buffer_size_t>(
			BUFFER_NAP_IP_HANDLER, 15));

	if (!moly.Process::bufferSizes(NODE_ROLE_NAP, bufferSizes))
	{
		cout << "Sending BUFFER_SIZES_M failed\n";
	}
	else
	{
		cout << "BUFFER_SIZES_M sent\n";
	}

	cout << msg << "BPUBLISHERS_PER_NAMESPACE_M" << endl;
	fgets(input, sizeof input, stdin);
	// RV
	publishers_namespace_t pubs;
	publishers_namespace_t::iterator it;
	pubs.push_back(pair<root_scope_t, publishers_t>(NAMESPACE_IP, 23));
	pubs.push_back(pair<root_scope_t, publishers_t>(NAMESPACE_HTTP, 15));
	pubs.push_back(pair<root_scope_t, publishers_t>(NAMESPACE_COAP, 4));

	if (!moly.Process::publishersNamespace(pubs))
	{
		cout << "Sending PUBLISHERS_NAMESPACE failed\n";
	}
	else
	{
		cout << "PUBLISHERS_NAMESPACE sent\n";

		for (it = pubs.begin(); it != pubs.end(); it++)
		{
			cout << it->first << "\t" << it->second << endl;
		}
	}

	cout << msg << endl;
	fgets(input, sizeof input, stdin);
	subscribers_namespace_t subs;
	subscribers_namespace_t::iterator subsIt;
	subs.push_back(pair<root_scope_t, subscribers_t>(NAMESPACE_IP, 23));
	subs.push_back(pair<root_scope_t, subscribers_t>(NAMESPACE_HTTP, 15));
	subs.push_back(pair<root_scope_t, subscribers_t>(NAMESPACE_COAP, 4));

	if (!moly.Process::subscribersNamespace(subs))
	{
		cout << "Sending SUBSCRIBERS_NAMESPACE failed\n";
	}
	else
	{
		cout << "SUBSCRIBERS_NAMESPACE sent\n";

		for (subsIt = subs.begin(); subsIt != subs.end(); subsIt++)
		{
			cout << subsIt->first << "\t" << subsIt->second << endl;
		}
	}

	cout << msg << endl;
	fgets(input, sizeof input, stdin);

	// RV
	matches_namespace_t matches;
	matches_namespace_t::iterator matchesIt;
	matches.push_back(pair<root_scope_t, matches_t>(NAMESPACE_IP, 23));
	matches.push_back(pair<root_scope_t, matches_t>(NAMESPACE_HTTP, 15));
	matches.push_back(pair<root_scope_t, matches_t>(NAMESPACE_COAP, 4));

	if (!moly.Process::matchesNamespace(matches))
	{
		cout << "Sending MATCHES_NAMESPACE failed\n";
	}
	else
	{
		cout << "MATCHES_NAMESPACE sent\n";

		for (matchesIt = matches.begin(); matchesIt != matches.end();
				matchesIt++)
		{
			cout << matchesIt->first << "\t" << matchesIt->second << endl;
		}
	}

	// TM
	cout << msg << endl;
	fgets(input, sizeof input, stdin);
	path_calculations_namespace_t pathCalculations;
	path_calculations_namespace_t::iterator pathCalculationsIt;
	pathCalculations.push_back(pair<root_scope_t, subscribers_t>(NAMESPACE_IP, 23));
	pathCalculations.push_back(pair<root_scope_t, subscribers_t>(NAMESPACE_HTTP, 15));
	pathCalculations.push_back(pair<root_scope_t, subscribers_t>(NAMESPACE_COAP, 4));

	if (!moly.Process::pathCalculations(pathCalculations))
	{
		cout << "Sending PATH_CALCULATIONS_NAMESPACE failed\n";
	}
	else
	{
		cout << "PATH_CALCULATIONS_NAMESPACE sent\n";

		for (pathCalculationsIt = subs.begin();
				pathCalculationsIt != subs.end(); pathCalculationsIt++)
		{
			cout << pathCalculationsIt->first << "\t"
					<< pathCalculationsIt->second << endl;
		}
	}

	return EXIT_SUCCESS;
}
