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
int rv_or_implicit;
int res_pings = 0;
int req_pings = 0;
int previous_ping = 0;
struct timeval req_tv;
string s_nodeID;
string c_nodeID;
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
/*Notice, the implementation here assumes that responses will  always arrive in the same sequence as requests*/
/*\TODO: modify to maintain histroy of requests and responses to match sequences, e.g. using vector<int>*/
void eventHandler(Event *ev) {
    struct timeval live_tv;
    struct timeval delay_tv;
    string nodeID;
    char * payload;
    int payload_size;
    stringstream req_str;
    /*rv_or_implicit = 0 -->  ev->id (SID/SID/RID) - (AAAAAAAAAAAAAAAA/serverID/clientID)*/
    /*rv_or_implicit = 1 -->  ev->id (SID/SID/SID) - (AAAAAAAAAAAAAAAA/serverID/clientID)*/
    switch (ev->type) {
        case START_PUBLISH:
        	cout << "START_PUBLISH: " << chararray_to_hex(ev->id) << endl;
			if ((res_pings == 0)||(res_pings > previous_ping+1)) {
				payload_size = sizeof(res_pings);
				payload = (char *)malloc(payload_size);
				snprintf(payload, payload_size+1, "%d", res_pings);
				gettimeofday(&req_tv, NULL);
				cout << "Publish ping to " << chararray_to_hex(ev->id) << endl;
				nb_ba->publish_data(ev->id, DOMAIN_LOCAL, NULL, 0, payload, payload_size+1);
			}
            break;
        case RE_PUBLISH:
            cout << "Ping: republish" << endl;
            prefix_id = string(ev->id, 0, ev->id.length() - PURSUIT_ID_LEN);
            id = string(ev->id, ev->id.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
            nb_ba->publish_info(id, prefix_id, DOMAIN_LOCAL, NULL, 0);
            break;
        case PUBLISHED_DATA:
			res_pings = atoi((char *)ev->data);
            gettimeofday(&live_tv, NULL);
			sleep(1);
            nodeID = (char*)malloc(PURSUIT_ID_LEN);
            nodeID = string(PURSUIT_ID_LEN - s_nodeID_len, '0');
            nodeID += s_nodeID;
            delay_tv.tv_sec = live_tv.tv_sec - req_tv.tv_sec;
            if (live_tv.tv_usec >= req_tv.tv_usec) {
                delay_tv.tv_usec = live_tv.tv_usec - req_tv.tv_usec;
            } else {
                delay_tv.tv_usec = live_tv.tv_usec + 1000000 - req_tv.tv_usec;
            }
            printf("Ping sequence %d has been received from Node %s, RTT delay: %ld s and %f ms\n", res_pings, nodeID.c_str() , delay_tv.tv_sec, (double)delay_tv.tv_usec/1000);
			if (!(res_pings > previous_ping)) {
				res_pings++;
			}
            /*Publish the next ping*/
            if (req_pings > res_pings) {
				previous_ping = res_pings;
                res_pings++;
                req_str << res_pings;
                prefix_id = string(PURSUIT_ID_LEN * 2, '0') + s_id + c_id;
                bin_prefix_id = hex_to_chararray(prefix_id);
                gettimeofday(&req_tv, NULL);
                if (rv_or_implicit == 1) {
                    id = string(PURSUIT_ID_LEN * 2 - (req_str.str()).length(), '0');
                    id += req_str.str();
                    bin_id = hex_to_chararray(id);
                    nb_ba->publish_info(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
                } else if(rv_or_implicit == 0){
                    payload_size = sizeof(res_pings);
                    payload = (char *)malloc(payload_size);
                    snprintf(payload, payload_size+1, "%d", res_pings);
                    nb_ba->publish_data(bin_prefix_id, DOMAIN_LOCAL, NULL, 0, payload, payload_size);
                }
            } else {
                cout << "Ping Complete, end the App..." << endl;
            }
            break;
        default:
            break;
    }
    delete ev;
}

int main(int argc, char* argv[]) {
    int user_or_kernel;
    switch (argc) {
            case 3:{
                /*By Default I assume blackadder is running in user space*/
                nb_ba = NB_Blackadder::Instance(true);
                /*construct the nodeID from the second arg*/
                rv_or_implicit = 1;
                s_nodeID = argv[1];
                s_nodeID_len = s_nodeID.size();
                c_nodeID = argv[2];
                c_nodeID_len = c_nodeID.size();
                req_pings = 1000;
                break;
            }
            case 4:{
                /*By Default I assume blackadder is running in user space*/
                nb_ba = NB_Blackadder::Instance(true);
                /*construct the nodeID from the second arg*/
                rv_or_implicit = 1;
                s_nodeID = argv[1];
                s_nodeID_len = s_nodeID.size();
                c_nodeID = argv[2];
                c_nodeID_len = c_nodeID.size();
                req_pings = atoi(argv[3]);
                break;
            }
            case 5:{
                /*By Default I assume blackadder is running in user space*/
                nb_ba = NB_Blackadder::Instance(true);
                rv_or_implicit = atoi(argv[1]);
                s_nodeID = argv[2];
                s_nodeID_len = s_nodeID.size();
                c_nodeID = argv[3];
                c_nodeID_len = c_nodeID.size();
                req_pings = atoi(argv[4]);
                break;
            }
        case 6:{
            user_or_kernel = atoi(argv[1]);
            if (user_or_kernel == 0) {
                nb_ba = NB_Blackadder::Instance(true);
            } else {
                nb_ba = NB_Blackadder::Instance(false);
            }
            rv_or_implicit = atoi(argv[2]);
            s_nodeID = argv[3];
            s_nodeID_len = s_nodeID.size();
            c_nodeID = argv[4];
            c_nodeID_len = c_nodeID.size();
            req_pings = atoi(argv[5]);
            break;
        }
        default:
            cout << "Error: Wrong number of input arguments" << endl
            << "App usage: sudo ./ping_publisher <user_or_kernel> (Optional) 1|0 <rv_or_implicit> (Optional) 1|0 <pinged_node> NODE_NONZERO_DIGITS (Required) <publishing_node> NODE_NONZERO_DIGITS (Required) <number_of_pings> NO_PINGS (Optional)" << endl
            << "Defaults: user_or_kernel 1 rv_or_implicit 1 number_of_pings 1000" << endl
            << "E.g.: sudo ./ping_publisher 130 244 5" << endl;
            break;
    }
    /*Set the callback function*/
    nb_ba->setCallback(eventHandler);
    /***************************/
    cout << "Process ID: " << getpid() << endl;
    /*****************************publish root_scope /AAAAAAAAAAAAAAAA ************************************/
    id = string(PURSUIT_ID_LEN * 2, '0');
    prefix_id = string();
    bin_id = hex_to_chararray(id);
    bin_prefix_id = hex_to_chararray(prefix_id);
    cout << "Publish root scope /" << id << endl;
    nb_ba->publish_scope(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
    prefix_id=id;
    bin_prefix_id = hex_to_chararray(prefix_id);
    /*Pub(client) publishes scope root_scope/Server_nodeID, and subscribes to scope root_scope/Client_nodeID*/
    s_id = string(((PURSUIT_ID_LEN * 2) - s_nodeID_len), '0');
    s_id += s_nodeID;
    s_bin_id = hex_to_chararray(s_id);
    c_id = string(((PURSUIT_ID_LEN * 2) - c_nodeID_len), '0');
    c_id += c_nodeID;
    c_bin_id = hex_to_chararray(c_id);
    nb_ba->publish_scope(s_bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
    nb_ba->subscribe_scope(c_bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
    /*Pub(client) publishes ping request (as item) root_scope/serverID/clientID, and*/
    /*subscribes to ping responses (published by the Server under) root_socpe/clientID/serverID*/
    /*Logic of Pub Scopes: allow the server to identify pinging node */
    /*Logic of Sub Scopes: client recieves response from desired server only*/
    if (rv_or_implicit == 1) {
        /*rv_or_implicit = 1 -> Ping request published inside (SID/SID/SID/)-root_scope/serverID/clientID*/
        prefix_id = prefix_id + s_id;
        bin_prefix_id = hex_to_chararray(prefix_id);
        nb_ba->publish_scope(c_bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
        prefix_id = prefix_id + c_id;
        /*First ping index is zero*/
        id = "0000000000000000";
        bin_id = hex_to_chararray(id);
        bin_prefix_id = hex_to_chararray(prefix_id);
        nb_ba->publish_info(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
        prefix_id = string(PURSUIT_ID_LEN * 2, '0') + c_id;
        bin_prefix_id = hex_to_chararray(prefix_id);
        nb_ba->subscribe_scope(s_bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
    } else {
        /*rv_or_implicit = 0 -> Ping request published as (SID/SID/RID)-root_scope/serverID/clientID*/
        prefix_id = prefix_id + s_id;
        bin_prefix_id = hex_to_chararray(prefix_id);
        nb_ba->publish_info(c_bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
        prefix_id = prefix_id + c_id;
        prefix_id = string(PURSUIT_ID_LEN * 2, '0') + c_id;
        bin_prefix_id = hex_to_chararray(prefix_id);
        nb_ba->subscribe_info(s_bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
    }
    nb_ba->join();
    nb_ba->disconnect();
    delete nb_ba;
    cout << "exiting...." << endl;
    return 0;
}
