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

#include "tm_igraph.hpp"
extern int EF_ALLOW_MALLOC_0;
#include <sstream>
#include <unistd.h>
#include <iostream>
#include <string>
#include <fstream>

TMIgraph::TMIgraph() {
	igraph_i_set_attribute_table(&igraph_cattribute_table);
	//igraph_empty(&graph, 0, IGRAPH_DIRECTED); // No, read from file instead
}

TMIgraph::~TMIgraph() {
	map<string, Bitvector *>::iterator nodeID_iLID_iter;
	map<int, Bitvector *>::iterator edge_LID_iter;
	for (nodeID_iLID_iter = nodeID_iLID.begin(); nodeID_iLID_iter != nodeID_iLID.end(); nodeID_iLID_iter++) {
		delete (*nodeID_iLID_iter).second;
	}
	for (edge_LID_iter = edge_LID.begin(); edge_LID_iter != edge_LID.end(); edge_LID_iter++) {
		delete (*edge_LID_iter).second;
	}
	igraph_i_attribute_destroy(&graph);
	igraph_destroy(&graph);
}

int TMIgraph::readTopology(const char *file_name) {
	int ret;
	Bitvector *lid;
	Bitvector *ilid;
	ifstream infile;
	std::string str;
	size_t found_5, found_6, first, second;
	FILE *instream;
	infile.open(file_name, ifstream::in);
	if (infile.fail()) {
		return -1;
	}
	/*first the Global graph attributes - c igraph does not do it!!*/
	while (infile.good()) {
		getline(infile, str);
		found_5 = str.find("<data key=\"FID_LEN\">");
		found_6 = str.find("<data key=\"g_FID_LEN\">");
		if ((found_5 != string::npos) || ((found_6 != string::npos))) {
			first = str.find(">");
			second = str.find("<", first);
			sscanf(str.substr(first + 1, second - first - 1).c_str(), "%d", &fid_len);
		}
		found_5 = str.find("<data key=\"TM\">");
		found_6 = str.find("<data key=\"g_TM\">");
		if ((found_5 != string::npos) || ((found_6 != string::npos))) {
			first = str.find(">");
			second = str.find("<", first);
			nodeID = str.substr(first + 1, second - first - 1);
			/*\TODO: the RM need to provide its nodeID to the TM when it comes alive - at the moment, it is assumed to be the same as the TMnodeID*/
			RMnodeID = nodeID;
		}
		found_5 = str.find("<data key=\"RV\">");
		found_6 = str.find("<data key=\"g_RV\">");
		if ((found_5 != string::npos) || ((found_6 != string::npos))) {
			first = str.find(">");
			second = str.find("<", first);
			RVnodeID = str.substr(first + 1, second - first - 1);
		}
		found_5 = str.find("<data key=\"TM_MODE\">");
		found_6 = str.find("<data key=\"g_TM_MODE\">");
		if ((found_5 != string::npos) || ((found_6 != string::npos))) {
			first = str.find(">");
			second = str.find("<", first);
			mode = str.substr(first + 1, second - first - 1);
		}
	}
	infile.close();
	instream = fopen(file_name, "r");
	ret = igraph_read_graph_graphml(&graph, instream, 0);
	fclose(instream);
	if (ret < 0) {
		return ret;
	}
	cout << "TM: " << igraph_vcount(&graph) << " nodes" << endl;
	cout << "TM: " << igraph_ecount(&graph) << " edges" << endl;

        //file to write the LIDs, so that ICN-SDN can access them
        std::ofstream outfile ("/tmp/lids");
	for (int i = 0; i < igraph_vcount(&graph); i++) {
		std::string nID = std::string(igraph_cattribute_VAS(&graph, "NODEID", i));
		std::string iLID = std::string(igraph_cattribute_VAS(&graph, "iLID", i));
		reverse_node_index.insert(pair<std::string, int>(nID, i));
		ilid = new Bitvector(iLID);
		nodeID_iLID.insert(pair<std::string, Bitvector *>(nID, ilid));
		vertex_iLID.insert(pair<int, Bitvector *>(i, ilid));
		RVFID.insert(pair<std::string, Bitvector *>(nID, NULL));
		TMFID.insert(pair<std::string, Bitvector *>(nID, NULL));
		//cout << "node " << i << " has NODEID " << nID << endl;
		//cout << "node " << i << " has ILID " << ilid->to_string() << endl;
	}
	for (int i = 0; i < igraph_ecount(&graph); i++) {
		std::string LID = std::string(igraph_cattribute_EAS(&graph, "LID", i));
		reverse_edge_index.insert(pair<std::string, int>(LID, i));
		lid = new Bitvector(LID);
		//LIDs.insert(lid);
		edge_LID.insert(pair<int, Bitvector *>(i, lid));
		igraph_integer_t head;
		igraph_integer_t tail;
		igraph_edge(&graph, i,&head,&tail);

		//convert head and tail to strings, so that they can be concatenated
                head++;
                tail++;
                long h = (long)head;
                long t = (long)tail;

                std::string head_str;
                std::stringstream strstream_head;
                strstream_head << h;
                strstream_head >> head_str;

                std::string tail_str;
                std::stringstream strstream_tail;
                strstream_tail << t;
                strstream_tail >> tail_str;

                string sourceNode = "0000000" + head_str;
                string destNode = "0000000" + tail_str;
                outfile << "Source " << sourceNode << " destination " << destNode << " LID " << lid->to_string() << endl;

		cout << "edge " << i
		<<" "<<head<<"->"<<tail<<" has LID  "
		<< lid->to_string() << endl;
	}
        outfile.close();
	return ret;
}
int TMIgraph::reportTopology(Moly &moly) {
    node_role_t node_type;
    uint16_t lid_int;
    uint32_t nodeID_int;
    uint16_t nighbour_nodeID_int;
    igraph_integer_t vertex;
    igraph_integer_t neighbour;
    igraph_integer_t eid;
    igraph_vector_t neis;
    string node_ID;
    string node_name;
    string link_name;
    string node_ID_suffex;
    string neighbour_ID_suffex;
    map<Bitvector * ,int> lid_to_int;
    map<string, int>::iterator node_it;
    map<string, int>::iterator edge_it;
    igraph_vector_init(&neis, 1);
    map<string, int>::iterator nighbour_nodeID;
    /*Publish nodes list*/
    ofstream topology_nodes;
    ofstream topology_links;
    topology_nodes.open("/home/point/topology_nodes.csv");
    topology_links.open("/home/point/topology_links.csv");
    for (node_it = reverse_node_index.begin(); node_it != reverse_node_index.end(); node_it++) {
        node_ID = node_it->first;
        node_ID_suffex = node_ID;
        node_ID_suffex.erase(0, node_ID_suffex.find_first_not_of('0'));
        nodeID_int = atol(node_ID_suffex.c_str());
        vertex = node_it->second;
        node_name = string(igraph_cattribute_VAS(&graph, "NAME", vertex));
        node_name = (node_name == " ") ? "FN" : node_name;
        node_type = NODE_ROLE_FN;
        topology_nodes << nodeID_int << "," << node_name << "," << node_type << ", 9" << endl;
        if(! moly.addNode(node_name, nodeID_int, node_type)){
            cout << "TM: moly failed to add Node " << nodeID_int << endl;
            return -1;
        } else
            cout << "TM: node: " << nodeID_int << " of type : " << node_type << " reported to MA" << endl;
        if (node_ID == RVnodeID) {
            usleep(40000);
            node_name = (node_name == " ") ? "RV" : node_name;
            node_type = NODE_ROLE_RV;
            topology_nodes << nodeID_int << "," << node_name << "," << node_type << ", 9" << endl;
            if(!moly.addNode(node_name, nodeID_int, node_type)) {
                cout << "TM: moly failed to send node type" << node_type << endl;
                return -1 ;
            }else
                cout << "TM: node: " << nodeID_int << " of type : " << node_type << " reported to MA" << endl;
        }
        if (node_ID == nodeID){
            usleep(40000);
            node_name = (node_name == " ") ? "TM" : node_name;
            node_type = NODE_ROLE_TM;
            topology_nodes << nodeID_int << "," << node_name << "," << node_type << ", 9" << endl;
            if(!moly.addNode(node_name, nodeID_int, node_type)) {
                cout << "TM: moly failed to send node type"<< node_type << endl;
                return -1;
            }else
                cout << "TM: node: " << nodeID_int << " of type : " << node_type << " reported to MA" << endl;
        }
        /*get the neighbouring vertecis of vertex*/
        igraph_neighbors(&graph, &neis, vertex, IGRAPH_OUT);
        for (int n = 0; n < igraph_vector_size(&neis); n++) {
            neighbour = VECTOR(neis)[n];
            for (nighbour_nodeID = reverse_node_index.begin() ; nighbour_nodeID != reverse_node_index.end(); nighbour_nodeID++) {
                if (nighbour_nodeID->second == neighbour) {
                    neighbour_ID_suffex = nighbour_nodeID->first;
                    neighbour_ID_suffex.erase(0, neighbour_ID_suffex.find_first_not_of('0'));
                    nighbour_nodeID_int = atoi(neighbour_ID_suffex.c_str());
                    break;
                }
            }
#if IGRAPH_V >= IGRAPH_V_0_6
            igraph_get_eid(&graph, &eid, vertex, neighbour, true, true);
#else
            igraph_get_eid(&graph, &eid, vertex, neighbour, true);
#endif
            link_name = string(igraph_cattribute_EAS(&graph, "NAME", eid));
            string LID(igraph_cattribute_EAS(&graph, "LID", eid), FID_LEN * 8);
            for(uint16_t k = 0; k < FID_LEN * 8; k++){
                if (LID[k] == '1'){
                    lid_int = k;
                    break;
                }
            }
            topology_links << lid_int << "," << nodeID_int << "," << nighbour_nodeID_int << "," << LINK_TYPE_802_3 << ",9" << endl;
            //FIXME the Moly::AddLink primitive has changed. extend to provide the correct port number instead of using link type as port number
            cout << "TM: report link " << lid_int << " to MA " << endl;
            if(!moly.addLink(lid_int, nighbour_nodeID_int , nodeID_int, 1, LINK_TYPE_802_3, link_name))
            {
                cout << "TM: moly failed to send link"<< nodeID_int << "->" << nighbour_nodeID_int << endl;
                return -1;
            }
            else {
                cout << "TM: link id: " << lid_int << " from " << nodeID_int << "-> " << nighbour_nodeID_int << " reported to MA" << endl;
                usleep(40000);
            }
        }
        usleep(40000);
    }
    topology_nodes.close();
    topology_links.close();
    return 0;
}
void TMIgraph::updateLinkState(const string &lid, const QoSList &status){
	// Get the edge ID from the map
	igraph_integer_t eid = reverse_edge_index[lid];
	//cout<<"lid="<<lid<<" eid="<<eid<<endl;
	
	// Parse and store
	QoSList::const_iterator it = status.begin();
	for (; it!=status.end(); ++it){
		
		// Store on the edges (in case we pring/export the GraphML)
		stringstream ss;
		ss<<"QoS_"<<(int)it->first;
		//cout<<" Adding "<<ss.str().c_str()<<endl;
		SETEAN(&graph, ss.str().c_str(), eid, it->second);
	}
	
	
	// Update the QoSLinkWeightMap of the TM
	// 1. Check that this link level exists
	uint8_t lp = (status.find(QoS_PRIO))->second;
	QoSLinkWeightMap::iterator mit = qlwm.find(lp);
	if (mit==qlwm.end()) {
		createNewQoSLinkPrioMap(lp);
		//return; Do not return... we have to fix all the other qos weights also
	}
	
	// 2. If there, update the link stats
	// - map iterator, mit, points to the proper line...
	// - eid points to the edges id: ASSUMPION: the weight vector is indexed by edge id
	igraph_vector_t * vec = &qlwm[lp];
	// way 2: igraph_vector_t * vec = &mit->second;
	int vsize = igraph_vector_size(vec);
	if (eid >= vsize){
		cerr<<"ABORT eid >= vsize"<<endl;
		return;
	}
	
	// Update all the eid positions in the QoS map
	mit = qlwm.begin();
	for (; mit!=qlwm.end(); ++mit){
		// Set the vector pointer
		vec = &mit->second;
		if (mit->first < lp){
			// For any priority lower than this edge's... avoid using this edge
			igraph_vector_set(vec, eid, UCHAR_MAX);
		}
		else {
			// For all the others keep the real weight
			igraph_vector_set(vec, eid, (MAX_PRIO-lp));
		}
	}
	
}

