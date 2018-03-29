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

/**
 * @file linkstate_monitor.cpp
 * @brief ICNApp/Mapp for monitoring the node state and report connectivity changes to the TM.
 */
#include <stdlib.h>
#include <map>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <getopt.h>
#include <bitvector.hpp>
#include <blackadder.hpp>
#include <mapiagent.hpp>

/*APIs instances*/
Blackadder *ba; /*!< blackadder ICNAPI*/
MapiAgent *mp; /*!< blackadder Management API (MAPI)*/
struct instances {
    Blackadder *_ba;
    MapiAgent *_mp;
};

/*configurations*/
double lsu_lifetime = 3; /*!< the lifetime of a nighbour before an update is due - default to 5sec*/
double lsu_interval = 1; /*!< the interval (in sec) between publishing two consective LSUs*/
double passed; /*!< passed time since the last recieved LSU */
bool connected = true; /*!< indicator for the connectivity status of the node*/
bool operation = true; /*!< keep threads running*/
bool bidirectional = true; /*!< the graph type, action on one direction causes the same action on the opposit direction*/
time_t live; /*!< live record of time*/
string nodeID; /*!< NODEID_LEN-character node identfier, obtained from the core using MAPI*/
Bitvector RVTMFID(FID_LEN * 8); /*!< FID to the TM and RV functions (assuming the two are located in the same node)*/


/*information/scope identifiers*/
string iID; /*!< PURSUIT_ID_LEN-character information identifier, used to publish notifications to the TM*/
string lsuID; /*!< PURSUIT_ID_LEN-character ICN identifier of LSU*/
string lsnID; /*!< PURSUIT_ID_LEN-character ICN identifier of LSNs to the TM*/
string lsuSID = string(PURSUIT_ID_LEN -1, SUFFIX_NOTIFICATION) + (char) LINK_STATE_UPDATE; /*!< SID used to broadcast visibility to neighbours*/
string lsnSID = string(PURSUIT_ID_LEN -1, SUFFIX_NOTIFICATION) + (char) LINK_STATE_NOTIFICATION; /*!< SID used to publish Link-State Notifications (LSN) the TM*/

/*messages*/
unsigned int lsu_size = sizeof(time_t);
unsigned int lsn_size =  sizeof (unsigned char) /*message type*/ + sizeof (unsigned char) /*network type (uni- or bi-directional)*/ + (unsigned int) NODEID_LEN /*affected node*/;
char *lsu = (char *) malloc (lsu_size);
char *lsn = (char *) malloc (lsn_size);

/*State*/
map<string, time_t> neighbours;     /*!< adjacent nodes (aka neighbours)*/
map<string, time_t>::iterator nit;  /*!< neighbours iterator*/
map<string, time_t>::iterator it;   /*!< point to neighbouring node or end of neighbours*/

/*Threads*/
pthread_t _lsu_worker, *lsu_worker = NULL; /*!< periodically publish a LSU and process LSUs of neighbours*/
pthread_t _event_listener, *event_listener = NULL; /*!< handling events recieved from blackadder ICN API*/

/*handle linkstate advertisement from neighbours*/
void handle_lsu(const string &ID) {
    using namespace std;
    string neighbour = string(ID, PURSUIT_ID_LEN, NODEID_LEN);
    if (!mp->get_rvtmfid(RVTMFID))
        cout << "SLM: couldn't get RVTMFID using MAPI" << endl;
    if (neighbours.size() == 0) {
        connected = true;
        mp->set_connection_status(RECONNECTED);
        neighbours.insert(pair<string, time_t>(neighbour, live));
        memset(lsn, ADD_LINK, sizeof(unsigned char));
        memset(lsn + sizeof (unsigned char), (unsigned char)bidirectional , sizeof (unsigned char));
        memcpy(lsn + sizeof(unsigned char) + sizeof (unsigned char), neighbour.c_str(), (unsigned int) NODEID_LEN);
        ba->publish_data(lsnID, IMPLICIT_RENDEZVOUS,(char *)RVTMFID._data, FID_LEN, lsn, lsn_size);
        cout << "LSM: added neighbour: " << neighbour << endl;
    } else {
        it = neighbours.find(neighbour);
        if (it == neighbours.end()) {
            neighbours.insert(pair<string, time_t>(neighbour, live));
            memset(lsn, ADD_LINK, sizeof(unsigned char));
            memset(lsn + sizeof (unsigned char), (unsigned char)bidirectional , sizeof (unsigned char));
            memcpy(lsn + sizeof(unsigned char) + sizeof (unsigned char), neighbour.c_str(), (unsigned int) NODEID_LEN);
            ba->publish_data(lsnID, IMPLICIT_RENDEZVOUS,(char *)RVTMFID._data, FID_LEN, lsn, lsn_size);
            cout << "LSM: added neighbour: " << neighbour << endl;
        }
        else {
            neighbours[neighbour]= live;
        }
    }
}

void sigfun(int sig) {
    (void) signal(SIGINT, SIG_DFL);
    operation=false;
    if (event_listener)
        pthread_cancel(*event_listener);
    if (lsu_worker) {
        pthread_cancel(*lsu_worker);
    }
    ba->disconnect();
    mp->disconnect();
    delete ba;
    delete mp;
    exit(0);
}

