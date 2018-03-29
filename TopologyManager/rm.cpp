/*
 * Copyright (C) 2015-2018  Mays AL-Naday
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

#include <signal.h>
#include <arpa/inet.h>
#include <set>
#include <map>
#include <vector>
#include <blackadder.hpp>
#include "tm_igraph.hpp"
#include <sstream>
#include <iostream>
#include <boost/algorithm/string.hpp>


using namespace std;

Blackadder *ba = NULL;
TMIgraph *tm_igraph = NULL;
pthread_t _event_listener, *event_listener = NULL;
sig_atomic_t listening = 1;

/*Resiliency scope, used by the TM to notify the RM of a new/end of a delivery tree*/
string resl_id = string(PURSUIT_ID_LEN*2-1, 'F') + "0"; // "FF..FFFFFFFFFFFFF0";
string resl_prefix_id = string();
string resl_bin_id = hex_to_chararray(resl_id);
string resl_bin_prefix_id = hex_to_chararray(resl_prefix_id);
/*Resiliency subscope-scope, used by the TM to notify the RM of a new/end of a unicast delivery path, the ICN ID is FF...F0/FF...F3/RM_NODEID*/
string uc_resl_id = string(PURSUIT_ID_LEN*2-1, 'F') + "3"; // "FF..FFFFFFFFFFFFF3"
string uc_resl_bin_id = hex_to_chararray(uc_resl_id);

std::string resl_resp_id = string(PURSUIT_ID_LEN*2-1, 'F') + "1"; // "FF..FFFFFFFFFFFFF1"
std::string resl_resp_prefix_id = string();
std::string resl_resp_bin_id = hex_to_chararray(resl_resp_id);
std::string resl_resp_bin_prefix_id = hex_to_chararray(resl_resp_prefix_id);

std::string resl_req_id = string(PURSUIT_ID_LEN*2-1, 'F') + "2"; // "FF..FFFFFFFFFFFFF2"
std::string resl_req_prefix_id = string();
std::string resl_req_bin_id = hex_to_chararray(resl_req_id);
std::string resl_req_bin_prefix_id = hex_to_chararray(resl_req_prefix_id);

/*RM ICN IDs*/
string UpdatePathId = resl_bin_id;
string UpdateUnicastPathId = resl_bin_id + uc_resl_bin_id ;
string DiscoverFailureId = resl_req_bin_id ;