void TMIgraph::createNewQoSLinkPrioMap(const uint8_t & map_prio){
	cout<<"Creating new priority plane: "<<(int)map_prio<<endl;
	// Create the line as a c array!
	int numedges=igraph_ecount(&graph);
	// cout<<"  # of Edges: "<<numedges<<endl;
	igraph_real_t * data = new igraph_real_t[numedges];
	
	map<int, Bitvector *>::iterator it;
	for (it = edge_LID.begin(); it != edge_LID.end(); ++it) {
		// Get edge ID
		int eid = it->first;
		
		// Get edge priority from the graph attr
		stringstream ss;
		ss<<"QoS_"<<(int)QoS_PRIO;
		uint8_t ep = EAN(&graph, ss.str().c_str(), eid);
		
		
		uint8_t weight = UCHAR_MAX;
		// If the current edge prio is the same of more that
		// keep the exact weight
		if (map_prio >= ep) weight = MAX_PRIO-ep; // P=98 -> weight=2
		
		// Assign the weight
		data[eid] = weight;
		//cout<<"  - Edge id="<<eid<<" Prio="<<(int)ep<<" Weight="<<(int)weight<<endl;
	}
	
	// Port data to vector
	igraph_vector_t vec;
	igraph_vector_view(&vec, data, numedges);
	
	// Add plane to the map
	qlwm[map_prio]=vec;
	
}
/* Update the topology graph with add/remove edge - carefull, the function assumes persistent vertex Ids,
 * meaning vertices don't get deleted from the graph, different function will be required for considering add/remove vertices*/
