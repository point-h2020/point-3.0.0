/*
 * bandwidth_subscriber.cpp
 *
 *  Created on: 6 Mar 2017
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
 *
 * How to make it:
 *
 * ~$ g++ -std=c++11 -g -Wall -O2 -c bandwidth_subscriber.cc
 * ~$ g++ -o bandwidth_subscriber bandwidth_subscriber.o -lblackadder -lpthread -lbampers -lboost_system
 */

#include <bampers/bampers.hh>
#include <blackadder.hpp>
#include <limits>
#include <time.h>
#include <signal.h>

Blackadder *blackadder;
bool run = true;
uint32_t throughputTotal = 0;
time_t *startTime = new time_t;

void readSignal(int signal);

int main()
{
	uint32_t sequenceReceived = 0;
	uint32_t sequenceLast = 0;
	uint32_t throughput = 0;
	time_t *heartBeat = new time_t;
	time_t *currentTime = new time_t;
	struct tm *timestamp;
	double timeDifference;
	IcnId cidData;
	IcnId cidFeedback;
	cidData.rootNamespace(NAMESPACE_EXPERIMENTATION);
	cidFeedback.rootNamespace(NAMESPACE_EXPERIMENTATION);
	cidData.append(EXPERIMENTAION_BANDWIDTH);
	cidFeedback.append(EXPERIMENTAION_BANDWIDTH);
	blackadder = Blackadder::Instance(true);
	// Publish root scope
	blackadder->publish_scope(cidData.binRootScopeId(), cidData.binEmpty(),
			DOMAIN_LOCAL, NULL, 0);
	cout << "Root namespace /" << cidData.rootScopeId() << " published\n";
	// publish EXPERIMENTATION scope
	blackadder->publish_scope(cidData.binId(), cidData.binPrefixId(),
			DOMAIN_LOCAL, NULL, 0);
	cout << "Scope /" << cidData.id() << " published under root scope /"
			<< cidData.rootScopeId() << endl;
	// advertise and subscribe to information items
	cidFeedback.append(EXPERIMENTATION_BANDWIDTH_FEEDBACK);
	cidData.append(EXPERIMENTATION_BANDWIDTH_DATA);
	blackadder->publish_info(cidFeedback.binId(), cidFeedback.binPrefixId(),
			DOMAIN_LOCAL, NULL, 0);
	cout << "Advertised to Feedback CID " << cidFeedback.print() << endl;
	blackadder->subscribe_info(cidData.binId(), cidData.binPrefixId(),
			DOMAIN_LOCAL, NULL, 0);
	cout << "Subscribed to Data CID " << cidData.print() << endl;
	// set heartbeat
	time(heartBeat);
	signal(SIGINT, readSignal);
	signal(SIGABRT, readSignal);
	signal(SIGTERM, readSignal);
	signal(SIGQUIT, readSignal);

	while (run)
	{
		IcnId icnId;
		Event event;
		blackadder->getEvent(event);
		string eventId = chararray_to_hex(event.id);
		icnId = eventId;

		switch (event.type)
		{
		case SCOPE_PUBLISHED:
			break;
		case SCOPE_UNPUBLISHED:
			break;
		case START_PUBLISH:
			break;
		case STOP_PUBLISH:
			break;
		case PUBLISHED_DATA:
		{
			if (throughputTotal == 0)
			{
				time(startTime);
			}

			time (currentTime);
			timeDifference = difftime(*currentTime, *heartBeat);
			memcpy(&sequenceReceived, event.data, sizeof(sequenceReceived));

			// feed back last received sequence number
			if (sequenceLast != 0 && sequenceReceived != (sequenceLast + 1))
			{
				timestamp = localtime(currentTime);
				cout << timestamp->tm_hour << ":" << setw(2) << setfill('0')
						<< timestamp->tm_min << ":"
						<< setw(2) << setfill('0') << timestamp->tm_sec
						<< "\tPacket loss: " << sequenceReceived -
						sequenceLast << endl;
				blackadder->publish_data(cidFeedback.binIcnId(), DOMAIN_LOCAL,
						NULL, 0, (void *)&sequenceLast, sizeof(sequenceLast));
			}
			else if (sequenceReceived == 0 && sequenceReceived < sequenceLast)
			{
				cout << "Reset sequence counter\n";
				sequenceLast = 0;
			}

			sequenceLast = sequenceReceived;
			throughput += event.data_len;

			// Print out results if heartbeat has been reached
			if ((float)timeDifference >= 1.0)
			{
				timestamp = localtime(currentTime);
				cout << timestamp->tm_hour << ":" << setw(2) << setfill('0')
						<< timestamp->tm_min << ":"
						<< setw(2) << setfill('0') << timestamp->tm_sec
						<< "\tThroughput: " << setw(8) << setfill(' ')
						<< throughput * 8 / 1000000.00 << " Mbps\tTotal # of "
						"packets: " << sequenceLast << endl;
				time(heartBeat);//reset heartBeat
				throughputTotal += throughput;
				throughput = 0;
			}

			break;
		}
		default:
			cout << "Unknown event type received from Blackadder: "
					<< (int)event.type << endl;
		}
	}
}

void readSignal(int signal)
{
	run = false;
	blackadder->disconnect();
	delete blackadder;
}
