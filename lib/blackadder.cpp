/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
 * Copyright (C) 2015-2018  Mays AL-Naday
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 3 as published by the Free Software Foundation.
 *
 * See LICENSE and COPYING for more details.
 */

#include "blackadder.hpp"

#ifdef __FreeBSD__
#include <sys/event.h>
static int kq = 0;
#endif

Blackadder* Blackadder::m_pInstance = NULL;

Blackadder::Blackadder(bool user_space) {
    int ret;

    if (user_space) {
#if HAVE_USE_NETLINK
        sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
#elif HAVE_USE_UNIX
        sock_fd = socket(PF_LOCAL, SOCK_DGRAM, 0);
#else
        sock_fd = -1;
        errno = EPFNOSUPPORT; /* XXX */
#endif
    } else {
#if HAVE_USE_NETLINK
        sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_BADDER);
#elif __FreeBSD__ /* XXX */
        sock_fd = socket(AF_BLACKADDER, SOCK_RAW, PROTO_BLACKADDER);
#else
        sock_fd = -1;
        errno = EPFNOSUPPORT; /* XXX */
#endif
    }
    if (sock_fd < 0) {
        perror("socket");
    }
#ifdef __APPLE__
    int bufsize = 229376; /* XXX */
    setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
#endif
#ifdef __FreeBSD__
    kq = kqueue();
    if (kq < 0) {
        perror("kqueue");
    }
    struct kevent kev;
    EV_SET(&kev, sock_fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0x0, 0, NULL);
    if (kevent(kq, &kev, 1, NULL, 0, NULL)) {
        perror("kevent");
    }
#endif
    /* source netlink address */
    memset(&s_nladdr, 0, sizeof (s_nladdr));
#if HAVE_USE_NETLINK
    s_nladdr.nl_family = AF_NETLINK;
    s_nladdr.nl_pad = 0;
    s_nladdr.nl_pid = getpid();
    ret = bind(sock_fd, (struct sockaddr *) &s_nladdr, sizeof (s_nladdr));
#elif HAVE_USE_UNIX
    if (user_space) {
# ifndef __linux__
        s_nladdr.sun_len = sizeof (s_nladdr);
# endif
        s_nladdr.sun_family = PF_LOCAL;
        /* XXX: Probably shouldn't use getpid() here. */
        ba_id2path(s_nladdr.sun_path, getpid());
        if (unlink(s_nladdr.sun_path) != 0 && errno != ENOENT)
            perror("unlink");
# ifdef __linux__
        ret = bind(sock_fd, (struct sockaddr *) &s_nladdr, sizeof(s_nladdr));
# else
        ret = bind(sock_fd, (struct sockaddr *) &s_nladdr, SUN_LEN(&s_nladdr));
# endif
    } else {
        if (sock_fd > 0)
            ret = 0;
        else {
            ret = -1;
            errno = EBADF;
        }
    }
#endif
    if (ret < 0) {
        perror("bind");
        exit(0);
    }
    /* destination netlink address */
    memset(&d_nladdr, 0, sizeof (d_nladdr));
#if HAVE_USE_NETLINK
    d_nladdr.nl_family = AF_NETLINK;
    d_nladdr.nl_pad = 0;
    if (user_space) {
        d_nladdr.nl_pid = PID_BLACKADDER; /* destined to user space blackadder */
    } else {
        d_nladdr.nl_pid = 0; /* destined to kernel */
    }
#elif HAVE_USE_UNIX
    if (user_space) {
# ifndef __linux__
        d_nladdr.sun_len = sizeof (d_nladdr);
# endif
        d_nladdr.sun_family = PF_LOCAL;
        ba_id2path(d_nladdr.sun_path, (user_space) ? PID_BLACKADDER : 0);
    }
#endif
}

Blackadder::~Blackadder() {
    if (sock_fd != -1) {
        close(sock_fd);
#if HAVE_USE_UNIX
        unlink(s_nladdr.sun_path);
#endif
#ifdef __FreeBSD__
        close(kq);
#endif
    }
}

Blackadder* Blackadder::Instance(bool user_space) {
    if (!m_pInstance) {
        m_pInstance = new Blackadder(user_space);
    }
    return m_pInstance;
}