//The Resiliency functions for Native ICN
void handleMulticastPathNotification (char *request, int request_len){
    cout << "-------------------------------- Path Notification --------------------------------" << endl;
    string request_str = string (request, request_len);
    unsigned char no_ids;
    unsigned char IDlen;
    unsigned char no_paths;
    unsigned char path_len;
    unsigned char no_alt_pubs;
    int path_idx = 0;
    int alt_pub_idx = 0;
    string alt_publisher;
    set<string> alternative_publishers;
    string path_v;
    set<string> path_vecs;
    memcpy(&no_ids, request , sizeof(no_ids));
    memcpy(&IDlen, request + sizeof(no_ids), sizeof(IDlen));
    int ids_size= (int)IDlen * PURSUIT_ID_LEN + sizeof(no_ids) + sizeof(IDlen);
    char *ids = (char *) malloc (ids_size);
    memcpy(ids, request , ids_size);
    string icn_id = string((const char *)(ids) , ids_size);
    cout << "RM: new delivery of: " << chararray_to_hex(icn_id) << endl;
    /* Extract the list of Path Vectors */
    memcpy(&no_paths, request + ids_size, sizeof(no_paths));
    if((int)no_paths > 0){
        path_idx = ids_size + sizeof(no_paths);
        for(int i = 1; i <= (int)no_paths ; i++){
            memcpy(&path_len, request + path_idx, sizeof(path_len));
            path_idx += sizeof(path_len);
            path_v = string((const string&)request_str, path_idx , (int)path_len);
            /*print out the path vectors*/
            path_vecs.insert(path_v);
            path_idx += (int)path_len;
        }
    } else {
        cout << "RM: delivery finished of : " << chararray_to_hex(icn_id) << endl;
        path_idx = /*sizeof(request_type) +*/ ids_size + sizeof(no_paths);
    }
    /* Update the Global Data Structure of Path Vectors which are currently utilized to deliver Data (path_info) */
    if(tm_igraph->path_info.find(icn_id) == tm_igraph->path_info.end()){
        if((!path_vecs.empty())){
            tm_igraph->path_info.insert(make_pair(icn_id, path_vecs));
            path_vecs.clear();
        }
    }else{
        tm_igraph->path_info.erase(icn_id);
        if((!path_vecs.empty()))
        tm_igraph->path_info.insert(make_pair(icn_id, path_vecs));
    }
    /*Extract the list of Alternative Publishers*/
    memcpy(&no_alt_pubs, request + path_idx, sizeof(no_alt_pubs));
    alt_pub_idx += sizeof(no_alt_pubs);
    for (int j = 0 ; j < (int) no_alt_pubs ; j ++){
        alt_publisher = string(request + path_idx + alt_pub_idx, PURSUIT_ID_LEN);
        alternative_publishers.insert(alt_publisher);
        alt_pub_idx += PURSUIT_ID_LEN;
    }
    /*Update the Global data structure of Alternative Publishers (ex_pub)*/
    if(tm_igraph->ex_pubs.find(icn_id) != tm_igraph->ex_pubs.end())
    tm_igraph->ex_pubs.erase(icn_id);
    tm_igraph->ex_pubs.insert(pair<string, set<string> >(icn_id, alternative_publishers));
    free(ids);
    cout << "-------------------------------- EoN --------------------------------" << endl;
}
//The Resiliency functions for *-over-ICN
void handleUnicastPathNotification (char *request, int request_len){
    cout << "-------------------------------- Path Notification --------------------------------" << endl;
    unsigned char no_ids;
    unsigned char IDlen;
    unsigned char no_paths;
    unsigned char path_len;
    unsigned char no_alt_subs;
    int path_idx = 0;
    int alt_sub_idx = 0;
    int ids_size = 0;
    string publisher;
    string alt_subscriber;
    string path_v;
    set<string> alternative_subscribers;
    pair<string, string> publisher_path = make_pair(string(), string());
    memcpy(&no_ids, request , sizeof(no_ids));
    memcpy(&IDlen, request + sizeof(no_ids), sizeof(IDlen));
    ids_size= (int)IDlen * PURSUIT_ID_LEN + sizeof(no_ids) + sizeof(IDlen);
    char *ids = (char *) malloc (ids_size);
    memcpy(ids, request , ids_size);
    string icn_id = string((const char *)(ids) , ids_size);
    cout << "RM: new delivery of: " << chararray_to_hex(icn_id) << endl;
    publisher = string(request + ids_size, NODEID_LEN);
    /* Extract the list of Path Vectors */
    memcpy(&no_paths, request + ids_size + NODEID_LEN, sizeof(no_paths));
    if((int)no_paths > 0){
        /*there is a delivery path for the publisher*/
        path_idx = ids_size + NODEID_LEN + sizeof(no_paths);
        memcpy(&path_len, request + path_idx, sizeof (path_len));
        path_idx += sizeof (path_len);
        path_v = string(request + path_idx, (int) path_len);
        path_idx += (int) path_len;
        publisher_path = make_pair(publisher, path_v);
    } else {
        /*delivery finished for this publisher*/
        cout << "RM: delivery finished of : " << chararray_to_hex(icn_id) << endl;
        path_idx = ids_size + NODEID_LEN + sizeof(no_paths);
    }
    /* Update the Global Data Structure of Path Vectors which are currently utilized to deliver Data (path_info) */
    if(tm_igraph->unicast_path_info.find(icn_id) == tm_igraph->unicast_path_info.end()){
        if (publisher_path.first != "") {
            tm_igraph->unicast_path_info[icn_id][publisher] = path_v;
        }
    }
    else{
        tm_igraph->unicast_path_info[icn_id].erase(publisher);
        if(publisher_path.first != "") {
            tm_igraph->unicast_path_info[icn_id][publisher] = path_v;
        }
    }
    /*Extract the list of Alternative Subscribers*/
    memcpy(&no_alt_subs, request + path_idx, sizeof(no_alt_subs));
    alt_sub_idx += sizeof(no_alt_subs);
    for (int j = 0 ; j < (int) no_alt_subs ; j++){
        alt_subscriber = string(request + path_idx + alt_sub_idx, NODEID_LEN);
        alternative_subscribers.insert(alt_subscriber);
        alt_sub_idx += NODEID_LEN;
    }
    /*Update the Global data structure of Alternative Publishers (ex_pub)*/
    if(tm_igraph->ex_subs.find(icn_id) != tm_igraph->ex_subs.end())
    tm_igraph->ex_subs.erase(icn_id);
    tm_igraph->ex_subs.insert(pair<string, set<string> >(icn_id, alternative_subscribers));
    free(ids);
    cout << "-------------------------------- EoN --------------------------------" << endl;
}
void requestMulticastTree(set<string>& ids, map<string, set<string> >& publishers, map<string, set<string> >& subscribers) {
    //	cout << "-------------------------------- Request Multicast Tree --------------------------------" << endl;
    unsigned int offset =0;
    unsigned char type;
    unsigned char strategy;
    unsigned char no_publishers;
    unsigned char no_subscribers;
    map<string, set<string> >::iterator alternatives;
    string notification_id = resl_resp_bin_id + tm_igraph->getNodeID();
    Bitvector *FID_to_tm = tm_igraph->calculateFID(tm_igraph->getRMNodeID(), tm_igraph->getNodeID());
    for (set<string>::iterator it = ids.begin(); it != ids.end(); it++) {
        alternatives = tm_igraph->ex_pubs.find((*it));
        publishers[(*it)].insert(alternatives->second.begin(), alternatives->second.end());
        /**
         *construct a Re-MATCH_PUB_SUBS packet that is to be sent to the TM for the broken paths
         */
        type = MATCH_PUB_SUBS;
        strategy = IMPLICIT_RENDEZVOUS;//strategy at this point is irrelevant to the communication between TM and TM_RV_BRK, however it has been considered to fill in the corresponding field in the packet and it may be used later on
        no_publishers = publishers[(*it)].size();
        no_subscribers = subscribers[(*it)].size();
        int notification_size = sizeof (type) + sizeof (strategy) + sizeof (no_publishers) + (int)no_publishers * NODEID_LEN + sizeof (no_subscribers) + (int)no_subscribers * NODEID_LEN + (*it).length();
        char * notification = (char *) malloc (notification_size);
        /*create the payload*/
        memcpy(notification, &type, sizeof (type));
        offset += sizeof (type);
        memcpy(notification + offset, &strategy, sizeof (strategy));
        offset += sizeof (strategy);
        memcpy(notification + offset, &no_publishers, sizeof (no_publishers));
        offset += sizeof (no_publishers);
        /*fill in the publishers*/
        for (set<string>::iterator pub = publishers[(*it)].begin(); pub != publishers[(*it)].end(); pub++) {
            memcpy(notification + offset, (*pub).c_str(), NODEID_LEN);
            offset += NODEID_LEN;
        }
        memcpy(notification + offset, &no_subscribers, sizeof (no_subscribers));
        offset += sizeof (no_subscribers);
        /*fill in the subscribers*/
        for (set<string>::iterator sub = subscribers[(*it)].begin(); sub != subscribers[(*it)].end(); sub++) {
            memcpy(notification + offset, (*sub).c_str(), NODEID_LEN);
            offset += NODEID_LEN;
        }
        /*fill in the ICN ID*/
        memcpy(notification + offset, (*it).c_str(), (*it).length());
        ba->publish_data(notification_id, IMPLICIT_RENDEZVOUS, (char *) FID_to_tm->_data, FID_LEN, notification, notification_size);
        free(notification);
    }
    delete FID_to_tm;
    //	cout << "-------------------------------- EoR --------------------------------" << endl;
}
void requestUnicastPaths(set<string>& ids, map<string, set<string> >& publishers, map<string, set<string> >& subscribers) {
    //	cout << "-------------------------------- Request Unicast Path(s) --------------------------------" << endl;
    unsigned int offset = 0;
    unsigned char type;
    unsigned char strategy;
    unsigned char no_publishers = 1;
    unsigned char no_subscribers;
    map<string, set<string> >::iterator alternatives;
    string notification_id = resl_resp_bin_id + uc_resl_bin_id + tm_igraph->getNodeID();
    Bitvector *FID_to_tm = tm_igraph->calculateFID(tm_igraph->getRMNodeID(), tm_igraph->getNodeID());
    for (set<string>::iterator it = ids.begin(); it != ids.end(); it++) {
        alternatives = tm_igraph->ex_subs.find((*it));
        subscribers[(*it)].insert(alternatives->second.begin(), alternatives->second.end());
        /**
         *construct a Re-MATCH_PUB_SUBS requests for each publisher that is to be sent to the TM
         */
        type = MATCH_PUB_SUBS;
        strategy = IMPLICIT_RENDEZVOUS;//strategy at this point is irrelevant to the communication between TM and TM_RV_BRK, however it has been considered to fill in the corresponding field in the packet and it may be used later on
        no_subscribers = subscribers[(*it)].size();
        int notification_size = sizeof (type) + sizeof (strategy) + sizeof (no_publishers) /* = 1 for unicast*/ + NODEID_LEN /*publisher*/ + sizeof (no_subscribers) + (int)no_subscribers * NODEID_LEN + (*it).length();
        for (set<string>::iterator pub_it = publishers[(*it)].begin(); pub_it != publishers[(*it)].end(); pub_it++) {
            offset = 0;
            char * notification = (char *) malloc (notification_size);
            /*create the payload*/
            memcpy(notification, &type, sizeof (type));
            offset += sizeof(type);
            memcpy(notification + offset, &strategy, sizeof (strategy));
            offset +=sizeof (strategy);
            memcpy(notification + offset, &no_publishers, sizeof (no_publishers));
            offset += sizeof (no_publishers);
            memcpy(notification + offset, (*pub_it).c_str(), NODEID_LEN);
            offset += NODEID_LEN;
            memcpy(notification + offset, &no_subscribers, sizeof(no_subscribers));
            offset += sizeof (no_subscribers);
            /*fill in the subscribers*/
            for (set<string>::iterator sub_it = subscribers[(*it)].begin(); sub_it != subscribers[(*it)].end(); sub_it++) {
                memcpy(notification + offset, (*sub_it).c_str(), NODEID_LEN);
                offset += NODEID_LEN;
            }
            /*fill in the ICN ID*/
            memcpy(notification + offset, (*it).c_str(), (*it).length());
            cout << "RM: request a unicast recovery path for :" << chararray_to_hex((*it)) << endl;
            ba->publish_data(notification_id, IMPLICIT_RENDEZVOUS, (char *) FID_to_tm->_data, FID_LEN, notification, notification_size);
            free(notification);
        }
    }
    delete FID_to_tm;
    //	cout << "-------------------------------- EoR --------------------------------" << endl;
}
void handleLinkStateNotification(char * request, int request_len) {
    cout << "-------------------------------- Link State Notification --------------------------------" << endl;
    int field_offset = 0;
    unsigned char net_type;
    const string path_delim = "->";
    string icn_id;
    string ip_icn_id;
    string lsn_publisher;
    string affected_node;
    string affected_pub;
    string affected_sub;
    string path;
    set<string> affected_ICNids;
    set<string> affected_UnicastICNids;
    vector<string> path_nodes;
    vector<string>::iterator publisher_it;
    vector<string>::iterator affected_it;
    map<string, set<string> > publishers;
    map<string, set<string> > subscribers;
    map<string, set<string> >::iterator info_selector;
    map<string, map<string, string> >::iterator unicast_info_selector;
    memcpy(&net_type, request + field_offset, sizeof (net_type));
    field_offset += sizeof (net_type);
    lsn_publisher = string(request, field_offset, NODEID_LEN);
    field_offset += NODEID_LEN;
    affected_node = string(request, field_offset, NODEID_LEN);
    field_offset += NODEID_LEN;
    /*Search the space of native ICN deliveries*/
    for (map<string, set<string> >::iterator icn_it = tm_igraph->path_info.begin(); icn_it != tm_igraph->path_info.end(); icn_it++) {
        icn_id = (*icn_it).first;
        for (set<string>::iterator path_it = icn_it->second.begin(); path_it != icn_it->second.end(); path_it++) {
            path = (* path_it);
            boost::split(path_nodes, path, boost::is_any_of(path_delim));
            publisher_it = find(path_nodes.begin(), path_nodes.end(), lsn_publisher);
            affected_it = find(path_nodes.begin(), path_nodes.end(), affected_node);
            affected_pub = path_nodes.front();
            affected_sub = path_nodes.back();
            publishers[icn_id].insert(affected_pub);
            subscribers[icn_id].insert(affected_sub);
            if ((publisher_it != path_nodes.end()) && (affected_it != path_nodes.end())) {
                affected_ICNids.insert(icn_id);
                cout << "RM: Information " << chararray_to_hex(icn_id) <<" on Path " << path <<  " Has Been Cut Between: " << lsn_publisher << " and " << affected_node << endl;
            }
        }
    }
    path_nodes.clear();
    /*Search the space of *-over-ICN deliveries*/
    for (map<string, map<string, string> >::iterator ip_it = tm_igraph->unicast_path_info.begin(); ip_it != tm_igraph->unicast_path_info.end(); ip_it++) {
        ip_icn_id = (*ip_it).first;
        for (map<string, string>::iterator path_it = ip_it->second.begin(); path_it != ip_it->second.end(); path_it++) {
            affected_pub = (*path_it).first;
            path = (*path_it).second;
            boost::split(path_nodes, path, boost::is_any_of(path_delim));
            publisher_it = find(path_nodes.begin(), path_nodes.end(), lsn_publisher);
            affected_it = find(path_nodes.begin(), path_nodes.end(), affected_node);
            affected_sub = path_nodes.back();
            publishers[ip_icn_id].insert(affected_pub);
            subscribers[ip_icn_id].insert(affected_sub);
            if ((publisher_it != path_nodes.end()) && (affected_it != path_nodes.end())) {
                affected_UnicastICNids.insert(ip_icn_id);
                cout << "RM: *-over-ICN Information " << chararray_to_hex(icn_id) <<" on Path " << path <<  " Has Been Cut Between: " << lsn_publisher << " and " << affected_node << endl;
            }
        }
    }
    if (!affected_ICNids.empty()) {
        requestMulticastTree(affected_ICNids, publishers, subscribers);
    } else {
        cout << "RM: no broken deliveries of native ICN information" << endl;
    }
    if (!affected_UnicastICNids.empty()) {
        requestUnicastPaths(affected_UnicastICNids, publishers, subscribers);
    } else {
        cout << "RM: no broken deliveries of *-over-ICN information" << endl;
    }
    cout << "-------------------------------- EoN --------------------------------" << endl;
}

