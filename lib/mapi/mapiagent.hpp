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
 * @file agent.hpp
 * @brief Agent of the Management API blocking library.
 */

#ifndef MAPI_AGENT_HPP
#define MAPI_AGENT_HPP

#include "mapi.hpp"

using namespace std;

/**@brief Agent child class of Mapi, providing a unique PID for each running Mapp and a set of Get/Set blocking methods
 * the agent also registers async callback to get updates from blackadder when management information are updated
 */
class MapiAgent{
public:
    /**@brief Default Constructor
     *
     */
    MapiAgent();
    /**@brief Denstructor
     *
     */
    ~MapiAgent();
    /**@breif create a new Mapi Instance or get set a pointer to existing Mapi instance
     */
    void set_MapiInstance(bool user_space);
    /* @brief a blocking method for retrieving the nodeID in - Char Array format - from blackadder
     *
     * @return true process complete successfuly, false the process failed
     */
    bool get_nodeid(string &nodeID);
    /* @brief a blocking method for retrieving the nodeID in - Integer format - from blackadder
     *
     * @return true process complete successfuly, false the process failed
     */
    bool get_nodeid(unsigned int &nodeID);
    /* @brief a blocking method for retrieving the RVTM_FID from blackadder
     *
     * @return true process complete successfuly, false the process failed
     */
     bool get_rvtmfid(Bitvector &FID);
    /* @brief a blocking method for setting the connection status of the node. The status indicates whether the node is connected to the network or
     * the nodes is disconnected from the network, possibly due to link or node failure
     *
     * @param status defines the connection status of the node (CONNECTED/DISCONNECTED)
     * @return true process complete successfuly, false the process failed
     */
    bool set_connection_status(unsigned char status);
    /**@breif notify blackadder of the Mapp termination and disconnect MAPI socket
     *
     */
    void disconnect();
private:
    static Mapi * _mp;
};

#endif
