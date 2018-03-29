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

#include <map>
#include <sstream>
#include <iomanip>
#include "network.hpp"
#include "odl_configuration.hpp"

Domain::Domain() {
    TM_node = NULL;
    RV_node = NULL;
    number_of_nodes = 0;
    number_of_connections = 0;
    number_of_pl_nodes = 0;
    number_of_p_connections = 0;
}

void Domain::printDomainData() {
    int no_nodes;
    no_nodes = network_nodes.size();
    //cout << "RV NODE: " << RV_node->label << endl;
    //cout << "TM NODE: " << TM_node->label << endl;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        cout << "****************************" << endl;
        cout << "Node " << nn->label << endl;
        cout << "Node Name " << nn->name << endl;
        if (overlay_mode.compare("mac_ml") == 0) {
            cout << "Node_Type " << nn->type << endl;
        }
        cout << "testbed_ip " << nn->testbed_ip << endl;
        cout << "operating_system " << nn->operating_system << endl;
        cout << "promisc mode" << nn->promisc << endl;
        cout << "*****bridge (when operating_system=ovs) " << nn->bridge
        << endl;
        cout << "*****sdn_implementation (if SDN)" << nn->sdn_implementation
        << endl;
        cout << "*****sdn_mac (when operating_system=ovs)" << nn->sdn_mac << endl;
        cout << "*****openflow_id (when operating_system=ovs) "
        << nn->openflow_id << endl;
        cout << "iLid " << nn->iLid.to_string() << endl;
        cout << "isRV " << nn->isRV << endl;
        cout << "isTM " << nn->isTM << endl;
        cout << "FID_to_RV " << nn->FID_to_RV.to_string() << endl;
        cout << "FID_to_TM " << nn->FID_to_TM.to_string() << endl;
        cout << "****************************" << endl;
        for (size_t j = 0; j < nn->connections.size(); j++) {
            NetworkConnection *nc = nn->connections[j];
            cout << "*****Name " << nc->name << endl;
            cout << "*****src_label " << nc->src_label << endl;
            cout << "*****dst_label " << nc->dst_label << endl;
            cout << "*****src_if " << nc->src_if << endl;
            cout << "*****dst_if " << nc->dst_if << endl;
            cout << "*****src_ip " << nc->src_ip << endl;
            cout << "*****dst_ip " << nc->dst_ip << endl;
            cout << "*****src_mac " << nc->src_mac << endl;
            cout << "*****dst_mac " << nc->dst_mac << endl;
            cout << "*****src_pt " << nc->src_pt << endl;
            cout << "*****dst_pt " << nc->dst_pt << endl;
            cout << "*****LID " << nc->LID.to_string() << endl;
            if (overlay_mode.compare("mac_ml") == 0) {
                cout << "*****in_pt " << nc->in_pt << endl;
                cout << "*****out_pt " << nc->out_pt << endl;
                cout << "*****lnk_type " << nc->lnk_type << endl;
            }
        }
    }
}

bool exists(vector<Bitvector> &LIDs, Bitvector &LID) {
    for (size_t i = 0; i < LIDs.size(); i++) {
        if (LIDs[i] == LID) {
            //cout << "duplicate LID" << endl;
            return true;
        }
    }
    return false;
}

NetworkNode *Domain::findNode(string label) {
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        if (nn->label.compare(label) == 0) {
            return nn;
        }
    }
    return NULL;
}

/**\todo: convert in/out args to bitvector rather than string
 *\toddo: align the code with the deployment style
 */
// converts an lid to the equivalent IPV6 representation. If the
// lid is 256 bits, then it will be ipv6src:ipv6dst
// see RFC1700 for byte order!
string Domain::lid2ipv6(string lid) {
    ostringstream os;
    std::reverse(lid.begin(), lid.end());
    int lipsinlen = 256;    //lid.length();
                            //note ceiling of lipsinlen/32
    for (int k = 0; k < 1 + (lipsinlen - 1) / 16; k++) {
        int pos = k * 16;
        int len = pos + 16 > lipsinlen ? lipsinlen - pos : 16;
        string tmp = lid.substr(pos, len);
        std::reverse(tmp.begin(), tmp.end());
        bitset<16> b(tmp);
        os << setfill('0') << setw(4) << hex << htons(b.to_ulong());
        if (k < (lipsinlen - 1) / 16) {
            os << ":";
        }
    }
    return (os.str());
}

void Domain::calculateLID(vector<Bitvector> &LIDs, int index) {
    int bit_position;
    int number_of_bits = (index / (fid_len * 8)) + 1;
    Bitvector LID;
    do {
        LID = Bitvector(fid_len * 8);
        for (int i = 0; i < number_of_bits; i++) {
            /*assign a bit in a random position*/
            bit_position = rand() % (fid_len * 8);
            LID[bit_position] = true;
        }
    } while (exists(LIDs, LID));
    LIDs[index] = LID;
}

/** Required method for dynamic deployment tool.
 * It calculates and returns random LID.
 */
Bitvector Domain::calculateLID() {
    int bit_position;
    Bitvector LID;
    do {
        LID = Bitvector(fid_len * 8);
        /**assign a bit in a random position
         */
        bit_position = rand() % (fid_len * 8);
        LID[bit_position] = true;
    } while (exists(lids, LID));
    lids.push_back(LID);
    return LID;
}

void Domain::assignLIDs() {
    int LIDCounter = 0;
    srand(0);
    //srand(66000);
    /*first calculated how many LIDs should I calculate*/
    int totalLIDs;
    bool mac_ml = (overlay_mode.compare("mac_ml") == 0);
    if (mac_ml) {
        cout << "Number of iLIDs is: " << number_of_pl_nodes
        << ", Number of LIDs is: " << number_of_p_connections
        << ", Number of Total connections is: " << number_of_connections
        << endl;
        totalLIDs = number_of_pl_nodes/*the iLIDs*/+ number_of_p_connections;
        // here to add pn node and pp connection count to eleminate the assignment of Lids for the oo links and oe-links
    } else {
        totalLIDs = number_of_nodes/*the iLIDs*/+ number_of_connections;
    }
    vector<Bitvector> LIDs(totalLIDs);
    vector<string> LIDs_IPv6(totalLIDs);
    for (int i = 0; i < totalLIDs; i++) {
        calculateLID(LIDs, i);
    }
#if 0
    for (int i = 0; i < totalLIDs; i++) {
        cout << "LID " << i << " : " << LIDs.at(i).to_string() << endl;
        LIDs_IPv6[i] = lid2ipv6(LIDs.at(i).to_string());
        cout << "IPv6 " << i << " : " << LIDs_IPv6[i] << endl;
    }
#endif
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        if (mac_ml) {
            if (nn->type == "PN") { /* packet layer node */
                nn->iLid = LIDs[LIDCounter];
                LIDCounter++;
            }
        } else {
            nn->iLid = LIDs[LIDCounter];
            LIDCounter++;
        }
        for (size_t j = 0; j < nn->connections.size(); j++) {
            NetworkConnection *nc = nn->connections[j];
            if (mac_ml) {
                if ((nc->lnk_type == "pp")) { /* packet-to-packet connection */
                    nc->LID = LIDs[LIDCounter];
                    LIDCounter++;
                } else {
                    nc->LID = Bitvector(fid_len * 8, 0);
                }
            } else {
                nc->LID = LIDs[LIDCounter];
                nc->LID_IPv6 = LIDs_IPv6[LIDCounter];
                LIDCounter++;
            }
        }
    }
    lids = LIDs;
    for (int i = 0; i < LIDCounter; i++) {
        cout << "LID " << i << " : " << LIDs.at(i).to_string() << endl;
        cout << "IPv6 " << i << " : " << LIDs_IPv6.at(i) << endl;
    }
}
/*This method is used for assigning sparce LIDs in large scale newtworks
 * it assings the same LID to both directions of link, plus iLID is the same for nodes with single connections (i.e. leaf nodes)
 */
void Domain::assignLIDs_rm() {
    unsigned int LIDsCounter = 0;
    Bitvector LID = Bitvector(fid_len * 8);
    string LID_IPv6;
    for (unsigned int i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        unsigned int node_connections = nn->connections.size();
        for (unsigned int j = 0; j < node_connections; j++) {
            if (nn->connections[j]->LID.zero()) {
                /*LIDs have not been assigned yet, generate LIDs for both directions of the link*/
                LID.clear();
                NetworkConnection *nc = nn->connections[j];
                LID = calculateLID();
                nc->LID = LID;
                LID_IPv6 = lid2ipv6(LID.to_string());
                cout << "LID: " << LID.to_string() << endl;
                cout << "IPv6: " << LID_IPv6 << endl;
                /*find the destination node of the connection*/
                NetworkNode * an = node_NODEID[nc->dst_label];
                unsigned int an_connections = an->connections.size();
                for (unsigned int k = 0; k < an_connections; k++) {
                    NetworkConnection *ac = an->connections[k];
                    if (ac->dst_label == nn->label) {
                        cout << "bidirectional LID for " << nn->label << "<->"
                        << an->label << endl;
                        ac->LID = LID;
                    }
                }
                if (node_connections == 1) {
                    /*leaf node therefore iLID can also be the same as LID*/
                    nn->iLid = LID;
                    cout << nn->label << " iLID: " << LID.to_string() << endl;
                }
                if (an_connections == 1) {
                    /*leaf node therefore iLID can also be the same as LID*/
                    an->iLid = LID;
                    cout << an->label << " iLID: " << LID.to_string() << endl;
                }
            }
        }
        if (node_connections > 1) {
            LID.clear();
            LID = calculateLID();
            nn->iLid = LID;
            cout << nn->label << " iLID: " << LID.to_string() << endl;
        }
    }
}
string Domain::getTestbedIPFromLabel(string label) {
    NetworkNode *nn = findNode(label);
    if (nn)
        return nn->testbed_ip;
    return "";
}

static void getHwaddrLabel(const string &os, string &hwaddr_label,
                           int &hwaddr_offset) {
    if (os.compare("Linux") == 0) {
        hwaddr_label = HWADDR_LABEL;
        hwaddr_offset = HWADDR_OFFSET;
    } else if (os.compare("FreeBSD") == 0) {
        hwaddr_label = HWADDR_LABEL_FREEBSD;
        hwaddr_offset = HWADDR_OFFSET_FREEBSD;
    } else if (os.compare("Darwin") == 0) {
        hwaddr_label = HWADDR_LABEL_DARWIN;
        hwaddr_offset = HWADDR_OFFSET_DARWIN;
    } else {
        hwaddr_label = HWADDR_LABEL;
        hwaddr_offset = HWADDR_OFFSET;
    }
}

