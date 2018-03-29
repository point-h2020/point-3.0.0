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

#ifndef TM_IGRAPH_HH
#define TM_IGRAPH_HH

#include <map>
#include <set>
#include <vector>
#include <string>
#include <igraph/igraph.h>
#include <climits>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <bitvector.hpp>
#include "blackadder_enums.hpp"


#include "tm_graph.hpp"
// igraph_version.hpp should be remade using make clean && make igraph_version.hpp
// if igraph major or minor version changes
#include "igraph_version.hpp"


using namespace std;

/**@brief (Topology Manager) This is a representation of the network topology (using the iGraph library) for the Topology Manager.
 */
class TMIgraph : public TMgraph {
public:
	/**@brief Contructor: creates an empty iGraph graph.
	 */
	TMIgraph();
	/**@brief destroys the graph and frees some meomry here and there.
	 *
	 */
	virtual ~TMIgraph();
	/**
	 * @brief reads the graphml file that describes the network topology and creates some indexes for making the operation faster.
	 *
	 * Currently iGraph cannot read the Global Graph variables and, therefore, this methid does it manually.
	 *
	 * @param name the /graphML file name
	 * @return <0 if there was a problem reading the file
	 */
	virtual int readTopology(const char *name);
    /** @brief report the Topology to MOLY for monitoring
     *
     */
    virtual int reportTopology(Moly &moly);
	/**@brief it calculates a LIPSIN identifier from source to destination using the shortest path.
	 *
	 * @param source the node label of the source node.
	 * @param destination the node label of the destination node.
	 * @return a pointer to a bitvector that represents the LIPSIN identifier.
	 */
	Bitvector *calculateFID(string &source, string &destination);
	/**@brief it calculates LIPSIN identifiers from a set of publishers to a set of subscribers using the shortest paths.
	 *
	 * @param publishers a reference to a set of node labels, representing the source nodes.
	 * @param subscribers a reference to a set of node labels, representing the destination nodes.
	 * @param result a reference to a map where the method will put node labels representing source nodes mapped to LIPSIN identifiers. Note that some of these identifiers may be NULL.
	 * @param path_vectors a reference to a map of information items and their corresponding delivery paths
	 */
	void calculateFID(set<string> &publishers, set<string> &subscribers, map<string, Bitvector *> &result, map<string, set<string> > &path_vectors);
	/**@brief it calculates LIPSIN identifiers from a publisher to a set of subscribers using the shortest paths. This involves selecting the closest subscriber to establish a relation with.
	 *
	 * @param publisher a reference to a node label, representing the source node. It represents the NAP publishing request on behalf of a client (cNAP)
	 * @param subscribers a reference to a set of node labels, representing the destination nodes. It represents the set of NAPs subscribing on behalf of servers (sNAPs)
	 * @param result a reference to a map where the method will put node labels representing destination nodes mapped to LIPSIN identifiers. It represents the FIDs from the publisher to each subscriber, notice that all of these identifiers will be NULL, except the one for the nearest subscriber.
	 * @param paths a reference to a map of information items and their corresponding delivery paths, notice that all of these identifiers will be NULL, except the one for the nearest subscriber.
	 */
	void UcalculateFID(string &publishers, set<string> &subscribers, map<string, Bitvector *> &result, map<string, string> &paths);
	/**@brief it calculates 3 sets of LIPSIN identifiers that are maintained by the TM at all times:
	 * RVFID: the FID from each node in the network to the RV node
	 * TMFID: the FID from each node in the network to the TM node
	 * TM_to_nodeFID: the FID from the TM to each node in the network
	 */
	void calculateRVTMFIDs();
	/**@brief used internally by the above method.
	 *
	 * @param source
	 * @param destination
	 * @param resultFID
	 * @param numberOfHops
	 * @param path a reference to the string of the result path vector
	 */
	void calculateFID(string &source, string &destination, Bitvector &resultFID, unsigned int &numberOfHops, string &path);
	/**@brief it assigns a set of newly calculated LIDs.
	 *
	 * @param NoLIDs the number of new LIDs (i.e. size of the LIDs set)
	 * @param NewLIDs the actual set of assigned LIDs
	 */
	void assignLIDs(int NoLIDs, set<Bitvector *> &NewLIDs);
	/**@brief it calculates a globally unique LID. uniqueness is ensured by checking against the TM state of all currently assigned LIDs
	 *
	 */
	Bitvector * calculateLID();
	/**@brief it updates the TM maintained states of edge_LIDs and reverse_edge_lid after each graph update
	 *
	 */
	void updateTMStates();
	//FIXME: Messy implementation...
	void calculateFID_weighted(string &source, string &destination, Bitvector &resultFID, unsigned int &numberOfHops, string &path, const igraph_vector_t *weights);
	void calculateFID_weighted(set<string> &publishers, set<string> &subscribers, map<string, Bitvector *> &result, map<string, set<string> > &path_vectors, const igraph_vector_t *weights);
	/** @brief update the graph object when a notification about a link-status change arrives (i.e. add or remove edges)
	 * carefull, the function assumes persistent vertex Ids, meaning vertices don't get deleted from the graph
	 * different function will be required for considering add/remove vertices
	 *
	 * @param srouce source of the edge (notification publisher)
	 * @param destination the node on the other side of the link
	 * @param bidirectional the type of update, uni- or bi-directional
	 * @param remove the type of operation (i.e. Link addition or removal)
	 */
	bool updateGraph(const string &source, const string &destination, bool bidirectional, bool remove, Moly &moly);
	/**@brief Update the links status whenever a LSM update arrives
	 *
	 *
	 * @param lid The link identifier
	 * @param status The latest known link status
	 *
	 * Note: Consider updating all the links of a node at the same time with an Overloaded
	 * updateLinkState(const LSMPacket &ptk)
	 */
	virtual void updateLinkState(const string &lid, const QoSList &status);
	
	
public:
	/**@brief the igraph graph
	 */
	igraph_t graph;
protected:
	
	/**
	 * @brief Create a new "line" into the QoSLinkWeightMap...
	 *
	 * Each "line"/entry represents the network plane for a specific queueing priority
	 */
	virtual void createNewQoSLinkPrioMap(const uint8_t & prio);
	
	
};

#endif