int Blackadder::create_and_send_buffers(unsigned char type, const string &id, const string &prefix_id, char strategy, void *str_opt, unsigned int str_opt_len) {
    int ret;
    struct msghdr msg;
    struct iovec iov[8];
    unsigned char id_len = id.length() / PURSUIT_ID_LEN;
    unsigned char prefix_id_len = prefix_id.length() / PURSUIT_ID_LEN;
    struct nlmsghdr _nlh, *nlh = &_nlh;
    memset(&msg, 0, sizeof(msg));
    memset(iov, 0, sizeof(iov));
    memset(nlh, 0, sizeof(*nlh));
    /* Fill the netlink message header */
    nlh->nlmsg_len = sizeof (struct nlmsghdr) + 1 /*type*/ + 1 /*for id length*/ + id.length() + 1 /*for prefix_id length*/ + prefix_id.length() + 1 /*strategy*/;
    if (str_opt != NULL) {
        nlh->nlmsg_len += str_opt_len;
    }
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 1;
    nlh->nlmsg_type = 0;
    iov[0].iov_base = nlh;
    iov[0].iov_len = sizeof (*nlh);
    iov[1].iov_base = &type;
    iov[1].iov_len = sizeof (type);
    iov[2].iov_base = &id_len;
    iov[2].iov_len = sizeof (id_len);
    iov[3].iov_base = (void *) id.c_str();
    iov[3].iov_len = id.length();
    iov[4].iov_base = &prefix_id_len;
    iov[4].iov_len = sizeof (prefix_id_len);
    iov[5].iov_base = (void *) prefix_id.c_str();
    iov[5].iov_len = prefix_id.length();
    iov[6].iov_base = &strategy;
    iov[6].iov_len = sizeof (strategy);
    if (str_opt != NULL) {
        iov[7].iov_base = (void *) str_opt;
        iov[7].iov_len = str_opt_len;
    }
    msg.msg_name = (void *) &d_nladdr;
    msg.msg_namelen = sizeof (d_nladdr);
    msg.msg_iov = iov;
    msg.msg_iovlen = (str_opt == NULL) ? 7 : 8;
    ret = sendmsg(sock_fd, &msg, 0);
    return ret;
}