void Domain::discoverMacAddresses(bool no_remote) {
    map<string, string> mac_addresses;
    string testbed_ip;
    string line;
    string mac_addr;
    FILE *fp_command;
    char response[1035];
    string command;
    string hwaddr_label;
    int hwaddr_offset;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        for (size_t j = 0; j < nn->connections.size(); j++) {
            NetworkConnection *nc = nn->connections[j];
            if (overlay_mode.compare("mac") == 0
                || overlay_mode.compare("mac_ml") == 0
                || overlay_mode.compare("mac_qos") == 0) {
                if (overlay_mode.compare("mac_ml") == 0
                    && (nn->type != "PN" || nc->src_if == "")) {
                    continue;
                }
                if (mac_addresses.find(nc->src_label + nc->src_if) == mac_addresses.end()) {
                    /*get source mac address*/
                    if (nc->src_mac.length() == 0 && nc->src_pt.length() == 0 && !no_remote) {
                        NetworkNode *src_node = findNode(nc->src_label);
                        if (src_node) {
                            getHwaddrLabel(src_node->operating_system, hwaddr_label, hwaddr_offset);
                            testbed_ip = src_node->testbed_ip;
                        }
                        if (sudo) {
                            command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + testbed_ip + " -t \"sudo ip addr show dev " + nc->src_if + " | grep " + hwaddr_label + "\"";
                        } else {
                            command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + testbed_ip + " -t \"ip addr show dev" + nc->src_if + " | grep " + hwaddr_label + "\"";
                        }
                        cout << command << endl;
                        fp_command = popen(command.c_str(), "r");
                        if (fp_command == NULL) {
                            printf("Failed to run command\n");
                            goto close_src;
                        }
                        /* Read the output a line at a time - output it. */
                        if (fgets(response, sizeof (response) - 1, fp_command) == NULL) {
                            cout << "Error or empty response." << endl;
                            goto close_src;
                        }
                        line = string(response);
                        mac_addr = line.substr(line.length() - hwaddr_offset, 17);
                        cout << mac_addr << endl;
                        nc->src_mac = mac_addr;
                        /* close */
                    close_src:
                        pclose(fp_command);
                    } else if (nc->src_mac.length() != 0) {
                        cout << nn->label << ": I already know the src mac address (" << nc->src_mac << ") for this connection (" << nc->src_label << ":" << nc->src_if << " -> " << nc->dst_label << ":" << nc->dst_if  << ")...it was hardcoded in the configuration file" << endl;
                    } else {
                        nc->src_mac = "00:00:00:00:00:00"; /* XXX */
                    }
                    mac_addresses[nc->src_label + nc->src_if] = nc->src_mac;
                } else {
                    nc->src_mac = mac_addresses[nc->src_label + nc->src_if];
                    //cout << "I learned this mac address: " << nc->src_label << ":" << nc->src_if << " - " << mac_addresses[nc->src_label + nc->src_if] << endl;
                }
                if (overlay_mode.compare("mac_ml") == 0
                    && (nn->type != "PN" || nc->dst_if == "")) {
                    continue;
                }
                if (mac_addresses.find(nc->dst_label + nc->dst_if) == mac_addresses.end()) {
                    /*get destination mac address*/
                    if (nc->dst_mac.length() == 0 && nc->dst_pt.length() == 0 && !no_remote) {
                        NetworkNode *dst_node = findNode(nc->dst_label);
                        if (dst_node) {
                            getHwaddrLabel(dst_node->operating_system, hwaddr_label, hwaddr_offset);
                            testbed_ip = dst_node->testbed_ip;
                        }
                        if (sudo) {
                            command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + testbed_ip + " -t \"sudo ip addr show dev " + nc->dst_if + " | grep "+ hwaddr_label + "\"";
                        } else {
                            command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + testbed_ip + " -t \"ip addr show dev " + nc->dst_if + " | grep " + hwaddr_label + "\"";
                        }
                        cout << command << endl;
                        fp_command = popen(command.c_str(), "r");
                        if (fp_command == NULL) {
                            printf("Failed to run command\n");
                            goto close_dst;
                        }
                        /* Read the output a line at a time - output it. */
                        if (fgets(response, sizeof (response) - 1, fp_command) == NULL) {
                            cout << "Error or empty response." << endl;
                            goto close_dst;
                        }
                        line = string(response);
                        mac_addr = line.substr(line.length() - hwaddr_offset, 17);
                        cout << mac_addr << endl;
                        nc->dst_mac = mac_addr;
                        /* close */
                    close_dst:
                        pclose(fp_command);
                    } else if (nc->dst_mac.length() != 0) {
                        cout << nn->label << ": I already know the dst mac address (" << nc->dst_mac << ") for this connection (" << nc->src_label << ":" << nc->src_if << " -> " << nc->dst_label << ":" << nc->dst_if  << ")...it was hardcoded in the configuration file" << endl;
                    } else {
                        nc->dst_mac = "00:00:00:00:00:00"; /* XXX */
                    }
                    mac_addresses[nc->dst_label + nc->dst_if] = nc->dst_mac;
                } else {
                    nc->dst_mac = mac_addresses[nc->dst_label + nc->dst_if];
                    //cout << "I learned this mac address: " << nc->dst_label << ":" << nc->dst_if << " - " << mac_addresses[nc->src_label + nc->src_if] << endl;
                }
            } else {
                //cout << "no need to discover mac addresses - it's a connection over raw IP sockets" << endl;
            }
        }
    }
}

NetworkNode *Domain::getNode(string label) {
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        if (nn->label.compare(label) == 0) {
            return nn;
        }
    }
    return NULL;
}

NetworkConnection *NetworkNode::getConnection(string src_label,
                                              string dst_label) {
    for (size_t i = 0; i < connections.size(); i++) {
        NetworkConnection *nc = connections[i];
        if ((nc->src_label.compare(src_label) == 0)
            && (nc->dst_label.compare(dst_label) == 0)) {
            return nc;
        }
    }
    return NULL;
}
///*check if I have already considered this connection (the other way) before*/
//            for (int k = 0; k < network_nodes.size(); k++) {
//                NetworkNode *neighbourNode = network_nodes[k];
//                if(neighbourNode->label.compare())
//            }

//Martin wrote this func?
string Domain::getNextMacAddress() {
    uint8_t m_address[6];
    string mac_str;
    std::stringstream ss;
    m_address[0] = (id >> 40) & 0xff;
    m_address[1] = (id >> 32) & 0xff;
    m_address[2] = (id >> 24) & 0xff;
    m_address[3] = (id >> 16) & 0xff;
    m_address[4] = (id >> 8) & 0xff;
    m_address[5] = (id >> 0) & 0xff;
    id++;
    ss.setf(std::ios::hex, std::ios::basefield);
    
    ss.fill('0');
    for (uint8_t i = 0; i < 5; i++) {
        ss << std::setw(2) << (uint32_t) m_address[i] << ":";
    }
    // Final byte not suffixed by ":"
    ss << std::setw(2) << (uint32_t) m_address[5];
    ss.setf(std::ios::dec, std::ios::basefield);
    ss.fill(' ');
    mac_str = ss.str();
    return mac_str;
}

/*probably the most inefficient way of doing it!!*/
void Domain::assignDeviceNamesAndMacAddresses() {
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        cout << "Looking at Node " << nn->label << ", Device Offset "
        << nn->deviceOffset << endl;
        for (size_t j = 0; j < nn->connections.size(); j++) {
            NetworkConnection *nc = nn->connections[j];
            cout << "Looking at Connection " << nc->src_label << " -> "
            << nc->dst_label << endl;
            /*check if I have already considered this connection (the other way) before*/
            if ((nc->src_if.empty()) && (nc->dst_if.empty())) {
                std::stringstream out1;
                out1 << nn->deviceOffset;
                nc->src_if = "eth" + out1.str();
                nc->src_mac = getNextMacAddress();
                std::stringstream out2;
                NetworkNode *neighbour = getNode(nc->dst_label);
                out2 << neighbour->deviceOffset;
                nc->dst_if = "eth" + out2.str();
                nc->dst_mac = getNextMacAddress();
                cout << "Assigning " << nn->label << ", device offset "
                << nn->deviceOffset << ", device name " << nc->src_if
                << ", MAC Address " << nc->src_mac << endl;
                cout << "Assigning " << neighbour->label << ", device offset "
                << neighbour->deviceOffset << ", device name "
                << nc->dst_if << ", MAC Address " << nc->dst_mac
                << endl;
                nn->deviceOffset++;
                neighbour->deviceOffset++;
                /*find the connection from the neighbour node and assign the same interfaces*/
                NetworkConnection *neighbourConnection =
                neighbour->getConnection(nc->dst_label, nc->src_label);
                neighbourConnection->src_if = nc->dst_if;
                neighbourConnection->dst_if = nc->src_if;
                neighbourConnection->src_mac = nc->dst_mac;
                neighbourConnection->dst_mac = nc->src_mac;
            } else {
                cout << "Already checked that connection" << endl;
                cout << "device name " << nc->src_if << ", MAC Address "
                << nc->src_mac << endl;
                cout << "device name " << nc->dst_if << ", MAC Address "
                << nc->dst_mac << endl;
            }
        }
    }
}

