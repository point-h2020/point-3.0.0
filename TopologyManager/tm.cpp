/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
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

#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <set>
#include <unordered_map>
#include <blackadder.hpp>
#include "tm_graph.hpp"
#include "tm_igraph.hpp"
#include "te_graph_mf.hpp"
/*QoS Stuff*/
#include <qos_structs.hpp>
#include <lsmpacket.hpp>
#include "tm_qos.hpp"
#include <metadatapacket.hpp>

/**@breif avoid broadcast notifications, which suffer high loss
 *\TODO: different mechansim for avoiding loss of notifications
 *
 */
#define UNICAST_NOTIFICATION
#ifdef UNICAST_NOTIFICATION
bool uc_notification = true;
#else
bool uc_notification = false;
#endif

using namespace std;
int start = 0;
extern pthread_t _te_thread, *te_thread;

Blackadder *ba = NULL;
Moly *moly = NULL;
TMgraph *tm_igraph = NULL;
pthread_t _event_listener, *event_listener = NULL;
pthread_t _moly_event_listener, *moly_event_listener = NULL;
sig_atomic_t listening = 1;
unordered_map<int, int> moly_tm_requests;

root_scope_t map_root_scope(uint64_t s);

std::string req_id = string(PURSUIT_ID_LEN*2-1, 'F') + "E"; // "FF..FFFFFFFFFFFFFE"
std::string req_prefix_id = string();
std::string req_bin_id = hex_to_chararray(req_id);
std::string req_bin_prefix_id = hex_to_chararray(req_prefix_id);

std::string uc_req_id = string(PURSUIT_ID_LEN*2-1, 'F') + "D"; // "FF..FFFFFFFFFFFFFD"
std::string uc_req_bin_id = hex_to_chararray(uc_req_id);


std::string resp_id = string();
std::string resp_prefix_id = string(PURSUIT_ID_LEN*2-1, 'F') + "D"; // "FF..FFFFFFFFFFFFFD"
std::string resp_bin_id = hex_to_chararray(resp_id);
std::string resp_bin_prefix_id = hex_to_chararray(resp_prefix_id);

std::string resl_id = string(PURSUIT_ID_LEN*2-1, 'F') + "0"; // "FF..FFFFFFFFFFFFF0";
std::string resl_prefix_id = string();
std::string resl_bin_id = hex_to_chararray(resl_id);
std::string resl_bin_prefix_id = hex_to_chararray(resl_prefix_id);

std::string resl_resp_id = string(PURSUIT_ID_LEN*2-1, 'F') + "1"; // "FF..FFFFFFFFFFFFF1"
std::string resl_resp_prefix_id = string();
std::string resl_resp_bin_id = hex_to_chararray(resl_resp_id);
std::string resl_resp_bin_prefix_id = hex_to_chararray(resl_resp_prefix_id);

std::string resl_req_id = string(PURSUIT_ID_LEN*2-1, 'F') + "2"; // "FF..FFFFFFFFFFFFF2"
std::string resl_req_bin_id = hex_to_chararray(resl_req_id);

/*Resiliency subscope-scope, used by the TM to notify the RM of a new/end of a unicast delivery path, the ICN ID is FF...F0/FF...F3/RM_NODEID*/
string uc_resl_id = string(PURSUIT_ID_LEN*2-1, 'F') + "3"; // "FF..FFFFFFFFFFFFF3"
string uc_resl_bin_id = hex_to_chararray(uc_resl_id);

/* Commented on 01-09-2016 to be replaced with known scopes from blackadder_enum.hpp
 *
string lsn_id = string(PURSUIT_ID_LEN*2-1, 'F') + "A"; // "FF..FFFFFFFFFFFFFA"
string lsn_prefix_id = string();
string lsn_bin_id = hex_to_chararray(lsn_id);
string lsn_bin_prefix_id = hex_to_chararray(lsn_prefix_id); 
 */
string lsn_bin_id = string(PURSUIT_ID_LEN -1, SUFFIX_NOTIFICATION) + (char) LINK_STATE_NOTIFICATION; /*!< SID used to publish Link-State Notifications (LSN) the TM*/
string lsn_bin_prefix_id = string();

/*Link state monitoring scope (Status Updates - QoS)*/
std::string lsm_scope = string(PURSUIT_ID_LEN*2-1, 'F') + "B"; // "FF..FFFFFFFFFFFFFB"
std::string lsm_bin_scope = hex_to_chararray(lsm_scope);

/*Path Management notification Scope - this should be consolidated later on with the resl_id as they could both fall under the same data structure*/
string pathMgmt_id = string(PURSUIT_ID_LEN * 2 - 1, 'F') + "9";
string pathMgmt_bin_id = hex_to_chararray(pathMgmt_id);
string pathMgmt_bin_full_id = pathMgmt_bin_id + pathMgmt_bin_id;

// Global II metadata cache
IIMetaCache metaCache;

/*TM - RM Notification IDs, append tm_igraph->getRMNodeID() befor publishing data using the ID*/
string UpdatePathId = resl_bin_id ;
string UpdateUnicastPathId = resl_bin_id + uc_resl_bin_id ;
string DiscoverFailureId = resl_req_bin_id ;
string DiscoverUnicastFailureId = resl_req_bin_id + uc_resl_bin_id ;
string	UnicastDeliveryId = req_bin_id + uc_req_bin_id ;
string	RecoverUnicastDeliveryId = resl_resp_bin_id + uc_resl_bin_id ;

void handleIIMetaData(char *request, int request_len){
    cout<<"Got Meta Data!!!"<<endl;
    MetaDataPacket pkt((uint8_t *)request, request_len);
    metaCache.update(pkt.getID_RAW(), pkt.getIIStatus(), ba);
    
    // TODO: We have to re-route this II
}

