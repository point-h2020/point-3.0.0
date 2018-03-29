/*
 * Copyright (C) 2010-2016  George Parisis and Dirk Trossen
 * Copyright (C) 2015-2018  Mays AL-Naday
 * All rights reserved.
 *
 * The NAP and MONA deployment is supported by Sebastian Robitzsch
 * The ODL deployment is supported by George Petropoulos
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 3 as published by the Free Software Foundation.
 *
 * See LICENSE and COPYING for more details.
 */

#ifndef NETWORK_HPP
#define	NETWORK_HPP

/* Values for target machines running Linux: */
#define HWADDR_LABEL   "link/ether"
#define HWADDR_OFFSET  41

#define HWADDR_LABEL_FREEBSD   "ether"
#define HWADDR_OFFSET_FREEBSD  19

#define HWADDR_LABEL_DARWIN   "ether"
#define HWADDR_OFFSET_DARWIN  20



#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <set>
#include <arpa/inet.h>
#include <algorithm>
#include <bitset>
#include "bitvector.hpp"
#include <math.h>
#include <boost/math/special_functions/factorials.hpp>


using namespace std;

class NetworkConnection;
class NetworkNode;
class NS3Application;
class NS3ApplicationAttribute;

typedef struct _ifmapelement {
	int fwport;
	int prio;
	int rate_lim;
} IfMapE;
typedef map<string, vector<IfMapE> > IfPortsMap;

/**@brief The domain merging result enumeration.
 *
 * NONE_ADDED indicates that a node was successfully added in the updated topology,
 * NODE_DELETED that a node was successfully deleted, while ERROR is thrown when
 * there was an issue during nodes' merging.
 *
 */
enum MergingResult { NODE_ADDED, NODE_DELETED, ERROR };

/**@brief  transform an LID to the equivelent IPv6
 *@param lid: string the LID in string form
 *@return ipv6: string of the IPv6
 *\todo: modify param and return to Bitvector rather than string
 */
string lid2ipv6(string lid);

/**@brief (Deployment Application) a representation of a network domain as read from the configuration file.
 * 
 * It contains all network nodes with their connections.
 * 
 */
