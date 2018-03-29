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

#ifndef CLICK_GLOBALCONF_HH
#define CLICK_GLOBALCONF_HH

#include <click/config.h>
#include <click/element.hh>
#include <click/confparse.hh>
#include <click/error.hh>

#include <../lib/blackadder_enums.hpp>
#include "common.hh"

CLICK_DECLS

/**@brief (Blackadder Core) GlobalConf is a Click Element that contains information that need to be shared across multiple Blackadder Elements.
 * 
 * See the configure method for a detailed description of this information.
 *
 */
class GlobalConf : public Element {
public:
    /** 
     * @brief @brief Constructor: it does nothing - as Click suggests
     */
    GlobalConf();
    /**
     * @brief Destructor: it does nothing - as Click suggests
     */
    ~GlobalConf();

    /**
     * @brief the class name - required by Click
     * @return 
     */
    const char *class_name() const {return "GlobalConf";}
    /**
     * @brief the port count - required by Click - GlobalConf has no ports. It is passed as an argument to other elements so that some global information can be shared,
     */
    const char *port_count() const {return "0/0";}
    /**
     * @brief an AGNOSTIC Element.
     * @return AGNOSTIC
     */
    const char *processing() const {return AGNOSTIC;}
    /**
     * @brief GlobalConf configuration...see details.
     * 
     * This Configuration is shared across multiple Blackadder's elements, like the LocalProxy, LocalRV and the Forwarder.
     * 
     * MODE: a String representing the mode in which Blackadder will overlay. It is "mac" and "ip" for overlaying on top of Ethernet and raw IP sockets, respectively.
     * 
     * NODEID: The (statistically unique) node label.
     * 
     * DEFAULTRV: a String representing the LIPSIN identifier to the domain's rendezvous node. It is wrapped to a BitVector.
     * 
     * iLID:      a String representing the internal Link Identifier. It is wrapped to a BitVector.
     * 
     * TMFID:     a String representing the LIPSIN identifier to the domain's Topology Manager. It is wrapped to a BitVector.
     */
    int configure(Vector<String>&, ErrorHandler*);
    /**
     * @brief The GlobalConf Element MUST be initialized and configured first.
     * 
     * @return it returns 100, the smallest returned value among all Blackadder's elements and, therefore, it starts FIRST.
     */
    int configure_phase() const {return 100;}
    /**@brief It does nothing since the GlobalConf Element does pretty much nothing.
     * 
     * @param errh
     * @return 
     */
    int initialize(ErrorHandler *errh);
    /**@brief It does nothing since nothing is dynamically allocated.
     */
    void cleanup(CleanupStage stage);
    /** @brief array of booleans that defines the node type: FW,RV,TM according to NODE_TYPEs in blackadder_enums.hpp
     *
     */
    bool type[NUM_TYPEs];
    /** @brief the Blackadder's node label.
     * 
     * This label should be statistically unique and it is self-assigned by the node itself.
     * The size of the label MUST be NODEID_LEN (see blackadder_enums.hpp).
     * Right now the deployment application writes the Blackadder Click configuration and all node labels are read by the topology configuration.
     */
    String nodeID;
    /**
     * @brief the default LIPSIN identifier to the domain rendez-vous node.
     * 
     * This LISPIN identifier includes the internal link assigned to the Blackadder node that is the domain's rendezvous point.
     * Right now it is calculated by the deployment application utility.
     */
    BABitvector defaultRV_dl;
    /**
     * @brief the default LIPSIN identifier FROM the domain rendez-vous node.
     * 
     * This LISPIN identifier is introduced for control plane availability module.
     * Right now it is calculated by the deployment application utility.
     */
    BABitvector defaultFromRV_dl;
    /**@brief The internal Link Identifier of this Blackadder node.
     * 
     * Right now it is calculated by the deployment application utility.
     */
    BABitvector iLID;
    /**@brief a LIPSIN identifier that points to the Blackadder node that runs the Topology Manager.
     * 
     * This LISPIN identifier includes the internal link assigned to the Blackadder node that runs the Topology Manager.
     * Right now it is calculated by the deployment application utility.
     */
    BABitvector TMFID;
    /**@brief This boolean variable denotes the mode in which this Blackadder node runs.
     * 
     * True for overlaying over Ethernet.
     * False for overlaying over raw IP sockets.
     * Right now it is assigned by the deployment application utility using the MODE configuration parameter.
     */
    bool use_mac;
    /** @brief That's the scope where the localRV Element subscribes using the implicit rendezvous strategy.
     * 
     * It is hardcoded to be the /FFFFFFFFFFFFFFFF
     */
    String RVScope;
    /** @brief That's the subscope where the localRV Element subscribes using the implicit rendezvous strategy for unicast notifications of *-over-icn.
     *
     * It is hardcoded to be the /FFFFFFFFFFFFFFFD and it will be subscribed to under the root scope /FFFFFFFFFFFFFFFF
     */
    String RVScopeUC;
    /** @brief That's the scope where the TopologyManager subscribes using the implicit rendezvous strategy.
     * 
     * It is hardcoded to be the /FFFFFFFFFFFFFFFE
     */
    String TMScope;
    /** @brief That's the scope where the TopologyManager subscribes for *-over-icn notifications using the implicit rendezvous strategy.
     * These notifications are for constructing unicast paths rather than the default ICN multicast trees
     * It is hardcoded to be the /FFFFFFFFFFFFFFFD and will be published under the root scope /FFFFFFFFFFFFFFFE
     */
    String TMScopeUC;
     /**@brief the Information identifier the Blackadder uses for reliable reqs and acks.
     * It is hardcoded to be the /FFFFFFFFFFFFFFFG
     */
     String controlReliabilityScope;
    /**@brief the Information identifier using which this Blackadder node will publish all RV requests.
     * 
     * It is hardcoded to be the /FFFFFFFFFFFFFFFF/NODEID. 
     * 
     * Note that the node label is the same size as the information labels (see blackadder_enums.hpp).
     */
    String nodeRVScope;
    /**@brief the Information identifier using which this Blackadder node will publish TM requests for the unicast paths of *-over-icn.
     * 
     * It is hardcoded to be the /FFFFFFFFFFFFFFFE/FFFFFFFFFFFFFFFC/NODEID.
     * 
     * Note that the node label is the same size as the information labels (see blackadder_enums.hpp).
     */
    String nodeTMScopeUC;
    /**@brief the Information identifier using which this Blackadder node will publish all TM requests.
     *
     * It is hardcoded to be the /FFFFFFFFFFFFFFFE/NODEID.
     *
     * Note that the node label is the same size as the information labels (see blackadder_enums.hpp).
     */
    String nodeTMScope;
    /**@brief the Information identifier to which this Blackadder node (the LocalProxy) will subscribe to receive all RV/TM notifications of explicit pub/sub activities.
     * 
     * It is hardcoded to be the /FFFFFFFFFFFFFFFD/NODEID. 
     * 
     * Note that the node label is the same size as the information labels (see blackadder_enums.hpp).
     */
    String notificationIID;
	/**@breif the Scope Identifier for which the TM publish path management Information
	 *
	 */
	String pathMgmtScope;
	/**@breif the Information Identifier for which the TM publish path management information
	 *
	 */
	String pathMgmtIID;
    /**@brief an indicator for whether the node is connected to the rest of the network or not
     * can be set to either one of two states: CONNECTED/DISCONNECTED
     * should only be updated by a Management app (Mapp)
     *
     */
    unsigned char status;	
    /* *-over-ICN Root Scopes*/
    String IP_ROOTSCOPE;
    String HTTP_ROOTSCOPE;
    String HTTP_DEFAULT_IID;
};

CLICK_ENDDECLS

#endif