void Domain::createNS3SimulationCode() {
    ofstream ns3_code;
    ns3_code.open((write_conf + "topology.cpp").c_str());
    /*First write all required stuff before starting to build the actual topology*/
    ns3_code << "#include <ns3/core-module.h>" << endl;
    ns3_code << "#include <ns3/network-module.h>" << endl;
    ns3_code << "#include <ns3/point-to-point-module.h>" << endl;
    ns3_code << "#include <ns3/blackadder-module.h>" << endl;
    ns3_code << "#include \"publisher.h\"" << endl; //Added publisher.h as a required header file in NS3 simulation code.
    ns3_code << "#include \"subscriber.h\"" << endl; //Added subscriber.h as a required header file in NS3 simulation code.
    ns3_code << "using namespace ns3;" << endl;
    ns3_code << "NS_LOG_COMPONENT_DEFINE(\"topology\");" << endl;
    ns3_code << "int main(int argc, char *argv[]) {" << endl;
    /*create all nodes now*/
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        ns3_code << "   Ptr<Node> node" << i << " = CreateObject<Node>();"
        << endl;
        std::stringstream out2;
        out2 << "node" << i;
        label_to_node_name.insert(pair<string, string>(nn->label, out2.str()));
        for (size_t j = 0; j < nn->connections.size(); j++) {
            NetworkConnection *nc = nn->connections[j];
            ns3_code << "   Ptr<PointToPointNetDevice> dev" << i << "_" << j
            << " = Create<PointToPointNetDevice>();" << endl;
            ns3_code << "   dev" << i << "_" << j
            << "->SetAddress (Mac48Address(\"" << nc->src_mac << "\"));"
            << endl;
            ns3_code << "   dev" << i << "_" << j
            << "->SetDataRate (DataRate(\"" << nc->rate << "\"));"
            << endl;
            ns3_code << "   dev" << i << "_" << j << "->SetMtu (" << nc->mtu
            << ");" << endl;
            std::stringstream out1;
            out1 << "dev" << i << "_" << j;
            address_to_device_name.insert(
                                          pair<string, string>(nc->src_mac, out1.str()));
            ns3_code << "   node" << i << "->AddDevice(dev" << i << "_" << j
            << ");" << endl;
            /*The type of queue can be a parameter in the initial topology file that is parsed (along with its attributes)*/
            ns3_code << "   Ptr<DropTailQueue> queue" << i << "_" << j
            << " = CreateObject<DropTailQueue > ();" << endl;
            ns3_code << "   dev" << i << "_" << j << "->SetQueue(queue" << i
            << "_" << j << ");" << endl;
        }
        ns3_code << endl;
    }
    int channel_counter = 0;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        for (size_t j = 0; j < nn->connections.size(); j++) {
            NetworkConnection *nc = nn->connections[j];
            if ((NS3devices_in_connections.find(nc->src_mac)
                 == NS3devices_in_connections.end())
                && (NS3devices_in_connections.find(nc->dst_mac)
                    == NS3devices_in_connections.end())) {
                    nc->proto_type = "0x080a";
                    ns3_code << "   Ptr<PointToPointChannel> channel"
                    << channel_counter
                    << " = CreateObject<PointToPointChannel>();" << endl;
                    ns3_code << "   channel" << channel_counter
                    << "->SetAttribute(\"Delay\", StringValue(\""
                    << nc->delay << "\"));" << endl;
                    ns3_code << "   "
                    << (*address_to_device_name.find(nc->src_mac)).second
                    << "->Attach(channel" << channel_counter << ");"
                    << endl;
                    ns3_code << "   "
                    << (*address_to_device_name.find(nc->dst_mac)).second
                    << "->Attach(channel" << channel_counter << ");"
                    << endl;
                    NS3devices_in_connections.insert(nc->src_mac);
                    NS3devices_in_connections.insert(nc->dst_mac);
                    NS3devices_in_connections.insert(nc->proto_type);
                    channel_counter++;
                    ns3_code << endl;
                }
        }
    }
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        ns3_code << "   Ptr<ClickBridge> click" << i
        << " = CreateObject<ClickBridge > ();" << endl;
        ns3_code << "   node" << i << "->AggregateObject(click" << i << ");"
        << endl;
        ns3_code << "   click" << i << "->SetClickFile(\"" << write_conf << "/"
        << nn->label << ".conf\");" << endl;
        ns3_code << "   Ptr<ServiceModel> servModel" << i
        << " = CreateObject<ServiceModel > (); " << endl;
        ns3_code << "   node" << i << "->AggregateObject(servModel" << i
        << "); " << endl;
    }
    ns3_code << endl;
    
    /*Start the topology manager application to the respective network node*/
    ns3_code
    << "   Ptr<TopologyManager> tm = CreateObject<TopologyManager > ();"
    << endl;
    ns3_code << "   tm->SetStartTime(Seconds(0.)); " << endl;
    ns3_code << "   tm->SetAttribute(\"Topology\", StringValue(\"" << write_conf
    << "topology.graphml\"));" << endl;
    ns3_code << "   " << (*label_to_node_name.find(TM_node->label)).second
    << "->AddApplication(tm); " << endl;
    ns3_code << endl;
    /*add applications and attributes*/
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        for (size_t j = 0; j < nn->applications.size(); j++) {
            NS3Application *app = nn->applications[j];
            ns3_code << "   Ptr<" << app->name << "> app" << i << "_" << j
            << " = CreateObject<" << app->name << "> ();" << endl;
            ns3_code << "   " << "app" << i << "_" << j
            << "->SetStartTime(Seconds(" << app->start << ")); "
            << endl;
            ns3_code << "   " << "app" << i << "_" << j
            << "->SetStopTime(Seconds(" << app->stop << ")); " << endl;
            for (size_t k = 0; k < app->attributes.size(); k++) {
                NS3ApplicationAttribute *attr = app->attributes[k];
                ns3_code << "   " << "app" << i << "_" << j
                << "->SetAttribute(\"" << attr->name << "\", "
                << attr->value << ");" << endl;
            }
            ns3_code << "   node" << i << "->AddApplication(app" << i << "_"
            << j << ");" << endl;
            ns3_code << endl;
        }
    }
    ns3_code << "   Simulator::Run();" << endl;
    ns3_code << "   Simulator::Destroy();" << endl;
    ns3_code << "   return 0;" << endl;
    
    ns3_code << "}" << endl;
    ns3_code.close();
}

int findOffset(vector<string> &unique, string &str) {
    for (size_t i = 0; i < unique.size(); i++) {
        if (unique[i].compare(str) == 0) {
            return i;
        }
    }
    return -1;
}

string Domain::openflowIdByLabel(string label) {
	for (size_t i = 0; i < network_nodes.size(); i++) {
		if (network_nodes[i]->label.compare(label) == 0)
			return network_nodes[i]->openflow_id;
	}
	return NULL;
}


void Domain::configureOdlController() {
	string ipv6_h1;
	string ipv6_h2;
	int node_no_connections;
	Bitvector mc_FID = Bitvector(fid_len * 8);
	int flow_counter = 0;
	OdlConfiguration oc = OdlConfiguration(odl_address);
	for (size_t i = 0; i < network_nodes.size(); i++) {
		vector<string> unique_srcips;
		NetworkNode *nn = network_nodes[i];
		if (nn->openflow_id.empty())
			continue;
		else {
			ostringstream nr_ss;
			nr_ss << "{\"input\": {"
					<< "\"noneName\": \"" << nn->openflow_id << "\","
					<< "\"noneId\": \"" << nn->label << "\"} }";
			string node_registry_entry = nr_ss.str();
			oc.configureNodeRegistry(node_registry_entry);
		}
		if (nn->operating_system.compare("ovs") == 0) {
			node_no_connections = nn->connections.size();
			if (nn->sdn_implementation.compare("bridges") == 0) {
				/*Bloom-filtered LIDs for connections of a bridge*/
				map<string, Bitvector> mc_FID;
				/*connections (ports) of the same bridge - notice connections cannot be to ports of different bridges*/
				map<string, vector<NetworkConnection*> > bridge_connections;
				/*bridge of the connection*/
				string bridge;
				for (size_t c = 0; c < node_no_connections; c++) {
					bridge = nn->connections[c]->bridge;
					(bridge_connections[bridge]).push_back(nn->connections[c]);
					if (mc_FID.find(bridge) == mc_FID.end()) {
						mc_FID[bridge] = Bitvector(fid_len * 8);
					}

					string dst_node = openflowIdByLabel(nn->connections[c]->dst_label);
					if (dst_node.empty()) {
						dst_node = "host:" + nn->connections[c]->dst_mac;

						ostringstream nr_ss;
						nr_ss << "{\"input\": {" << "\"noneName\": \""
								<< dst_node << "\"," << "\"noneId\": \""
								<< nn->connections[c]->dst_label << "\"} }";
						string node_registry_entry = nr_ss.str();
						oc.configureNodeRegistry(node_registry_entry);
					}
					ostringstream ncr_ss;
					ncr_ss << "{\"input\": {\"noneConnectorName\": \""
							<< nn->openflow_id << ":"
							<< nn->connections[c]->src_pt << "\","
							<< "\"srcNode\": \"" << nn->openflow_id << "\","
							<< "\"dstNode\": \"" << dst_node << "\"}}";

					string node_connector_registry_entry = ncr_ss.str();
					oc.configureNodeConnectorRegistry(
							node_connector_registry_entry);

					//if ff_port is not empty, then configure the fast failover group
					if (!nn->connections[c]->ff_pt.empty()) {
						std::stringstream group_stream;
						group_stream << (i+1) << nn->connections[c]->src_pt;
						string group_id = group_stream.str();
						ostringstream ff_group_ss;
						ff_group_ss << "{\"group\": {"
								<< "\"group-type\": \"group-ff\","
								<< "\"buckets\": {"
								<< "\"bucket\": [{"
								<< "\"action\": {"
								<< "\"output-action\": { \"output-node-connector\": \""
								<< nn->connections[c]->src_pt << "\" },"
								<< "\"order\": \"0\"},"
								<< "\"bucket-id\": \"1\","
								<< "\"watch_port\": \"" << nn->connections[c]->src_pt << "\"},{"
								<< "\"action\": {"
								<< "\"output-action\": { \"output-node-connector\": \""
								<< nn->connections[c]->ff_pt << "\" },"
								<< "\"order\": \"0\"},"
								<< "\"bucket-id\": \"2\","
								<< "\"watch_port\": \"" << nn->connections[c]->ff_pt << "\"}]},"
								<< "\"barrier\": \"false\","
								<< "\"group-name\": \"group\","
								<< "\"group-id\": \"" << group_id << "\"}}";

						string group = ff_group_ss.str();
						oc.configureGroup(nn->openflow_id, group, atoi(group_id.c_str()));
					}
				}
				for (map<string, Bitvector>::iterator it = mc_FID.begin();
						it != mc_FID.end(); it++) {
					bridge = (*it).first;
					int bridge_no_connections =
							bridge_connections[bridge].size();
					/*create the priorities array to provide unique and incremenal priority values for the connections of each bridge*/
					unsigned int priority_counter[bridge_no_connections];
					/*some compilers complaint about initializing variable length array with declaration, memset is the safest choice*/
					memset(priority_counter, 0,
							bridge_no_connections * sizeof(int));
					for (size_t f = 1; f < bridge_no_connections; f++) {
						int priority_coef = boost::math::factorial<double>(
								bridge_no_connections)
								/ (boost::math::factorial<double>(f)
										* boost::math::factorial<double>(
												bridge_no_connections - f));
						priority_counter[f] = priority_counter[f - 1]
								+ priority_coef;
					}
					int priority_base = 1000;
					int priority = 0;
					int combinations = pow(2, bridge_no_connections);
					for (int j = 1; j < combinations; j++) {
						std::bitset<64> pos(j);
						string output_ports;
						mc_FID[bridge].clear();
						int no_links = 0;
						ostringstream actions_ss;
						actions_ss << "\"action\": [";
						for (size_t k = 0; k < bridge_no_connections; k++) {
							if (pos[k] == 1) {
								mc_FID[bridge] = mc_FID[bridge]
										| bridge_connections[bridge][k]->LID;
								if (no_links > 0) {
									output_ports = output_ports + ",";
									actions_ss << ",";
								}
								output_ports = output_ports
										+ bridge_connections[bridge][k]->src_pt;
								no_links++;
								//if ff_port is empty, then configure output port action -as usual-
								//otherwise configure the group action
								if (bridge_connections[bridge][k]->ff_pt.empty()) {
									actions_ss << "{\"order\": " << k << ","
											<< "\"output-action\": { \"output-node-connector\": "
											<< bridge_connections[bridge][k]->src_pt
											<< " } }";
								}
								else {
									std::stringstream group_stream;
									group_stream << (i+1) << bridge_connections[bridge][k]->src_pt;
									string group_id = group_stream.str();
									actions_ss << "{\"order\": " << k << ","
											<< "\"group-action\": { \"group-id\": "
											<< group_id
											<< " } }";
								}
							}
						}
						actions_ss << "]";
						string actions = actions_ss.str();
						ipv6_h1 = lid2ipv6(mc_FID[bridge].to_string()).substr(0,
								39);
						ipv6_h2 = lid2ipv6(mc_FID[bridge].to_string()).substr(
								40, 79);
						/*formula to increase the priority based on the number of bitpositions set to 1, because multicast rules should have higher priority than unicast or less multicast*/
						priority = priority_base
								+ priority_counter[no_links - 1];
						priority_counter[no_links - 1] += 1;

						ostringstream ss;
						ss
								<< "{ \"flow\": { \"barrier\": \"false\", \"cookie\": \"10\", "
								<< "\"cookie_mask\": \"10\", \"flow-name\": \"flow\", "
								<< "\"id\": \"" << flow_counter
								<< "\", \"priority\": \"" << priority << "\", "
								<< "\"table_id\": \"0\", \"instructions\": { \"instruction\": "
								<< "{ \"apply-actions\": {" << actions << " }, "
								<< "\"order\": \"0\" } }, \"match\": { \"ethernet-match\": "
								<< "{ \"ethernet-type\": { \"type\": \"34525\" }, "
								<< "\"ethernet-destination\": { \"address\": \"00:00:00:00:00:00\" } }, "
								<< "\"ipv6-source-address-no-mask\": \""
								<< ipv6_h1 << "\", "
								<< "\"ipv6-source-arbitrary-bitmask\": \""
								<< ipv6_h1 << "\", "
								<< "\"ipv6-destination-address-no-mask\": \""
								<< ipv6_h2 << "\", "
								<< "\"ipv6-destination-arbitrary-bitmask\": \""
								<< ipv6_h2 << "\" } } }";
						string flow = ss.str();
						oc.configureFlow(nn->openflow_id, 0, flow, flow_counter);
						flow_counter++;
					}
				}
			}
			else {
				for (size_t j = 0; j < node_no_connections; j++) {
					NetworkConnection *nc = nn->connections[j];
					ipv6_h1 = lid2ipv6(nc->LID.to_string()).substr(0, 39);
					ipv6_h2 = lid2ipv6(nc->LID.to_string()).substr(40, 79);
					std::stringstream group_stream;
					group_stream << (i+1) << nc->src_pt;
					string group_id = group_stream.str();
					//if ff_port is not empty, then configure the fast failover group
					if (!nc->ff_pt.empty()) {
						ostringstream ff_group_ss;
						ff_group_ss << "{\"group\": {"
								<< "\"group-type\": \"group-ff\","
								<< "\"buckets\": {" << "\"bucket\": [{"
								<< "\"action\": {"
								<< "\"output-action\": { \"output-node-connector\": \""
								<< nc->src_pt << "\" }," << "\"order\": \"0\"},"
								<< "\"bucket-id\": \"1\","
								<< "\"watch_port\": \"" << nc->src_pt << "\"},{"
								<< "\"action\": {"
								<< "\"output-action\": { \"output-node-connector\": \""
								<< nc->ff_pt << "\" }," << "\"order\": \"0\"},"
								<< "\"bucket-id\": \"2\","
								<< "\"watch_port\": \"" << nc->ff_pt << "\"}]},"
								<< "\"barrier\": \"false\","
								<< "\"group-name\": \"group\","
								<< "\"group-id\": \"" << group_id << "\"}}";

						string group = ff_group_ss.str();
						//cout << "------- group " << nn->connections[c]->src_pt << "------" << endl;
						//cout << group << endl;
						oc.configureGroup(nn->openflow_id, group,
								atoi(group_id.c_str()));
					}
					if (nn->sdn_implementation.compare("tables") == 0) {
						//if there more connections, then go to next table
						if (j < node_no_connections - 1) {
							ostringstream ss;
							//add flow rule with arbitrary bitmask and goto next table
							ss << "{ \"flow\": { \"barrier\": \"false\", \"cookie\": \"10\", "
									<< "\"cookie_mask\": \"10\", \"flow-name\": \"flow\", "
									<< "\"id\": \"" << flow_counter
									<< "\", \"priority\": \"1000\", "
									<< "\"table_id\": \"" << j << "\", "
									<< "\"instructions\": {\"instruction\": [{\"order\": \"0\","
									<< "\"apply-actions\": {\"action\": {\"order\": \"0\",";

							//if ff_port is empty, then configure output port action -as usual-
							//otherwise configure the group action
							if (nc->ff_pt.empty()) {
								ss << "\"output-action\": { \"output-node-connector\": \""
										<< nc->src_pt << "\" }}}},";
							} else {
								ss << "\"group-action\": { \"group-id\": \""
										<< group_id << "\" }}}},";
							}

							ss  << "{\"order\": \"1\",\"go-to-table\": { \"table_id\": \""
									<< j + 1 << "\" }}]}, "
									<< "\"match\": { \"ethernet-match\": "
									<< "{ \"ethernet-type\": { \"type\": \"34525\" }, "
									<< "\"ethernet-destination\": { \"address\": \"00:00:00:00:00:00\" } }, "
									<< "\"ipv6-source-address-no-mask\": \""
									<< ipv6_h1 << "\", "
									<< "\"ipv6-source-arbitrary-bitmask\": \""
									<< ipv6_h1 << "\", "
									<< "\"ipv6-destination-address-no-mask\": \""
									<< ipv6_h2 << "\", "
									<< "\"ipv6-destination-arbitrary-bitmask\": \""
									<< ipv6_h2 << "\" } } }";
							string flow = ss.str();
							oc.configureFlow(nn->openflow_id, j, flow, flow_counter);
							flow_counter++;

							ss.str("");
							ss.clear();
							//add default flow rule with goto next table
							ss
									<< "{ \"flow\": { \"barrier\": \"false\", \"cookie\": \"10\", "
									<< "\"cookie_mask\": \"10\", \"flow-name\": \"flow\", "
									<< "\"id\": \"" << flow_counter
									<< "\", \"priority\": \"500\", "
									<< "\"table_id\": \"" << j << "\", "
									<< "\"instructions\": {\"instruction\": {\"order\": \"0\","
									<< "\"go-to-table\": { \"table_id\": \""
									<< j + 1 << "\" }}}, "
									<< "\"match\": { \"ethernet-match\": "
									<< "{ \"ethernet-type\": { \"type\": \"34525\" }, "
									<< "\"ethernet-destination\": { \"address\": \"00:00:00:00:00:00\" } } "
									<< " } } }";
							string flow2 = ss.str();
							oc.configureFlow(nn->openflow_id, j, flow2, flow_counter);
							flow_counter++;
						}
						//else no more go to next table
						else {
							ostringstream ss;
							ss
									<< "{ \"flow\": { \"barrier\": \"false\", \"cookie\": \"10\", "
									<< "\"cookie_mask\": \"10\", \"flow-name\": \"flow\", "
									<< "\"id\": \"" << flow_counter
									<< "\", \"priority\": \"1000\", "
									<< "\"table_id\": \"" << j << "\", "
									<< "\"instructions\": {\"instruction\": [{\"order\": \"0\","
									<< "\"apply-actions\": {\"action\": {\"order\": \"0\",";

							//if ff_port is empty, then configure output port action -as usual-
							//otherwise configure the group action
							if (nc->ff_pt.empty()) {
								ss << "\"output-action\": { \"output-node-connector\": \"" << nc->src_pt << "\" }}}}]},";
							} else {
								ss << "\"group-action\": { \"group-id\": \"" << group_id << "\" }}}}]},";
							}
							ss << "\"match\": { \"ethernet-match\": "
									<< "{ \"ethernet-type\": { \"type\": \"34525\" }, "
									<< "\"ethernet-destination\": { \"address\": \"00:00:00:00:00:00\" } }, "
									<< "\"ipv6-source-address-no-mask\": \""
									<< ipv6_h1 << "\", "
									<< "\"ipv6-source-arbitrary-bitmask\": \""
									<< ipv6_h1 << "\", "
									<< "\"ipv6-destination-address-no-mask\": \""
									<< ipv6_h2 << "\", "
									<< "\"ipv6-destination-arbitrary-bitmask\": \""
									<< ipv6_h2 << "\" } } }";
							string flow = ss.str();
							oc.configureFlow(nn->openflow_id, j, flow, flow_counter);
							flow_counter++;
						}
					}
				}
			}
		}
	}
}