void handleMulticastPathRequest(char *request, int request_len) {
    cout<<"---------------- REQUEST --------------------"<<endl;
    unsigned char request_type;
    unsigned char strategy;
    unsigned char no_publishers;
    unsigned char no_alt_publishers;
    unsigned char no_subscribers;
    unsigned char no_pathvectors;
    unsigned char no_ids;
    unsigned char IDLen;
    unsigned char response_type;
    int idx = 0;
    string pathvectors_Field = string();
    string alt_publishers;
    string nodeID;
    set<string> publishers;
    set<string> subscribers;
    set<string>::iterator subscriber_it;
    map<string, Bitvector *> result = map<string, Bitvector *>();
    map<string, Bitvector *>::iterator map_iter;
    map<string, set<string> > pathvectors;
    map<string, set<string> >::iterator tmp_map_it;
    
    memcpy(&request_type, request, sizeof (request_type));
    memcpy(&strategy, request + sizeof (request_type), sizeof (strategy));
    // Early break for II metadata
    if (request_type==QoS_METADATA){
        handleIIMetaData(request, request_len);
        return;
    }
    if (request_type == MATCH_PUB_SUBS || request_type == UPDATE_FID) {
        if(request_type == MATCH_PUB_SUBS) {
            cout<<"handleRequest: MATCH_PUB_SUBS\n";
        }
        if(request_type == UPDATE_FID) {
            cout<<"handleRequest: UPDATE_FID\n";
        }
        no_alt_publishers = 0;
        no_pathvectors = 0;
        /*this a request for topology formation*/
        memcpy(&no_publishers, request + sizeof (request_type) + sizeof (strategy), sizeof (no_publishers));
        cout << "Publishers: ";
        for (int i = 0; i < (int) no_publishers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (strategy) + sizeof (no_publishers) + idx, PURSUIT_ID_LEN);
            cout << nodeID << " ";
            idx += PURSUIT_ID_LEN;
            publishers.insert(nodeID);
        }
        cout << endl;
        cout << "Subscribers: ";
        memcpy(&no_subscribers, request + sizeof (request_type) + sizeof (strategy) + sizeof (no_publishers) + idx, sizeof (no_subscribers));
        for (int i = 0; i < (int) no_subscribers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (strategy) + sizeof (no_publishers) + sizeof (no_subscribers) + idx, PURSUIT_ID_LEN);
            cout << nodeID << " ";
            idx += PURSUIT_ID_LEN;
            subscribers.insert(nodeID);
        }
        cout << endl;
        // MOVED ON TOP OF CALC + BUG FIX??
        // extract the Information IDs for this MATCH_PUB_SUBS request...
        int ids_size= request_len- sizeof(request_type) - sizeof(strategy) - sizeof(no_publishers) - (int)no_publishers * PURSUIT_ID_LEN - sizeof(no_subscribers) - (int)no_subscribers * PURSUIT_ID_LEN ;
        char *ids =(char *) malloc (ids_size);
        memcpy(&no_ids, request + sizeof(request_type) + sizeof(strategy) + sizeof(no_publishers) + (int)no_publishers * PURSUIT_ID_LEN + sizeof(no_subscribers) + (int)no_subscribers * PURSUIT_ID_LEN , sizeof(no_ids));
        memcpy(&IDLen, request + sizeof(request_type) + sizeof(strategy) + sizeof(no_publishers) + (int)no_publishers * PURSUIT_ID_LEN + sizeof(no_subscribers) + (int)no_subscribers * PURSUIT_ID_LEN + sizeof(no_ids), sizeof(IDLen));
        memcpy (ids, request + sizeof(request_type) + sizeof(strategy) + sizeof(no_publishers) + (int)no_publishers * PURSUIT_ID_LEN + sizeof(no_subscribers) + (int)no_subscribers * PURSUIT_ID_LEN + sizeof(no_ids) + sizeof(IDLen), ids_size);
        string ids_str = string((const char *)(ids) , (int)IDLen * PURSUIT_ID_LEN);

	// Keep track of the requests per namespace and report the count for current one to Moly
	string tmp_str = chararray_to_hex(ids_str);
	root_scope_t root_scope = map_root_scope(strtoul(tmp_str.substr(0, 16).c_str(), NULL, 16));
	moly_tm_requests[root_scope] += 1;
	printf("Sending %d requests in %X to Moly.\n", moly_tm_requests[root_scope], root_scope);
	path_calculations_namespace_t pathCalculations;
	pathCalculations.push_back(pair<root_scope_t, subscribers_t>(root_scope, moly_tm_requests[root_scope]));
	moly->Process::pathCalculations(pathCalculations);

        if (tm_igraph->getExten(QOS) && tm_igraph->isQoSMapOk()) {
            // before doing anything... check that we do not need to subscribe to the
            // MetaData of that item...
            cout<<"Request is for II="<<chararray_to_hex(ids_str)<<endl;
            bool added = metaCache.sendQueryIfNeeded(ids_str, ba);
            // Get the priority
            int prio = DEFAULT_QOS_PRIO;
            // Try to avoid messing with the cache while quering
            // IT IS NOT THREAD-SAFE!
            if (!added)
            prio = metaCache.getIIQoSPrio(ids_str, ba);
            
            // Get the available network class (ie. map II priority 98
            // to net 95 for a net that supports 0,95 and 99)
            uint16_t netprio = tm_igraph->getWeightKeyForIIPrio(prio);
            cout<<"TM: QoS: II Priority: "<<prio;
            cout<<"Mappend on NET Priority Class: "<<netprio<<endl;
            tm_igraph->calculateFID_weighted(publishers, subscribers, result, pathvectors, &tm_igraph->qlwm[netprio]);
        }
        
        else{
            // Core calc. here...
            tm_igraph->calculateFID(publishers, subscribers, result, pathvectors);
        }
        /*notify publishers*/
        for (map_iter = result.begin(); map_iter != result.end(); map_iter++) {
            if ((*map_iter).second == NULL) {
                cout << "Publisher " << (*map_iter).first << ", FID: NULL" << endl;
                response_type = STOP_PUBLISH;
                int response_size = request_len - sizeof(strategy) - sizeof (no_publishers) - no_publishers * PURSUIT_ID_LEN - sizeof (no_subscribers) - no_subscribers * PURSUIT_ID_LEN;
                char *response = (char *) malloc(response_size);
                memcpy(response, &response_type, sizeof (response_type));
                int ids_index = sizeof (request_type) + sizeof (strategy) + sizeof (no_publishers) + no_publishers * PURSUIT_ID_LEN + sizeof (no_subscribers) + no_subscribers * PURSUIT_ID_LEN;
                memcpy(response + sizeof (response_type), request + ids_index, request_len - ids_index);
                /*get the FID to the publisher*/
                string destination = (*map_iter).first;
                string response_id = resp_bin_prefix_id + (*map_iter).first;
                ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(destination)->_data, FID_LEN, response, response_size);
                /*When resiliency support is in effect, construct a string of the alternative publishers, which will be sent to the TM_RV_BRK*/
                if(tm_igraph->getExten(RS)){
                    (int)no_alt_publishers++;
                    alt_publishers.append((*map_iter).first);
                }
                free(response);
            } else {
                cout << "Publisher " << (*map_iter).first << ", FID: " << (*map_iter).second->to_string() << endl;
                if (request_type == UPDATE_FID ) {
                    response_type = UPDATE_FID;
                } else {
                    response_type = START_PUBLISH;
                }
                int response_size = request_len - sizeof(strategy) - sizeof (no_publishers) - no_publishers * PURSUIT_ID_LEN - sizeof (no_subscribers) - no_subscribers * PURSUIT_ID_LEN + FID_LEN;
                char *response = (char *) malloc(response_size);
                memcpy(response, &response_type, sizeof (response_type));
                int ids_index = sizeof (request_type) + sizeof (strategy) + sizeof (no_publishers) + no_publishers * PURSUIT_ID_LEN + sizeof (no_subscribers) + no_subscribers * PURSUIT_ID_LEN;
                memcpy(response + sizeof (response_type), request + ids_index, request_len - ids_index);
                memcpy(response + sizeof (response_type) + request_len - ids_index, (*map_iter).second->_data, FID_LEN);
                /*find the FID to the publisher*/
                string destination = (*map_iter).first;
                string response_id = resp_bin_prefix_id + (*map_iter).first;
                ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(destination)->_data, FID_LEN, response, response_size);
                delete (*map_iter).second;
                free(response);
                /*When resiliency support is in effect, construct a string of the path vectors, which will be sent to the TM_RV_BRK*/
                if(tm_igraph->getExten(RS)){
                    tmp_map_it = pathvectors.find((*map_iter).first);
                    for(set<string>::iterator path_it = tmp_map_it->second.begin(); path_it != tmp_map_it->second.end() ; path_it++){
                        unsigned char pathvector_Size;
                        pathvector_Size = (*path_it).size();
                        pathvectors_Field += pathvector_Size;
                        pathvectors_Field += (*path_it);
                        (int)no_pathvectors++;
                    }
                }
            }
        }
        /*If resiliency support is in effect, create a notification of this delivery update and publish it to the TM_RV_BRK*/
        if(tm_igraph->getExten(RS)){
            response_type = UPDATE_DELIVERY;
            int pathvector_FieldSize = 0;
            if(no_pathvectors > 0)
            pathvector_FieldSize = pathvectors_Field.size();
            int update_response_size = sizeof(response_type) + sizeof(no_ids) + sizeof(IDLen) + ((int)no_ids * (int)IDLen * PURSUIT_ID_LEN) + sizeof(no_pathvectors) + pathvector_FieldSize + sizeof(no_alt_publishers) + (int)no_alt_publishers * NODEID_LEN;
            char * response = (char *) malloc(update_response_size);
            memcpy(response, &response_type, sizeof(response_type));
            memcpy(response + sizeof(response_type), &no_ids, sizeof(no_ids));
            memcpy(response + sizeof(response_type) + sizeof(no_ids), &IDLen, sizeof(IDLen));
            memcpy(response + sizeof(response_type) + sizeof(no_ids) + sizeof(IDLen), ids , ids_size);
            memcpy(response + sizeof(response_type) + sizeof(no_ids) + sizeof(IDLen) + (int)IDLen * PURSUIT_ID_LEN, &no_pathvectors, sizeof(no_pathvectors));
            memcpy(response + sizeof(response_type) + sizeof(no_ids) + sizeof(IDLen) + (int)IDLen * PURSUIT_ID_LEN + sizeof(no_pathvectors) , (char *)pathvectors_Field.c_str() , pathvector_FieldSize);
            memcpy(response + sizeof(response_type) + sizeof(no_ids) + sizeof(IDLen) + (int)IDLen * PURSUIT_ID_LEN + sizeof(no_pathvectors) + pathvector_FieldSize , &no_alt_publishers , sizeof(no_alt_publishers));
            memcpy(response + sizeof(response_type) + sizeof(no_ids) + sizeof(IDLen) + (int)IDLen * PURSUIT_ID_LEN + sizeof(no_pathvectors) + pathvector_FieldSize  + sizeof(no_alt_publishers) , (char *)alt_publishers.c_str() , ((int)no_alt_publishers * NODEID_LEN));
            string response_id = UpdatePathId + tm_igraph->getRMNodeID();
            ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(tm_igraph->RMnodeID)->_data, FID_LEN,response, update_response_size);
            free(response);
        }
        free(ids);
        //        }
        // Use QoS map if: (a) enabled and (b) is initialised
    }
    else if ((request_type == SCOPE_PUBLISHED) || (request_type == SCOPE_UNPUBLISHED)) {
        /*this a request to notify subscribers about a new scope*/
        memcpy(&no_subscribers, request + sizeof (request_type) + sizeof (strategy), sizeof (no_subscribers));
//		cout << "TM: ICN SCOPE_PUBLISHED Notification, Subscribers: " << (int) no_subscribers << endl;
        for (int i = 0; i < (int) no_subscribers; i++) {
            nodeID = string(request + sizeof (request_type) + sizeof (strategy) + sizeof (no_subscribers) + idx, PURSUIT_ID_LEN);
			cout << nodeID << endl;
            int response_size = request_len - sizeof(strategy) - sizeof (no_subscribers) - no_subscribers * PURSUIT_ID_LEN + FID_LEN;
            int ids_index = sizeof (request_type) + sizeof (strategy) + sizeof (no_subscribers) + no_subscribers * PURSUIT_ID_LEN;
            char *response = (char *) malloc(response_size);
            string response_id = resp_bin_prefix_id + nodeID;
            memcpy(response, &request_type, sizeof (request_type));
            memcpy(response + sizeof (request_type), request + ids_index, request_len - ids_index);
            ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, tm_igraph->getTM_to_nodeFID(nodeID)->_data, FID_LEN, response, response_size);
            idx += PURSUIT_ID_LEN;
            free(response);
        }
    } else {
        cout<<"UNKNOWN operation!\n";
    }
    cout<<"---------------- EoR --------------------\n"<<endl;
}