/*process Blackadder ICN events*/
void * ba_handler(void *arg){
    using namespace std;
    Blackadder *ba = (Blackadder *)arg;
    Event ev;
    string recieved_SID;
    unsigned int depth;
    while (operation) {
        ba->getEvent(ev);
        switch (ev.type) {
            case PUBLISHED_DATA:
                depth = (ev.id.length() / PURSUIT_ID_LEN) - 1;
                recieved_SID = get_chararray_sid(ev.id, depth);
                if (recieved_SID == lsuSID) {
                    handle_lsu(ev.id);
                }
                else
                    cout << "LSM: recieved SID is: " << recieved_SID << endl;
                break;
            case PAUSE_PUBLISH:
                /*shouldn't really recieve this event, but just checking now*/
                cout << "LSM: Pause publish recieved" << endl;
                break;
            case RESUME_PUBLISH:
                /*shouldn't really recieve this event, but just checking now*/
                cout << "LSM: resume publish recieved" << endl;
                break;
            default:
                break;
        }
    }
    return NULL;
}

void * lsu_handler(void *args){
    using namespace std;
    struct instances *_instances = (struct instances *)args;
    Blackadder *ba = (Blackadder *)(_instances->_ba);
    MapiAgent *mp = (MapiAgent *)(_instances->_mp);
    while (operation) {
        time(&live);
        if (connected) {
            if (!mp->get_rvtmfid(RVTMFID))
                cout << "SLM: couldn't get RVTMFID using MAPI" << endl;
            lsu = ctime(&live);
            ba->publish_data(lsuID, BROADCAST_IF, NULL, 0, lsu, lsu_size);
            for (nit = neighbours.begin(); nit != neighbours.end();) {
                passed = difftime(live, (*nit).second);
                if (passed > lsu_lifetime) {
                    memset(lsn, REMOVE_LINK, sizeof (unsigned char));
                    memset(lsn + sizeof (unsigned char), (unsigned char)bidirectional, sizeof(unsigned char));
                    memcpy(lsn + sizeof (unsigned char) + sizeof (unsigned char), nit->first.c_str(), (unsigned int) NODEID_LEN);
                    ba->publish_data(lsnID, IMPLICIT_RENDEZVOUS,(char *)RVTMFID._data, FID_LEN, lsn, lsn_size);
                    cout << "LSM: failing neighbour: " << (nit->first) << ", " << passed << " sec" << endl;
                    /*there is only one neighbour and it has expired so it is fare to assume DISCONNECTION status*/
                    if (neighbours.size() == 1) {
                        connected = false;
                        mp->set_connection_status(DISCONNECTED);
                        cout << "LSM: disconnected" << endl;
                    }
                    /*it is an implementation choice to remove the node after 1.5 times the LSU lifetime and republish the notification to the TM,
                     in case it was not recieved the first time due to change in RVTMFID*/
                    if (passed > (lsu_lifetime * 1.5)) {
                        cout << "LSM: Removed neighbour: " << (nit->first) << ", " << passed << " sec" << endl;
                        neighbours.erase(nit++);
                    } else
                        nit++;
                }
                else
                    nit++;
            }
            sleep(lsu_interval);
        }
    }
    delete _instances;
    return NULL;
}

int main(int argc, char* argv[]) {
    using namespace std;
    (void) signal(SIGINT, sigfun);
    char opt;
    /*process cli flags*/
    while ((opt = getopt (argc, argv, "n:i:d:")) != -1){
        switch (opt)
        {
            case 'n':
                bidirectional = atoi(optarg);
                break;
            case 'd':
                /*configure the lifetime to other value than the default 7 seconds*/
                lsu_lifetime = atof(optarg);
                cout << "LSM: Lifetime configured to be: " << lsu_lifetime << " secs." << endl;
                break;
            case 'i':
                /*the interval between publishing LSUs*/
                lsu_interval = atof(optarg);
                cout << "LSM: LSU interval configured to be: " << lsu_interval << " secs." << endl;
                break;
            case '?':
                if (isprint (optopt))
                    fprintf (stderr, "LSM: Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,
                             "LSM: Unknown option character `\\x%x'.\n",
                             optopt);
                return 1;
            default:
                cout << "LSM: something went wrong, aborting..." << endl;
                abort ();
        }
    }
    /*instanciate blackadder and mapi*/
    ba = Blackadder::Instance(true);
    mp = new MapiAgent();
    mp->set_MapiInstance(true);
    struct instances args;
    args._ba = ba;
    args._mp = mp;
    /*get the nodeID*/
    if (!mp->get_nodeid(nodeID)) {
        cout << "LSM: couldn't get the nodeID" << endl;
        return -1;
    }
    iID = nodeID;
    lsuID = lsuSID + iID;
    lsnID = lsnSID + iID;
    /*subscribe for LSU updates*/
    ba->subscribe_scope(lsuSID, "", BROADCAST_IF, NULL, 0);
    /*create the threads*/
    pthread_create(&_event_listener, NULL, ba_handler, (void *) ba);
    event_listener = &_event_listener;
    pthread_create(&_lsu_worker, NULL, lsu_handler, (void *) &args);
    lsu_worker = &_lsu_worker;
    /*join the threads*/
    pthread_join(*event_listener, NULL);
    pthread_join(*lsu_worker, NULL);
    free(lsu);
    free(lsn);
    return 0;
}