void Domain::writeOFlows(bool odl_enabled) {
    ofstream flow_conf;
    //    string curr_bridge = "";
    string ipv6_h1;
    string ipv6_h2;
    int node_no_connections;
    Bitvector mc_FID = Bitvector(fid_len * 8);
    for (size_t i = 0; i < network_nodes.size(); i++) {
        vector<string> unique_srcips;
        NetworkNode *nn = network_nodes[i];
        if (odl_enabled && !nn->openflow_id.empty())
            continue;
        if (nn->operating_system.compare("ovs") == 0) {
            flow_conf.open((write_conf + nn->label + ".sh").c_str());
            node_no_connections = nn->connections.size();
            /*get the path to sdn (ovs) executables*/
            string sdn_path;
            sdn_path = nn->sdn_path;
            if (nn->sdn_implementation.compare("bridges") == 0) {
                /*Bloom-filtered LIDs for connections of a bridge*/
                map<string, Bitvector> mc_FID;
                /*connections (ports) of the same bridge - notice connections cannot be to ports of different bridges*/
                map<string, vector<NetworkConnection*> > bridge_connections;
                /*bridge of the connection*/
                string bridge;
                for (size_t c = 0; c < node_no_connections; c++) {
                    bridge = nn->connections[c]->bridge;
                    (bridge_connections[bridge]).push_back(nn->connections[c]);
                    if (mc_FID.find(bridge) == mc_FID.end()) {
                        mc_FID[bridge] = Bitvector(fid_len * 8);
                    }
                }
                for (map<string, Bitvector>::iterator it = mc_FID.begin();
                     it != mc_FID.end(); it++) {
                    bridge = (*it).first;
                    int bridge_no_connections =
                    bridge_connections[bridge].size();
                    /*create the priorities array to provide unique and incremenal priority values for the connections of each bridge*/
                    unsigned int priority_counter[bridge_no_connections];
                    /*some compilers complaint about initializing variable length array with declaration, memset is the safest choice*/
                    memset(priority_counter, 0,
                           bridge_no_connections * sizeof(int));
                    for (size_t f = 1; f < bridge_no_connections; f++) {
                        int priority_coef = boost::math::factorial<double>(
                                                                           bridge_no_connections)
                        / (boost::math::factorial<double>(f)
                           * boost::math::factorial<double>(
                                                            bridge_no_connections - f));
                        priority_counter[f] = priority_counter[f - 1]
                        + priority_coef;
                    }
                    flow_conf << sdn_path << "ovs-ofctl -O OpenFlow13 add-flow "
                    << bridge << " \"dl_dst="
                    << nn->sdn_mac
                    << ", table=0,ipv6,priority=500, action= drop\""
                    << endl;
                    int priority_base = 1000;
                    int priority = 0;
                    int combinations = pow(2, bridge_no_connections);
                    for (int j = 1; j < combinations; j++) {
                        std::bitset<64> pos(j);
                        string output_ports;
                        mc_FID[bridge].clear();
                        int no_links = 0;
                        for (size_t k = 0; k < bridge_no_connections; k++) {
                            if (pos[k] == 1) {
                                mc_FID[bridge] = mc_FID[bridge]
                                | bridge_connections[bridge][k]->LID;
                                if (no_links > 0) {
                                    output_ports = output_ports + ",";
                                }
                                output_ports = output_ports
                                + bridge_connections[bridge][k]->src_pt;
                                no_links++;
                            }
                        }
                        ipv6_h1 = lid2ipv6(mc_FID[bridge].to_string()).substr(0,
                                                                              39);
                        ipv6_h2 = lid2ipv6(mc_FID[bridge].to_string()).substr(
                                                                              40, 79);
                        /*formula to increase the priority based on the number of bitpositions set to 1, because multicast rules should have higher priority than unicast or less multicast*/
                        priority = priority_base
                        + priority_counter[no_links - 1];
                        priority_counter[no_links - 1] += 1;
                        flow_conf
                        << sdn_path << "ovs-ofctl -O OpenFlow13 add-flow "
                        << bridge << " \"dl_dst="
                        << nn->sdn_mac
                        << ", table=0, ipv6, priority=" << priority
                        << ", ipv6_src=" << ipv6_h1 << "/" << ipv6_h1
                        << ", ipv6_dst=" << ipv6_h2 << "/" << ipv6_h2
                        << ", action=output:" << output_ports << " \""
                        << endl;
                    }
                }
            } else {
                for (size_t j = 0; j < node_no_connections; j++) {
                    NetworkConnection *nc = nn->connections[j];
                    ipv6_h1 = lid2ipv6(nc->LID.to_string()).substr(0, 39);
                    ipv6_h2 = lid2ipv6(nc->LID.to_string()).substr(40, 79);
                    if (nn->sdn_implementation.compare("tables") == 0) {
                        //pipeline implementation
                        if (j < node_no_connections - 1) {
                            flow_conf
                            << sdn_path << "ovs-ofctl -O OpenFlow13 add-flow "
                            << nn->bridge
                            << " \"dl_dst="
                            << nn->sdn_mac
                            << ", table="
                            << j << ", ipv6,priority=500, action="
                            << "resubmit(," << j + 1 << ")\"" << endl;
                            flow_conf
                            << sdn_path << "ovs-ofctl -O OpenFlow13 add-flow "
                            << nn->bridge
                            << " \"dl_dst="
                            << nn->sdn_mac
                            << ", ipv6_src="
                            << ipv6_h1 << "/" << ipv6_h1 << ",ipv6_dst="
                            << ipv6_h2 << "/" << ipv6_h2 << ",table="
                            << j << ",ipv6,priority=1000,action=output:"
                            << nc->src_pt << ",resubmit(," << j + 1
                            << ")\"" << endl;
                        } else {
                            flow_conf
                            << sdn_path << "ovs-ofctl -O OpenFlow13 add-flow "
                            << nn->bridge
                            << " \"dl_dst="
                            << nn->sdn_mac
                            << ", ipv6_src="
                            << ipv6_h1 << "/" << ipv6_h1 << ",ipv6_dst="
                            << ipv6_h2 << "/" << ipv6_h2 << ",table="
                            << j << ",ipv6,priority=1000,action=output:"
                            << nc->src_pt << "\"" << endl;
                            flow_conf
                            << sdn_path << "ovs-ofctl -O OpenFlow13 add-flow "
                            << nn->bridge
                            << " \"dl_dst="
                            << nn->sdn_mac
                            << ", table="
                            << j << ", ipv6,priority=500, action="
                            << "drop\"" << endl;
                        }
                    }
                }
            }
            flow_conf.close();
        }
    }
}