void handleUnicastPathRequest (char *request, int request_len)
{
    cout << "---------------- *-over-ICN REQUEST --------------------" << endl;
    /*variable declarations*/
    unsigned char request_type;
    unsigned char no_publishers;
    unsigned char no_subscribers;
    unsigned char no_ids;
    unsigned char IDLen;
    unsigned char response_type;
    unsigned char strategy;
    unsigned char no_alternative_subscribers = 0;
    unsigned char path_len = 0;
    unsigned char no_paths = 0;
    string nodeID;
    string publisher;
    string response_id;
    string alternative_subscribers = string();
    string path = string();
    set<string> subscribers;
    map<string, Bitvector *> result = map<string, Bitvector *>();
    map<string, Bitvector *>::iterator map_iter;
    map<string, string> paths;
    unsigned int idx = 0;
    unsigned int ids_size;
    unsigned int offset = 0;
    /***********************/
    memcpy(&request_type, request, sizeof (request_type));
    offset += sizeof (request_type);
    memcpy(&strategy, request + offset, sizeof (strategy));
    offset += sizeof (strategy);
    //	cout << "TM: Request type: " << chararray_to_hex(string(1,request_type)) << endl;
    switch (request_type) {
            case MATCH_PUB_SUBS:
            cout << "TM: New Match_Pub_Sub or Update_Fid in *-over-ICN" << endl;
            case UPDATE_FID:
        {
            /*this a request for topology formation. Notice that no_publishers here should be 1*/
            memcpy(&no_publishers, request + offset, sizeof (no_publishers));
            offset += sizeof (no_publishers);
            publisher = string(request + offset, PURSUIT_ID_LEN);
            cout << "Publisher: "<< publisher << endl;
            idx += NODEID_LEN;
            offset += NODEID_LEN;
            cout << "Subscribers: ";
            memcpy(&no_subscribers, request + offset, sizeof (no_subscribers));
            offset += sizeof (no_subscribers);
            for (int i = 0; i < no_subscribers; i++) {
                nodeID = string(request + offset, PURSUIT_ID_LEN);
                cout << nodeID << " ";
                idx += NODEID_LEN;
                offset += NODEID_LEN;
                subscribers.insert(nodeID);
            }
            cout << endl;
            /*extract the Information IDs for this MATCH_PUB_SUBS request*/
            ids_size = request_len - offset;
            char * ids = (char *) malloc(ids_size);
            memcpy(&no_ids, request + offset, sizeof (no_ids));
            offset += sizeof (no_ids);
            memcpy(&IDLen, request + offset, sizeof (IDLen));
            offset += sizeof (IDLen);
            memcpy (ids, request + offset, (int)IDLen * PURSUIT_ID_LEN);
            offset += (int)IDLen * PURSUIT_ID_LEN;

  	    // Keep track of the requests per namespace and report the count for current one to Moly
	    string ids_str = string((const char *)(ids) , (int)IDLen * PURSUIT_ID_LEN);
	    string tmp_str = chararray_to_hex(ids_str);
	    root_scope_t root_scope = map_root_scope(strtoul(tmp_str.substr(0, 16).c_str(), NULL, 16));
	    moly_tm_requests[root_scope] += 1;
	    printf("Sending %d requests in %X to Moly.\n", moly_tm_requests[root_scope], root_scope);
	    path_calculations_namespace_t pathCalculations;
	    pathCalculations.push_back(pair<root_scope_t, subscribers_t>(root_scope, moly_tm_requests[root_scope]));
	    moly->Process::pathCalculations(pathCalculations);

            /*Select a subscriber and find a unicast path between the publisher and the subscriber*/
            tm_igraph->UcalculateFID(publisher, subscribers, result, paths);
            map_iter = result.begin();
            /*don't like the loop but it is the only way to find out the value of the map*/
            for(map_iter = result.begin(); map_iter != result.end(); map_iter++)
            {
                if ((*map_iter).second != NULL) {
                    /*determine the response type, if this is a MATCH_PUB_SUB then no FID has been provided before and therefore response type should be START_PUBLISH*/
                    response_type = (request_type == MATCH_PUB_SUBS)? START_PUBLISH : UPDATE_FID;
                    /*construct the response packet to the publisher with the FID of the nearest subscriber*/
                    int response_size = request_len - sizeof(strategy) - sizeof (no_publishers) - no_publishers * PURSUIT_ID_LEN - sizeof (no_subscribers) - no_subscribers * PURSUIT_ID_LEN + FID_LEN;
                    char * response = (char *) malloc (response_size);
                    memcpy(response, &response_type, sizeof (response_type));
                    int ids_index = sizeof (request_type) + sizeof (strategy) + sizeof (no_publishers) + no_publishers * PURSUIT_ID_LEN + sizeof (no_subscribers) + no_subscribers * PURSUIT_ID_LEN;
                    memcpy(response + sizeof (response_type), request + ids_index, request_len - ids_index);
                    memcpy(response + sizeof (response_type) + request_len - ids_index, (*map_iter).second->_data, FID_LEN);
                    /*get FID to the publisher*/
                    response_id = resp_bin_prefix_id + publisher;
                    ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, tm_igraph->getTM_to_nodeFID(publisher)->_data, FID_LEN, response, response_size);
                    /*for resilience, collect the path information to be published to the RM*/
                    if(tm_igraph->getExten(RS)){
                        (int)no_paths++;
                        path = paths[(*map_iter).first];
                        path_len = path.size();
                        
                    }
                    /*free the memory allocated to FIDs and the response*/
                    delete (*map_iter).second;
                    free(response);
                    break;
                }
                else {
                    if(tm_igraph->getExten(RS)){
                        (int)no_alternative_subscribers++;
                        alternative_subscribers.append((*map_iter).first);
                    }
                }
            }
            /*If resiliency support is in effect, create a notification of this delivery update and publish it to the TM_RV_BRK*/
            if((tm_igraph->getExten(RS)) && ((int)path_len > 0)){
                offset = 0;
                response_type = UPDATE_DELIVERY;
                unsigned int response_size = sizeof(response_type) + sizeof(no_ids) + sizeof(IDLen) + ((int)IDLen * PURSUIT_ID_LEN) + NODEID_LEN + sizeof (no_paths) + sizeof(path_len) + (int)path_len + sizeof(no_alternative_subscribers) + (int)no_alternative_subscribers * NODEID_LEN;
                char * response = (char *) malloc(response_size);
                memcpy(response, &response_type, sizeof(response_type));
                offset += sizeof (request_type);
                memcpy(response + offset, &no_ids, sizeof(no_ids));
                offset += sizeof (no_ids);
                memcpy(response + offset, &IDLen, sizeof(IDLen));
                offset += sizeof (IDLen);
                memcpy(response + offset, ids , (int)IDLen * PURSUIT_ID_LEN);
                offset += (int)IDLen * PURSUIT_ID_LEN;
                memcpy(response + offset, (char*)publisher.c_str(), NODEID_LEN);
                offset += NODEID_LEN;
                memcpy(response + offset, &no_paths, sizeof (no_paths));
                offset += sizeof (no_paths);
                memcpy(response + offset, &path_len, sizeof (path_len));
                offset += sizeof (path_len);
                memcpy(response + offset, (char *)path.c_str(), (int)path_len);
                offset += (int)path_len;
                memcpy(response + offset, &no_alternative_subscribers , sizeof(no_alternative_subscribers));
                offset += sizeof (no_alternative_subscribers);
                memcpy(response + offset, (char *)alternative_subscribers.c_str(), (int)no_alternative_subscribers * NODEID_LEN);
                string response_id = UpdateUnicastPathId + tm_igraph->getRMNodeID();
                ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, tm_igraph->getTM_to_nodeFID(tm_igraph->RMnodeID)->_data, FID_LEN, response, response_size);
                free(response);
            }
            free(ids);
        }
            break;
            case MATCH_PUB_iSUBS:
        {
            cout << "TM: request type: "  << (int) MATCH_PUB_iSUBS << ", response type: " << (int)UPDATE_FID_iSUB << endl;
            /*this a request for topology formation. Notice that no_publishers here should be 1*/
            /*there is no strategy at the moment of MATCH_PUB_iSUBS requests*/
            offset -= sizeof(strategy);
            response_type = UPDATE_FID_iSUB;
            memcpy(&no_publishers, request + offset, sizeof (no_publishers));
            offset += sizeof (no_publishers);
            publisher = string(request + offset, PURSUIT_ID_LEN);
            cout << "Publisher: "<< publisher << endl;
            offset += NODEID_LEN;
            cout << "iSubscribers: ";
            memcpy(&no_subscribers, request + offset, sizeof (no_subscribers));
            offset += sizeof (no_subscribers);
            int response_size = sizeof(response_type) + sizeof(no_subscribers) + (int) no_subscribers * NODEID_LEN /*set of isubscribers*/ + (int) no_subscribers * FID_LEN /*FID per isubscriber*/;
            char * response = (char *) malloc(response_size);
            /*put the response type*/
            memcpy(response, &response_type, sizeof (response_type));
            /*put the number of subscribers*/
            memcpy(response + sizeof (response_type), &no_subscribers, sizeof(no_subscribers));
            for (int i = 0; i < (int) no_subscribers; i++) {
                nodeID = string(request + offset, NODEID_LEN);
                memcpy(response + sizeof (response_type) + sizeof (no_subscribers) + idx, (char *) nodeID.c_str(), NODEID_LEN);
                cout << nodeID << " " << endl;;
                idx += NODEID_LEN;
                offset += NODEID_LEN;
                Bitvector * FID_to_isubscriber = tm_igraph->calculateFID(publisher, nodeID);
                cout << "TM: FID: " << FID_to_isubscriber->to_string() << endl;
                memcpy(response + sizeof(response_type) + sizeof (no_subscribers) + idx, (char *)FID_to_isubscriber->_data, FID_LEN);
                idx += FID_LEN;
            }
            response_id = resp_bin_prefix_id + publisher;
            ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(publisher)->_data, FID_LEN, response, response_size);
            free(response);
            cout << endl;
        }
            break;
            case SCOPE_PUBLISHED:
//			cout << "TM: *-over-ICN SCOPE_PUBLISHED Notification, Subscribers: " << (int) no_subscribers << endl;
            case SCOPE_UNPUBLISHED:
        {
            cout << "TM: Scope change in *-over-ICN" << endl;
            memcpy(&no_subscribers, request + offset, sizeof (no_subscribers));
            offset += no_subscribers;
            for (int i = 0; i < (int) no_subscribers; i++) {
                nodeID = string(request + offset + idx, PURSUIT_ID_LEN);
                idx += PURSUIT_ID_LEN;
                /*keep the request type size because the response need to pass the same request type to subs*/
                int response_size = request_len - sizeof (strategy) - sizeof (no_subscribers) - no_subscribers * PURSUIT_ID_LEN + FID_LEN;
                int ids_index = offset + no_subscribers * PURSUIT_ID_LEN;
                char * response = (char *) malloc(response_size);
                memcpy(response, &request_type, sizeof (request_type));
                memcpy(response + sizeof(request_type), request + ids_index, request_len - ids_index);
                response_id = resp_bin_prefix_id + nodeID;
                ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, tm_igraph->getTM_to_nodeFID(nodeID)->_data, FID_LEN, response, response_size);
                free(response);
            }
        }
            break;
        default:
            break;
    }
    cout << "---------------- EoR *-over-ICN  --------------------\n" << endl;
}
void handleLinkStateNotificationPM(char *request, int request_len, const string &request_publisher) {
    cout<<"---------------- LSN REQUEST--------------------"<<endl;
    bool update = false;
    bool remove = false;
    bool match = false;
    bool bidirectional = false;
    unsigned int no_affectedLIDs;
    unsigned int no_nodes = tm_igraph->reverse_node_index.size();
    unsigned int offset = 0;
    unsigned char response_type;
    unsigned char lsn_type;
    unsigned char net_type;
    map<ICNEdge, Bitvector *> freedLIDs;
    map<ICNEdge, Bitvector *>::iterator lid_it;
    map<string, Bitvector *>::iterator return_fid_it;
    Bitvector AND_CMP (FID_LEN * 8);
    Bitvector * AllzeroFID = new Bitvector(FID_LEN * 8);
    Bitvector * update_TM_to_nodeFID = new Bitvector(FID_LEN * 8);
    Bitvector * update_RVTMFID = new Bitvector(FID_LEN * 8);
    Bitvector * TM_to_all_FID = new Bitvector(FID_LEN * 8);
    /*parse the control message feilds*/
    memcpy(&lsn_type, request, sizeof(lsn_type));
    offset += sizeof(lsn_type);
    memcpy(&net_type, request + offset, sizeof(net_type));
    offset += sizeof(net_type);
    string nodeID;
    string affected_node = string(request, offset, NODEID_LEN);
    remove = (lsn_type == REMOVE_LINK);
    /*new update from LSM whereby bidirectional is sent rather than letter*/
    bidirectional = (bool)net_type;
    update = tm_igraph->updateGraph(affected_node, request_publisher, bidirectional, remove, * moly);
    if (update) {
        string response_id;
        if(remove){
            cout << "TM: Link Failure: " << request_publisher << " - " << affected_node << endl;
            int response_size = sizeof(response_type) + FID_LEN;
            char * response = (char *) malloc (response_size);
            freedLIDs = tm_igraph->getFreedLIDs();
            no_affectedLIDs = freedLIDs.size();
            return_fid_it = tm_igraph->TM_to_nodeFID.begin();
            for (unsigned int i = 0; i < no_nodes; i++) {
                /*check all LIDs for the TM->Node direction before checking the Node->RV/TM direction, because otherwise a break on both directions will not be correctly detected*/
                lid_it = freedLIDs.begin();
                for (unsigned int j = 0; j < no_affectedLIDs; j++) {
                    nodeID = (*return_fid_it).first;
                    AND_CMP = *(lid_it->second) & *((*return_fid_it).second);
                    match = (AND_CMP == *(lid_it->second));
                    if(match){
                        /*TM-to-node FID is affected*/
                        update_TM_to_nodeFID = tm_igraph->calculateFID(tm_igraph->getNodeID(), nodeID);
                        /*update the TM_to_nodeFID state */
                        tm_igraph->setTM_to_nodeFID(nodeID, update_TM_to_nodeFID);
                    }
                    /*publish affected LIDs in unicast mode to individual nodes instead of broadcast*/
                    if(uc_notification){
//                        cout << "TM: publish affected LIDs to node: " << nodeID << endl;
                        ba->publish_data(pathMgmt_bin_full_id, IMPLICIT_RENDEZVOUS, (char *)tm_igraph->getTM_to_nodeFID(nodeID)->_data, FID_LEN, (lid_it->second)->_data, FID_LEN);
                    }
                    lid_it++;
                }
                /*Now, check all the LIDs for the Node->RV/TM direction - having a working TM_to_nodeFID*/
                if (!(*(tm_igraph->getTM_to_nodeFID(nodeID))).zero()) {
                    /*The node still reachable with new FID, so check if RV/TM FIDs are affected*/
                    lid_it = freedLIDs.begin();
                    for (unsigned int j = 0; j < no_affectedLIDs; j++) {
                        AND_CMP = *(lid_it->second) & *(tm_igraph->getRVFID(nodeID));
                        if (AND_CMP == *(lid_it->second)) {
                            /*RVFID is affected, calculate and send an updated RVFID*/
                            update_RVTMFID = tm_igraph->calculateFID(nodeID, tm_igraph->getRVNodeID());
                            /*update the RVFID state*/
                            tm_igraph->setRVFID(nodeID, update_RVTMFID);
                            response_type = UPDATE_RVFID;
                            memcpy(response, &response_type, sizeof (response_type));
                            memcpy(response + sizeof (response_type), (char *)update_RVTMFID->_data, FID_LEN);
                            response_id = resp_bin_prefix_id + nodeID;
                            ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(nodeID)->_data, FID_LEN, response, response_size);
                        }
                        AND_CMP = *(lid_it->second) & *tm_igraph->getTMFID(nodeID);
                        if (AND_CMP == *(lid_it->second)) {
                            /*TMFID is affected, calculate and send an updated TMFID*/
                            update_RVTMFID = tm_igraph->calculateFID(nodeID, tm_igraph->getNodeID());
                            /*update the TMFID state*/
                            tm_igraph->setTMFID(nodeID, update_RVTMFID);
                            response_type = UPDATE_TMFID;
                            memcpy(response, &response_type, sizeof (response_type));
                            memcpy(response + sizeof (response_type), (char *)update_RVTMFID->_data, FID_LEN);
                            response_id = resp_bin_prefix_id + nodeID;
                            ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(nodeID)->_data, FID_LEN, response, response_size);
                        }
                        lid_it++;
                    }
                }
                else {
                    /*no TM_to_nodeFID is found, so it is safe to assume that the node is disconnected and therefore no RVFID or TMFID will be found either*/
                    cout << "TM: Node: " << nodeID << " is now isolated" << endl;
                    tm_igraph->setRVFID(nodeID, AllzeroFID);
                    tm_igraph->setTMFID(nodeID, AllzeroFID);
                }
                *TM_to_all_FID = *TM_to_all_FID | *(tm_igraph->TM_to_nodeFID[nodeID]);
                return_fid_it++;
            }
            lid_it = freedLIDs.begin();
            /*Publish affected LIDs*/
            if (!uc_notification) {
                cout << "TM: publish affected LIDs " << endl;
                for (unsigned int j = 0; j < no_affectedLIDs; j++) {
                    ba->publish_data(pathMgmt_bin_full_id, IMPLICIT_RENDEZVOUS, (char *)TM_to_all_FID->_data, FID_LEN, (lid_it->second)->_data, FID_LEN);
                    lid_it++;
                }
            }
            free(response);
        }
        else {
            /*either a new link is being added or a broken link is restored, in either case check if there are any unreachable nodes*/
            cout << "TM: Link Restoration: " << request_publisher << " - " << affected_node << endl;
            int response_size = sizeof(response_type) + FID_LEN;
            char * response = (char *) malloc (response_size);
            return_fid_it = tm_igraph->TM_to_nodeFID.begin();
            for (unsigned int j = 0; j < no_nodes; j++) {
                nodeID = (*return_fid_it).first;
                if((*(*return_fid_it).second).zero()){
                    /*the node was disconnected, calculate a new TM_to_nodeFID*/
                    update_TM_to_nodeFID = tm_igraph->calculateFID(tm_igraph->getNodeID(), nodeID);
                    if (!(*update_TM_to_nodeFID).zero()) {
                        cout << "TM: node " << nodeID << " reconnected to ICN, will provide it with RV/TM FIDs" << endl;
                        /*there is now a path to the node, update the TM_to_nodeFID*/
                        tm_igraph->setTM_to_nodeFID(nodeID, update_TM_to_nodeFID);
                        /*calculate and send the new RVFID*/
                        update_RVTMFID = tm_igraph->calculateFID(nodeID, tm_igraph->getRVNodeID());
                        tm_igraph->setRVFID(nodeID, update_RVTMFID);
                        response_type = UPDATE_RVFID;
                        memcpy(response, &response_type, sizeof (response_type));
                        memcpy(response + sizeof (response_type), (char *)update_RVTMFID->_data, FID_LEN);
                        response_id = resp_bin_prefix_id + nodeID;
                        ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(nodeID)->_data, FID_LEN, response, response_size);
                        /*calculate and send the new TMFID*/
                        update_RVTMFID = tm_igraph->calculateFID(nodeID, tm_igraph->getNodeID());
                        tm_igraph->setTMFID(nodeID, update_RVTMFID);
                        response_type = UPDATE_TMFID;
                        memcpy(response, &response_type, sizeof (response_type));
                        memcpy(response + sizeof (response_type), (char *)update_RVTMFID->_data, FID_LEN);
                        response_id = resp_bin_prefix_id + nodeID;
                        ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(nodeID)->_data, FID_LEN, response, response_size);
                    }
                }
                return_fid_it++;
            }
            free(response);
        }
    }
    else {
        cout << "TM: no update to be published" << endl;
    }
    delete AllzeroFID;
    delete update_TM_to_nodeFID;
    delete update_RVTMFID;
    delete TM_to_all_FID;
    cout<<"---------------- EoR --------------------"<<endl;
}
void handleLinkStateNotificationRM(char *request, int request_len, const string &request_publisher) {
    cout<<"---------------- LSN REQUEST--------------------"<<endl;
    bool update = false;
    bool remove = false;
    bool match = false;
    bool bidirectional = false;
    unsigned int no_affectedLIDs;
    unsigned int no_nodes = tm_igraph->reverse_node_index.size();
    unsigned int offset = 0;
    unsigned char response_type;
    unsigned char rm_request_type;
    unsigned char lsn_type;
    unsigned char net_type;
    map<ICNEdge, Bitvector *> freedLIDs;
    map<ICNEdge, Bitvector *>::iterator lid_it;
    map<string, Bitvector *>::iterator return_fid_it;
    Bitvector AND_CMP (FID_LEN * 8);
    Bitvector * AllzeroFID = new Bitvector(FID_LEN * 8);
    Bitvector * update_TM_to_nodeFID = new Bitvector(FID_LEN * 8);
    Bitvector * update_RVTMFID = new Bitvector(FID_LEN * 8);
    Bitvector * TM_to_all_FID = new Bitvector(FID_LEN * 8);
    /*parse the control message feilds*/
    memcpy(&lsn_type, request, sizeof(lsn_type));
    offset += sizeof(lsn_type);
    memcpy(&net_type, request + offset, sizeof(net_type));
    offset += sizeof(net_type);
    string nodeID;
    string affected_node = string(request, offset, NODEID_LEN);
    remove = (lsn_type == REMOVE_LINK);
    bidirectional = (bool) net_type;
    update = tm_igraph->updateGraph(affected_node, request_publisher, bidirectional, remove, * moly);
    if (update) {
        string response_id;
        string rm_request_id;
        if(remove){
            cout << "TM: Link Failure: " << request_publisher << " - " << affected_node << endl;
            /*check the TM FIDs state and update affected FIDs, if any*/
            int response_size = sizeof(response_type) + FID_LEN;
            char * response = (char *) malloc (response_size);
            freedLIDs = tm_igraph->getFreedLIDs();
            no_affectedLIDs = freedLIDs.size();
            return_fid_it = tm_igraph->TM_to_nodeFID.begin();
            for (unsigned int i = 0; i < no_nodes; i++) {
                /*check all LIDs for the TM->Node direction before checking the Node->RV/TM direction, because otherwise a break on both directions will not be correctly detected*/
                lid_it = freedLIDs.begin();
                for (unsigned int j = 0; j < no_affectedLIDs; j++) {
                    nodeID = (*return_fid_it).first;
                    AND_CMP = *(lid_it->second) & *((*return_fid_it).second);
                    match = (AND_CMP == *(lid_it->second));
                    if(match){
                        /*TM-to-node FID is affected*/
                        update_TM_to_nodeFID = tm_igraph->calculateFID(tm_igraph->getNodeID(), nodeID);
                        /*update the TM_to_nodeFID state */
                        tm_igraph->setTM_to_nodeFID(nodeID, update_TM_to_nodeFID);
                    }
                    lid_it++;
                }
                /*Now, check all the LIDs for the Node->RV/TM direction - having a working TM_to_nodeFID*/
                if (!(*(tm_igraph->getTM_to_nodeFID(nodeID))).zero()) {
                    /*The node still reachable with new FID, so check if RV/TM FIDs are affected*/
                    lid_it = freedLIDs.begin();
                    for (unsigned int j = 0; j < no_affectedLIDs; j++) {
                        AND_CMP = *(lid_it->second) & *(tm_igraph->getRVFID(nodeID));
                        if (AND_CMP == *(lid_it->second)) {
                            /*RVFID is affected, calculate and send an updated RVFID*/
                            update_RVTMFID = tm_igraph->calculateFID(nodeID, tm_igraph->getRVNodeID());
                            /*update the RVFID state*/
                            tm_igraph->setRVFID(nodeID, update_RVTMFID);
                            response_type = UPDATE_RVFID;
                            memcpy(response, &response_type, sizeof (response_type));
                            memcpy(response + sizeof (response_type), (char *)update_RVTMFID->_data, FID_LEN);
                            response_id = resp_bin_prefix_id + nodeID;
                            ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(nodeID)->_data, FID_LEN, response, response_size);
                        }
                        AND_CMP = *(lid_it->second) & *tm_igraph->getTMFID(nodeID);
                        if (AND_CMP == *(lid_it->second)) {
                            /*TMFID is affected, calculate and send an updated TMFID*/
                            update_RVTMFID = tm_igraph->calculateFID(nodeID, tm_igraph->getNodeID());
                            /*update the TMFID state*/
                            tm_igraph->setTMFID(nodeID, update_RVTMFID);
                            response_type = UPDATE_TMFID;
                            memcpy(response, &response_type, sizeof (response_type));
                            memcpy(response + sizeof (response_type), (char *)update_RVTMFID->_data, FID_LEN);
                            response_id = resp_bin_prefix_id + nodeID;
                            ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(nodeID)->_data, FID_LEN, response, response_size);
                        }
                        lid_it++;
                    }
                }
                else {
                    /*no TM_to_nodeFID is found, so it is safe to assume that the node is disconnected and therefore no RVFID or TMFID will be found either*/
                    cout << "TM: Node: " << nodeID << " is now isolated" << endl;
                    tm_igraph->setRVFID(nodeID, AllzeroFID);
                    tm_igraph->setTMFID(nodeID, AllzeroFID);
                }
                *TM_to_all_FID = *TM_to_all_FID | *(tm_igraph->TM_to_nodeFID[nodeID]);
                return_fid_it++;
            }
            /*request RM Assistance to discover failed information delivery*/
            offset = 0;
            rm_request_type = DISCOVER_FAILURE;
            int rm_request_size = sizeof(rm_request_type) + sizeof(net_type) + NODEID_LEN /*request_publisher*/ + NODEID_LEN /*affected_node*/;
            char * rm_request = (char *) malloc (rm_request_size);
            memcpy(rm_request, &rm_request_type, sizeof(rm_request_type));
            offset += sizeof (rm_request_type);
            memcpy(rm_request + offset , &net_type, sizeof (net_type));
            offset += sizeof (net_type);
            memcpy(rm_request + offset, (char*)request_publisher.c_str() , NODEID_LEN);
            offset += NODEID_LEN;
            memcpy(rm_request + offset, (char*)affected_node.c_str(), NODEID_LEN);
            offset += NODEID_LEN;
            rm_request_id = DiscoverFailureId + tm_igraph->getRMNodeID();
            ba->publish_data(rm_request_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(tm_igraph->RMnodeID)->_data, FID_LEN, rm_request, rm_request_size);
            free(rm_request);
            free(response);
        }
        else {
            /*either a new link is being added or a broken link is restored, in either case check if there are any unreachable nodes*/
            cout << "TM: Link Restoration: " << request_publisher << " - " << affected_node << endl;
            int response_size = sizeof(response_type) + FID_LEN;
            char * response = (char *) malloc (response_size);
            return_fid_it = tm_igraph->TM_to_nodeFID.begin();
            for (unsigned int j = 0; j < no_nodes; j++) {
                nodeID = (*return_fid_it).first;
                if((*(*return_fid_it).second).zero()){
                    /*the node was disconnected, calculate a new TM_to_nodeFID*/
                    update_TM_to_nodeFID = tm_igraph->calculateFID(tm_igraph->getNodeID(), nodeID);
                    if (!(*update_TM_to_nodeFID).zero()) {
                        cout << "TM: node " << nodeID << " reconnected to ICN, will provide it with RV/TM FIDs" << endl;
                        /*there is now a path to the node, update the TM_to_nodeFID*/
                        tm_igraph->setTM_to_nodeFID(nodeID, update_TM_to_nodeFID);
                        /*calculate and send the new RVFID*/
                        update_RVTMFID = tm_igraph->calculateFID(nodeID, tm_igraph->getRVNodeID());
                        tm_igraph->setRVFID(nodeID, update_RVTMFID);
                        response_type = UPDATE_RVFID;
                        memcpy(response, &response_type, sizeof (response_type));
                        memcpy(response + sizeof (response_type), (char *)update_RVTMFID->_data, FID_LEN);
                        response_id = resp_bin_prefix_id + nodeID;
                        ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(nodeID)->_data, FID_LEN, response, response_size);
                        /*calculate and send the new TMFID*/
                        update_RVTMFID = tm_igraph->calculateFID(nodeID, tm_igraph->getNodeID());
                        tm_igraph->setTMFID(nodeID, update_RVTMFID);
                        response_type = UPDATE_TMFID;
                        memcpy(response, &response_type, sizeof (response_type));
                        memcpy(response + sizeof (response_type), (char *)update_RVTMFID->_data, FID_LEN);
                        response_id = resp_bin_prefix_id + nodeID;
                        ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) tm_igraph->getTM_to_nodeFID(nodeID)->_data, FID_LEN, response, response_size);
                    }
                }
                return_fid_it++;
            }
            free(response);
        }
    }
    else {
        cout << "TM: no update to be published" << endl;
    }
    delete AllzeroFID;
    delete update_TM_to_nodeFID;
    delete update_RVTMFID;
    delete TM_to_all_FID;
    cout<<"---------------- EoR --------------------"<<endl;
    
}
void handleLSMUpdate(uint8_t * data, int data_len, const string & request_pub){
    //cout<<"Got a LSM update!! Node="<<request_pub<<endl;
    LSMPacket pkt(data, data_len);
    // Update the graph for each link
    for (uint i=0; i<pkt.getLinksLen(); i++){
        tm_igraph->updateLinkState(pkt.getLidStr(i), pkt.getLinkStatus(i));
        QoSList l=pkt.getLinkStatus(i);
    }
    // DEBUG: Print the qos link weight map
    // cout<<(tm_igraph->qlwm)<<endl;
}