bool TMIgraph::updateGraph(const string &source, const string &destination, bool bidirectional, bool remove, Moly &moly){
    uint16_t sourceInt;
    uint16_t destinationInt;
    string LID;
    string sourceSuffix;
    string destinationSuffix;
    igraph_vector_t v;
	igraph_es_t es;
	igraph_vector_t eids;
	igraph_vector_t new_eids;
	Bitvector *lid ;
    uint16_t lid_int;
	std::set<Bitvector *> lids;
	std::set<Bitvector *>::iterator lid_it;
	bool ret = false;
	bool exists = false;
    bool broken = false;
	int NoEdges;
	int source_vertex = (*reverse_node_index.find(source)).second;
	int destination_vertex = (*reverse_node_index.find(destination)).second;
	ICNEdge forward_edge = pair<int, int>(source_vertex, destination_vertex);
	ICNEdge backward_edge = pair<int, int>(destination_vertex, source_vertex);
	/*initialize igraph vectors*/
	if (bidirectional) {
		/*both directions are processed*/
		NoEdges = 2;
		igraph_vector_init(&v, 4);
		/*forward edge*/
		VECTOR(v)[0] = source_vertex;
		VECTOR(v)[1] = destination_vertex;
		/*backward edge*/
		VECTOR(v)[2] = destination_vertex;
		VECTOR(v)[3] = source_vertex;
        broken = (freedLIDs.find(forward_edge) != freedLIDs.end()) && (freedLIDs.find(backward_edge) != freedLIDs.end());
	}
	else {
		/*only reported direction is processed*/
		NoEdges = 1;
		igraph_vector_init(&v, 2);
		VECTOR(v)[0] = source_vertex;
		VECTOR(v)[1] = destination_vertex;
        broken = (freedLIDs.find(forward_edge) != freedLIDs.end());
	}
	igraph_vector_init(&eids, NoEdges);
	igraph_vector_init(&new_eids, NoEdges);
	/*get the edge ids*/
#if IGRAPH_V >= IGRAPH_V_0_7
	igraph_get_eids(&graph, &eids, &v, NULL, true, false);
#elif IGRAPH_V >= IGRAPH_V_0_6
	igraph_get_eids(&graph, &eids, &v, NULL, true, false);
#else
	igraph_get_eids(&graph, &eids, &v, true);
#endif
	exists = (VECTOR(eids)[0] > -1);
	/*even if bidrectional = true, if the forward edge exists, it is fair to assume the backward edge also exists*/
	if (remove && exists) {
		/*remove an existing edge(s)*/
		lid = (*edge_LID.find(VECTOR(eids)[0])).second;
        LID = lid->to_string();
        for(uint16_t pos = 0; pos < FID_LEN; pos++)
        {
            if (LID[pos] == '1')
            {
                lid_int = pos ;
                break;
            }
        }
		freedLIDs.insert(pair<ICNEdge, Bitvector *>(forward_edge, lid));
        /*report linkstate to moly*/
        sourceSuffix = source;
        sourceSuffix.erase(0, sourceSuffix.find_first_not_of('0'));
        sourceInt = atoi(sourceSuffix.c_str());
        destinationSuffix = destination;
        destinationSuffix.erase(0, destinationSuffix.find_first_not_of('0'));
        destinationInt = atoi(destinationSuffix.c_str());
        //FIXME the Moly::AddLink primitive has changed. fix port number to correct value rather than using linktype as port number
        if(!moly.linkState(lid_int, destinationInt, sourceInt, LINK_TYPE_802_3, LINK_TYPE_802_3, LINK_STATE_DOWN))
        {
            cout << "TM: moly failed to update link state "<< sourceInt << "->" << destinationInt << "to DOWN" << endl;
            return -1;
        }
        else {
            cout << "TM: link: " << sourceInt << "-> " << destinationInt << " state DOWN reported to MA" << endl;
        }
		if (NoEdges == 2) {
			lid = (*edge_LID.find(VECTOR(eids)[1])).second;
			freedLIDs.insert(pair<ICNEdge, Bitvector *>(backward_edge, lid));
            LID = lid->to_string();
            for(uint16_t pos = 0; pos < FID_LEN * 8; pos++)
            {
                if (LID[pos] == '1')
                {
                    lid_int = pos ;
                    break;
                }
            }
            /*report link state to moly*/
            //FIXME the Moly::AddLink primitive has changed. fix port number to correct value rather than using linktype as port number
            if(!moly.linkState(lid_int, sourceInt, destinationInt, LINK_TYPE_802_3, LINK_TYPE_802_3, LINK_STATE_DOWN))
            {
                cout << "TM: moly failed to update link state "<< destinationInt << "->" << sourceInt << "to DOWN" << endl;
                return -1;
            }
            else {
                cout << "TM: link: " << destinationInt << "-> " << sourceInt << " state DOWN reported to MA" << endl;
            }
		}
		igraph_es_pairs(&es, &v, true);
		igraph_delete_edges(&graph, es);
		cout << "TM: removed " << NoEdges << " edges" << endl;
		updateTMStates();
		ret = true;
	}
	else if ((!remove) && (!exists) && broken){
		/*add a non existing edge(s), but assuming the vertices exists*/
		igraph_add_edges(&graph, &v, 0);
		cout << "TM: added " << NoEdges << " edges" << endl;
		/*restore the LID of the recovered edge*/
		lid = (*freedLIDs.find(forward_edge)).second;
        LID = lid->to_string();
        for(uint16_t pos = 0; pos < FID_LEN; pos++)
        {
            if (LID[pos] == '1')
            {
                lid_int = pos ;
                break;
            }
        }
		/*get the new edge IDs*/
#if IGRAPH_V >= IGRAPH_V_0_7
		igraph_get_eids(&graph, &new_eids, &v, NULL, true, false);
#elif IGRAPH_V >= IGRAPH_V_0_6
		igraph_get_eids(&graph, &new_eids, &v, NULL, true, false);
#else
		igraph_get_eids(&graph, &new_eids, &v, true);
#endif
		/*add the LID of the forward edge*/
		igraph_cattribute_EAS_set(&graph, "LID", VECTOR(new_eids)[0], lid->to_string().c_str());
		freedLIDs.erase(forward_edge);
        /*report link state to moly*/
        sourceSuffix = source;
        sourceSuffix.erase(0, sourceSuffix.find_first_not_of('0'));
        sourceInt = atoi(sourceSuffix.c_str());
        destinationSuffix = destination;
        destinationSuffix.erase(0, destinationSuffix.find_first_not_of('0'));
        destinationInt = atoi(destinationSuffix.c_str());
        //FIXME the Moly::AddLink primitive has changed. fix port number to correct value rather than using linktype as port number
        if(!moly.linkState(lid_int, destinationInt, sourceInt, LINK_TYPE_802_3, LINK_TYPE_802_3, LINK_STATE_UP))
        {
            cout << "TM: moly failed to update link state "<< sourceInt << "->" << destinationInt << "to UP" << endl;
            return -1;
        }
        else {
            cout << "TM: link: " << sourceInt << "-> " << destinationInt << " state UP reported to MA" << endl;
        }
		if (NoEdges == 2) {
			/*add the backward edge attributes*/
			lid = (*freedLIDs.find(backward_edge)).second;
            LID = lid->to_string();
            for(uint16_t pos = 0; pos < FID_LEN; pos++)
            {
                if (LID[pos])
                {
                    lid_int = pos ;
                    break;
                }
            }
			igraph_cattribute_EAS_set(&graph, "LID", VECTOR(new_eids)[1], lid->to_string().c_str());
			freedLIDs.erase(backward_edge);
            //FIXME the Moly::AddLink primitive has changed. fix port number to correct value rather than using linktype as port number
            if(!moly.linkState(lid_int, sourceInt, destinationInt, LINK_TYPE_802_3, LINK_TYPE_802_3, LINK_STATE_UP))
            {
                cout << "TM: moly failed to update link state "<< destinationInt << "->" << sourceInt << "to UP" << endl;
                return -1;
            }
            else {
                cout << "TM: link: " << destinationInt << "-> " << sourceInt << " state UP reported to MA" << endl;
            }
		}
		updateTMStates();
		ret = true;
	}
	else {
		cout << "TM: no action taken, either edge exists and therefore cannot be added again or does not exist and therefore cannot be removed again" << endl;
	}
	igraph_es_destroy(&es);
	igraph_vector_destroy(&v);
	igraph_vector_destroy(&eids);
	igraph_vector_destroy(&new_eids);
	return ret;
}

