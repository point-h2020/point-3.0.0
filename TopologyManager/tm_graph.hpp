/*
 * Copyright (C) 2012-2013  Mays Al-naday, Andreas Bontozoglou and Martin Reed
 * Copyright (C) 2015-2018  Mays Al-naday
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

#ifndef TM_GRAPH_HH
#define TM_GRAPH_HH

#include <map>
#include <set>
#include <string>
#include <igraph/igraph.h>
#include <climits>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <inttypes.h>
/*MOLY*/
#include <moly/moly.hh>
#include <moly/enum.hh>


#include "tm_max_flow.hpp"

#include <bitvector.hpp>
#include "blackadder_enums.hpp"
/*igraph_version.hpp should be remade using make clean && make igraph_version.hpp
 *if igraph major or minor version changes
 */
#include "igraph_version.hpp"

/*QoS part*/
#include "qos_structs.hpp"
typedef std::map<uint8_t, igraph_vector_t> QoSLinkWeightMap;
typedef std::pair<int, int> ICNEdge;

#define SUPPORT_SIZE	5
#define TE	0
#define QOS	1
#define RS	2
#define PM	3
#define ML	4

using namespace std;

/** @brief This is a (abstract) generic helper class to carry out TE functions
 it should be inherited from.
 */
class TMgraph {
public:
    /** @brief Constructs a traffic engineering instance of the graph
     */
    TMgraph();
    /**
     * @brief destroys the TE instance
     */
    virtual ~TMgraph();
    //virtual void initialise();
    
    /**
     * @brief reads the graphml file that describes the network topology and creates some indexes for making the operation faster.
     *
     * Currently iGraph cannot read the Global Graph variables and, therefore, this methid does it manually.
     *
     * @param name the /graphML file name
     * @return <0 if there was a problem reading the file
     *
     *
     */
    virtual int readTopology(const char *name)=0;
    /** @brief report the Topology to MOLY for monitoring
     *
     */
    virtual int reportTopology(Moly &moly) = 0;
    /**@breif retreive the TM nodeID
     *
     */
    virtual string& getNodeID();
    /**@breif retreive the RM nodeID
     *
     */
    virtual string& getRVNodeID();
    /**@breif retreive the RV nodeID
     *
     */
    virtual string& getRMNodeID();
    /*@breif operation Mode
     *
     */
    virtual string& getMode();
    /**@breif retreive a TM extension state
     *
     */
    virtual bool getExten(int idx);
    /**@breif set the state of a TM extension
     *
     */
    virtual void setExten(int idx);
    /**@breif retreive cached free LIDs due to edge removal
     *
     */
    virtual std::map<ICNEdge, Bitvector *> & getFreedLIDs();
    /**@breif retreive a TM_to_nodeFID from TM to a nodeID
     *
     * @param nodeID the ID of the network node
     * @return TM_to_nodeFID
     */
    virtual Bitvector * getTM_to_nodeFID(const string &nodeID);
    /*@breif retreive a RVFID from a node to the RV
     *
     */
    virtual Bitvector * getRVFID(const string &nodeID);
    virtual Bitvector * getTMFID(const string &nodeID);
    virtual void setTM_to_nodeFID(const string &nodeID, Bitvector * FID);
    virtual void setRVFID(const string &nodeID, Bitvector * FID);
    virtual void setTMFID(const string &nodeID, Bitvector * FID);
    virtual int& get_reverse_node_index(const string &nodeid);
    virtual void calculateFID(string &source, string &destination,
                              Bitvector &resultFID,
                              unsigned int &numberOfHops, string &path
                              ) = 0;
    
    virtual Bitvector* calculateFID(string &source,
                                    string &destination
                                    ) = 0;
    
    virtual void calculateFID(set<string> &publishers,
                              set<string> &subscribers,
                              map<string, Bitvector* > &result,
                              map<string, set<string> > &path_vectors
                              ) = 0;
    virtual void UcalculateFID(string &publisher,
                               set<string> &subscribers,
                               map<string, Bitvector *> &result,
                               map<string, string> &paths
                               ) = 0;
    virtual void calculateRVTMFIDs() = 0;
    virtual bool updateGraph(const string &source,
                             const string &destination,
                             bool bidirectional,
                             bool remove,
                             Moly &moly
                             ) = 0;
    virtual void assignLIDs(int NoLIDs, set<Bitvector *> &NewLIDs) = 0;
    
    virtual Bitvector* calculateLID() = 0;
    
    virtual void updateTMStates() = 0;
	
    //------ QoS --------------------------------------------------------------
    /**
     * @brief A Map that keeps the weight set for each QoS level.
     *
     * This efficiently (without using copies of the graph) slices the network
     * and maps each flow to the best available QoS queues...
     *
     * TODO: Document
     */
    QoSLinkWeightMap qlwm;
    