/*this function gets the mac address in a string pattern that can be used by the Classifier Element of click*/
const string getMACPattern(string &mac_addr) {
    string mac_addr_pattern = mac_addr;
    mac_addr_pattern.erase(
                           std::remove(mac_addr_pattern.begin(), mac_addr_pattern.end(), ':'),
                           mac_addr_pattern.end());
    return mac_addr_pattern;
}

void Domain::writeClickFiles(bool montoolstub, bool dump_supp, bool no_lnxcap, bool no_cpr) {
    ofstream click_conf;
    ofstream write_TMFID;
    string mac_addr_pattern = string();
    for (size_t i = 0; i < network_nodes.size(); i++) {
        vector<string> unique_ifaces;
        vector<string> unique_ifaces_end;
        vector<string> unique_ifacesmac;
        vector<string> unique_srcips;
        // Map Interface to ports for QoS (e.g.: eth0: 1,2)
        IfPortsMap ifmap;
        map<string, string> macmap;
        NetworkNode *nn = network_nodes[i];
        string node_type = "FW";
        if (nn->isRV) {
            node_type += ":RV";
        }
        if (nn->isTM) {
            node_type += ":TM";
        }
        if (nn->operating_system.compare("ovs") != 0) {
            click_conf.open((write_conf + nn->label + ".conf").c_str());
            write_TMFID.open((write_conf + nn->label + "_TMFID.txt").c_str());
            if (montoolstub && (nn->running_mode.compare("user") == 0)) {
                click_conf
                << "require(blackadder); \n\nControlSocket(\"TCP\",55000);\n\n "
                << endl << endl;
            } else {
                click_conf << "require(blackadder);" << endl << endl;
            }
            /*Blackadder Elements First*/
            click_conf << "globalconf::GlobalConf(TYPE " << node_type
            << ", MODE " << overlay_mode << ", NODEID " << nn->label
            << "," << endl;
            click_conf << "DEFAULTRV " << nn->FID_to_RV.to_string() << ","
            << endl;
			click_conf << "DEFAULTFromRV " << nn->FID_from_RV.to_string() << "," << endl;

            click_conf << "TMFID     " << nn->FID_to_TM.to_string() << ","
            << endl;
            write_TMFID << nn->FID_to_TM.to_string() << endl;
            click_conf << "iLID      " << nn->iLid.to_string() << ");" << endl
            << endl;
            
            click_conf << "localRV::LocalRV(globalconf);" << endl;
            
            string cpr_enabled="true"; if (no_cpr)  cpr_enabled="false"; //enable/disable CPR during deployment
            
            click_conf << "netlink::Netlink();" << endl
            << "tonetlink::ToNetlink(netlink);" << endl
            << "fromnetlink::FromNetlink(netlink);" << endl
			<< "contr_r::ControlReliability(globalconf, "<<cpr_enabled<<");" << endl;

            /*define the netlink elements for management communications through MAPI*/
            click_conf << "tomgmnetlink::ToMGMNetlink(netlink);" << endl
            << "frommgmnetlink::FromMGMNetlink(netlink);" << endl
            << endl;
            click_conf << "proxy::LocalProxy(globalconf);" << endl << endl;
            click_conf << "fw::Forwarder(globalconf," << nn->connections.size()
            << "," << endl;
            for (size_t j = 0; j < nn->connections.size(); j++) {
                NetworkConnection *nc = nn->connections[j];
                /*Connection with SDN node - Ethertype = 86dd*/
                if (nc->dst_pt.length() > 0) {
                    NetworkNode *dst_nn = node_NODEID[nc->dst_label];
                    /*set the destination mac addr to the sdn group mac of the sdn node*/
                    nc->dst_mac = dst_nn->sdn_mac;
                    nc->proto_type = "0x86dd";
                } else if (nc->dst_if.length() > 0) {
                    nc->proto_type = "0x080a";
                }
                int offset;
                if (overlay_mode.compare("mac") == 0
                    || (overlay_mode.compare("mac_ml") == 0
                        && nn->type.compare("PN") == 0)) {
                        if (overlay_mode.compare("mac_ml") == 0
                            && nc->lnk_type != "pp") {
                            nc->dst_mac = "00:00:00:00:00:00";
                        }
                        // Default MAC behaviour
                        if ((offset = findOffset(unique_ifaces, nc->src_if))
                            == -1) {
                            unique_ifaces.push_back(nc->src_if);
                            unique_ifacesmac.push_back(nc->src_mac);
                            //Notice: this assumes that if a src_if is connected to ovs switch port, it will not connect to any other port
                            if (nc->dst_pt.length() > 0) {
                                unique_ifaces_end.push_back("eth_pt");
                            } else {
                                unique_ifaces_end.push_back("eth_if");
                            }
                            click_conf << unique_ifaces.size() << "," << nc->src_mac
                            << "," << nc->dst_mac << "," << nc->proto_type
                            << "," << nc->LID.to_string();
                        } else {
                            click_conf << offset + 1 << "," << nc->src_mac << ","
                            << nc->dst_mac << "," << nc->proto_type << ","
                            << nc->LID.to_string();
                        }
                        
                        // Do not ignore duplicates and use a new port!
                    } else if (overlay_mode.compare("mac_qos") == 0) {
                        // Add the interface either way! (Do not count uniques)
                        // Add to the map
                        IfMapE tmp_ime;
                        tmp_ime.fwport = j + 1;
                        tmp_ime.prio = nc->priority;
                        tmp_ime.rate_lim = nc->rate_lim;
                        ifmap[nc->src_if].push_back(tmp_ime);
                        macmap[nc->src_if] = nc->src_mac;
                        // Config...
                        click_conf << j + 1 << "," << nc->src_mac << ","
                        << nc->dst_mac << "," << nc->LID.to_string();
                    } else {
                        if ((offset = findOffset(unique_srcips, nc->src_ip))
                            == -1) {
                            unique_srcips.push_back(nc->src_ip);
                            //cout << "PUSHING BACK " << nc->src_ip << endl;
                            //cout << unique_srcips.size() << endl;
                            click_conf << unique_srcips.size() << "," << nc->src_ip
                            << "," << nc->dst_ip << ","
                            << nc->LID.to_string();
                        } else {
                            click_conf << offset + 1 << "," << nc->src_ip << ","
                            << nc->dst_ip << "," << nc->LID.to_string();
                        }
                    }
                if (j < nn->connections.size() - 1) {
                    /* We assume that all if and else clauses above print a line */
                    click_conf << "," << endl;
                }
            }
            click_conf << ");" << endl << endl;
            if ((overlay_mode.compare("mac") == 0)
                || ((overlay_mode.compare("mac_ml") == 0))) {
                for (size_t j = 0; j < unique_ifaces.size(); j++) {
                    click_conf << "tsf" << j << "::ThreadSafeQueue(1000);"
                    << endl;
                    if (nn->running_mode.compare("user") == 0) {
                        if (nn->operating_system.compare("Darwin") == 0) {
                            click_conf << "fromdev" << j << "::FromDevice("
                            << unique_ifaces[j] << ", PROMISC true";
                            click_conf << ", OUTBOUND true"
                            << ", BPF_FILTER \"ether src not "
                            << unique_ifacesmac[j] << "\"" << ");"
                            << endl << "todev" << j << "::ToDevice("
                            << unique_ifaces[j] << ");" << endl;
                        } else {
                            click_conf << "fromdev" << j << "::FromDevice("
                            << unique_ifaces[j];
                            /*PROMISC must be set if the connection is with a SDN node, becasue*
                             *the dst_if = 00:00:00:00:00:00 so the packet wont be passed
                             *unless the interface is in PROMISC mode
                             */
                            if (unique_ifaces_end[j].compare("eth_pt") == 0)
                                click_conf << ", PROMISC true";
                            if (!no_lnxcap)
                                click_conf << ", METHOD LINUX";
                            click_conf << ");" << endl << "todev" << j
                            << "::ToDevice(" << unique_ifaces[j] << ");"
                            << endl;
                        }
                    } else {
                        click_conf << "fromdev" << j << "::FromDevice("
                        << unique_ifaces[j] << ", BURST 8);" << endl
                        << "todev" << j << "::ToDevice("
                        << unique_ifaces[j] << ", BURST 8);" << endl;
                    }
                }
                /*Necessary Click Elements*/
            } else if (overlay_mode.compare("mac_qos") == 0) {
                // write down the map + the scheduler
                
                // For each interface (PHY in theory)
                IfPortsMap::iterator it = ifmap.begin();
                // Build priority list for the LinkMon module
                stringstream priolist;
                for (; it != ifmap.end(); ++it) {
                    // For each port on the FW module (and device)
                    for (size_t i = 0; i < it->second.size(); i++) {
                        // Add a counter (incoming rate / demand)
                        click_conf << "inC_" << it->first << "_" << i + 1
                        << "::Counter();" << endl;
                        // Add the Q in the middle
                        click_conf << "tsQ_" << it->first << "_" << i + 1
                        << "::ThreadSafeQueue(10000);" << endl;
                        // Add the outgoing counter (outgoing rate / utilization)
                        click_conf << "outC_" << it->first << "_" << i + 1
                        << "::Counter();" << endl;
                        
                        // Add the outgoing (ofcource) rate limiter!
                        click_conf << "BS_" << it->first << "_" << i + 1
                        << "::BandwidthShaper("
                        << it->second[i].rate_lim << ");" << endl;
                        
                        priolist << it->second[i].prio << ",";
                        //if (i<it->second.size()-1) priolist<<",";
                    }
                    
                    // Add the sceduler for this device!
                    click_conf << "sc_" << it->first << "::SimplePrioSched();"
                    << endl;
                    
                    // Add the classifier for this device!
                    click_conf << endl << "cf_" << it->first
                    << "::Classifier(12/080a);" << endl;
                    
                    // Finally add the device (unique cause it is a map)
                    if (nn->running_mode.compare("user") == 0) {
                        if (nn->operating_system.compare("Darwin") == 0) {
                            click_conf << "fromdev_" << it->first
                            << "::FromDevice(" << it->first
                            << ", PROMISC true";
                            if (!no_lnxcap)
                                click_conf << ", METHOD LINUX";
                            click_conf << ", OUTBOUND true"
                            << ", BPF_FILTER \"ether dst "
                            << macmap[it->first] << "\"" << ");" << endl
                            << "todev_" << it->first << "::ToDevice("
                            << it->first << ");" << endl;
                        } else {
                            click_conf << "fromdev_" << it->first
                            << "::FromDevice(" << it->first;
                            if (!no_lnxcap)
                                click_conf << ", METHOD LINUX";
                            click_conf << ");" << endl << "todev_" << it->first
                            << "::ToDevice(" << it->first << ");"
                            << endl;
                        }
                    } else
                        // not userspace
                        click_conf << "fromdev_" << it->first << "::FromDevice("
                        << it->first << ", BURST 8);" << endl
                        << "todev_" << it->first << "::ToDevice("
                        << it->first << ", BURST 8);" << endl;
                } // Eo Interfaces
                
                // Add the monitoring element
                // TODO: Pass the intervals as parameters to avoid recompiling
                // Terminate the priolist with a dummy -1 (to avoid removing comma :) sry...)
                click_conf << "lnmon::LinkMon(globalconf,fw, 1, 10,"
                << priolist.str() << "-1);" << endl;
            } else {
                /*raw sockets here*/
                click_conf << "tsf" << "::ThreadSafeQueue(10000);" << endl;
                if (nn->running_mode.compare("user") == 0) {
                    click_conf << "rawsocket" << "::RawSocket(UDP, 55555)"
                    << endl;
                    //click_conf << "classifier::IPClassifier(dst udp port 55000 and src udp port 55000)" << endl;
                } else {
                    cerr
                    << "Something is wrong...I should not build click config using raw sockets for node "
                    << nn->label << "that will run in kernel space"
                    << endl;
                }
            }
            
            /*Now link all the elements appropriately*/
            click_conf << endl << endl << "proxy[0]->tonetlink;" << endl
            << "fromnetlink->[0]proxy;" << endl;
            /*link the netlink elements of management comms through MAPI*/
            click_conf << "proxy[3]->tomgmnetlink;" << endl
            << "frommgmnetlink->[3]proxy;" << endl;
            click_conf << "localRV[0]->[1]proxy[1]->[0]localRV;" << endl;
			click_conf << "proxy[2]-> [0]contr_r[0] -> [0]fw[0] -> [1]contr_r[1] -> [2]proxy;" << endl;
			
           // << "proxy[2]-> [0]fw[0] -> [2]proxy;" << endl;
            if ((overlay_mode.compare("mac") == 0)
                || ((overlay_mode.compare("mac_ml") == 0))) {
                for (size_t j = 0; j < unique_ifaces.size(); j++) {
                    if (nn->running_mode.compare("kernel") == 0) {
                        click_conf << endl << "classifier" << j
                        << "::Classifier(" << "12/080a,-);" << endl;
                        if (montoolstub && j == 0
                            && (nn->running_mode.compare("user") == 0)) {
                            click_conf << "fw[" << (j + 1) << "]->tsf" << j
                            << "->outc::Counter()->todev" << j << ";"
                            << endl;
                            click_conf << "fromdev" << j << "->classifier" << j
                            << "[0]->inc::Counter()  -> [" << (j + 1)
                            << "]fw;" << endl;
                        } else {
                            click_conf << "fw[" << (j + 1) << "]->tsf" << j
                            << "->todev" << j << ";" << endl;
                            click_conf << "fromdev" << j << "->classifier" << j
                            << "[0]->[" << (j + 1) << "]fw;" << endl;
                        }
                        click_conf << "classifier" << j << "[1]->ToHost()"
                        << endl;
                    } else {
                        if (unique_ifaces_end[j].compare("eth_pt") == 0) {
                            /*check the promisc mode of blackadder node*/
                            if (nn->promisc) {
                                /*blackadder only filter on ether_type, the kernel handles packet filteration based on promisc mode */
                                click_conf << endl << "classifier" << j
                                << "::Classifier(12/86dd);" << endl;
                            } else {
                                /*blackadder filter packets based on destination MAC address, which should be 00...00 for the SDN port*/
                                click_conf << endl << "classifier" << j
                                << "::Classifier(0/000000000000 12/86dd);"
                                << endl;
                            }
                            //                            click_conf << endl << "classifier" << j << "::Classifier(12/86dd);" << endl;
                            click_conf << "fw[" << (j + 1) << "]->tsf" << j
                            << "->todev" << j << ";" << endl;
                            click_conf << "fromdev" << j << "->classifier" << j
                            << "[0]->[" << (j + 1) << "]fw;" << endl;
                            
                        } else if (montoolstub && j == 0
                                   && (nn->running_mode.compare("user") == 0)) {
                            if (nn->promisc) {
                                click_conf << endl << "classifier" << j
                                << "::Classifier(12/080a);" << endl;
                            } else {
                                /*blackadder filter packets based on destination MAC address, which should be the MAC of the recieving interface*/
                                mac_addr_pattern = getMACPattern(
                                                                 unique_ifacesmac[j]);
                                click_conf << endl << "classifier" << j
                                << "::Classifier(" << "0/"
                                << mac_addr_pattern << " 12/080a);"
                                << endl;
                            }
                            click_conf << "fw[" << (j + 1) << "]->tsf" << j
                            << "->outc::Counter()->todev" << j << ";"
                            << endl;
                            click_conf << "fromdev" << j << "->classifier" << j
                            << "[0]->inc::Counter()  -> [" << (j + 1)
                            << "]fw;" << endl;
                        } else {
                            if (nn->promisc) {
                                click_conf << endl << "classifier" << j
                                << "::Classifier(12/080a);" << endl;
                            } else {
                                /*blackadder filter packets based on destination MAC address, which should be the MAC of the recieving interface*/
                                mac_addr_pattern = getMACPattern(
                                                                 unique_ifacesmac[j]);
                                click_conf << endl << "classifier" << j
                                << "::Classifier(" << "0/"
                                << mac_addr_pattern << " 12/080a);"
                                << endl;
                            }
                            click_conf << "fw[" << (j + 1) << "]->tsf" << j
                            << "->todev" << j << ";" << endl;
                            click_conf << "fromdev" << j << "->classifier" << j
                            << "[0]->[" << (j + 1) << "]fw;" << endl;
                        }
                    }
                }
            } else if (overlay_mode.compare("mac_qos") == 0) {
                
                // For each interface
                IfPortsMap::iterator it = ifmap.begin();
                int it_count = 1; // :)
                for (; it != ifmap.end(); ++it) {
                    // For each port on the FW module (and device)
                    for (size_t i = 0; i < it->second.size(); i++) {
                        stringstream ss;
                        //ss << it->first << "_" << (it->second[i].fwport);
                        ss << it->first << "_" << (i + 1);
                        string ifpname = ss.str();
                        // Connect the fw->inC->Q->BS->outC->scheduler!
                        click_conf << "fw[" << it->second[i].fwport
                        << "] -> inC_" << ifpname << " -> " << " tsQ_"
                        << ifpname << " -> BS_" << it->first << "_"
                        << ((i + 1)/* *it_count*/) << " -> outC_"
                        << ifpname << " -> [" << i << "]sc_"
                        << it->first << ";" << endl;
                    }
                    
                    // Finally connect scheduler<->Interface
                    click_conf << "sc_" << it->first << " -> " << "todev_"
                    << it->first << ";" << endl;
                    click_conf << "fromdev_" << it->first << " -> cf_"
                    << it->first << "[0] ->inc_" << it->first
                    << "::Counter()  -> [" << (it_count++) << "]fw;"
                    << endl;
                } // Eo Interfaces
                  // Connect the link monitoring module
                click_conf << "lnmon[0]->[3]proxy[3]->[0]lnmon;" << endl;
            } else {
                /*raw sockets here*/
                if (montoolstub) {
                    click_conf
                    << "fw[1] -> tsf -> outc::Counter() -> rawsocket -> IPClassifier(dst udp port 55555 and src udp port 55555)[0]-> inc::Counter()  -> [1]fw"
                    << endl;
                } else {
                    click_conf
                    << "fw[1] ->  tsf -> rawsocket -> IPClassifier(dst udp port 55555 and src udp port 55555)[0] -> [1]fw"
                    << endl;
                }
            }
            if (dump_supp && nn->isRV
                && nn->running_mode.compare("user") == 0) {
                click_conf << "\ncs :: ControlSocket(TCP, 55500);" << endl;
            }
            click_conf.close();
            write_TMFID.close();
        }
    }
}

