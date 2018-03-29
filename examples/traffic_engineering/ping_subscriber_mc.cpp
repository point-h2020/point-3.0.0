/*
 * Copyright (C) 2015-2017  Mays AL-Naday
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 3 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * See LICENSE and COPYING for more details.
 */

#include <blackadder.hpp>
#include <signal.h>

#include <sstream> 
#include <iostream>
#include <sys/time.h>


Blackadder *ba = NULL;
pthread_t _event_listener, *event_listener = NULL;
pthread_t _ping_listener, *ping_listener = NULL;
sig_atomic_t listening = 1;
/*Path Management notification Scope - this should be consolidated later on with the resl_id as they could both fall under the same data structure*/
string pathMgmt_id = string(PURSUIT_ID_LEN * 2 - 1, 'F') + "9";
string pathMgmt_bin_id = hex_to_chararray(pathMgmt_id);
string pathMgmt_prefix_id = string();
string pathMgmt_bin_prefix_id = hex_to_chararray(pathMgmt_prefix_id);
string pathMgmt_bin_full_id = pathMgmt_bin_id + pathMgmt_bin_id;
int no_pings = 0;
struct timeval live_tv;
list<string> nodeIds;

void *ping_handler (void *arg){
	char * payload;
	int payload_size;
	string isubID = string((const char *) arg, PURSUIT_ID_LEN * 3);
	cout << "Implicit Subscription ID: " << chararray_to_hex(isubID) << endl;
	while (nodeIds.size()>0) {
		payload_size = sizeof(no_pings);
		payload = (char *)malloc(payload_size);
		gettimeofday(&live_tv, NULL);
		snprintf(payload, payload_size+1, "%d", no_pings);
		ba->publish_data(isubID, DOMAIN_LOCAL, NULL, 0, nodeIds, payload, payload_size);
		no_pings++;
		printf("Direct ping sequence %d has been successfuly published.\n", no_pings);
		sleep(1);
	}
	cout << "No more publishers, will stop sending pings..." << endl;
	return NULL;
}

void eventHandler(Event *ev) {
    string id;
    string bin_id;
    string prefix_id;
    string bin_prefix_id;
    string c_id;
    string father_scope;
    switch (ev->type) {
		case PUBLISHED_DATA_iSUB:
			sleep(2);
			cout << "implicit subscription recieved from :"
			<< chararray_to_hex(ev->nodeId)
			<< " for ID: "
			<< chararray_to_hex(ev->isubID)
			<< endl;
			nodeIds.push_front(ev->nodeId);
			cout << "request a multicast delivery" << endl;
			if (nodeIds.size() < 2) {
				ba->publish_info(ev->isubID.substr(ev->isubID.length() - PURSUIT_ID_LEN), ev->isubID.substr(0, ev->isubID.length() - PURSUIT_ID_LEN), DOMAIN_LOCAL, NULL, 0);
				/*Set the Event Handler Listning Thread*/
				pthread_create(&_ping_listener, NULL, ping_handler, (void*)ev->isubID.c_str());
				ping_listener = &_ping_listener;
				pthread_detach(*ping_listener);
				/***************************/
			}
		case START_PUBLISH_iSUB:
			cout << "PING: recieved START_PUBLISH_iSUB" << endl;
        default:
            break;
    }
}
void *event_listener_loop(void *arg) {
    Blackadder *ba = (Blackadder *) arg;
    while (listening) {
        Event ev;
        ba->getEvent(ev);
        if (ev.type == UNDEF_EVENT) {
            if (!listening)
                cout << "Ping App: final event" << endl;
            return NULL;
        } else{
            eventHandler(&ev);
        }
    }
    return NULL;
}
void sigfun(int sig) {
    listening = 0;
    if (event_listener)
        pthread_cancel(*event_listener);
	if(ping_listener)
		pthread_cancel(*ping_listener);
    (void) signal(SIGINT, SIG_DFL);
     ba->disconnect();
    delete ba;
    cout << "Ping App: disconnecting" << endl;
    exit(0);
}
int main(int argc, char* argv[]) {
    int user_or_kernel;
    string nodeID;
    size_t nodeID_len;
    string id;
    string prefix_id;
    string bin_id;
    string bin_prefix_id;
    switch (argc) {
            case 2:{
                /*By Default I assume blackadder is running in user space*/
                ba = Blackadder::Instance(true);
                /*construct the nodeID from the second arg*/
                nodeID = argv[1];
                nodeID_len = nodeID.size();
            }
            break;
            case 3:{
				user_or_kernel = atoi(argv[1]);
				if (user_or_kernel == 0) {
					ba = Blackadder::Instance(true);
				} else {
					ba = Blackadder::Instance(false);
				}
                nodeID = argv[2];
                nodeID_len = nodeID.size();
            }
            break;
            default:
            cout << "Error: Wrong number of input arguments" << endl
            << "App Usage: sudo ./ping_subscriber <user_or_kernel> (Optional) 1|0 <rv_or_implicit> (Optional) 1|0 <pinged_node> NODE_NONZERO_DIGITS" << endl
            << "Defaults: user_or_kernel 1 rv_or_implicit 1" << endl
            << "E.g.: sudo ./ping_subscriber 244" << endl;
            break;
    }
    
    /*Set the Event Handler Listning Thread*/
    pthread_create(&_event_listener, NULL, event_listener_loop, (void *) ba);
    event_listener = &_event_listener;
    /***************************/
    cout << "Process ID: " << getpid() << endl;
    id = string(PURSUIT_ID_LEN * 2, '0');
    prefix_id = string();
    bin_id = hex_to_chararray(id);
    bin_prefix_id = hex_to_chararray(prefix_id);
    ba->publish_scope(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
    
    id = string((PURSUIT_ID_LEN * 2) - nodeID_len, '0');
    id += nodeID;
    prefix_id = string(PURSUIT_ID_LEN * 2, '0');
    bin_id = hex_to_chararray(id);
    bin_prefix_id = hex_to_chararray(prefix_id);
    /*Server Sub to: 0000000000000000/serverID to recieve ping requests from all clients for any sequence of pings*/
    /*clients will publish requests in: 0000000000000000/serverID/0000000000000105*/
	/*Server response will be published in: 0000000000000000/0000000000000105/serverID*/
    ba->subscribe_scope(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
	/*Subscribe to Path Management Information provided by the TM*/
	ba->subscribe_scope(pathMgmt_bin_id, pathMgmt_bin_prefix_id, DOMAIN_LOCAL,NULL, 0);

    sleep(5);
    pthread_join(*event_listener, NULL);
    cout << "Ping App: disconnecting" << endl;
	ba->unpublish_scope(bin_prefix_id, string(), DOMAIN_LOCAL, NULL, 0);
    ba->disconnect();
    delete ba;
    cout << "exiting...." << endl;
    return 0;
}