void TMIgraph::assignLIDs(int NoLIDs, std::set<Bitvector *> &NewLIDs){
	Bitvector * lid;
	for (int i = 0 ; i < NoLIDs; i++) {
		lid = calculateLID();
		NewLIDs.insert(lid);
	}
}
Bitvector * TMIgraph::calculateLID() {
	Bitvector * lid = new Bitvector(FID_LEN * 8);
	int bit_position = 0;
	do {
		/*set a random bit position to 1*/
		bit_position = rand() % (FID_LEN * 8);
		(*lid)[bit_position] = true;
	} while(LIDs.find(lid) != LIDs.end());
	return lid;
}
void TMIgraph::updateTMStates(){
	unsigned int no_edges = igraph_ecount(&graph);
	/*update edge states after as a consequence of graph update*/
	edge_LID.clear();
	reverse_edge_index.clear();
	for (unsigned int i = 0; i < no_edges; i++) {
		std::string LID = string(igraph_cattribute_EAS(&graph, "LID", i));
		reverse_edge_index.insert(pair<string, int>(LID, i));
		Bitvector* lid = new Bitvector(LID);
		edge_LID.insert(pair<int, Bitvector *>(i, lid));
	}
}

Bitvector *TMIgraph::calculateFID(string &source, string &destination) {
	int vertex_id;
	Bitvector *result = new Bitvector(FID_LEN * 8);
	igraph_vs_t vs;
	igraph_vector_ptr_t res;
	igraph_vector_t to_vector;
	igraph_vector_t *temp_v;
	igraph_integer_t eid;
	
	/*find the vertex id in the reverse index*/
	int from = (*reverse_node_index.find(source)).second;
	igraph_vector_init(&to_vector, 1);
	VECTOR(to_vector)[0] = (*reverse_node_index.find(destination)).second;
	/*initialize the sequence*/
	igraph_vs_vector(&vs, &to_vector);
	/*initialize the vector that contains pointers*/
	igraph_vector_ptr_init(&res, 1);
	temp_v = (igraph_vector_t *) VECTOR(res)[0];
	temp_v = (igraph_vector_t *) malloc(sizeof (igraph_vector_t));
	VECTOR(res)[0] = temp_v;
	igraph_vector_init(temp_v, 1);
	/*run the shortest path algorithm from "from"*/
#if IGRAPH_V >= IGRAPH_V_0_7
	igraph_get_shortest_paths(&graph, &res, NULL, from, vs, IGRAPH_OUT,NULL,NULL);
#elif IGRAPH_V == IGRAPH_V_0_6
	igraph_get_shortest_paths(&graph, &res, NULL, from, vs, IGRAPH_OUT);
#else
	igraph_get_shortest_paths(&graph, &res, from, vs, IGRAPH_OUT);
#endif
	/*check the shortest path to each destination*/
	temp_v = (igraph_vector_t *) VECTOR(res)[0];
	//click_chatter("Shortest path from %s to %s", igraph_cattribute_VAS(&graph, "NODEID", from), igraph_cattribute_VAS(&graph, "NODEID", VECTOR(*temp_v)[igraph_vector_size(temp_v) - 1]));
	/*now let's "or" the FIDs for each link in the shortest path*/
	for (int j = 0; j < igraph_vector_size(temp_v) - 1; j++) {
#if IGRAPH_V >= IGRAPH_V_0_6
		igraph_get_eid(&graph, &eid, VECTOR(*temp_v)[j], VECTOR(*temp_v)[j + 1], true, true);
#else
		igraph_get_eid(&graph, &eid, VECTOR(*temp_v)[j], VECTOR(*temp_v)[j + 1], true);
#endif
		//click_chatter("node %s -> node %s", igraph_cattribute_VAS(&graph, "NODEID", VECTOR(*temp_v)[j]), igraph_cattribute_VAS(&graph, "NODEID", VECTOR(*temp_v)[j + 1]));
		//click_chatter("link: %s", igraph_cattribute_EAS(&graph, "LID", eid));
		string LID(igraph_cattribute_EAS(&graph, "LID", eid), FID_LEN * 8);
		for (int k = 0; k < FID_LEN * 8; k++) {
			if (LID[k] == '1') {
				(*result)[ FID_LEN * 8 - k - 1].operator |=(true);
			}
		}
		//click_chatter("FID of the shortest path: %s", result.to_string().c_str());
	}
	if (igraph_vector_size(temp_v) > 0) {
		/*now, if a path is found, for all destinations "or" the internal linkID*/
		vertex_id = (*reverse_node_index.find(destination)).second;
		string iLID(igraph_cattribute_VAS(&graph, "iLID", vertex_id));
		//click_chatter("internal link for node %s: %s", igraph_cattribute_VAS(&graph, "NODEID", vertex_id), iLID.c_str());
		for (int k = 0; k < FID_LEN * 8; k++) {
			if (iLID[k] == '1') {
				(*result)[ FID_LEN * 8 - k - 1].operator |=(true);
			}
		}
	}
	igraph_vector_destroy((igraph_vector_t *) VECTOR(res)[0]);
	igraph_vector_destroy(&to_vector);
	igraph_vector_ptr_destroy_all(&res);
	igraph_vs_destroy(&vs);
	return result;
}