void Domain::writeNS3ClickFiles() {
    ofstream click_conf;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        vector<string> unique_ifaces;
        NetworkNode *nn = network_nodes[i];
        click_conf.open((write_conf + nn->label + ".conf").c_str());
        /*Blackadder Elements First*/
        click_conf << "globalconf::GlobalConf(MODE mac, NODEID " << nn->label
        << "," << endl;
        click_conf << "DEFAULTRV " << nn->FID_to_RV.to_string() << "," << endl;
        click_conf << "TMFID     " << nn->FID_to_TM.to_string() << "," << endl;
        click_conf << "iLID      " << nn->iLid.to_string() << ");" << endl
        << endl;
        click_conf << "localRV::LocalRV(globalconf);" << endl;
        click_conf << "toApps::ToSimDevice(tap0);" << endl
        << "fromApps::FromSimDevice(tap0);" << endl;
        click_conf << "proxy::LocalProxy(globalconf);" << endl << endl;
        click_conf << "fw::Forwarder(globalconf," << nn->connections.size()
        << "," << endl;
        for (size_t j = 0; j < nn->connections.size(); j++) {
            NetworkConnection *nc = nn->connections[j];
            nc->proto_type = "0x080a";
            int offset;
            if ((offset = findOffset(unique_ifaces, nc->src_if)) == -1) {
                unique_ifaces.push_back(nc->src_if);
                if (j == nn->connections.size() - 1) {
                    click_conf << unique_ifaces.size() << "," << nc->src_mac
                    << "," << nc->dst_mac << "," << nc->proto_type
                    << "," << nc->LID.to_string() << ");" << endl
                    << endl;
                } else {
                    click_conf << unique_ifaces.size() << "," << nc->src_mac
                    << "," << nc->dst_mac << "," << nc->proto_type
                    << "," << nc->LID.to_string() << "," << endl;
                }
            } else {
                if (j == nn->connections.size() - 1) {
                    click_conf << offset + 1 << "," << nc->src_mac << ","
                    << nc->dst_mac << "," << nc->proto_type << ","
                    << nc->LID.to_string() << ");" << endl << endl;
                } else {
                    click_conf << offset + 1 << "," << nc->src_mac << ","
                    << nc->dst_mac << "," << nc->proto_type << ","
                    << nc->LID.to_string() << "," << endl;
                }
            }
        }
        for (size_t j = 0; j < unique_ifaces.size(); j++) {
            click_conf << "tsf" << j << "::ThreadSafeQueue(10000);" << endl;
            click_conf << "fromsimdev" << j << "::FromSimDevice("
            << unique_ifaces[j] << ");" << endl << "tosimdev" << j
            << "::ToSimDevice(" << unique_ifaces[j] << ");" << endl;
        }
        /*Now link all the elements appropriately*/
        click_conf << "proxy[0]->toApps;" << endl << "fromApps->[0]proxy;"
        << endl << "localRV[0]->[1]proxy[1]->[0]localRV;" << endl
        << "proxy[2]-> [0]fw[0] -> [2]proxy;" << endl;
        for (size_t j = 0; j < unique_ifaces.size(); j++) {
            click_conf << "fw[" << (j + 1) << "]->tsf" << j << "->tosimdev" << j
            << ";" << endl;
            click_conf << "fromsimdev" << j << "->[" << (j + 1) << "]fw;"
            << endl;
        }
        click_conf.close();
    }
}

