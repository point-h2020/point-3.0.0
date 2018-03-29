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
#include <list>

Blackadder *ba = NULL;
pthread_t _event_listener, *event_listener = NULL;
sig_atomic_t listening = 1;

int res_pings = 0;
int req_pings = 0;
struct timeval req_tv;
string s_nodeID;
string c_nodeID;
//iSUB nodeID in the format of PURSUIT_ID_LEN
string cnodeID;
string bin_cnodeID;
size_t s_nodeID_len;
size_t c_nodeID_len;
string s_id;
string c_id;
string s_bin_id;
string c_bin_id;
string id;
string prefix_id;
string bin_id;
string bin_prefix_id;

/*Path Management notification Scope - this should be consolidated later on with the resl_id as they could both fall under the same data structure*/
string pathMgmt_id = string(PURSUIT_ID_LEN * 2 - 1, 'F') + "9";
string pathMgmt_bin_id = hex_to_chararray(pathMgmt_id);
string pathMgmt_prefix_id = string();
string pathMgmt_bin_prefix_id = hex_to_chararray(pathMgmt_prefix_id);
string pathMgmt_bin_full_id = pathMgmt_bin_id + pathMgmt_bin_id;
void eventHandler(Event *ev) {
    struct timeval live_tv;
    struct timeval delay_tv;
    string nodeID;
    char * payload;
    int payload_size;
    stringstream req_str;
//	unsigned char id_len;
	string sub_id;
	string sub_bin_id;
    switch (ev->type) {
        case START_PUBLISH:
            payload_size = sizeof(res_pings);
            payload = (char *)malloc(payload_size);
            snprintf(payload, payload_size+1, "%d", res_pings);
            gettimeofday(&req_tv, NULL);
			cout << "Implicit Subscription sent..." << endl;
			sub_id = prefix_id + s_id;
			sub_bin_id = bin_prefix_id + hex_to_chararray(s_id);
//			id_len = sub_bin_id.length()/PURSUIT_ID_LEN;
			ba->publish_data_isub(ev->id, DOMAIN_LOCAL, NULL, 0, sub_bin_id, payload, payload_size);
            break;
        case PUBLISHED_DATA:
			/*normal publication*/
			gettimeofday(&live_tv, NULL);
			res_pings = atoi((char *)ev->data);
			nodeID = string(PURSUIT_ID_LEN - s_nodeID_len, '0');
			nodeID += s_nodeID;
			delay_tv.tv_sec = live_tv.tv_sec - req_tv.tv_sec;
			if (live_tv.tv_usec >= req_tv.tv_usec) {
				delay_tv.tv_usec = live_tv.tv_usec - req_tv.tv_usec;
			} else {
				delay_tv.tv_usec = live_tv.tv_usec + 1000000 - req_tv.tv_usec;
			}
			printf("Ping sequence %d has been recieved from Node %s: Lost pings: %d ,RTT delay: %ld s and %f ms \n", res_pings, (char *) nodeID.c_str() , res_pings - req_pings -1, delay_tv.tv_sec, (double)delay_tv.tv_usec/1000);
			req_pings = res_pings;
			gettimeofday(&req_tv, NULL);
			break;
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
    (void) signal(SIGINT, SIG_DFL);
    ba->disconnect();
    delete ba;
    cout << "Ping App: disconnecting" << endl;
    exit(0);
}


int main(int argc, char* argv[]) {
    int user_or_kernel;
    switch (argc) {
		case 3:{
			/*By Default I assume blackadder is running in user space*/
			ba = Blackadder::Instance(true);
			/*construct the nodeID from the second arg*/
			s_nodeID = argv[1];
			s_nodeID_len = s_nodeID.size();
			c_nodeID = argv[2];
			c_nodeID_len = c_nodeID.size();
			break;
		}
		case 4:{
			user_or_kernel = atoi(argv[1]);
			if (user_or_kernel == 0) {
				ba = Blackadder::Instance(true);
			} else {
				ba = Blackadder::Instance(false);
			}
			/*By Default I assume blackadder is running in user space*/
			ba = Blackadder::Instance(true);
			/*construct the nodeID from the second arg*/
			s_nodeID = argv[2];
			s_nodeID_len = s_nodeID.size();
			c_nodeID = argv[3];
			c_nodeID_len = c_nodeID.size();
			break;
		}
		default:
            cout << "Error: Wrong number of input arguments" << endl
            << "App usage: sudo ./ping_publisher <user_or_kernel>  <pinged_node> NODE_NONZERO_DIGITS (Required) <publishing_node> NODE_NONZERO_DIGITS (Required)" << endl
            << "Defaults: user_or_kernel 1 rv_or_implicit 1 number_of_pings 1000" << endl
            << "E.g.: sudo ./ping_publisher 130 244 5" << endl;
            break;
    }
    /*Set the Event Handler Listning Thread*/
    pthread_create(&_event_listener, NULL, event_listener_loop, (void *) ba);
    event_listener = &_event_listener;
    /***************************/
    cout << "Process ID: " << getpid() << endl;
    /*****************************publish root_scope /0000000000000000 ************************************/
    id = string(PURSUIT_ID_LEN * 2, '0');
    prefix_id = string();
    bin_id = hex_to_chararray(id);
    bin_prefix_id = hex_to_chararray(prefix_id);
    ba->publish_scope(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
    prefix_id=id;
    bin_prefix_id = hex_to_chararray(prefix_id);
    /*Pub(client) publishes scope root_scope/Server_nodeID, and subscribes to scope root_scope/0000000000000105*/
    s_id = string(((PURSUIT_ID_LEN * 2) - s_nodeID_len), '0');
    s_id += s_nodeID;
    s_bin_id = hex_to_chararray(s_id);
    c_id = string(((PURSUIT_ID_LEN * 2) - c_nodeID_len), '0');
	/*for testing iSub I will hardcode the scope to be of clientNAP 105*/
	c_id += "105";
	c_bin_id = hex_to_chararray(c_id);
	ba->publish_scope(s_bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);

	cnodeID = string((PURSUIT_ID_LEN - c_nodeID_len), '0') + c_nodeID;
	bin_cnodeID = hex_to_chararray(cnodeID);
    /*Pub(client) publishes ping request (as item) root_scope/ServerID/0000000000000105, and*/
    /*subscribes to ping responses (published by the Server under) root_socpe/0000000000000105/ServerID*/
	prefix_id = prefix_id + s_id;
	bin_prefix_id = hex_to_chararray(prefix_id);
	ba->publish_info(c_bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
	prefix_id = prefix_id + c_id;
	prefix_id = string(PURSUIT_ID_LEN * 2, '0') + c_id;
	bin_prefix_id = hex_to_chararray(prefix_id);
	/*Subscribe to Path Management Information provided by the TM*/
	ba->subscribe_scope(pathMgmt_bin_id, pathMgmt_bin_prefix_id, DOMAIN_LOCAL,NULL, 0);
	
    sleep(5);
    pthread_join(*event_listener, NULL);
    cout << "Ping App: disconnecting" << endl;
	
	ba->unpublish_scope(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
	ba->unpublish_scope(s_bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
	

    ba->disconnect();
    delete ba;
    cout << "exiting...." << endl;
    return 0;
}