/*main function for rendezvous*/
void TMIgraph::calculateFID(set<string> &publishers, set<string> &subscribers, map<string, Bitvector *> &result, map<string, set<string> > &path_vectors) {
	set<string>::iterator subscribers_it;
	set<string>::iterator publishers_it;
	string bestPublisher;
	Bitvector resultFID(FID_LEN * 8);
	Bitvector bestFID(FID_LEN * 8);
	unsigned int numberOfHops = 0;
	set<string> paths_per_pub;
	string best_path;
	/*first add all publishers to the hashtable with NULL FID*/
	for (publishers_it = publishers.begin(); publishers_it != publishers.end(); publishers_it++) {
		string pub = *publishers_it;
		result.insert(pair<string, Bitvector *>(pub, NULL));
		path_vectors.insert(pair<string, set<string> >(pub, paths_per_pub));
	}
	for (subscribers_it = subscribers.begin(); subscribers_it != subscribers.end(); subscribers_it++) {
		/*for all subscribers calculate the number of hops from all publishers (not very optimized...don't you think?)*/
		unsigned int minimumNumberOfHops = UINT_MAX;
		for (publishers_it = publishers.begin(); publishers_it != publishers.end(); publishers_it++) {
			string curr_path;
			resultFID.clear();
			string str1 = (*publishers_it);
			string str2 = (*subscribers_it);
			calculateFID(str1, str2, resultFID, numberOfHops, curr_path);
			if (minimumNumberOfHops > numberOfHops) {
				minimumNumberOfHops = numberOfHops;
				bestPublisher = *publishers_it;
				bestFID = resultFID;
				if(extension[RS])		//resiliency
					best_path = curr_path;
			}
		}
		/*When the resiliency support is in effect, return the set of path vectors for each publisher*/
		if(extension[RS]){
	  cout<<"Best path is: "<<best_path<<endl;
	  if(!(path_vectors.empty())){
		  for(map<string, set<string> >::iterator b_b_it = path_vectors.begin(); b_b_it != path_vectors.end(); b_b_it++){
			  if ((*b_b_it).first == bestPublisher){
				  b_b_it->second.insert(best_path);
			  }
		  }
	  } else{
		  set<string> add_b_path;
		  add_b_path.insert(best_path);
		  path_vectors.insert(pair<string, set<string> >(bestPublisher, add_b_path));
	  }
		}
		//cout << "best publisher " << bestPublisher << " for subscriber " << (*subscribers_it) << " -- number of hops " << minimumNumberOfHops - 1 << endl;
		if ((*result.find(bestPublisher)).second == NULL || minimumNumberOfHops == UINT_MAX) {
			/*add the publisher to the result*/
			result[bestPublisher] = new Bitvector(bestFID);
		} else {
			//cout << "/*update the FID for the publisher*/" << endl;
			Bitvector *existingFID = (*result.find(bestPublisher)).second;
			/*or the result FID*/
			*existingFID = *existingFID | bestFID;
		}
	}
}
void TMIgraph::calculateFID(string &source, string &destination, Bitvector &resultFID, unsigned int &numberOfHops, string &path)
{
	igraph_vs_t vs;
	igraph_vector_ptr_t res;
	igraph_vector_t to_vector;
	igraph_vector_t *temp_v;
	igraph_integer_t eid;
	
	/*find the vertex id in the reverse index*/
	int from = (*reverse_node_index.find(source)).second;
	igraph_vector_init(&to_vector, 1);
	VECTOR(to_vector)[0] = (*reverse_node_index.find(destination)).second;
	/*initialize the sequence*/
	igraph_vs_vector(&vs, &to_vector);
	/*initialize the vector that contains pointers*/
	igraph_vector_ptr_init(&res, 1);
	temp_v = (igraph_vector_t *) VECTOR(res)[0];
	temp_v = (igraph_vector_t *) malloc(sizeof (igraph_vector_t));
	VECTOR(res)[0] = temp_v;
	igraph_vector_init(temp_v, 1);
	/*run the shortest path algorithm from "from"*/
#if IGRAPH_V >= IGRAPH_V_0_7
	igraph_get_shortest_paths(&graph, &res, NULL, from, vs, IGRAPH_OUT,NULL,NULL);
#elif  IGRAPH_V >= IGRAPH_V_0_6
	igraph_get_shortest_paths(&graph, &res, NULL, from, vs, IGRAPH_OUT);
#else
	igraph_get_shortest_paths(&graph, &res, from, vs, IGRAPH_OUT);
#endif
	for (int j = 0; j < igraph_vector_size(temp_v); j++) {
		path += igraph_cattribute_VAS(&graph, "NODEID", VECTOR(*temp_v)[j]); //VECTOR(*temp_v)[j];
		if (j < igraph_vector_size(temp_v)-1) {
			path+="->";
		}
	}
	/*check the shortest path to each destination*/
	temp_v = (igraph_vector_t *) VECTOR(res)[0];
	/*now let's "or" the FIDs for each link in the shortest path*/
	for (int j = 0; j < igraph_vector_size(temp_v) - 1; j++) {
#if IGRAPH_V >= IGRAPH_V_0_6
		igraph_get_eid(&graph, &eid, VECTOR(*temp_v)[j], VECTOR(*temp_v)[j + 1], true, true);
#else
		igraph_get_eid(&graph, &eid, VECTOR(*temp_v)[j], VECTOR(*temp_v)[j + 1], true);
#endif
		Bitvector *lid = (*edge_LID.find(eid)).second;
		(resultFID) = (resultFID) | (*lid);
	}
	numberOfHops = igraph_vector_size(temp_v);
	if(numberOfHops == 0)
		numberOfHops=UINT_MAX;
	/*now for the destination "or" the internal linkID*/
	Bitvector *ilid = (*nodeID_iLID.find(destination)).second;
	(resultFID) = (resultFID) | (*ilid);
	//cout << "FID of the shortest path: " << resultFID.to_string() << endl;
	igraph_vector_destroy((igraph_vector_t *) VECTOR(res)[0]);
	igraph_vector_destroy(&to_vector);
	igraph_vector_ptr_destroy_all(&res);
	igraph_vs_destroy(&vs);
}