class Domain {
public:
    /**@brief constructor
     */
    Domain();
    /**@brief a vector containing all NetworkNode
     */
    vector<NetworkNode *> network_nodes;
	/**@breif a map containing all NetworkNodes, identified by their NodeIDs
	 */
	map<string, NetworkNode *> node_NODEID;
    /**@brief a pointer to a NetworkNode that is the TopologyManager of the domain.
     */
    NetworkNode *TM_node;
    /**@brief a pointer to a NetworkNode that is the Rendezvous node of the domain.
     */
    NetworkNode *RV_node;
    /**@brief number of nodes in the domain.
     */
    unsigned int number_of_nodes;
    /**@brief number of packet layer nodes (PN) in the multilayer domain. Optical nodes (ON) don't get assigned LIDs.
     */
    unsigned int number_of_pl_nodes;
    /**@brief number of connections in the domain.
     */
    unsigned int number_of_connections;
    /**@brief number of packet connections in the multilayer domain. Overlay-to-overlay connections (oo) don't get assigned LIDs.
     */
    unsigned int number_of_p_connections;
    /**@brief the length of an information identifier.
     */
    int ba_id_len;
    /**@brief the length of a LIPSIN Identifier.
     */
    int fid_len;
    /**@brief the full path of Click home.
     */
    string click_home;
    /**@brief the full path where configuration files will be written.
     */
    string write_conf;
    /**@brief the user using which deployment will remotely access network nodes.
     */
    string user;
    /**@brief whether sudo will be used when executing remote commands.
     */
    bool sudo;
    /**@brief the overlay mode. mac or ip
     */
    string overlay_mode;
    /**@brief the opendaylight sdn controller ip address
     */
    string odl_address;
    /*for NS3 deployment*/
    uint64_t id;
    /*It maps ethernet addresses to device variables' name in NS3*/
    map<string, string> address_to_device_name;
    /*a set that contains the NS3 devices already participating in a point to point connection - to avoid duplication of connections*/
    set<string> NS3devices_in_connections;
    map<string, string> label_to_node_name;
    /********************************************************************************************************/
    /**@brief It prints an ugly representation of the Domain.
     */
    void printDomainData();
    /**@brief -
     * 
     */
    void assignLIDs();
	/**@brief assign LIDs following basic resource management whereby a single LID is assigned to both directions of a link. since Ethernet prevents output interface to be the same as input one, false positive should not occur.
	 *
	 */
	void assignLIDs_rm();
    /**@brief -
     * 
     * @param LIDs
     * @param index
     */
    void calculateLID(vector<Bitvector> &LIDs, int index);
    /**@brief  transform an LID to the equivelent IPv6
     *@param lid: string the LID in string form
     *@return ipv6: string of the IPv6
     *\todo: modify param and return to Bitvector rather than string
     */
    string lid2ipv6(string lid);
    /**@brief for each network node (and if the MAC address wasn't preassigned) it will ssh and learn the MAC address for all ethernet interfaces found in the configuration file. Also supports mac_ml overlay mode.
     *
     * @param no_remote If true, MAC address detection over SSH is not attempted.
     */
    void discoverMacAddresses(bool no_remote);
    /**@brief returns the testbed IP address (dotted decimal string) given a node label.
     * 
     * @param label a node label
     * @return the testbed IP address (dotted decimal string)
     */
    string getTestbedIPFromLabel(string label);
    /**@brief It locally creates and stores all Click/Blackadder configuration files for all network nodes (depending on the running mode). Also supports mac_ml overlay mode.
     *
     * @param montoolstub generate monitor tool counter stub or not
     */
    void writeClickFiles(bool montoolstub, bool dump_supp, bool no_lnxcap, bool no_cpr=false);
    void writeOFlows(bool odl_enabled);
    void writeNS3ClickFiles();
    /**@brief Given a node label, it returns a pointer to the respective NetworkNode.
     * 
     * @param label a node label.
     * @return a pointer to a NetworkNode.
     */
    NetworkNode *findNode(string label);
    /**@brief It copies the right Click/Blackadder configuration file to the right network node.
     */
    void scpClickFiles();
    /**@brief Copies the .graphml file to the right folder of the right network node.
     * 
     * @param name the .graphml file name.
     */
    void scpTMConfiguration(string name);
    /**@brief Copies the tar gz file to all  network nodes and decompresses it at target home folder.
     * 
     * @param tgzfile to get transferred and decompressed.
     */
    void scpClickBinary(string tgzfile);
    /**@brief It ssh'es network nodes and runs Click in the correct mode (user ot kernel)
	 *
	 * @param log boolean to decide whether to log click output in a text file or not
	 * @param odl_enabled boolean to indicate whether odl is enabled
     */
    void startClick(bool log, bool odl_enabled);
    /**@brief It ssh'es network nodes and runs NAP in the correct mode (user ot kernel)
     *
     * @param log boolean to decide whether to log click output in a text file or not
     */
    void startNAP(bool log);
    /**@breif function to start the monitoring agents in each node.
     *
     */
    void startMA();
    /**@brief It starts the Topology Manager in the right network node (not implemented).
     *
     * @param log boolean to decide whether to log the TM output in a text file or not
     */
    void startTM(bool log, string &extension);
    /**@breif function to start the Link State Monitor in each node.
     *
     */
    void startLSM(bool log);
    /**@brief It ssh'es network nodes and kills Click from user space and kernel space
     *
     */
    void killClick();
    /**@brief It ssh'es network nodes and kills NAP from user space
     *
     */
    void killNAP();
    /**@brief It ssh'es network nodes and kills Monitoring Agents from user space
     *
     */
    void killMA();
    /**@brief It ssh'es network nodes and kills Link State Monitors from user space
     *
     */
    void killLSM();
    /**@breif add flows to OVS-based switch, such as pica8
     *
     */
//    void startOVS();
    /**@brief Writes the standard deployment config file format to a file.
     *
     * Writes the standard deployment config file format to a file. It is used along with barabasi albert model because the graph is autogenerated and the experiment can be repeated.
     *
     */
    string writeConfigFile(string filename);
    /**@brief for NS3 deployment - assigns device names to all devices in the topology (from eth0 to ethN)
     * Each connection will have a different ethernet interface
     */
    void assignDeviceNamesAndMacAddresses();
    NetworkNode *getNode(string label);
    string getNextMacAddress();
    void createNS3SimulationCode();
    /**@brief It merges the new node to the existing domain.
     *
     * @param new_dm The new node and its connections to be added to the existing topology.
     * @return It returns the merging result: NODE_ADDED when node is added, NODE_DELETED
     * when node is deleted, or ERROR when there is an error while merging.
     */
    MergingResult mergeDomains(Domain &new_dm);
    /**@brief It calculates a random LID
     *
     * @return The generated LID in the form of Bitvector.
     */
    Bitvector calculateLID();
    /**@brief It generates a random label, based on the number of existing network nodes.
     *
     * @return The generated label as string.
     */
    string generateLabel();
    /**@brief The existing domain's LIDs.
     */
    vector<Bitvector> lids;
    /**@brief It checks whether a label exists.
     *
     * @param label The label to be found.
     * @return True if label exists, otherwise false.
     */
    bool labelExists(string label);