    /**
     * @brief Check if the QoS map is OK (initialised).
     */
    virtual bool isQoSMapOk();
    
    // Virtual Function to calculateFIDs weighted... FIXME: Messy, sub-class!!!
    virtual void calculateFID_weighted(string &source, string &destination, Bitvector &resultFID, unsigned int &numberOfHops, string &path, const igraph_vector_t *weights)=0;
    virtual void calculateFID_weighted(set<string> &publishers, set<string> &subscribers, map<string, Bitvector *> &result, map<string, set<string> > &path_vectors, const igraph_vector_t *weights)=0;
    
    /**
     * @brief Returns the correct network weights for given II priority
     *
     * If the II priority does not exist in the network, this function will
     * downgrade it to the closest/higher available (but smaller than the II prio).
     * Default priority is considered 0 (zero) and it should exist in all networks
     * as BE...
     *
     */
    virtual uint16_t getWeightKeyForIIPrio(uint8_t prio);
    
    /**
     * @brief Update the links status whenever a LSM update arrives
     *
     * @param lid The link identifier
     * @param status The latest known link status
     *
     * Note: Consider updating all the links of a node at the same time with an Overloaded
     * updateLinkState(const LSMPacket &ptk)
     */
    virtual void updateLinkState(const string &lid, const QoSList &status)=0;
    
    /**
     * @brief the set of extension features which can be supported in the TM:
     * currently there are: TE, QoS, Resiliency, ML
     */
    bool extension[SUPPORT_SIZE]; //{TE, qos, resiliency, Multilayer}
    /**
     * @brief the length in bytes of the LIPSIN identifier.
     */
    int fid_len;
    /**
     * @brief mode in which this Blackadder node runs - the TM must create the appropriate netlink socket.
     */
    std::string mode;
    /**
     * @brief the node label of this Blackadder node.
     */
    std::string nodeID;
    /**
     * @brief the node label of the RV node
     */
    std::string RVnodeID;
    /**
     * @brief the node label of the RM (i.e. Resiliency Manager) node
     */
    std::string RMnodeID;
    /** @breif an index that maps freed LIDs after edges are removed - possibly due to faiure
     */
    std::map<ICNEdge, Bitvector *> freedLIDs;
    /**@brief number of connections in the graph.
     */
    int number_of_connections;
    /**@brief number of nodes in the graph.
     */
    int number_of_nodes;
    /**@brief an index that maps node labels to igraph vertex ids.
     */
    std::map<std::string, int> reverse_node_index;
    /**@brief an index that maps node labels to igraph edge ids.
     */
    std::map<std::string, int> reverse_edge_index;
    /**@brief an index that maps node labels to their internal link Identifiers.
     */
    std::map<std::string, Bitvector *> nodeID_iLID;
    /**@brief an index that maps igraph vertex ids to internal link Identifiers.
     */
    std::map<int, Bitvector *> vertex_iLID;
    /**@brief an index that maps igraph edge ids to link Identifiers.
     */
    std::map<int, Bitvector *> edge_LID;
    /**@breif an index that maps flowing information items to their delivering paths
     */
    std::map<std::string, set<string> > path_info;
    /**@breif an index that maps flowing information items to their unicast delivering paths
     * for unicast ICNID is not suffecient to uniquely identify the delivery path, and the publisher information is needed as well
     * Key: ICNID, Publisher
     * Value: Path
     */
    std::map<std::string, std::map<std::string, std::string> > unicast_path_info;
    /**@breif an index that maps information items to their 'Alternative' Publisher (Standby Pubs)
     * used by the RM for proactive resilience
     */
    std::map<std::string, set<string> > ex_pubs;
    /**@breif an index that maps information items to their 'Alternative' Subscribers (Standby Subs)
     * used by the RM for proactive resilience
     */
    std::map<std::string, set<string> > ex_subs;
    /** @breif an index that stores all the assigned LIDs in the graph
     *
     */
    std::set<Bitvector *> LIDs;
    /** @breif an index that maps NodeID to the RVFID towards the node
     *
     */
    std::map<std::string, Bitvector *> RVFID;
    /** @breif an index that maps NodeID to the TMFID towards the node
     *
     */
    std::map<std::string, Bitvector *> TMFID;
    /** @breif an index that maps NodeID to the FID from the TM to the node
     *
     */
    std::map<std::string, Bitvector *> TM_to_nodeFID;
    /*****************/
};


// debugging...
std::ostream& operator<<(std::ostream& out, const QoSLinkWeightMap & qm);

#endif