void DispathToHandler(const string &id, const char * request, int request_len){
    unsigned char request_type;
    unsigned char payload_len;
    memcpy(&request_type, request, sizeof (request_type));
    payload_len = request_len - sizeof (request_type);
    char * payload = (char *) malloc (payload_len);
    memcpy(payload, request + sizeof (request_type), payload_len);
    cout << "RM request type:" << (int)request_type << endl;
    if (id == UpdatePathId) {
        switch (request_type) {
                case UPDATE_DELIVERY:
                handleMulticastPathNotification(payload, payload_len);
                break;
            default:
                cout << "RM: Unknow request: " << (int)request_type << endl;
                break;
        }
    }
    else if (id == UpdateUnicastPathId) {
        cout << "RM: unicast delivery" << endl;
        switch (request_type) {
                case UPDATE_DELIVERY:
                handleUnicastPathNotification(payload, payload_len);
                break;
            default:
                cout << "RM: Unknow unicast request: " << (int)request_type << endl;
                break;
        }
    }
    else if (id == DiscoverFailureId) {
        switch (request_type) {
                case DISCOVER_FAILURE:
                handleLinkStateNotification(payload, payload_len);
                break;
            default:
                cout << "RM: Unknow request: " << (int)request_type << endl;
                break;
        }
    }
}