    /**@brief The method which configures the ODL controller with ABM rules.
     */
    void configureOdlController();

    /**@brief The method which returns the openflow identifier of a node, by its label.
     *
     * @param label The label to be checked.
     * @return The openflow identifier of the node -if exists-
     */
    string openflowIdByLabel(string label);

};

class NetworkNode {
public:
    NetworkConnection *getConnection(string src_label, string dst_label);
    /***members****/
	string testbed_ip; //read from configuration file
	string label; //read from configuration file
	string name; //user friendly name, read from configuration file
    bool promisc;
	string running_mode; //user or kernel
	string operating_system; //read from configuration file /*operating system*/
	string bridge; //read from configuration file the OVS bridge for each interface in openflow switch - notice bridge is an element of both the connection and the node
    //string name; //read from configuration file /*node name*/
	string type; //optical or electronic
	string sdn_implementation; // tables or bridges
	string sdn_path; //directory path to find OVS binaries - in linux reference OVS that's /usr/bin, in PICA /ovs/bin, in other brands may be different. Default to linux $PATH when no input is provided
	string openflow_id; //the switch openflow identifier
    bool isRV; //read from configuration file
    bool isTM; //read from configuration file
    Bitvector iLid; //will be calculated
    Bitvector FID_to_RV; //will be calculated
    Bitvector FID_from_RV; //will be calculated
    Bitvector FID_to_TM; //will be calculated
    string sdn_mac; //group mac address to be used for distinquishing ICN flow rules in SDN switches
    vector<NetworkConnection *> connections;
    /*for NS3 deployment*/
    int deviceOffset;
    vector<NS3Application *> applications;
};

class NetworkConnection {
public:
    /***members****/
	string src_label; //read from configuration file
	string dst_label; //read from configuration file
    
	string src_pt; //read from configuration file the source port of an openflow switch
	string dst_pt; //read from configuration file the destination port of an openflow switch

    string src_if; //read from configuration file /*e.g. tap0 or eth1*/
    string dst_if; //read from configuration file /*e.g. tap0 or eth1*/

    string src_ip; //read from configuration file /*an IP address - i will not resolve mac addresses in this case*/
    string dst_ip; //read from configuration file /*an IP address - i will not resolve mac addresses in this case*/

    string src_mac; //will be retrieved using ssh
    string dst_mac; //will be retrieved using ssh
    
    string in_pt; //input node for overlay node, read from configuration file /*e.g. in_pt="1"*/
    string out_pt; //output port for overlay node read from configuration file /*e.g. out_pt="1"*/
    string bridge; //read from configuration file the OVS bridge for each interface in openflow switch - notice bridge is an element of both the connection and the node
	
	string name; // freindly name, read from configuration file
    string proto_type; // 0x86dd if dst_pt != "" - i.e. BA->SDN connection, 0x080a if dst_if != "" - i.e. BA->BA or SDN->BA connection
    string lnk_type; //optical or electronic
    Bitvector LID; //will be calculated
    string LID_IPv6; //the LID transfromation to IPv6
    /*NS3 related attributes*/
    int mtu;
    string rate;
    string delay;
        
    int priority; //link priority. If not there is set to 0 (default - BE)
    int rate_lim; //link rate limit for the bandwidth shaper. If not there is set to 10Gbps

    string ff_pt; //the failover port, read from configuration file /*e.g. ff_pt="1"*/
};

class NS3Application {
public:
	string name;
	string start;
	string stop;
	vector<NS3ApplicationAttribute *> attributes;
};

class NS3ApplicationAttribute {
public:
	string name;
	string value;
};

#endif	/* NETWORK_HPP */