void Domain::scpTMConfiguration(string TM_conf) {
    FILE *scp_command;
    string command;
    command = "scp -o ConnectTimeout=5 " + write_conf + TM_conf + " " + user + "@" + TM_node->testbed_ip + ":" + write_conf;
    cout << command << endl;
    scp_command = popen(command.c_str(), "r");
    /* close */
    pclose(scp_command);
}

void Domain::scpClickFiles() {
    FILE *scp_command;
    string command, fidtm_cmd, of_command;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        if (overlay_mode.compare("mac_ml") == 0 && nn->type.compare("PN") != 0) {
            //avoid scp to optical nodes
            continue;
        }
        command = "scp -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + write_conf + nn->label + ".conf" + " " + user + "@" + nn->testbed_ip + ":" + write_conf;
        fidtm_cmd= "scp -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + write_conf + nn->label + "_TMFID.txt" + " " + user + "@" + nn->testbed_ip + ":" + write_conf;
        if(nn->operating_system.compare("ovs") == 0){
            of_command = "scp -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + write_conf + nn->label + ".sh" + " " + user + "@" + nn->testbed_ip + ":" + write_conf;
            cout << of_command << endl;
            scp_command = popen(of_command.c_str(), "r");
            if (scp_command == NULL) {
                cerr << "Failed to scp Of_script file to node " << nn->label << endl;
            }
        } else {
            scp_command = popen(command.c_str(), "r");
            if (scp_command == NULL) {
                cerr << "Failed to scp click file to node " << nn->label << endl;
            }
            pclose(scp_command);
            cout << fidtm_cmd << endl;
            scp_command = popen(fidtm_cmd.c_str(), "r");
            if (scp_command == NULL) {
                cerr << "Failed to scp TMFID file to node " << nn->label << endl;
            }
        }
        /* close */
        pclose(scp_command);
    }
}

void Domain::killClick() {
    FILE *ssh_command;
    string command;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        if(nn->operating_system.compare("ovs") != 0){
            if (overlay_mode.compare("mac_ml") == 0 && nn->type.compare("PN") != 0) {
                // avoid starting Click in optical nodes
                continue;
            }
            /*kill click first both from kernel and user space*/
            if (sudo) {
                command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " -t \"sudo pkill -9 click\"";
            } else {
                command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " -t \"pkill -9 click\"";
            }
            cout << command << endl;
            ssh_command = popen(command.c_str(), "r");
            if (ssh_command == NULL) {
                cerr << "Failed to stop click at node " << nn->label << endl;
            }
            pclose(ssh_command);
            if (sudo) {
                command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " -t \"sudo " + click_home + "sbin/click-uninstall\"";
            } else {
                command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " -t \"" + click_home + "sbin/click-uninstall \"";
            }
            cout << command << endl;
            ssh_command = popen(command.c_str(), "r");
            if (ssh_command == NULL) {
                cerr << "Failed to stop click at node " << nn->label << endl;
            }
            pclose(ssh_command);
        }
    }
}

void Domain::killMA() {
    FILE *ssh_command;
    string command;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        if(nn->operating_system.compare("ovs") != 0){
            if (overlay_mode.compare("mac_ml") == 0 && nn->type.compare("PN") != 0) {
                // avoid starting Click in optical nodes
                continue;
            }
            /*kill click first both from kernel and user space*/
            if (sudo) {
                command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " -t \"sudo pkill -9 mona\"";
            } else {
                command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " -t \"pkill -9 mona\"";
            }
            cout << command << endl;
            ssh_command = popen(command.c_str(), "r");
            if (ssh_command == NULL) {
                cerr << "Failed to stop click at node " << nn->label << endl;
            }
            pclose(ssh_command);
        }
    }
}

void Domain::killNAP() {
    FILE *ssh_command;
    string command;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        if(nn->operating_system.compare("ovs") != 0){
            if (overlay_mode.compare("mac_ml") == 0 && nn->type.compare("PN") != 0) {
                // avoid starting Click in optical nodes
                continue;
            }
            /*kill click first both from kernel and user space*/
            if (sudo) {
                command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " -t \"sudo pkill -15 nap\"";
            } else {
                command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " -t \"pkill -15 nap\"";
            }
            cout << command << endl;
            ssh_command = popen(command.c_str(), "r");
            if (ssh_command == NULL) {
                cerr << "Failed to stop NAP at node " << nn->label << endl;
            }
            pclose(ssh_command);
        }
    }
}
void Domain::killLSM() {
    FILE *ssh_command;
    string command;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        if(nn->operating_system.compare("ovs") != 0){
            if (overlay_mode.compare("mac_ml") == 0 && nn->type.compare("PN") != 0) {
                // avoid starting Click in optical nodes
                continue;
            }
            /*kill click first both from kernel and user space*/
            if (sudo) {
                command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " -t \"sudo killall -9 linkstate_monitor\"";
            } else {
                command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " -t \"killall -9 linkstate_monitor\"";
            }
            cout << command << endl;
            ssh_command = popen(command.c_str(), "r");
            if (ssh_command == NULL) {
                cerr << "Failed to stop click at node " << nn->label << endl;
            }
            pclose(ssh_command);
        }
    }
}

void Domain::startClick(bool log, bool odl_enabled) {
    FILE *ssh_command;
    string command;
    string filename;
    string tm_filename;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        if (log) {
            filename = "/tmp/ba_" + nn->label + ".log";
        } else {
            filename = "/dev/null";
        }
        if(nn->operating_system.compare("ovs") != 0){
            if (overlay_mode.compare("mac_ml") == 0 && nn->type.compare("PN") != 0) {
                // avoid starting Click in optical nodes
                continue;
            }
            /*start click*/
            if (nn->running_mode.compare("user") == 0) {
                if (sudo) {
                    command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " \"sudo " + click_home + "bin/click " + write_conf + nn->label + ".conf > " + filename + " 2>&1 &\"";
                } else {
                    command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " \"" + click_home + "bin/click " + write_conf + nn->label + ".conf > " + filename + " 2>&1 &\"";
                }
                cout << command << endl;
                ssh_command = popen(command.c_str(), "r");
                if (ssh_command == NULL) {
                    cerr << "Failed to start click at node " << nn->label << endl;
                }
                pclose(ssh_command);
            } else {
                if (sudo) {
                    command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " \"sudo " + click_home + "sbin/click-install " + write_conf + nn->label + ".conf > " + filename + " 2>&1 &\"";
                } else {
                    command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " \"" + click_home + "sbin/click-install " + write_conf + nn->label + ".conf > " + filename + " 2>&1 &\"";
                }
                cout << command << endl;
                ssh_command = popen(command.c_str(), "r");
                if (ssh_command == NULL) {
                    cerr << "Failed to start click at node " << nn->label << endl;
                }
                pclose(ssh_command);
            }
        } else {
            //if odl is enabled and openflow_id is not empty, then skip starting sh scripts in switch
            if (!(odl_enabled && !nn->openflow_id.empty())) {
                if (sudo) {
                    command =
                    "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 "
                    + user + "@" + nn->testbed_ip
                    + " -t \"sudo chmod +x " + write_conf
                    + nn->label + ".sh; sudo " + write_conf
                    + nn->label + ".sh > /dev/null 2>&1 \"";
                } else {
                    command =
                    "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 "
                    + user + "@" + nn->testbed_ip
                    + " -t \"chmod +x " + write_conf + nn->label
                    + ".sh; " + write_conf + nn->label
                    + ".sh > /dev/null 2>&1 \"";
                }
                cout << command << endl;
                ssh_command = popen(command.c_str(), "r");
                if (ssh_command == NULL) {
                    cerr << "Failed to start OF script at node " << nn->label
                    << endl;
                }
                pclose(ssh_command);
            }
        }
    }
}

/*Incomplete function yet, but don't want to lose the code
 void Domain::startOVS() {
 FILE *ssh_command;
 string command;
 string filename;
 for (size_t i = 0; i < network_nodes.size(); i++) {
 NetworkNode *nn = network_nodes[i];
 if (nn->operating_system.compare("ovs") == 0) {
 command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + "admin@" + nn->testbed_ip + " -t \"ovs-ofctl -O OpenFlow13 del-flows " + nn->bridge " | grep ""ipv6"" \"";
 command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + "admin@" + nn->testbed_ip + " -t \"chmod +x " + write_conf + nn->label + ".sh; " + write_conf + nn->label + ".sh\"";
 cout << command << endl;
 ssh_command = popen(command.c_str(), "r");
 if (ssh_command == NULL) {
 cerr << "Failed to start OF script at node " << nn->label << endl;
 }
 pclose(ssh_command);
 
 }
 }
 }
 }
 
 }
 */
void Domain::startMA() {
    FILE *ssh_command;
    string command;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        if(nn->operating_system.compare("ovs") != 0){
            if (overlay_mode.compare("mac_ml") == 0 && nn->type.compare("PN") != 0) {
                // avoid starting Click in optical nodes
                continue;
            }
            /*start click*/
            if (nn->running_mode.compare("user") == 0) {
                
                if (sudo) {
                    command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " \"screen -d -m -S mona sudo mona\"";
                } else {
                    command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " \"screen -d -m -S mona sudo mona\"";
                }
                cout << command << endl;
                ssh_command = popen(command.c_str(), "r");
                if (ssh_command == NULL) {
                    cerr << "Failed to start click at node " << nn->label << endl;
                }
                pclose(ssh_command);
            }
        }
    }
}