void TMIgraph::UcalculateFID(string &publisher,
							 set<string> &subscribers,
							 map<string, Bitvector *> &result,
							 map<string, string > &paths
							 )
{
	set<string>::iterator subscribers_it;
	string bestSubscriber;
	string bestPath;
	Bitvector resultFID(FID_LEN * 8);
	Bitvector bestFID(FID_LEN * 8);
	unsigned int numberOfHops = 0;
	unsigned int minimumNumberOfHops;
    minimumNumberOfHops = UINT_MAX;
	/*first add all subscribers to the result map with NULL FID*/
	for (subscribers_it = subscribers.begin(); subscribers_it != subscribers.end(); subscribers_it++) {
		string subscriber = (*subscribers_it);
		result.insert(pair<string, Bitvector *> (subscriber, NULL));
		paths.insert(pair<string, string> (subscriber, ""));
		string path;
		resultFID.clear();
		calculateFID(publisher, subscriber, resultFID, numberOfHops, path);
		cout << "TM: path: " << path << endl;
        cout << "TM: number of hops: " << numberOfHops << endl;
		/*check for the min hop-count path, i.e. nearest subscriber*/
		if (minimumNumberOfHops > numberOfHops) {
			minimumNumberOfHops = numberOfHops;
			bestSubscriber = subscriber;
			bestFID = resultFID;
			bestPath = path;
		}
	}
	/*if a path is found then pass it to the result table*/
	if (bestSubscriber != "") {
		result[bestSubscriber] = new Bitvector(bestFID);
		paths[bestSubscriber] = bestPath;
		cout << "TM: Best Subscriber: " << bestSubscriber << ", Best Path: " << paths[bestSubscriber] << ", FID: " << result[bestSubscriber]->to_string() << endl;
	}
}

