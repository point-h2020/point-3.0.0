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

#include <nb_blackadder.hpp>

#include <sstream> 
#include <iostream>
#include <sys/time.h>

NB_Blackadder *nb_ba;

int no_pings;
int rv_or_implicit;
string c_id;
string id;
string prefix_id;
string bin_id;
string bin_prefix_id;
string published_scope_id;
struct timeval live_tv;

void eventHandler(Event *ev) {
    char * payload;
    int payload_size;
    switch (ev->type) {
        case SCOPE_PUBLISHED:
            /*rv_or_implicit =1 -> Clients publish their pings as: AAAAAAAAAAAAAAAA/serverID/clientID/PingSequenceID*/
            /*Therefore Server need to subscribe to the child scope AAAAAAAAAAAAAAAA/serverID/clientID*/
            published_scope_id = chararray_to_hex((string)ev->id);
            cout << "SCOPE_PUBLISHED: " << published_scope_id << endl;
            id = string(published_scope_id, (PURSUIT_ID_LEN * 2) * 2, PURSUIT_ID_LEN * 2);
            prefix_id = string(published_scope_id, 0, (PURSUIT_ID_LEN * 2) * 2);
            bin_id = hex_to_chararray(id);
            bin_prefix_id = hex_to_chararray(prefix_id);
            cout << "Subscribing to /" << prefix_id << "/" << id << endl;
            nb_ba->subscribe_scope(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
            break;
        case PUBLISHED_DATA:
            no_pings = atoi((char *)ev->data);
            c_id = chararray_to_hex((string)ev->id);
            if ((no_pings < 1) & (rv_or_implicit == 0)) {
                /*rv_or_implicit = 0 -> firstping is publish_info under (SID/SID/RID) - (AAAAAAAAAAAAAAAA/clientID/serverID)*/
                prefix_id = string(c_id, 0, PURSUIT_ID_LEN * 2) + string(c_id, (PURSUIT_ID_LEN * 2) * 2, PURSUIT_ID_LEN * 2);
                id = string(c_id, PURSUIT_ID_LEN * 2, PURSUIT_ID_LEN * 2);
                bin_prefix_id = hex_to_chararray(prefix_id);
                bin_id = hex_to_chararray(id);
                nb_ba->publish_info(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
            }
            else if ((no_pings >= 1) & (rv_or_implicit == 0)) {
                /*rv_or_implicit = 0 -> subsequent pings are publish_data directly under (SID/SID/RID) - (AAAAAAAAAAAAAAAA/clientID/serverID)*/
                /*No More RV involvement*/
                prefix_id = string(c_id, 0, PURSUIT_ID_LEN * 2) + string(c_id, (PURSUIT_ID_LEN * 2) * 2, PURSUIT_ID_LEN * 2);
                bin_prefix_id = hex_to_chararray(prefix_id);
                id = string(c_id, PURSUIT_ID_LEN * 2, PURSUIT_ID_LEN * 2);
                bin_id = hex_to_chararray(id);
                payload_size = ev->data_len;
                payload = (char *)malloc(payload_size);
                snprintf(payload, payload_size+1, "%d", no_pings);
                nb_ba->publish_data(bin_prefix_id + bin_id, DOMAIN_LOCAL, NULL, 0, payload, payload_size);
                printf("Direct response for ping sequence %d has been successfully published to %s\n", no_pings, string(prefix_id, PURSUIT_ID_LEN * 3, NODEID_LEN).c_str());
            }
            else {
                /*rv_or_implicit = 1 -> pings published as: (SID/SID/SID/RID) - (AAAAAAAAAAAAAAAA/clientID/serverID/PingSequenceID)*/
                prefix_id = string(published_scope_id, 0, PURSUIT_ID_LEN * 2) + string(published_scope_id, (PURSUIT_ID_LEN * 2) * 2, PURSUIT_ID_LEN * 2) + string(published_scope_id, PURSUIT_ID_LEN * 2, PURSUIT_ID_LEN * 2);
                bin_prefix_id = hex_to_chararray(prefix_id);
                /*ping sequence = information ID*/
                id = string(c_id, (PURSUIT_ID_LEN * 2) * 3, PURSUIT_ID_LEN * 2);
                bin_id = hex_to_chararray(id);
                nb_ba->publish_info(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
            }
            break;
        case START_PUBLISH:
        	cout << "START_PUBLISH received for " << published_scope_id << endl;
            payload_size = sizeof(no_pings);
            payload = (char *)malloc(payload_size);
            gettimeofday(&live_tv, NULL);
            snprintf(payload, payload_size+1, "%d", no_pings);
            nb_ba->publish_data(bin_prefix_id + bin_id, DOMAIN_LOCAL, NULL, 0, payload, payload_size+1);
            printf("RV response for ping sequence %d has been successfuly published to %s\n", no_pings, string(prefix_id, PURSUIT_ID_LEN * 3, NODEID_LEN).c_str());
            break;
        case RE_PUBLISH:
            cout << "Ping: republish ping " << endl;
            prefix_id = string(ev->id, 0, ev->id.length() - PURSUIT_ID_LEN);
            id = string(ev->id, ev->id.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
            nb_ba->publish_info(id, prefix_id, DOMAIN_LOCAL, NULL, 0);
            break;
        default:
            break;
    }
    delete ev;
}

int main(int argc, char* argv[]) {
    int user_or_kernel;
    string nodeID;
    size_t nodeID_len;
    switch (argc) {
		case 2:{
			/*By Default I assume blackadder is running in user space*/
			nb_ba = NB_Blackadder::Instance(true);
			rv_or_implicit = 1;
			/*construct the nodeID from the second arg*/
			nodeID = argv[1];
			nodeID_len = nodeID.size();
			break;
		}
		case 3:{
			/*By Default I assume blackadder is running in user space*/
			nb_ba = NB_Blackadder::Instance(true);
			rv_or_implicit = atoi(argv[1]);
			nodeID = argv[2];
			nodeID_len = nodeID.size();
			break;
		}
		case 4:{
			user_or_kernel = atoi(argv[1]);
			if (user_or_kernel == 0) {
				nb_ba = NB_Blackadder::Instance(true);
			} else {
				nb_ba = NB_Blackadder::Instance(false);
			}
			rv_or_implicit = atoi(argv[2]);
			nodeID = argv[3];
			nodeID_len = nodeID.size();
			break;
		}
		default:
		cout << "Error: Wrong number of input arguments" << endl
		<< "App Usage: sudo ./ping_subscriber <user_or_kernel> (Optional) 1|0 <rv_or_implicit> (Optional) 1|0 <pinged_node> NODE_NONZERO_DIGITS" << endl
		<< "Defaults: user_or_kernel 1 rv_or_implicit 1" << endl
		<< "E.g.: sudo ./ping_subscriber 244" << endl;
		break;
    }
    /*Set the callback function*/
    nb_ba->setCallback(eventHandler);
    /***************************/
    cout << "Process ID: " << getpid() << endl;
	// Subsribe to /0/NID
    id = string((PURSUIT_ID_LEN * 2) - nodeID_len, '0');
    id += nodeID;
    prefix_id = string(PURSUIT_ID_LEN * 2, '0');
    bin_id = hex_to_chararray(id);
    bin_prefix_id = hex_to_chararray(prefix_id);
    cout << "Subscribing to /" << prefix_id << "/" << id << endl;
    /*Server Sub to: AAAAAAAAAAAAAAAA/serverID to recieve ping requests from all clients for any sequence of pings*/
    /*clients will publish requests in: AAAAAAAAAAAAAAAA/serverID/clientID/PingSequenceID*/
    /*Server will later on make further subscription to AAAAAAAAAAAAAAAA/serverID/clientID to recieve ping requests*/
    nb_ba->subscribe_scope(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
    nb_ba->join();
    nb_ba->disconnect();
    delete nb_ba;
    cout << "exiting...." << endl;
    return 0;
}