void Blackadder::disconnect() {
    if (sock_fd == -1) {
        cout << "Blackadder Library: Socket already closed" << endl;
        return;
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
    nlh->nlmsg_pid = getpid();
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
    ret = sendmsg(sock_fd, &msg, 0);
    if (ret < 0) {
        perror("Blackadder Library: Failed to send disconnection message");
    }
    close(sock_fd);
#if HAVE_USE_UNIX
    unlink(s_nladdr.sun_path);
#endif
    sock_fd = -1;
}

void Blackadder::publish_scope(const string&id, const string&prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    int ret;
    if (id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send PUBLISH_SCOPE request - wrong ID size" << endl;
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send PUBLISH_SCOPE request - wrong prefix_id size" << endl;
    } else if (id.length() == 0) {
        cout << "Blackadder Library: Could not send PUBLISH_SCOPE request - id cannot be empty" << endl;
    } else {
        ret = create_and_send_buffers(PUBLISH_SCOPE, id, prefix_id, strategy, str_opt, str_opt_len);
        if (ret < 0) {
            perror("Blackadder Library: Could not send PUBLISH_SCOPE request");
        }
    }
}

void Blackadder::publish_info(const string&id, const string&prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    int ret;
    if (id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send PUBLISH_INFO request - wrong ID size" << endl;
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send PUBLISH_INFO request - wrong prefix_id size" << endl;
    } else if (prefix_id.length() == 0) {
        cout << "Blackadder Library: Could not send PUBLISH_INFO request - prefix_id cannot be empty" << endl;
    } else if (prefix_id.length() == 0) {
        cout << "Blackadder Library: Could not send PUBLISH_INFO request - prefix_id cannot be empty" << endl;
    } else {
        ret = create_and_send_buffers(PUBLISH_INFO, id, prefix_id, strategy, str_opt, str_opt_len);
        if (ret < 0) {
            perror("Blackadder Library: Could not send PUBLISH_INFO request");
        }
    }
}

void Blackadder::unpublish_scope(const string&id, const string&prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    int ret;
    if (id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send UNPUBLISH_SCOPE request - wrong ID size" << endl;
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send UNPUBLISH_SCOPE request - wrong prefix_id size" << endl;
    } else if (id.length() == 0) {
        cout << "Blackadder Library: Could not send UNPUBLISH_SCOPE request - id cannot be empty" << endl;
    } else if (id.length() / PURSUIT_ID_LEN > 1) {
        cout << "Blackadder Library: Could not send UNPUBLISH_SCOPE request - id cannot consist of multiple fragments" << endl;
    } else {
        ret = create_and_send_buffers(UNPUBLISH_SCOPE, id, prefix_id, strategy, str_opt, str_opt_len);
        if (ret < 0) {
            perror("Blackadder Library: Could not send UNPUBLISH_SCOPE request");
        }
    }
}

void Blackadder::unpublish_info(const string&id, const string&prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    int ret;
    if (id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send UNPUBLISH_INFO request - wrong ID size" << endl;
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send UNPUBLISH_INFO request - wrong prefix_id size" << endl;
    } else if (id.length() == 0) {
        cout << "Blackadder Library: Could not send UNPUBLISH_INFO request - id cannot be empty" << endl;
    } else if (prefix_id.length() == 0) {
        cout << "Blackadder Library: Could not send UNPUBLISH_INFO request - prefix_id cannot be empty" << endl;
    } else if (id.length() / PURSUIT_ID_LEN > 1) {
        cout << "Blackadder Library: Could not send UNPUBLISH_INFO request - id cannot consist of multiple fragments" << endl;
    } else {
        ret = create_and_send_buffers(UNPUBLISH_INFO, id, prefix_id, strategy, str_opt, str_opt_len);
        if (ret < 0) {
            perror("Blackadder Library: Could not send UNPUBLISH_INFO request");
        }
    }
}

void Blackadder::subscribe_scope(const string&id, const string&prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    int ret;
    if (id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send SUBSCRIBE_SCOPE request - wrong ID size" << endl;
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send SUBSCRIBE_SCOPE request - wrong prefix_id size" << endl;
    } else if (id.length() == 0) {
        cout << "Blackadder Library: Could not send SUBSCRIBE_SCOPE request - id cannot be empty" << endl;
    } else if (id.length() / PURSUIT_ID_LEN > 1) {
        cout << "Blackadder Library: Could not send SUBSCRIBE_SCOPE request - id cannot consist of multiple fragments" << endl;
    } else {
        ret = create_and_send_buffers(SUBSCRIBE_SCOPE, id, prefix_id, strategy, str_opt, str_opt_len);
        if (ret < 0) {
            perror("Blackadder Library: Could not send SUBSCRIBE_SCOPE request");
        }
    }
}

void Blackadder::subscribe_info(const string&id, const string&prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    int ret;
    if (id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send SUBSCRIBE_INFO request - wrong ID size" << endl;
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send SUBSCRIBE_INFO request - wrong prefix_id size" << endl;
    } else if (id.length() == 0) {
        cout << "Blackadder Library: Could not send SUBSCRIBE_INFO request - id cannot be empty" << endl;
    } else if (prefix_id.length() == 0) {
        cout << "Blackadder Library: Could not send SUBSCRIBE_INFO request - prefix_id cannot be empty" << endl;
    } else if (id.length() / PURSUIT_ID_LEN > 1) {
        cout << "Blackadder Library: Could not send SUBSCRIBE_INFO request - id cannot consist of multiple fragments" << endl;
    } else {
        ret = create_and_send_buffers(SUBSCRIBE_INFO, id, prefix_id, strategy, str_opt, str_opt_len);
        if (ret < 0) {
            perror("Blackadder Library: Could not send SUBSCRIBE_INFO request");
        }
    }
}

void Blackadder::unsubscribe_scope(const string&id, const string&prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    int ret;
    if (id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send UNSUBSCRIBE_SCOPE request - wrong ID size" << endl;
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send UNSUBSCRIBE_SCOPE request - wrong prefix_id size" << endl;
    } else if (id.length() == 0) {
        cout << "Blackadder Library: Could not send UNSUBSCRIBE_SCOPE request - id cannot be empty" << endl;
    } else if (id.length() / PURSUIT_ID_LEN > 1) {
        cout << "Blackadder Library: Could not send UNSUBSCRIBE_SCOPE request - id cannot consist of multiple fragments" << endl;
    } else {
        ret = create_and_send_buffers(UNSUBSCRIBE_SCOPE, id, prefix_id, strategy, str_opt, str_opt_len);
        if (ret < 0) {
            perror("Blackadder Library: Could not send UNSUBSCRIBE_SCOPE request");
        }
    }
}

void Blackadder::unsubscribe_info(const string&id, const string&prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    int ret;
    if (id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send UNSUBSCRIBE_INFO request - wrong ID size" << endl;
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send UNSUBSCRIBE_INFO request - wrong prefix_id size" << endl;
    } else if (id.length() == 0) {
        cout << "Blackadder Library: Could not send UNSUBSCRIBE_INFO request - id cannot be empty" << endl;
    } else if (prefix_id.length() == 0) {
        cout << "Blackadder Library: Could not send UNSUBSCRIBE_INFO request - prefix_id cannot be empty" << endl;
    } else if (id.length() / PURSUIT_ID_LEN > 1) {
        cout << "Blackadder Library: Could not send UNSUBSCRIBE_INFO request - id cannot consist of multiple fragments" << endl;
    } else {
        ret = create_and_send_buffers(UNSUBSCRIBE_INFO, id, prefix_id, strategy, str_opt, str_opt_len);
        if (ret < 0) {
            perror("Blackadder Library: Could not send UNSUBSCRIBE_INFO request");
        }
    }
}

void Blackadder::publish_data(const string&id, unsigned char strategy, void *str_opt, unsigned int str_opt_len, void *data, unsigned int data_len) {
    int ret;
    struct msghdr msg;
    struct iovec iov[7];
    memset(&msg, 0, sizeof(msg));
    memset(iov, 0, sizeof(iov));
    if (id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send  - wrong ID size" << endl;
    } else {
        unsigned char type = PUBLISH_DATA;
        unsigned char id_len = id.length() / PURSUIT_ID_LEN;
        struct nlmsghdr _nlh, *nlh = &_nlh;
        memset(nlh, 0, sizeof(*nlh));
        /* Fill the netlink message header */
        nlh->nlmsg_len = sizeof (struct nlmsghdr) + 1 /*type*/ + 1 /*for id length*/ + id.length() + sizeof (strategy);
        if (str_opt != NULL) {
            nlh->nlmsg_len += str_opt_len;
        }
        nlh->nlmsg_len += data_len;
        nlh->nlmsg_pid = getpid();
        nlh->nlmsg_flags = 1;
        nlh->nlmsg_type = 0;
        iov[0].iov_base = nlh;
        iov[0].iov_len = sizeof (*nlh);
        iov[1].iov_base = &type;
        iov[1].iov_len = sizeof (type);
        iov[2].iov_base = &id_len;
        iov[2].iov_len = sizeof (id_len);
        iov[3].iov_base = (void *) id.c_str();
        iov[3].iov_len = id.length();
        iov[4].iov_base = (void *) &strategy;
        iov[4].iov_len = sizeof (strategy);
        if (str_opt == NULL) {
            iov[5].iov_base = (void *) data;
            iov[5].iov_len = data_len;
            iov[6].iov_base = NULL;
            iov[6].iov_len = 0;
        } else {
            iov[5].iov_base = (void *) str_opt;
            iov[5].iov_len = str_opt_len;
            iov[6].iov_base = (void *) data;
            iov[6].iov_len = data_len;
        }
        msg.msg_name = (void *) &d_nladdr;
        msg.msg_namelen = sizeof (d_nladdr);
        msg.msg_iov = iov;
        msg.msg_iovlen = (str_opt == NULL) ? 6 : 7;
        ret = sendmsg(sock_fd, &msg, 0);
        }
        if (ret < 0) {
            perror("Blackadder Library: Failed to publish data ");
        }
}
bool Blackadder::publish_data(const string&id,
                              unsigned char strategy,
                              void *str_opt,
                              unsigned int str_opt_len,
                              std::list <string> &nodeIds,
                              void * data,
                              unsigned int data_len
                              ) {
    int ret;
    struct msghdr msg;
    struct iovec iov[9];
    memset(&msg, 0, sizeof(msg));
    memset(iov, 0, sizeof(iov));
    if (id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send  - wrong ID size" << endl;
    } else {
        unsigned char type = PUBLISH_DATA_iMULTICAST;
        unsigned char id_len = id.length() / PURSUIT_ID_LEN;
        unsigned char nodeIdListSize = nodeIds.size();
        string nodeIdsStream;
        struct nlmsghdr _nlh, *nlh = &_nlh;
        memset(nlh, 0, sizeof(*nlh));
        /* Fill the netlink message header */
        nlh->nlmsg_len = sizeof (struct nlmsghdr) + 1 /*type*/ + 1 /*for id length*/ + id.length() + sizeof (strategy) + sizeof(nodeIdListSize) + nodeIdListSize * NODEID_LEN;
        if (str_opt != NULL) {
            nlh->nlmsg_len += str_opt_len;
        }
        for (std::list<string>::iterator it = nodeIds.begin(); it != nodeIds.end(); ++it) {
            nodeIdsStream += (*it);
        }
        nlh->nlmsg_len += data_len;
        nlh->nlmsg_pid = getpid();
        nlh->nlmsg_flags = 1;
        nlh->nlmsg_type = 0;
        iov[0].iov_base = nlh;
        iov[0].iov_len = sizeof (*nlh);
        iov[1].iov_base = &type;
        iov[1].iov_len = sizeof (type);
        iov[2].iov_base = &id_len;
        iov[2].iov_len = sizeof (id_len);
        iov[3].iov_base = (void *) id.c_str();
        iov[3].iov_len = id.length();
        iov[4].iov_base = (void *) &strategy;
        iov[4].iov_len = sizeof (strategy);
        if (str_opt == NULL) {
            iov[5].iov_base = &nodeIdListSize;
            iov[5].iov_len = sizeof (nodeIdListSize);
            iov[6].iov_base = (void *) nodeIdsStream.c_str();
            iov[6].iov_len = nodeIdListSize * NODEID_LEN;
            iov[7].iov_base = (void *) data;
            iov[7].iov_len = data_len;
            iov[8].iov_base = NULL;
            iov[8].iov_len = 0;
        } else {
            iov[5].iov_base = (void *) str_opt;
            iov[5].iov_len = str_opt_len;
            iov[6].iov_base = &nodeIdListSize;
            iov[6].iov_len = sizeof (nodeIdListSize);
            iov[7].iov_base = (void *) nodeIdsStream.c_str();
            iov[7].iov_len = nodeIdListSize * NODEID_LEN;
            iov[8].iov_base = (void *) data;
            iov[8].iov_len = data_len;
        }
        msg.msg_name = (void *) &d_nladdr;
        msg.msg_namelen = sizeof (d_nladdr);
        msg.msg_iov = iov;
        msg.msg_iovlen = (str_opt == NULL) ? 8 : 9;
        ret = sendmsg(sock_fd, &msg, 0);
    }
    if (ret < 0) {
        perror("Blackadder Library: Failed to publish data ");
    }
    return ret;
}
bool Blackadder::publish_data_isub(const string&id,
                                   unsigned char strategy,
                                   void *str_opt,
                                   unsigned int str_opt_len,
                                   const string &isubID,
                                   void * data,
                                   unsigned int data_len
                                   ) {
    int ret;
    struct msghdr msg;
    struct iovec iov[9];
    memset(&msg, 0, sizeof(msg));
    memset(iov, 0, sizeof(iov));
    if (id.length() % PURSUIT_ID_LEN != 0) {
        cout << "Blackadder Library: Could not send  - wrong ID size" << endl;
    } else {
        unsigned char type = PUBLISH_DATA_iSUB;
        unsigned char id_len = id.length() / PURSUIT_ID_LEN;
        unsigned char isubID_len = isubID.length() / PURSUIT_ID_LEN;
        struct nlmsghdr _nlh, *nlh = &_nlh;
        memset(nlh, 0, sizeof(*nlh));
        /* Fill the netlink message header */
        nlh->nlmsg_len = sizeof (struct nlmsghdr) + 1 /*type*/ + 1 /*for id length*/ + id.length() + sizeof (strategy) + sizeof (isubID_len) + (int) isubID_len * PURSUIT_ID_LEN;
        if (str_opt != NULL) {
            nlh->nlmsg_len += str_opt_len;
        }
        nlh->nlmsg_len += data_len;
        nlh->nlmsg_pid = getpid();
        nlh->nlmsg_flags = 1;
        nlh->nlmsg_type = 0;
        iov[0].iov_base = nlh;
        iov[0].iov_len = sizeof (*nlh);
        iov[1].iov_base = &type;
        iov[1].iov_len = sizeof (type);
        iov[2].iov_base = &id_len;
        iov[2].iov_len = sizeof (id_len);
        iov[3].iov_base = (void *) id.c_str();
        iov[3].iov_len = id.length();
        iov[4].iov_base = (void *) &strategy;
        iov[4].iov_len = sizeof (strategy);
        if (str_opt == NULL) {
            /*hash(URL) coming with iSUB*/
            iov[5].iov_base = &isubID_len;
            iov[5].iov_len = sizeof (isubID_len);
            iov[6].iov_base = (void *) isubID.c_str();
            iov[6].iov_len = isubID_len * PURSUIT_ID_LEN;
            iov[7].iov_base = (void *) data;
            iov[7].iov_len = data_len;
            iov[8].iov_base = NULL;
            iov[8].iov_len = 0;
        } else {
            iov[5].iov_base = (void *) str_opt;
            iov[5].iov_len = str_opt_len;
            /*hash(URL) coming with iSUB*/
            iov[6].iov_base = &isubID_len;
            iov[6].iov_len = sizeof (isubID_len);
            iov[7].iov_base = (void *) isubID.c_str();
            iov[7].iov_len = isubID_len * PURSUIT_ID_LEN;
            iov[8].iov_base = (void *) data;
            iov[8].iov_len = data_len;
        }
        msg.msg_name = (void *) &d_nladdr;
        msg.msg_namelen = sizeof (d_nladdr);
        msg.msg_iov = iov;
        msg.msg_iovlen = (str_opt == NULL) ? 8 : 9;
        ret = sendmsg(sock_fd, &msg, 0);
    }
    if (ret < 0) {
        perror("Blackadder Library: Failed to publish data ");
    }
    return ret;
}
void Blackadder::getEvent(Event &ev) {
    return getEventIntoBuf(ev, NULL, 0);
}
/*the event object should be already allocated*/
void Blackadder::getEventIntoBuf(Event &ev, void *data, unsigned int data_len) {
    int total_buf_size = 0;
    int bytes_read;
    unsigned char id_len;
    unsigned char isubID_len;
    struct msghdr msg;
    struct iovec iov;
    unsigned char *ptr = NULL;
    memset(&msg, 0, sizeof (msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    iov.iov_base = fake_buf;
    iov.iov_len = 1;
#ifdef __linux__
    total_buf_size = recvmsg(sock_fd, &msg, MSG_PEEK | MSG_TRUNC);
#else
# ifdef __APPLE__
    socklen_t _option_len = sizeof(total_buf_size);
    if (recvmsg(sock_fd, &msg, MSG_PEEK) < 0 || getsockopt(sock_fd, SOL_SOCKET, SO_NREAD, &total_buf_size, &_option_len) < 0)
# elif defined(__FreeBSD__)
    struct kevent kev;
    if (kevent(kq, NULL, 0, &kev, 1, NULL) < 0 || (total_buf_size = kev.data/*XXX*/) < 0)
# else
    if (recvmsg(sock_fd, &msg, MSG_PEEK) < 0 || ioctl(sock_fd, FIONREAD, &total_buf_size) < 0)
# endif
    {
        cout << "recvmsg/ioctl: " << errno << endl;
        total_buf_size = -1;
    }
#endif
    if (total_buf_size > 0) {
        if (data == NULL) {
            iov.iov_base = malloc(total_buf_size);
            if (!iov.iov_base) {
                ev.type = UNDEF_EVENT;
                return;
            }
            iov.iov_len = total_buf_size;
        } else {
            iov.iov_base = data;
            iov.iov_len = data_len;
        }
        bytes_read = recvmsg(sock_fd, &msg, 0);
        if (bytes_read < 0) {
            //perror("recvmsg for data");
            free(iov.iov_base);
            ev.type = UNDEF_EVENT;
            return;
        }
        if (bytes_read < sizeof(struct nlmsghdr)) {
            cout << "read " << bytes_read << " bytes, not enough" << endl;
            free(iov.iov_base);
            ev.type = UNDEF_EVENT;
            return;
        }
        ev.buffer = iov.iov_base;
        ptr = (unsigned char *)ev.buffer + sizeof(struct nlmsghdr);
        ev.type = *ptr; ptr += sizeof(ev.type);
        id_len  = *ptr; ptr += sizeof(id_len);
        ev.id = string((char *)ptr, ((int)id_len) * PURSUIT_ID_LEN);
        ptr += ((int)id_len) * PURSUIT_ID_LEN;
        if (ev.type == PUBLISHED_DATA) {
            ev.data = (void *)ptr;
            ev.data_len = bytes_read - (ptr - (unsigned char *)ev.buffer);
        } else if (ev.type == PUBLISHED_DATA_iSUB){
            ev.nodeId = string((char *)ptr, NODEID_LEN);
            ptr += NODEID_LEN;
            isubID_len = *ptr;
            ptr += sizeof (isubID_len);
            ev.isubID = string((char *) ptr, ((int)isubID_len) * PURSUIT_ID_LEN);
            ptr += ((int) isubID_len) * PURSUIT_ID_LEN;
            ev.data = (void *)ptr;
            ev.data_len = bytes_read - (ptr - (unsigned char *)ev.buffer);
        } else {
            ev.data = NULL;
            ev.data_len = 0;
        }
    } else if (errno == EINTR) {
        /* Interrupted system call. */
        ev.type = UNDEF_EVENT;
        return;
    } else {
        ev.type = UNDEF_EVENT;
        return;
    }
}

Event::Event()
: type(0), id(), nodeId(), isubID(), data(NULL), data_len(0), buffer(NULL) {
}

Event::Event(Event &ev) {
    type = ev.type;
    id = ev.id;
    data_len = ev.data_len;
    nodeId = ev.nodeId;
    isubID = ev.isubID;
    buffer = malloc(sizeof (struct nlmsghdr) + sizeof (type) + sizeof (unsigned char) + id.length() + sizeof (nodeId) + sizeof (unsigned char) + isubID.length() + data_len);
    memcpy(buffer, ev.buffer, sizeof (struct nlmsghdr) + sizeof (type) + sizeof (unsigned char) + id.length() + sizeof (nodeId) + sizeof (unsigned char) + isubID.length() + data_len);
    data = (char *) buffer + sizeof (struct nlmsghdr) + sizeof (type) + sizeof (unsigned char) + id.length() + sizeof (nodeId) + sizeof(unsigned char) + isubID.length();
}

Event::~Event() {
    if (buffer != NULL) {
        free(buffer);
    }
}

string get_chararray_sid(const string &icnid, unsigned int &depth) {
    string sid;
    int id_len = icnid.length() / PURSUIT_ID_LEN;
    /*modify depth in case it is larger than the SID*/
    if (depth > (id_len -1)) {
        depth = id_len -1;
    }
    sid = string(icnid, 0, depth * PURSUIT_ID_LEN);
    return sid;
}

string hex_to_chararray(const string &hexstr) {
    vector<unsigned char> bytes = vector<unsigned char>();
    for (string::size_type i = 0; i < hexstr.size() / 2; ++i) {
        istringstream iss(hexstr.substr(i * 2, 2));
        unsigned int n;
        iss >> hex >> n;
        bytes.push_back(static_cast<unsigned char> (n));
    }
    bytes.data();
    string result = string((const char *) bytes.data(), bytes.size());
    return result;
}

string chararray_to_hex(const string &str) {
    ostringstream oss;
    for (string::size_type i = 0; i < str.size(); ++i) {
        if ((unsigned short) (unsigned char) str[i] > 15) {
            oss << hex << (unsigned short) (unsigned char) str[i];
        } else {
            oss << hex << '0';
            oss << hex << (unsigned short) (unsigned char) str[i];
        }
    }
    return oss.str();
}

vector<int> split_int_string(string &input_str, char delim_char) {
    stringstream ss(input_str);
    string str_token;
    vector<int> result;
    
    while (getline(ss, str_token, delim_char)) {
        if (str_token != "") {
            int tmp_item;
            stringstream(str_token) >> tmp_item;
            result.push_back(tmp_item);
        }
    }
    
    return(result);
}