void Domain::startTM(bool log, string &extension) {
    FILE *ssh_command;
    string command;
    string filename;
    /*kill the topology manager first*/
    if (sudo) {
        command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + TM_node->testbed_ip + " -t \"sudo pkill -9 tm\"";
    } else {
        command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + TM_node->testbed_ip + " -t \"pkill -9 tm\"";
    }
    
    cout << command << endl;
    ssh_command = popen(command.c_str(), "r");
    if (ssh_command == NULL) {
        cerr << "Failed to stop Topology Manager at node " << TM_node->label << endl;
    }
    pclose(ssh_command);
    /*direct the log output*/
    if (log) {
        filename = "/tmp/tm.log";
    } else {
        filename = "/dev/null";
    }
    /*now start the TM*/
    if (sudo) {
        command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + TM_node->testbed_ip + " \"sudo /home/" + user + "/blackadder/TopologyManager/tm " + extension + " " + write_conf + "topology.graphml > " + filename + " 2>&1 &\"";
    } else {
        command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + TM_node->testbed_ip + " \"/home/" + user + "/blackadder/TopologyManager/tm " + extension + " " + write_conf + "topology.graphml > " + filename + " 2>&1 &\"";
    }
    cout << command << endl;
    ssh_command = popen(command.c_str(), "r");
    if (ssh_command == NULL) {
        cerr << "Failed to start Topology Manager at node " << TM_node->label << endl;
    }
    pclose(ssh_command);
}

void Domain::scpClickBinary(string tgzfile) {
    string command;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        
        command = "scp ./" + tgzfile + "  " + user + "@" + nn->testbed_ip + ":";
        cout << command << endl;
        system(command.c_str());
        
        command = "ssh -X " + user + "@" + nn->testbed_ip + " -t 'tar zxvf ~/"
        + tgzfile + "'";
        cout << command << endl;
        system(command.c_str());
        
    }
}

void Domain::startNAP(bool log) {
    FILE *ssh_command;
    string command;
    string filename;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        if (log) {
            filename = "/tmp/nap_" + nn->label + ".log";
        } else {
            filename = "/dev/null";
        }
        if(nn->operating_system.compare("ovs") != 0){
            if (overlay_mode.compare("mac_ml") == 0 && nn->type.compare("PN") != 0) {
                // avoid starting Click in optical nodes
                continue;
            }
            /*start click*/
            if (nn->running_mode.compare("user") == 0) {
                if (sudo) {
                    command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user +
                    "@" + nn->testbed_ip + " screen -d -m -S nap sudo nap -c /etc/nap/nap-" + nn->label + ".cfg";
                } else {
                    command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user +
                    "@" + nn->testbed_ip + " screen -d -m -S nap sudo nap -c /etc/nap/nap-" + nn->label + ".cfg";
                }
                cout << command << endl;
                ssh_command = popen(command.c_str(), "r");
                if (ssh_command == NULL) {
                    cerr << "Failed to start NAP at node " << nn->label << endl;
                }
                pclose(ssh_command);
            }
        }
    }
}
void Domain::startLSM(bool log) {
    FILE *ssh_command;
    string command;
    string filename;
    for (size_t i = 0; i < network_nodes.size(); i++) {
        NetworkNode *nn = network_nodes[i];
        if (log) {
            filename = "/tmp/lsm_" + nn->label + ".log";
        } else {
            filename = "/dev/null";
        }
        if(nn->operating_system.compare("ovs") != 0){
            if (overlay_mode.compare("mac_ml") == 0 && nn->type.compare("PN") != 0) {
                // avoid starting Click in optical nodes
                continue;
            }
            /*start click*/
            if (nn->running_mode.compare("user") == 0) {
                if (sudo) {
                    command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " \"sudo /home/" + user + "/blackadder/examples/traffic_engineering/linkstate_monitor > " + filename + " 2>&1 &\"";
                } else {
                    command = "ssh -o \"StrictHostKeyChecking no\" -o ConnectTimeout=5 " + user + "@" + nn->testbed_ip + " \"sudo /home/" + user + "/blackadder/examples/traffic_engineering/linkstate_monitor > " + filename + " 2>&1 &\"";
                }
                cout << command << endl;
                ssh_command = popen(command.c_str(), "r");
                if (ssh_command == NULL) {
                    cerr << "Failed to start LSM at node " << nn->label << endl;
                }
                pclose(ssh_command);
            }
        }
    }
}

/** Required method for dynamic deployment tool.
 * It merges new node and connections to an existing domain.
 */
MergingResult Domain::mergeDomains(Domain &new_dm) {
    std::cout << "Merging domains" << std::endl;
    if (new_dm.network_nodes[0]->label.compare("0000000x") == 0) {
        std::cout << "Node candidate for addition." << std::endl;
        /** generate random label
         */
        new_dm.network_nodes[0]->label = generateLabel();
        for (std::vector<NetworkConnection*>::iterator it =
             new_dm.network_nodes[0]->connections.begin();
             it != new_dm.network_nodes[0]->connections.end();) {
            NetworkConnection* nc = *it;
            /** calculate new LID
             */
            nc->LID = calculateLID();
            string dst_ip_address = nc->dst_label;
            nc->src_label = new_dm.network_nodes[0]->label;
            nc->proto_type = "0x080a";
            bool attachment_found = false;
            for (int n = 0; n < network_nodes.size(); n++) {
                NetworkNode* nn = network_nodes[n];
                /** check if attachment node exists
                 */
                if (nn->testbed_ip.compare(dst_ip_address) == 0) {
                    std::cout << "Found attachment node, creating connection."
                    << std::endl;
                    /** create new connection and add it to attachment node's connections
                     */
                    NetworkConnection* new_nc = new NetworkConnection();
                    new_nc->src_if = nc->dst_if;
                    new_nc->dst_if = nc->src_if;
                    new_nc->proto_type = "0x080a";
                    new_nc->dst_label = new_dm.network_nodes[0]->label;
                    new_nc->src_label = nn->label;
                    new_nc->LID = calculateLID();
                    nn->connections.push_back(new_nc);
                    /** add attachment node to the domain that needs to be updated
                     */
                    new_dm.network_nodes.push_back(nn);
                    /** update dst_label with attachment node's label
                     */
                    nc->dst_label = nn->label;
                    attachment_found = true;
                    break;
                }
            }
            /** if no attachment node is found, remove this connection, otherwise proceed.
             */
            if (!attachment_found) {
                std::cout << "No attachment node found, removing connection."
                << std::endl;
                it = new_dm.network_nodes[0]->connections.erase(it);
            } else
                ++it;
        }
        /** add new node (the first one) to existing domain
         */
        if (new_dm.network_nodes[0]->connections.size() != 0) {
            std::cout
            << "New node has active connections, hence will be added to topology."
            << std::endl;
            new_dm.network_nodes[0]->iLid = calculateLID();
            network_nodes.push_back(new_dm.network_nodes[0]);
            number_of_nodes++;
            return NODE_ADDED;
        } else
            return ERROR;
    } else if (new_dm.network_nodes[0]->label.compare("xxxxxxxx") == 0) {
        std::cout << "Node candidate for deletion." << std::endl;
        string label_deleted;
        /** iterate list of nodes to delete the candidate
         */
        for (std::vector<NetworkNode *>::iterator it = network_nodes.begin();
             it != network_nodes.end();) {
            NetworkNode* nn = *it;
            if (nn->testbed_ip.compare(new_dm.network_nodes[0]->testbed_ip)
                == 0) {
                std::cout << "Found node to be deleted, removing it."
                << std::endl;
                label_deleted = nn->label;
                it = network_nodes.erase(it);
                number_of_nodes--;
            } else
                ++it;
        }
        /** iterate the list of remaining nodes to delete the non-existing connections
         */
        for (std::vector<NetworkNode *>::iterator it = network_nodes.begin();
             it != network_nodes.end();) {
            NetworkNode* nn = *it;
            /** iterate its list of connections
             */
            for (std::vector<NetworkConnection *>::iterator it2 =
                 nn->connections.begin(); it2 != nn->connections.end();) {
                NetworkConnection* nc = *it2;
                /** if deleted node's label exists either as src or dst, delete the connection
                 */
                if (nc->src_label.compare(label_deleted) == 0
                    || nc->dst_label.compare(label_deleted) == 0) {
                    std::cout << "Deleting connection." << std::endl;
                    it2 = nn->connections.erase(it2);
                } else
                    ++it2;
            }
            ++it;
        }
        return NODE_DELETED;
    }
    std::cout << "Merging completed" << std::endl;
}

/** Required method for dynamic deployment tool.
 * It checks if generated label exists.
 */
bool Domain::labelExists(string label) {
    for (int i = 0; i < network_nodes.size(); i++)
        if (network_nodes[i]->label.compare(label) == 0)
            return true;
    return false;
}

/** Required method for dynamic deployment tool.
 * It generates label based on network nodes size.
 */
string Domain::generateLabel() {
    string new_label = "";
    int previous_network_size = network_nodes.size();
    do {
        std::ostringstream ss;
        ss << std::setw(ba_id_len) << std::setfill('0')
        << (++previous_network_size);
        new_label = ss.str();
    } while (labelExists(new_label));
    return new_label;
}

// Planetlab Support starts Here

string Domain::writeConfigFile(string filename) {
    
    ofstream configfile;
    
    for (size_t i = 0; i < network_nodes.size(); i++) {
        configfile.open((write_conf + filename).c_str());
        //Domain Global stuff here
        configfile << "BLACKADDER_ID_LENGTH = " << ba_id_len << ";\n";
        configfile << "LIPSIN_ID_LENGTH = " << fid_len << ";\n";
        configfile << "CLICK_HOME = \"" << click_home << "\";\n";
        configfile << "WRITE_CONF = \"" << write_conf << "\";\n";
        configfile << "USER = \"" << user << "\";\n";
        if (sudo == 1) {
            configfile << "SUDO = true;\n";
        } else {
            configfile << "SUDO = false;\n";
        }
        configfile << "OVERLAY_MODE = \"" << overlay_mode << "\";\n\n\n";
        //network
        configfile << "network = {\n";
        configfile << "    nodes = (\n";
        if (overlay_mode.compare("ip") == 0) {
            for (size_t i = 0; i < network_nodes.size(); i++) {
                NetworkNode *nn = network_nodes[i];
                configfile << "     {\n";
                configfile << "       testbed_ip = \"" << nn->testbed_ip
                << "\";\n";
                configfile << "       running_mode = \"" << nn->running_mode
                << "\";\n";
                configfile << "       label = \"" << nn->label << "\";\n";
                if ((nn->isRV == true) && (nn->isTM == true)) {
                    configfile << "       role = [\"RV\",\"TM\"];\n";
                } else if (nn->isRV == true) {
                    configfile << "       role = [\"RV\"];\n";
                } else if (nn->isTM == true) {
                    configfile << "       role = [\"TM\"];\n";
                }
                configfile << "       connections = (\n";
                for (size_t j = 0; j < nn->connections.size(); j++) {
                    NetworkConnection *nc = nn->connections[j];
                    configfile << "           {\n";
                    configfile << "               to = \"" << nc->dst_label
                    << "\";\n";
                    configfile << "               src_ip = \"" << nc->src_ip
                    << "\";\n";
                    configfile << "               dst_ip = \"" << nc->dst_ip
                    << "\";\n";
                    if (j != (nn->connections.size() - 1)) {
                        configfile << "           },\n";
                    } else {
                        configfile << "           }\n";
                    }
                }
                configfile << "        );\n";
                if (i != (network_nodes.size() - 1)) {
                    configfile << "     },\n";
                } else {
                    configfile << "     }\n";
                }
            }
            
        } else {
#if 0
            for (size_t i = 0; i < network_nodes.size(); i++) {
                NetworkNode *nn = network_nodes[i];
                
                for (size_t j = 0; j < nn->connections.size(); j++) {
                    NetworkConnection *nc = nn->connections[j];
                }
            }
#endif
        }
        configfile << "    );\n};";
    }
    
    configfile.close();
    return (write_conf + filename);
}
