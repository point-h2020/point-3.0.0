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
 * @file mapi.cpp
 * @brief Management API blocking library.
 */

#include "mapi.hpp"

Mapi* Mapi::_instance = NULL;

Mapi::Mapi(bool user_space) {
    /*set the seed for the random number generator*/
    srand(time(NULL));
    int ret;
    if (user_space) {
        mapi_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
        if( mapi_fd < 0) {
            errno = EPFNOSUPPORT;
            perror("MAPI socket cannot be created");
        }
        cout << "MAPI: file descriptor: " << mapi_fd << endl;
    } else {
        cout << "MAPI is not supported with kernel-level Blackadder" << endl;
        exit(0);
    }
#ifdef __APPLE__
    int bufsize = 229376;
    setsockopt(mapi_fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    setsockopt(mapi_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
#endif
    /*set the source adddress of the netlink socket*/
    memset(&s_nladdr, 0, sizeof (s_nladdr));
    /*defined the socket params*/
    s_nladdr.nl_family = AF_NETLINK;
    s_nladdr.nl_pad = 0;
    _pid = getpid() + (rand() % 100 + 5) ;
    s_nladdr.nl_pid = _pid;
    ret = bind(mapi_fd, (struct sockaddr *) &s_nladdr, sizeof (s_nladdr));
    if (ret < 0) {
        perror("MAPI could not bind to application for the first time, try every second");
        cout << "PID first attempted is: " << s_nladdr.nl_pid << endl;
        while (ret < 0) {
            sleep(1);
            _pid = getpid() + (rand() % 100 + 5) ;
            s_nladdr.nl_pid = _pid;
            ret = bind(mapi_fd, (struct sockaddr *) &s_nladdr, sizeof (s_nladdr));
        }
    }
    cout << "MAPI: bound to PID: " << s_nladdr.nl_pid << endl;
    /*set the destination socket address*/
    memset(&d_nladdr, 0, sizeof (d_nladdr));
    /*define socket params*/
    d_nladdr.nl_family = AF_NETLINK;
    d_nladdr.nl_pad = 0;
    d_nladdr.nl_pid = PID_MAPI;
}

/*accessable method to either construct new MAPI object if non exists, or return a pointer to the existing MAPI object*/
Mapi* Mapi::Instance(bool user_space) {
    if (! user_space) {
        cout << "MAPI is not supported with kernel-level Blackadder" << endl;
        exit(0);
    }
    if (! _instance) {
        _instance = new Mapi(user_space);
    }
    return _instance;
}
/*create and send a MAPI message to blackadder, requesting information or asking to set a configurtion*/
bool Mapi::request(unsigned char type, unsigned int length, void *data) {
    int ret;
    struct msghdr request;
    struct iovec feilds[3];
    struct mapi_nlmsg _nlheader;
    struct mapi_nlmsg *nlheader = &_nlheader;
    /*set the memory of the message*/
    memset(&request, 0, sizeof (request));
    memset(&feilds, 0, sizeof (feilds));
    memset(nlheader, 0, sizeof (_nlheader));
    /*fill the netlink header length*/
    nlheader->nlmsg_len = sizeof (_nlheader) + 1 /*type*/ + length /*value length*/;
    nlheader->nlmsg_pid = _pid;
    nlheader->nlmsg_flags = 1;
    /*standard nlmsg type: 0 == ignore this type*/
    nlheader->nlmsg_type = 0;
    feilds[0].iov_base = nlheader;
    feilds[0].iov_len = sizeof (_nlheader);
    feilds[1].iov_base = &type;
    feilds[1].iov_len = sizeof (type);
    /*length may be 0 (i.e. no value) for GET type of messages*/
    if (length > 0) {
        feilds[2].iov_base = (void *) data;
        feilds[2].iov_len = length;
    }
    request.msg_name = (void *) &d_nladdr;
    request.msg_namelen = sizeof (d_nladdr);
    request.msg_iov = feilds;
    request.msg_iovlen = (length > 0) ? 3 : 2;
    ret = sendmsg(mapi_fd, &request, 0);
    if (ret < 0) {
        perror("MAPI couldn't send the message");
        cout << "MAPI: file descriptor: " << mapi_fd << endl;
        return false;
    }
    return true;
}
bool Mapi::response(struct response &_response) {
    unsigned int response_length;
    unsigned char *offset = NULL;
    struct nlmsghdr *nlh;
    struct iovec iov;
    struct msghdr response_msg;
    /*set the netlink msg*/
    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = _pid;
    nlh->nlmsg_flags = 0;
    /*set the socket msg */
    memset(&response_msg, 0, sizeof (response_msg));
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    response_msg.msg_iov = &iov;
    response_msg.msg_iovlen = 1;
//    cout << "MAPI: about to recieve message for pid" << nlh->nlmsg_pid << endl;
    response_length = recvmsg(mapi_fd, &response_msg, 0);
    if (response_length <= 0) {
        cout << "MAPI: recieved response of length " << response_length << endl;
        return false;
    }
    offset = (unsigned char *) NLMSG_DATA(nlh);
    _response._type = *offset;
    offset += sizeof(_response._type);
    _response._length = *offset;
//    cout << "MAPI: recieved response of length " << _response._length << endl;
    offset += sizeof(_response._length);
    _response._data = malloc(_response._length);//freed in mapiagent.ccp:66
    memcpy(_response._data, offset, _response._length);
//    cout << "MAPI: recieved response data: " << chararray_to_hex((const char *)_response._data) << endl;
    free(nlh);
    return true;
}
void Mapi::disconnect() {
    if (mapi_fd == -1) {
        cout << "MAPI: Socket already closed" << endl;
    }
    int ret;
    struct msghdr msg;
    struct iovec iov[2];
    struct nlmsghdr _nlh, *nlh = &_nlh;
    memset(&msg, 0, sizeof(msg));
    memset(iov, 0, sizeof(iov));
    memset(nlh, 0, sizeof(*nlh));
    unsigned char type = DISCONNECT;
    /* Fill the netlink message header */
    nlh->nlmsg_len = sizeof (struct nlmsghdr) + 1 /*type*/;
    nlh->nlmsg_pid = _pid;
    nlh->nlmsg_flags = 1;
    nlh->nlmsg_type = 0;
    iov[0].iov_base = nlh;
    iov[0].iov_len = sizeof (*nlh);
    iov[1].iov_base = &type;
    iov[1].iov_len = sizeof (type);
    msg.msg_name = (void *) &d_nladdr;
    msg.msg_namelen = sizeof (d_nladdr);
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;
    ret = sendmsg(mapi_fd, &msg, 0);
    if (ret < 0) {
        perror("MAPI: Failed to send disconnection message");
    }
    close(mapi_fd);
    mapi_fd = -1;
}
int Mapi::get_fd() {
    return mapi_fd;
}
