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

#ifndef CLICK_ACTIVENODE_HH
#define CLICK_ACTIVENODE_HH

#include "common.hh"
#include <click/vector.hh>

CLICK_DECLS

class LocalHost;

/**
 * @brief (Blackadder Core) ActivePublication represents an active publication of an application or click element or another Linux module.
 
 * It can be either a scope or an information item. The LocalProxy only handles active publications.
 */
class ActiveNode {
public:
    /**@brief Constructor: It constructs an ActivePublication object using the provided values.
     * 
     * @param _fullID the full identifier of the published scope or item, which has size of PURSUIT_ID_LEN * number of fragments in the graph. 
     * Therefore it is the identifier starting from a root of the information graph.
     * LocalProxy does not know the information graph. Only RV elements know the graphs for which they are rendezvous points.
     * @param _strategy the dissemination strategy assigned to this scope or item.
     * @param _isScope is it a scope? 
     * @return 
     */
    ActiveNode(String _nodeID);
    /**
     * @brief Destructor: there is nothing dynamically allocated so it is the default destructor
     */
    ~ActiveNode();
    /** @brief The identifier of the scope or information item starting from the root of the information graph
     */
    String nodeID;
	/** @brief A Click HashTable<LocalHost *, unsigned char> mapping each publisher (application or click element) to a START_PUBLISH or STOP_PUBLISH byte.
	 *
	 *  That way the LocalProxy knows which publisher is notified for each active publication. Only one publisher can be notified at a specific time.
	 */
	PublisherHashMap publishers;
    /** @brief The LIPSIN identifier to the node that is the nodeID, of the implicit subscriber.
     * 
     *  It can be the internal Link Identifier is the strategy is NODE_LOCAL or a preconfigured FID to the domain's rendezvous.
     */
    BABitvector FID_to_node;
};

CLICK_ENDDECLS

#endif