void TMIgraph::calculateRVTMFIDs()
{
	map<string, int>::iterator subscriber;
	string subscriber_nodeID;
	unsigned int subscribers_count = reverse_node_index.size();
	subscriber = reverse_node_index.begin();
	for (unsigned int i = 0; i < subscribers_count; i++) {
		subscriber_nodeID = subscriber->first;
		RVFID[subscriber_nodeID] = new Bitvector(FID_LEN * 8);
		TMFID[subscriber_nodeID] = new Bitvector(FID_LEN * 8);
		TM_to_nodeFID[subscriber_nodeID] = new Bitvector(FID_LEN * 8);
		RVFID[subscriber_nodeID] = calculateFID(subscriber_nodeID, getRVNodeID());
		TMFID[subscriber_nodeID] = calculateFID(subscriber_nodeID, getNodeID());
		TM_to_nodeFID[subscriber_nodeID] = calculateFID(getNodeID(),subscriber_nodeID);
		subscriber++;
	}
}
// Copy/Pasting to avoid conflicts (FIXME: Messy)
void TMIgraph::calculateFID_weighted(set<string> &publishers, set<string> &subscribers, map<string, Bitvector *> &result, map<string, set<string> > &path_vectors, const igraph_vector_t *weights) {
	set<string>::iterator subscribers_it;
	set<string>::iterator publishers_it;
	string bestPublisher;
	Bitvector resultFID(FID_LEN * 8);
	Bitvector bestFID(FID_LEN * 8);
	unsigned int numberOfHops = 0;
	string curr_path="";
	set<string> paths_per_pub;
	string best_path;
	/*first add all publishers to the hashtable with NULL FID*/
	for (publishers_it = publishers.begin(); publishers_it != publishers.end(); publishers_it++) {
		string publ = *publishers_it;
		result.insert(pair<string, Bitvector *>(publ, NULL));
		path_vectors.insert(pair<string, set<string> >(publ, paths_per_pub));
	}
	for (subscribers_it = subscribers.begin(); subscribers_it != subscribers.end(); subscribers_it++) {
		/*for all subscribers calculate the number of hops from all publishers (not very optimized...don't you think?)*/
		unsigned int minimumNumberOfHops = UINT_MAX;
		for (publishers_it = publishers.begin(); publishers_it != publishers.end(); publishers_it++) {
			resultFID.clear();
			string str1 = (*publishers_it);
			string str2 = (*subscribers_it);
			calculateFID_weighted(str1, str2, resultFID, numberOfHops, curr_path, weights);
			
			cout<<"Testing distance from "<<str1<<"->"<<str2<< "="<<numberOfHops<<endl;
			// Urban: BUG? local pub/sub didn't work without the || part bellow
			//  || numberOfHops==UINT_MAX
			if (minimumNumberOfHops > numberOfHops) {
				minimumNumberOfHops = numberOfHops;
				bestPublisher = *publishers_it;
				bestFID = resultFID;
				best_path=curr_path;
			}
			curr_path="";
		}
		
		cout<<"Best path is: "<<best_path<<endl;
		if(!(path_vectors.empty())){
	  for(map<string, set<string> >::iterator b_b_it = path_vectors.begin(); b_b_it != path_vectors.end(); b_b_it++){
		  if ((*b_b_it).first == bestPublisher){
			  b_b_it->second.insert(best_path);
		  }
	  }
		} else{
	  set<string> add_b_path;
	  add_b_path.insert(best_path);
	  path_vectors.insert(pair<string, set<string> >(bestPublisher, add_b_path));
		}
		//cout << "best publisher " << bestPublisher << " for subscriber " << (*subscribers_it) << " -- number of hops " << minimumNumberOfHops - 1 << endl;
		if ((*result.find(bestPublisher)).second == NULL || minimumNumberOfHops == UINT_MAX) {
			/*add the publisher to the result*/
			//cout << "FID1: " << bestFID.to_string() << endl;
			result[bestPublisher] = new Bitvector(bestFID);
		} else {
			//cout << "/*update the FID for the publisher*/" << endl;
			Bitvector *existingFID = (*result.find(bestPublisher)).second;
			/*or the result FID*/
			*existingFID = *existingFID | bestFID;
		}
	}
}
void TMIgraph::calculateFID_weighted(string &source, string &destination, Bitvector &resultFID, unsigned int &numberOfHops, string &path, const igraph_vector_t *weights) {
	igraph_vs_t vs;
	igraph_vector_ptr_t res;
#if IGRAPH_V >= IGRAPH_V_0_6
	igraph_vector_ptr_t edges;
#endif
	igraph_vector_t to_vector;
	igraph_vector_t *temp_v;
	igraph_integer_t eid;
	/*find the vertex id in the reverse index*/
	int from = (*reverse_node_index.find(source)).second;
	igraph_vector_init(&to_vector, 1);
	VECTOR(to_vector)[0] = (*reverse_node_index.find(destination)).second;
	/*initialize the sequence*/
	igraph_vs_vector(&vs, &to_vector);
	/*initialize the vector that contains pointers*/
	igraph_vector_ptr_init(&res, 1);
	temp_v = (igraph_vector_t *) VECTOR(res)[0];
	temp_v = (igraph_vector_t *) malloc(sizeof (igraph_vector_t));
	VECTOR(res)[0] = temp_v;
	igraph_vector_init(temp_v, 1);
	/*run the shortest path algorithm from "from"*/
#if IGRAPH_V >= IGRAPH_V_0_6
	cout<<"v0.6 Init"<<endl;
	// Init a vector ptr
	igraph_vector_ptr_init(&edges, 1);
	VECTOR(edges)[0]=calloc(1, sizeof(igraph_vector_t));
	igraph_vector_init((igraph_vector_t*)VECTOR(edges)[0],1);
	cout<<"v0.6 Calculation..."<<endl;
#if IGRAPH_V >= IGRAPH_V_0_7
	int rc=igraph_get_shortest_paths_dijkstra(&graph, &res, &edges, from, vs, weights, IGRAPH_OUT,NULL,NULL);
	cout<<"rc="<<rc<<endl;
#else
	int rc=igraph_get_shortest_paths_dijkstra(&graph, &res, &edges, from, vs, weights, IGRAPH_OUT);
	cout<<"rc="<<rc<<endl;
#endif //IGRAPH_V >= IGRAPH_V_0_6
#else
	igraph_get_shortest_paths_dijkstra(&graph, &res, from, vs, weights, IGRAPH_OUT);
#endif
	/*construct a string of the current path represented by nodes*/
	for (int j = 0; j < igraph_vector_size(temp_v); j++) {
		path += igraph_cattribute_VAS(&graph, "NODEID", VECTOR(*temp_v)[j]); //VECTOR(*temp_v)[j];
		if (j < igraph_vector_size(temp_v)-1) {
			path+="->";
		}
	}
	/*check the shortest path to each destination*/
	temp_v = (igraph_vector_t *) VECTOR(res)[0];
	/*now let's "or" the FIDs for each link in the shortest path*/
	
#if IGRAPH_V >= IGRAPH_V_0_6
	cout<<"Constructing FID based on edges..."<<endl;
	igraph_vector_t *temp_ev = (igraph_vector_t *)VECTOR(edges)[0];
	for (int j = 0; j < igraph_vector_size(temp_ev); j++) {
		eid = VECTOR(*temp_ev)[j];
		Bitvector *lid = (*edge_LID.find(eid)).second;
		(resultFID) = (resultFID) | (*lid);
	}
#else
	for (int j = 0; j < igraph_vector_size(temp_v) - 1; j++) {
		igraph_get_eid(&graph, &eid, VECTOR(*temp_v)[j], VECTOR(*temp_v)[j + 1], true);
		Bitvector *lid = (*edge_LID.find(eid)).second;
		(resultFID) = (resultFID) | (*lid);
	}
#endif
#if IGRAPH_V >= IGRAPH_V_0_6
	numberOfHops = igraph_vector_size(temp_ev);
#else
	numberOfHops = igraph_vector_size(temp_v);
#endif
	
	if(numberOfHops == 0) numberOfHops=UINT_MAX;
	// Fix: Cause it seems that edges without weight are getting ignored from dijkstra
	// and these are the iLIDs
	if (source == destination) numberOfHops=1;
	
	/*now for the destination "or" the internal linkID*/
	Bitvector *ilid = (*nodeID_iLID.find(destination)).second;
	(resultFID) = (resultFID) | (*ilid);
	//cout << "FID of the shortest path: " << resultFID.to_string() << endl;
	igraph_vector_destroy((igraph_vector_t *) VECTOR(res)[0]);
	//     igraph_vector_destroy((igraph_vector_t *) VECTOR(edge)[0]);
	igraph_vector_destroy(&to_vector);
	igraph_vector_ptr_destroy_all(&res);
	igraph_vs_destroy(&vs);
}

