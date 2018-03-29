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

/**
 * Implement methods that are standard for all TMs
 */

#include "tm_graph.hpp"


TMgraph::TMgraph(){
	// Initialize all features to false (0)
	memset(&extension,0,sizeof(extension));
}
TMgraph::~TMgraph(){}
//void TMgraph::initialise(){}


/*Getters and Setters to access TM statess*/
string& TMgraph::getNodeID() {
	return nodeID;
}
string& TMgraph::getRVNodeID() {
	return RVnodeID;
}
string& TMgraph::getRMNodeID() {
	return RMnodeID;
}
string& TMgraph::getMode() {
	return mode;
}
bool TMgraph::getExten(int idx) {
	if (idx<0 || idx>SUPPORT_SIZE) return false;
	return extension[idx];
}
void TMgraph::setExten(int idx){
	if (idx<0 || idx>SUPPORT_SIZE) return;
	extension[idx] = true;
}
std::map<ICNEdge, Bitvector *> & TMgraph::getFreedLIDs() {
	return freedLIDs;
}
Bitvector * TMgraph::getTM_to_nodeFID(const string &nodeID){
	return (*TM_to_nodeFID.find(nodeID)).second;
}
Bitvector * TMgraph::getRVFID(const string &nodeID){
	return (*RVFID.find(nodeID)).second;
}
Bitvector * TMgraph::getTMFID(const string &nodeID){
	return (*TMFID.find(nodeID)).second;
}
void TMgraph::setTM_to_nodeFID(const string &nodeID, Bitvector * FID) {
	*TM_to_nodeFID[nodeID] = *FID;
}
void TMgraph::setRVFID(const string &nodeID, Bitvector * FID) {
	*RVFID[nodeID] = *FID;
}
void TMgraph::setTMFID(const string &nodeID, Bitvector * FID) {
	*TMFID[nodeID] = *FID;
}
int& TMgraph::get_reverse_node_index(const string &nodeid){
	return (*reverse_node_index.find(nodeid)).second;
}
//
// --------------- QoS Map Part ------------------------------------------------------
//



ostream& operator<<(std::ostream& out, const QoSLinkWeightMap & qm){
	QoSLinkWeightMap::const_iterator it = qm.begin();
	for (; it != qm.end(); ++it){
		out<<(int)it->first<<"\t";
		
		// Vector
		int vsize = igraph_vector_size(&it->second);
		for (int i=0; i<vsize; i++){
			out<<(int)igraph_vector_e(&it->second,i)<<"\t";
		}
		out<<endl;
	}
	
	return out;
}


uint16_t TMgraph::getWeightKeyForIIPrio(uint8_t prio){
	
	
	// Try to get the weight map directly
	QoSLinkWeightMap::iterator it = qlwm.find(prio);
	// Found!
	if (it!=qlwm.end()) return it->first;
	
	// Not Found... Assume minimum
	cout<<qlwm<<"--------- Looking ---^"<<endl;
	uint8_t netprio = 0;
	int mindist=10000;
	for (it=qlwm.begin(); it!=qlwm.end(); ++it){
		// Ignore higher priorities...
		if (it->first > prio) continue;
		
		// If this key is less than the II prio
		// and their diff is smaller than before...
		if (prio - it->first < mindist){
			netprio = it->first;
			mindist = prio - it->first;
		}
	}
	
	return netprio;
}

bool TMgraph::isQoSMapOk(){
	// Search for default priority that should be there
	// If it is not there it means that "simple" forwarders are
	// used for this network and the default matching should
	// be used (rather than weighted map).
	QoSLinkWeightMap::iterator it = qlwm.find(0);
	
	return (it!=qlwm.end());
}