/*\TODO: introduce a function dispatcher based on type and prefixID, to clean up the mess of the event_listner_loop. accordingly also clean up the handling functions above*/
void *event_listener_loop(void *arg) {
    Blackadder *ba = (Blackadder *) arg;
    string prefix_id;
    string publisher;
    while (listening) {
        Event ev;
        ba->getEvent(ev);
        if (ev.type == UNDEF_EVENT) {
            if (!listening)
            cout << "TM: final event" << endl;
            return NULL;
        } else if (ev.type == PUBLISHED_DATA) {
            publisher = ev.id.substr(ev.id.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
            prefix_id = ev.id.substr(0, ev.id.length() - PURSUIT_ID_LEN);
            if ((prefix_id == lsn_bin_id) && (tm_igraph->getExten(PM))){
                /*Path Management*/
                cout << "TM: handle network change through path management" << endl;
                handleLinkStateNotificationPM((char *) ev.data, ev.data_len, publisher);
            }
            else if ((prefix_id == lsn_bin_id) && (tm_igraph->getExten(RS))) {
                /*Resilience using the central RM*/
                handleLinkStateNotificationRM((char *) ev.data, ev.data_len, publisher);
            }
            else if ((prefix_id==lsm_bin_scope) && (tm_igraph->getExten(QOS))) { 	// mac_qos
                /*Call the handler for LSM... This should update the internal link*/
                /*status structure and the Graph's edge weight map...*/
                handleLSMUpdate( (uint8_t *) ev.data, ev.data_len, publisher);
            }
            /*request for unicast path of *-over-icn*/
            else if ((prefix_id == UnicastDeliveryId) || (prefix_id == RecoverUnicastDeliveryId)) {
                cout << "TM: request for unicast path" << endl;
                start++;
                handleUnicastPathRequest((char*) ev.data, ev.data_len);
            }
            else {
                cout << "TM: id: " << chararray_to_hex(ev.id) << endl;
                handleMulticastPathRequest((char *) ev.data, ev.data_len);
            }
        } else {
            cout << "TM: I am not expecting any other notification...FATAL" << endl;
        }
    }
    return NULL;
}
void *moly_listener_loop(void *arg) {
	Moly *moly = (Moly *)arg;
	if (!moly->Process::startReporting()) {
		cout << "Communication with moinitoring agent failed ... exiting" << endl ;
	} else {
	        cout << "TM: recieved a startReporting trigger, will report the topology information to MOLY.." << endl;
		tm_igraph->reportTopology(*moly);
	}
	return NULL;
}


void sigfun(int sig) {
    listening = 0;
    if (event_listener){
        pthread_cancel(*event_listener);
    }
    if (moly_event_listener) {
        pthread_cancel(*moly_event_listener);
    }
    (void) signal(SIGINT, SIG_DFL);
}

int main(int argc, char* argv[]) {
    opterr = 0;
    bool te = false;
    long te_delay=60;
    double te_e=0.1;
    double defaultBW=1e9;
    int index = 0;
    char c;
    while ((c = getopt (argc, argv, "prqtdu:")) != -1){
        switch (c)
        {
                case 't':
                cout << "TM: TE Extension." << endl;
                te = true;
                break;
                case 'd':
                sscanf(optarg,"%ld",&te_delay);
                break;
                case 'b':
                sscanf(optarg,"%lg",&defaultBW);
                break;
                case 'e':
                sscanf(optarg,"%lg",&te_e);
                break;
                case 'q':
                index = 1;
                cout << "TM: QoS Extension." << endl;
                break;
                case 'r':
                index = 2;
                cout << "TM: Resilience Extension." << endl;
                break;
                case 'p':
                index = 3;
                cout << "TM: Path Management Extension." << endl;
                break;
                case 'u':
                uc_notification = (bool)atoi(optarg);
                break;
                case '?':
                if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                fprintf (stderr,
                         "Unknown option character `\\x%x'.\n",
                         optopt);
                return 1;
            default:
                abort ();
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "The topolgy file is missing\n");
        exit(EXIT_FAILURE);
    }
    
    (void) signal(SIGINT, sigfun);
    cout << "TM: starting - process ID: " << getpid() << endl;
    if (te) {
        tm_igraph = new TEgraphMF();
        tm_igraph->setExten(TE);
        cout<<"Setting delay to "<<te_delay<<endl;
        ((TEgraphMF*)tm_igraph)->initialise(te_delay,te_e,defaultBW);
        cout << "TM: TE Support is in effect" << endl;
    } else {
        tm_igraph = new TMIgraph();
        if (index > 0) {
            tm_igraph->setExten(index);
        }
    }
    /*read the graphML file that describes the topology*/
    if (tm_igraph->readTopology(argv[optind]) < 0) {
        cout << "TM: couldn't read topology file...aborting" << endl;
        exit(0);
    }
    cout << "Blackadder Node: " << tm_igraph->getNodeID() << endl;
    /*Calculate RV/TMFIDs and TM_to_nodeFID*/
    tm_igraph->calculateRVTMFIDs();
    cout << "TM: Calculated all RV/TMFIDs" << endl;
    /***************************************************/
    /*Report topology data with Moly*/
	moly = new Moly();
	pthread_create(&_moly_event_listener, NULL, moly_listener_loop, (void *)moly);
	moly_event_listener = &_moly_event_listener;
	pthread_detach(*moly_event_listener);
    /*------------------------------*/
    if (tm_igraph->getMode().compare("kernel") == 0) {
        ba = Blackadder::Instance(false);
    } else {
        ba = Blackadder::Instance(true);
    }
    
    if(tm_igraph->getExten(TE)) {
        cout << "TE extension detected: Spawning..." << endl;
        pthread_create(&_te_thread, NULL, te_loop,(void*)tm_igraph);
        te_thread = &_te_thread;
    }
    pthread_create(&_event_listener, NULL, event_listener_loop, (void *) ba);
    event_listener = &_event_listener;
    ba->subscribe_scope(req_bin_id, req_bin_prefix_id, IMPLICIT_RENDEZVOUS, NULL, 0);
    ba->subscribe_scope(uc_req_bin_id, req_bin_id, IMPLICIT_RENDEZVOUS, NULL, 0);
    ba->subscribe_scope(lsn_bin_id, lsn_bin_prefix_id, IMPLICIT_RENDEZVOUS, NULL, 0);
    ba->subscribe_scope(resl_resp_bin_id, resl_resp_bin_prefix_id, IMPLICIT_RENDEZVOUS, NULL, 0);
    /*resilience for *-over-ICN through the central RM*/
    ba->subscribe_scope(uc_resl_bin_id,resl_resp_bin_id, IMPLICIT_RENDEZVOUS, NULL, 0);
    /*Path Management info*/
    ba->publish_scope(pathMgmt_bin_id, lsn_bin_prefix_id, IMPLICIT_RENDEZVOUS, NULL, 0);
    ba->publish_info(pathMgmt_bin_id, pathMgmt_bin_id, IMPLICIT_RENDEZVOUS, NULL, 0);
    // Subscribe to the LSM scope to get updates
    ba->subscribe_scope(lsm_bin_scope, "", DOMAIN_LOCAL, NULL, 0);
    sleep(5);
    pthread_join(*event_listener, NULL);
    cout << "TM: disconnecting" << endl;
    ba->disconnect();
    delete ba;
    delete tm_igraph;
	delete moly;
    cout << "TM: exiting" << endl;
    return 0;
}

root_scope_t map_root_scope(uint64_t s) {
    if (s < 65536)
        return s;
    else {
        enum root_namespaces_t s_mapped = NAMESPACE_UNKNOWN;
        if (s == strtoul("FFFFFFFFFFFFFFFF", NULL, 16))
            s_mapped = NAMESPACE_APP_RV;
        if (s == strtoul("FFFFFFFFFFFFFFFE", NULL, 16))
            s_mapped = NAMESPACE_RV_TM;
        if (s == strtoul("FFFFFFFFFFFFFFFD", NULL, 16))
            s_mapped = NAMESPACE_TM_APP;
        if (s == strtoul("FFFFFFFFFFFFFFFB", NULL, 16))
            s_mapped = NAMESPACE_LINK_STATE;
        if (s == strtoul("FFFFFFFFFFFFFFFA", NULL, 16))
            s_mapped = NAMESPACE_PATH_MANAGEMENT;
        return s_mapped;
    }
}
