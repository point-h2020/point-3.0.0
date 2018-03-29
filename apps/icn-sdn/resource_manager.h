/*
 * Copyright (C) 2017  George Petropoulos
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 3 as published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of
 * the BSD license.
 *
 * See LICENSE and COPYING for more details.
 */

#ifndef APPS_BOOTSTRAPPING_RESOURCE_MANAGER_H_
#define APPS_BOOTSTRAPPING_RESOURCE_MANAGER_H_

#include <iostream>
#include <map>
#include <vector>
#include "../../deployment/network.hpp"
#include "../../deployment/graph_representation.hpp"
#include "../../deployment/igraph_version.hpp"
#include "messages/messages.pb.h"
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace eu::point::tmsdn::impl;
class attachment_information;

/**@brief The resource manager module.
 * It implements methods to generate unique node, link and internal link identifiers,
 * keep track of assigned ones for new ICN nodes and maintain the topology graph.
 *
 * @author George Petropoulos
 * @version cycle-2
 *
 */
class ResourceManager {
public:
	/**@brief The constructor.
	 *
	 */
	ResourceManager();
	/**@brief The constructor with arguments.
	 *
	 * @param tm_nid The node identifier of the TM.
	 * @param tm_of_id The openflow identifier of the TM host.
	 * @param attached_switch_of_id The openflow identifier of the SDN switch attached to the TM.
	 *
	 */
	ResourceManager(string tm_nid, string tm_of_id, string attached_switch_of_id);

	virtual ~ResourceManager();

	/**@brief The method which generates a new node identifier, given node's information.
	 *
	 * It is used to generate unique node identifier for requests performed by the
	 * SDN controller. New nodes are identified by their SDN controller-internal
	 * identifier.
	 *
	 * @param node_information The new node SDN controller-internal identifier.
	 * @return The unique node identifier.
	 *
	 */
	string generate_node_id(string node_information);

	/**@brief The method which creates a resource offer for a received resource request.
	 *
	 * It is used to generate unique link identifier for resource requests via the TM-SDN interface.
	 * It is the main method which orchestrates the rest of functions for the generation of
	 * unique resources for SDN nodes.
	 * It first checks whether the node has not unique identifiers and then creates, otherwise
	 * returns the already provided ones. All are then included in the resource offer to be sent
	 * back to the SDN controller.
	 *
	 * @param request The received resource request.
	 * @param offer The resource offer to be filled with all the unique identifiers.
	 *
	 */
	void generate_link_id(
			TmSdnMessage::ResourceRequestMessage::RecourceRequest request,
			TmSdnMessage::ResourceOfferMessage::RecourceOffer *offer);

	/**@brief The method which generates a new link identifier, given node's information.
	 *
	 * It is used to generate unique link identifier for requests performed by the
	 * SDN controller. New links are identified by the SDN controller-internal
	 * identifiers of the 2 nodes being connected.
	 *
	 * @param node_information The one side of the new link, identified by node's SDN
	 * controller-internal identifier.
	 * @param attached_node_information The second side of the new link, identified by
	 * node's SDN controller-internal identifier.
	 * @return The generated link identifier.
	 *
	 */
	string generate_link_id(string node_information,
			string attached_node_information);

	/**@brief The method which generates a new link identifier by requests performed by the
	 * native ICN bootstrapping protocol.
	 *
	 * It is used to generate unique link identifier for requests performed directly by the
	 * new node. New links are identified by the TMFID of the new node, and the MAC
	 * address of the new link interface.
	 *
	 * @param tmfid The new node's TMFID.
	 * @param mac_address The MAC address of the new link interface.
	 * @param attached_mac The MAC address of the attached network interface.
	 * @return The generated attachment information.
	 *
	 */
	attachment_information generate_link_id_from_icn(string tmfid, string mac_address,
			string attached_mac);

	/**@brief The method which generates new link identifiers by requests performed by the
	 * native ICN bootstrapping protocol.
	 *
	 * It is used to generate unique link identifier for requests performed directly by the
	 * new node. New links are identified by the source node identifier of the new node, and the MAC
	 * address of the new link interface.
	 *
	 * @param src_node_id The source node identifier.
	 * @param mac_addresses The list of MAC address of the new link interfaces.
	 * @return The generated vector of attachment information.
	 *
	 */
	vector<attachment_information> generate_link_ids(string src_node_id,
			vector<string> mac_addresses);

	/**@brief The method which generates a new link internal identifier.
	 *
	 * It is used to generate unique internal link identifier for a new node.
	 *
	 * @param src_node_id The new node's identifier.
	 * @param flag A boolean flag indicating whether internal lid should be calculated.
	 * @return The generated internal link identifier.
	 *
	 */
	string generate_internal_link_id(string src_node_id, bool flag);

