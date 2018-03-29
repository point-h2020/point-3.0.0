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
 * @file mapi.hpp
 * @brief Management API blocking library.
 */

#ifndef MAPI_HPP
#define MAPI_HPP


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <bitvector.hpp>
#include <blackadder_enums.hpp>
#include <ba_fnv.hpp>

using namespace std;

#define MAX_PAYLOAD 1024
/* @brief a wrapper class that exposes a set of function calls that allow applications to 
 *  request blackadder-related information or set configurations / trigger actions according to some changes
 *
 */
class Mapi {
public:
    /**@breif default Constructor that calls Instance method with user_space = true (i.e. running blackadder in user space);
     *
     */
    Mapi();
    /**@breif Distructor that closes the netlink socket
     *
     */
    virtual ~Mapi(){};
    /* @brief a method that creates a MAPI instance when it is called the first time, for any susequent times the method returns a pointer to the existing instance of MAPI
     * Therefore, only a single instance of MAPI is created that runs acorss all applications.
     * The method calls the private constructor
     * @param user_space: ture: MAPI netlink socket communicates with blackadder created in userspace, false: MAPI socket communicates with kernel-level blackadder
     *
     */
    static Mapi* Instance(bool user_space);
    /**@breif disconnect before terminating the application, including: 1- notify blackadder that the Mapp will close, to remove the process from its record
     * 2- close the Mapp socket
     *
     */
    void disconnect();
    /** @brief private method to create the message by stitching together the different fields
     */
    bool request(unsigned char type, unsigned int length, void *data);
    /* @brief process the response sent back from blackadder
     *
     */
    bool response(struct response &_response);

private:
    /* @brief Private Constructor that creates and binds the netlink socket and create the approperiate sockaddr_nl for sending requests to blackadder
     */
    Mapi(bool user_space);
    /**@breif returns MAPI socket file descriptor
     *
     */
    int get_fd();
    /** @brief netlink socket file descriptor
     */
    int mapi_fd;
    /** @brief netlink socket address as random process id
     */
    uint32_t _pid;
    /**@brief the netlink socket destination sockaddr_nl structures, the destination is always blackadder with PID_MAPI.
     */
    struct sockaddr_nl s_nladdr, d_nladdr;
    /**@brief the single static MAPI object an application can access.
     */
    static Mapi* _instance;
};
struct response {
    
    uint8_t _type;
    uint32_t _length;
    void * _data;
};

static const struct response EMPTY_RESPONSE = { UNDEF_RESPONSE, 0, NULL};

extern "C" {
    
    struct mapi_nlmsg {
        uint32_t nlmsg_len;
        uint16_t nlmsg_type;
        uint16_t nlmsg_flags;
        uint32_t nlmsg_seq;
        uint32_t nlmsg_pid;
    };
}
#endif /* BLACKADDER_HPP */

