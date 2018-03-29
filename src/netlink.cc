/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
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

#include "netlink.hh"
#if !CLICK_LINUXMODULE && !CLICK_BSDMODULE
#include <fcntl.h>
#include <unistd.h>
#endif

CLICK_DECLS

#if CLICK_LINUXMODULE
Netlink::Netlink() : nl_sk(NULL) {
}
#else
Netlink::Netlink() {
}
#endif

Netlink::~Netlink() {
    click_chatter("Netlink: destroyed!");
}

int Netlink::initialize(ErrorHandler */*errh*/) {
#if !CLICK_LINUXMODULE && !CLICK_BSDMODULE
    int ret;
    int management_ret = 0;
#if HAVE_USE_NETLINK
    struct sockaddr_nl s_nladdr;
    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    /*mfhaln: socket for management
     */
    struct sockaddr_nl management_s_nladdr;
    management_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);

#elif HAVE_USE_UNIX
    fd = socket(PF_LOCAL, SOCK_DGRAM, 0);
    /*mfhaln: socket for management
     */
    management_fd = socket(PF_LOCAL, SOCK_DGRAM, 0);
# ifdef __APPLE__
    int bufsize = 229376; /* XXX */
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    /*mfhaln: socket for management
     */
    setsockopt(management_fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    setsockopt(management_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));

# endif
#endif
    //fcntl(fd, O_NONBLOCK);
    /* source address */
    memset(&s_nladdr, 0, sizeof (s_nladdr));
    /*mfhaln: socket for management
     */
    memset(&management_s_nladdr, 0, sizeof (management_s_nladdr));
    
#if HAVE_USE_NETLINK
    s_nladdr.nl_family = AF_NETLINK;
    s_nladdr.nl_pad = 0;
    s_nladdr.nl_pid = 9999;
    ret = bind(fd, (struct sockaddr*) &s_nladdr, sizeof (s_nladdr));
    /*mfhaln: socket for management
     */
    management_s_nladdr.nl_family = AF_NETLINK;
    management_s_nladdr.nl_pad = 0;
    management_s_nladdr.nl_pid = PID_MAPI;
    ret = bind(management_fd, (struct sockaddr*) &management_s_nladdr, sizeof (management_s_nladdr));

#elif HAVE_USE_UNIX
# ifndef __linux__
    s_nladdr.sun_len = sizeof (s_nladdr);
    /*mfhaln: socket for management
     */
    management_s_nladdr.sun_len = sizeof (management_s_nladdr);
# endif
    s_nladdr.sun_family = PF_LOCAL;
    ba_id2path(s_nladdr.sun_path, 9999); /* XXX */
    if (unlink(s_nladdr.sun_path) != 0 && errno != ENOENT)
        perror("unlink");
    /*mfhaln: socket for management
     */
    management_s_nladdr.sun_family = PF_LOCAL;
    ba_id2path(management_s_nladdr.sun_path, PID_MAPI);
    if (unlink(management_s_nladdr.sun_path) != 0 && errno != ENOENT)
        perror("unlink management");

# ifdef __linux__
    ret = bind(fd, (struct sockaddr *) &s_nladdr, sizeof(s_nladdr));
    /*mfhaln: socket for management
     */
    management_ret = bind(management_fd, (struct sockaddr *) &management_s_nladdr, sizeof(management_s_nladdr));
# else
    ret = bind(fd, (struct sockaddr *) &s_nladdr, SUN_LEN(&s_nladdr));
    /*mfhaln: socket for management
     */
    management_ret = bind(management_fd, (struct sockaddr *) &management_s_nladdr, SUN_LEN(&management_s_nladdr));
# endif
#endif
    if (ret == -1) {
        perror("bind: ");
        return -1;
    }
    /*mfhaln: socket for management
     */
    if(management_ret == -1) {
        perror("bind for management: ");
        return -1;
    }
#endif /* !CLICK_LINUXMODULE && !CLICK_BSDMODULE */
    return 0;
}

void Netlink::cleanup(CleanupStage stage) {
    if (stage >= CLEANUP_INITIALIZED) {
#if CLICK_LINUXMODULE
        /*release the socket*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
        if (nl_sk != NULL)
            sock_release(nl_sk->sk_socket);
#else
        netlink_kernel_release(nl_sk);
#endif
#elif CLICK_BSDMODULE
        ba_socket_cleanup();
#else
        close(fd);
        /*mfhaln: socket for management
         */
        close(management_fd);
#if HAVE_USE_UNIX
        unlink(s_nladdr.sun_path);
        /*mfhaln: socket for management
         */
        unlink(management_s_nladdr.sun_path);
#endif
#endif
    }
    click_chatter("Netlink: Cleaned up!");
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Netlink)