	/**@brief The method which translates a numerical node identifier to a string of 8 characters.
	 *
	 * @param number The numerical node id.
	 * @return The string node id of 8 characters.
	 *
	 */
	string number_to_node_id(int number);

	/**@brief The method which creates a unique node identifier.
	 *
	 * It checks the list of available node identifiers and assigns a unique one.
	 *
	 * @return The numerical node identifier.
	 *
	 */
	int generate_node_id_number();

	/**@brief The method which creates a unique link identifier.
	 *
	 * It checks the list of available link identifiers and assigns a unique one.
	 *
	 * @return The numerical link identifier.
	 *
	 */
	int generate_link_id_bit_position();

	/**@brief The method which translates the position of '1' bit in a given LID into a string LID.
	 *
	 * @param number The position of '1' bit
	 * @return The string link identifier of 256 characters.
	 *
	 */
	string position_to_lid(int number);

	/**@brief The method which adds a new node to the topology graph.
	 *
	 * @param src The new node's node identifier.
	 * @param tmrv A boolean flag indicating whether the new node is the TM/RV.
	 *
	 */
	void addNode(string src, bool tmrv);

	/**@brief The method which adds a new node connector to the topology graph.
	 *
	 * @param src_node The new node's node identifier.
	 * @param dst_node The new node's neighboring node identifier for the new node connector.
	 * @param lid_bits The given link identifier of the node.
	 * @param ilid_bits The assigned internal link identifier of the node.
	 *
	 */
	void addNodeConnector(string src_node, string dst_node, string lid_bits, string ilid_bits);

	/**@brief The method which adds a new node connector to the topology graph.
	 *
	 * @param src_node The new node's node identifier.
	 * @param src_mac The new node's mac address.
	 * @param dst_node The new node's attached node identifier for the new node connector.
	 * @param dst_mac The new node's attached node mac address
	 * @param lid_bits The given link identifier of the node.
	 * @param ilid_bits The assigned internal link identifier of the node.
	 *
	 */
	void addNodeConnector(string src_node, string src_mac, string dst_node, string dst_mac,
			string lid_bits, string ilid_bits);

	/**@brief The method which adds only one side of a new node connector to the topology graph.
	 *
	 * @param src_node The new node's node identifier.
	 * @param src_mac The new node's mac address.
	 * @param lid_bits The given link identifier of the node.
	 *
	 */
	void addNodeConnectorOneSide(string src_node, string src_mac, string lid);

	/**@brief The method which updates both sides of a node connector to the topology graph.
	 *
	 * @param src_node The new node's node identifier.
	 * @param src_mac The new node's MAC address.
	 * @param dst_node The new node's attached node identifier for the new node connector.
	 * @param dst_mac The new node's attached node MAC address.
	 *
	 */
	void updateNodeConnectors(string src_node, string src_mac, string dst_node, string dst_mac);

	/**@brief The method which generates the new graphml file for the topology manager.
	 *
	 */
	void generateGraphmlFile();

	/**@brief The method which updates the topology graph.
	 *
	 */
	void updateGraph();

	/**@brief The method which returns the attached node of a new node.
	 *
	 * It uses the given TMFID to identify which is the attached node.
	 *
	 * @param tmfid The TMFID of the neighboring node.
	 * @return The node identifier of the neighboring node.
	 *
	 */
	string get_attached_node(string tmfid);

	/**@brief The method which returns the FID to reach a new node.
	 *
	 * @param node The identifier of the node.
	 * @return The FID to reach the new node.
	 *
	 */
	string get_fid_to_node(string node);

	/**@brief The given link identifiers.
	 *
	 */
	vector<int> given_lids;


private:
	/**@brief The given node identifiers per SDN id.
	 *
	 */
	map<string, int> given_node_ids;

	/**@brief The given link identifiers per SDN id.
	 *
	 */
	map<string, int> given_link_ids;

	/**@brief The given internal link identifiers per SDN id.
	 *
	 */
	map<string, int> given_internal_link_ids;

	/**@brief The given node identifiers.
	 *
	 */
	vector<int> given_nids;

	/**@brief The topology graph.
	 *
	 */
	Domain dm;
	GraphRepresentation graph = GraphRepresentation(&dm, false);
	string tm_label = "00000001";
};

/**@brief The class including all required parameters of a new node, and its attachment.
 *
 */
class attachment_information {
public:
	string new_node_id;
	string new_node_lid;
	string new_node_ilid;
	string attached_node_id;
};

#endif /* APPS_BOOTSTRAPPING_RESOURCE_MANAGER_H_ */
