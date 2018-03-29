/*
 * bandwidth_publisher.cpp
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
 * ~$ g++ -std=c++11 -g -Wall -O2 -c bandwidth_publisher.cc
 * ~$ g++ -o bandwidth_publisher bandwidth_publisher.o -lblackadder -lpthread -lbampers -lboost_system
 */

#include <blackadder.hpp>
#include <bampers/bampers.hh>
#include <chrono>
#include <mutex>
#include <signal.h>
#include <stdlib.h>
#include <thread>
#include <time.h>

Blackadder *blackadder;
uint32_t publishingInterval = 10000;//[ns]
uint32_t sequence = 0;
bool publishData = false;
bool run = true;
bool justWarmingUp = true; // speed up fast up to a saturation point
std::mutex feedbackMutex;

IcnId cidData;
IcnId cidFeedback;

void dataPusher();

void readSignal(int signal);

int main()
{
	std::thread dataPusherThread;
	uint32_t sequenceReceived;
	time_t *currentTime = new time_t;
	struct tm *timestamp;
	cidData.rootNamespace(NAMESPACE_EXPERIMENTATION);
	cidFeedback.rootNamespace(NAMESPACE_EXPERIMENTATION);
	cidData.append(EXPERIMENTAION_BANDWIDTH);
	cidFeedback.append(EXPERIMENTAION_BANDWIDTH);
	blackadder = Blackadder::Instance(true);
	dataPusherThread = std::thread(dataPusher);
	blackadder->publish_scope(cidData.binRootScopeId(), cidData.binEmpty(),
			DOMAIN_LOCAL, NULL, 0);
	cout << "Published root scope /" << cidData.rootScopeId() << endl;
	blackadder->publish_scope(cidData.binId(), cidData.binPrefixId(),
			DOMAIN_LOCAL, NULL, 0);
	cout << "Published scope /" <<  cidData.id() << " under root namespace /"
			<< cidData.rootScopeId() << endl;
	cidData.append(EXPERIMENTATION_BANDWIDTH_DATA); //info item
	cidFeedback.append(EXPERIMENTATION_BANDWIDTH_FEEDBACK); //info item
	blackadder->publish_info(cidData.binId(), cidData.binPrefixId(),
			DOMAIN_LOCAL, NULL, 0);
	cout << "Advertised CID (EXP > BW > DATA) " << cidData.print() << endl;
	signal(SIGINT, readSignal);
	signal(SIGABRT, readSignal);
	signal(SIGTERM, readSignal);
	signal(SIGQUIT, readSignal);

	while (run)
	{
		IcnId icnId;
		Event event;
		string eventId;
		blackadder->getEvent(event);

		eventId = chararray_to_hex(event.id);
		icnId = eventId;

		switch (event.type)
		{
		case SCOPE_PUBLISHED:
			break;
		case SCOPE_UNPUBLISHED:
			break;
		case START_PUBLISH:
			cout << "START_PUBLISH received for " << icnId.print() << endl;
			publishData = true;
			blackadder->subscribe_info(cidFeedback.binId(),
					cidFeedback.binPrefixId(), DOMAIN_LOCAL, NULL, 0);
			cout << "Subscribed to feedback channel CID " << cidFeedback.print()
					<< endl;
			break;
		case STOP_PUBLISH:
			cout << "STOP_PUBLISH received for " << icnId.print() << endl;
			publishData = false;
			break;
		case PUBLISHED_DATA:
			time(currentTime);
			timestamp = localtime(currentTime);
			feedbackMutex.lock();

			if (atoi(icnId.id().c_str()) == EXPERIMENTATION_BANDWIDTH_FEEDBACK)
			{
				justWarmingUp = false;
				memcpy(&sequenceReceived, event.data, sizeof(sequenceReceived));
				cout << timestamp->tm_hour << ":" << setw(2) << setfill('0')
						<< timestamp->tm_min << ":"
						<< setw(2) << setfill('0') << timestamp->tm_sec
						<< "\tPacket loss\tCurrent Interval: "
						<< publishingInterval << " ns\tLast published sequence"
						": " << sequence << "\t\tLast received sequence on "
						"subscriber: " << sequenceReceived << " (" << sequence -
						sequenceReceived << " lost)" << endl;
				publishingInterval = publishingInterval + 1000;//[ns]
			}

			feedbackMutex.unlock();
			break;
		default:
			cout << "Unknown event type received from Blackadder: "
					<< (int)event.type << endl;
		}
	}

	return 0;
}

void dataPusher()
{
	time_t *heartBeat = new time_t;
	time_t *currentTime = new time_t;
	double timeDifference;
	struct tm *timestamp;
	uint32_t throughput = 0;
	uint32_t numberOfPacketsAtLastHeartbeat = 0;
	cout << "Started data pusher thread .. waiting for start publish\n";

	// wait until START_PUBLISH arrives
	while (!publishData)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	size_t dataLength = 1300;
	void *data = malloc(dataLength);
	bzero(data, dataLength);
	cout << "Start publishing data under " << cidData.print() << endl;
	time(heartBeat);

	// now publish until publishData is false
	while (run && publishData)
	{
		time (currentTime);
		memcpy(data, &sequence, sizeof(sequence));
		memcpy((uint8_t *)data + sizeof(sequence), currentTime,
				sizeof(*currentTime));
		blackadder->publish_data(cidData.binIcnId(), DOMAIN_LOCAL, 0, 0, data,
				dataLength);
		throughput += dataLength;
		feedbackMutex.lock();

		if (justWarmingUp)
		{
			if (publishingInterval > 0)
			{
				publishingInterval = publishingInterval - 1;
				std::this_thread::sleep_for(
						std::chrono::nanoseconds(publishingInterval));
			}
			else
			{
				justWarmingUp = false;
			}
		}
		else
		{
			timeDifference = difftime(*currentTime, *heartBeat);

			if ((float)timeDifference >= 1.0)
			{
				timestamp = localtime(currentTime);
				cout << timestamp->tm_hour << ":" << setw(2) << setfill('0')
						<< timestamp->tm_min << ":"
						<< setw(2) << setfill('0') << timestamp->tm_sec
						<< "\tThroughput: " << setw(8) << setfill(' ')
						<< throughput * 8 / 1000000.00 << " Mbps\t# of sent "
						"packets: " << sequence - numberOfPacketsAtLastHeartbeat
						<< "\ttotal # of packets: " << sequence
						<< "\tPublishing Interval: " << publishingInterval
						<< "ns" << endl;
				// (re-)set values
				numberOfPacketsAtLastHeartbeat = sequence;
				time(heartBeat);

				if (publishingInterval >= 100)
				{
					publishingInterval = publishingInterval - 100;
				}
				else if (publishingInterval > 0)
				{
					publishingInterval = publishingInterval - 1;
				}

				throughput = 0;
			}

			if (publishingInterval != 0)
			{
				std::this_thread::sleep_for(
						std::chrono::nanoseconds(publishingInterval));
			}
		}

		sequence++;
		feedbackMutex.unlock();
	}

	free(data);
	cout << "dataPusher thread stopped\n";
}

void readSignal(int signal)
{
	run = false;
	publishData = false;
	cout << "Disconnect from Blackadder core\n";
	blackadder->disconnect();
	delete blackadder;
	exit(0);
}
