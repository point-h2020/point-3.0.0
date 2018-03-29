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

/**
 * @file mapiagent.cpp
 * @brief Agent of the Management API blocking library.
 */
#include "mapiagent.hpp"

Mapi* MapiAgent::_mp = NULL;

MapiAgent::MapiAgent() {};
MapiAgent::~MapiAgent() {};

void MapiAgent::set_MapiInstance(bool user_space) {
    if (! user_space) {
        cout << "MAPI: not supported with kernel-level Blackadder" << endl;
        exit(0);
    }
    _mp = Mapi::Instance(user_space);
}
bool MapiAgent::get_nodeid(string &nodeID) {
    unsigned int length = 0;
    struct response _response;
    if (! _mp->request(GET_NODEID, length, NULL)) {
        perror("MAPI: could not get the nodeID");
        return false;
    }
    if((! _mp->response(_response)) || (_response._type != NODEID_IS)) {
        perror("MAPI: could not get the nodeID");
        _response = EMPTY_RESPONSE;
        return false;
    }
    nodeID = (const char *)(_response._data);
    free(_response._data);//allocated in mapi.cpp:152
    return true;
}
bool MapiAgent::get_nodeid(unsigned int &nodeID) {
    unsigned int length = 0;
    struct response _response;
    string nodeID_str;
    if (! _mp->request(GET_NODEID, length, NULL)) {
        perror("MAPI: could not get the nodeID");
        return false;
    }
    if((! _mp->response(_response)) || (_response._type != NODEID_IS)) {
        perror("MAPI: could not get the nodeID");
        _response = EMPTY_RESPONSE;
        return false;
    }
    nodeID_str = (const char *)(_response._data);
    nodeID_str.erase(0, nodeID_str.find_first_not_of('0'));
    nodeID = atol(nodeID_str.c_str());
    free(_response._data);//allocated in mapi.cpp:152
    return true;
}

bool MapiAgent::get_rvtmfid(Bitvector &FID){
    unsigned int length = 0;
    struct response _response;
    if (! _mp->request(GET_RVTMFID, length, NULL)) {
        perror("MAPI: could not get the RVTM FID");
        return false;
    }
    if((! _mp->response(_response)) || (_response._type != RVTMFID_IS)){
        perror("MAPI: could not get the RVTMFID");
        _response = EMPTY_RESPONSE;
        return false;
    }
    memcpy(FID._data, _response._data, FID_LEN);
    return true;
}

bool MapiAgent::set_connection_status(unsigned char status) {
    char * data = (char *) malloc(sizeof(status));
    memcpy(data, &status, sizeof(status));
    if (! _mp->request(SET_CONNECTION_STATUS, sizeof(data), data)) {
        perror("MAPI: could not set the connection status");
        return false;
    }
    free(data);
    return true;
}
void MapiAgent::disconnect() {
    _mp->disconnect();
    delete _mp;
}
