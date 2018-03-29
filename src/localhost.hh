/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 3 as published by the Free Software Foundation.
 *
 * See LICENSE and COPYING for more details.
 */

#ifndef CLICK_LOCALHOST_HH
#define CLICK_LOCALHOST_HH

#include <../lib/blackadder_enums.hpp>
#include "common.hh"

CLICK_DECLS

/**
 * @brief (Blackadder Core) The LocalHost class represents a local software entity that accesses Blackadder's world using the provided service model.
 * 
 * Such entity can be a user-space application (or a kernel module) as well as other Click Elements (for instance the rendezvous element).
 * @param type the type of the software entity. LOCAL_PROCESS for an application or a kernel module, CLICK_MODULE for a click element.
 * @param id a local identifier of the software entity. It can be the netlink port at which an application or a kerbel module expects data, or the out Click port to the Click Element.
 * @return 
 */
class LocalHost {
public:
    /** @brief Constructor:
     * @param type the type of the software entity. LOCAL_PROCESS for an application or a kernel module, CLICK_MODULE for a click element.
     * @param id a local identifier of the software entity. It can be the netlink port at which an application or a kerbel module expects data, or the out Click port to the Click Element.
     */
    LocalHost(int type, int id);
    /** @brief A Click's String uniquely identifying the LocalHost, by adding an "app", or "click" to the ineteger id. 
     */
    String localHostID;
    /**@brief type the type of the software entity. LOCAL_PROCESS for an application or a kernel module, CLICK_MODULE for a click element.
     */
    int type;
    /**@brief a local identifier of the software entity. It can be the netlink port at which an application or a kerbel module expects data, or the out Click port to the Click Element.
     */
    int id;
    /** @brief A set of String Items identifying ActivePublications. The LocalProxy uses this set.
     */
    StringSet activePublications;
    /** @brief A set of String Items identifying ActiveSubscriptions. The LocalProxy uses this set.
     * 
     */
    StringSet activeSubscriptions;
};

CLICK_ENDDECLS

#endif