void *event_listener_loop(void *arg) {
    Blackadder *ba = (Blackadder *) arg;
    while (listening) {
        Event ev;
        ba->getEvent(ev);
        if (ev.type == PUBLISHED_DATA) {
            string prefix_id = string(ev.id, 0, ev.id.length() - PURSUIT_ID_LEN);
            DispathToHandler(prefix_id, (char *) ev.data, ev.data_len);
        }
        else if (ev.type == UNDEF_EVENT && !listening) {
            cout << "RM: final event" << endl;
        }
    }
    return NULL;
}

void sigfun(int sig) {
    listening = 0;
    if (event_listener)
    pthread_cancel(*event_listener);
    (void) signal(SIGINT, SIG_DFL);
}

int main(int argc, char* argv[]) {
    (void) signal(SIGINT, sigfun);
    cout << "RM: starting - process ID: " << getpid() << endl;
    if (argc < 2) {
        cout << "RM: the topology file is missing" << endl;
        exit(0);
    }
    tm_igraph = new TMIgraph();
    /*read the graphML file that describes the topology*/
    if (tm_igraph->readTopology(argv[1]) < 0) {
        cout << "RM: couldn't read topology file...aborting" << endl;
        exit(0);
    }
    cout << "RM Node: " << tm_igraph->RMnodeID << endl;
    /***************************************************/
    if (tm_igraph->mode.compare("kernel") == 0) {
        ba = Blackadder::Instance(false);
    } else {
        ba = Blackadder::Instance(true);
    }
    pthread_create(&_event_listener, NULL, event_listener_loop, (void *) ba);
    event_listener = &_event_listener;
    ba->subscribe_scope(resl_bin_id, resl_bin_prefix_id, IMPLICIT_RENDEZVOUS, NULL, 0);
    ba->subscribe_scope(resl_req_bin_id, resl_req_bin_prefix_id, IMPLICIT_RENDEZVOUS, NULL, 0);
    /*subscribe to the unicast subscope of resilience*/
    ba->subscribe_scope(uc_resl_bin_id, resl_bin_id, IMPLICIT_RENDEZVOUS, NULL, 0);
    ba->subscribe_scope(uc_resl_bin_id, resl_req_bin_id, IMPLICIT_RENDEZVOUS, NULL, 0);
    
    pthread_join(*event_listener, NULL);
    cout << "RM: disconnecting" << endl;
    ba->disconnect();
    delete ba;
    delete tm_igraph;
    cout << "RM: exiting" << endl;
    return 0;
}
