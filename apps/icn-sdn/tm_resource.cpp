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

#include "tm_resource.h"

using namespace std;

TMResource::TMResource() {

}

TMResource::TMResource(ResourceManager *rm_): rm(rm_) {

}


TMResource::~TMResource() {
	// TODO Auto-generated destructor stub
}

void TMResource::init() {

	std::cout << "Initializing TM Resource Manager." << endl;

	ba = Blackadder::Instance(true);
	//gets from the resource manager the LID of the first interface,
	//on which the module will listen for resource requests
	string lid_string = rm->position_to_lid(rm->given_lids[1]);
	Bitvector LID = Bitvector(lid_string);
	cout << "Listening link local to lid " << lid_string << endl;

	//subscribe to resource/request scope, in order to receive Resource Request messages
	//it subscibes to LINK_LOCAL strategy, hence listens for messages
	//only to the specified interface (with LID)
	ba->subscribe_scope(bin_request_id, bin_resource_id, LINK_LOCAL, (char*)LID._data, FID_LEN);
	cout << "Subscribed to "
			<< chararray_to_hex(bin_resource_id + bin_request_id)
			<< " scope and waiting for resource requests." << endl;
	string full_id;
	bool operation = true;

	//loop to wait for blackadder events
	while (operation) {
		Event ev;
		ba->getEvent(ev);
		switch (ev.type) {
		//received published_data
		case PUBLISHED_DATA:
			full_id = chararray_to_hex(ev.id);
			cout << "ID " << ev.id << endl;
			cout << "Published data " << full_id << endl;
			//if scope id is request scope, hence a Resource Request message,
			//then call the resource manager to assign unique resources
			//and return them as Resource Offer message, thus publish them to
			//the appropriate scope.
			if (full_id.find("4e4e") != string::npos) {
				cout << "Received resource request publication "
						<< (char *) ev.data << endl;
				//extract the received parameters of the new node.
				char* data = (char *) ev.data;
				cout << "Received data: " << data << endl;
				//always the first 256 chars are the TMFID of the new node's neighbor
				string received_tmfid = string(data, 0, 256);
				cout << "Received TMFID: " << received_tmfid << endl;
				//the next 17 chars are the MAC address of the new node
				string received_mac = string(data, 256, 17);
				cout << "Received MAC address: " << received_mac << endl;
				//the next 17 chars are the MAC address of the new node's neighbor
				string attached_mac = string(data, 256 + 17, 17);
				cout << "Received attached MAC address: " << attached_mac << endl;

				//request resources for the new node
				attachment_information info = rm->generate_link_id_from_icn(
						received_tmfid, received_mac, attached_mac);
				//the next 1 char indicates how many MAC addresses the new node has
				int number_of_mac_addresses = stoi(
						string(data, 256 + 17 + 17, 1));
				cout << "Received number of mac addresses: "
						<< number_of_mac_addresses << endl;

				vector<string> received_mac_addresses;
				//for the received number of MAC addresses,
				//examine the next 17 chars to extract the MAC address and add them to a list.
				for (int i = 0; i < number_of_mac_addresses; i++) {
					string additional_mac = string(data, 257 + 17 + 17 + 17 * i, 17);
					cout << "MAC Address #" << i << ": " << additional_mac
							<< endl;
					received_mac_addresses.push_back(additional_mac);
				}
				//for the list of MAC addresses, request from the resource manager
				//a unique LID for each.
				vector<attachment_information> connections_info =
						rm->generate_link_ids(info.new_node_id,
								received_mac_addresses);

				//prepare the resource offer to be sent as a response to the new node
				//the resource offer includes, the new node id (8 chars),
				//the new node's internal LID (256 chars), the LID for the
				//new node's main interface (256 chars), and as many LIDs
				//as the received MAC addresses from the resource request.
				//hence the offer will provide resources for all interfaces
				//of the new node.
				string offer_string = info.new_node_id + info.new_node_ilid
						+ info.new_node_lid;

				for (int i = 0; i < number_of_mac_addresses; i++) {
					offer_string = offer_string
							+ connections_info[i].new_node_lid;
				}

				char* offer = (char *) malloc(
						8 + 256 + 256 + 256 * number_of_mac_addresses + 1);
				snprintf(offer,
						8 + 256 + 256 + 256 * number_of_mac_addresses + 1,
						offer_string.c_str());

				//calculate the FID from the TM to the new node.
				string fidtonode = rm->get_fid_to_node(info.new_node_id);
				Bitvector *fid_to_new_node = new Bitvector(fidtonode);

				//publish the resource offer to new node
				//using IMPLICIT RENDEZVOUS strategy, which means direct communication
				//with the new node
				ba->publish_data(bin_resource_id + bin_offer_id,
						IMPLICIT_RENDEZVOUS, (char *) fid_to_new_node->_data, FID_LEN,
						offer,
						8 + 256 + 256 + 256 * number_of_mac_addresses + 1);

				//prepare the update request to its attached node,
				//including the new node's id (8 chars) and mac address (17 chars)
				char* update_request = (char *) malloc(8 + 17 + 1);
				snprintf(update_request, 8 + 17 + 1, (info.new_node_id + received_mac).c_str());
				string fidtoattachednode = rm->get_fid_to_node(info.attached_node_id);
				Bitvector *fid_to_attached_node = new Bitvector(fidtoattachednode);

				//publish the update request (update/request scope) to the attached node
				//using IMPLICIT RENDEZVOUS strategy, which means direct communication
				//with the attached node
				ba->publish_data(bin_update_id + bin_request_id,
						IMPLICIT_RENDEZVOUS, (char *) fid_to_attached_node->_data,
						FID_LEN, update_request, 8 + 17 + 1);

				//subscribe to update offer scopes
				//(update/offer and update/offer/attached_node_id scopes)
				//to receive update offers by the attached node
				ba->subscribe_scope(bin_offer_id, bin_update_id,
						LINK_LOCAL, (char*) LID._data, FID_LEN);
				ba->subscribe_scope(info.attached_node_id, bin_update_id + bin_offer_id,
						LINK_LOCAL, (char*) LID._data, FID_LEN);
				cout << "Subscribed to "
							<< chararray_to_hex(bin_update_id + bin_offer_id)
							<< info.attached_node_id
							<< " scope and waiting for update offers." << endl;

				free(offer);
				free(update_request);
				break;

			}
			//if scope id is update scope, hence a Update Request message,
			//then just extract the relevant parameters (these are not used anywhere anymore).
			else if (full_id.find("0404") != string::npos) {
				cout << "ID " << ev.id << endl;
				string src_node, dst_node;
				src_node = string(ev.id, 2*PURSUIT_ID_LEN, NODEID_LEN);
				dst_node = string(ev.id, 2*PURSUIT_ID_LEN + NODEID_LEN, NODEID_LEN);
				cout << "Received update offer publication " << (char *) ev.data
						<< " for src/dst " << src_node << "/" << dst_node << endl;
				char* data = (char *) ev.data;
				string received_mac = string(data, 0, 17);
				cout << "Received mac address: " << received_mac << endl;

				//rm->updateNodeConnectors(src_node, received_mac, dst_node);
				break;
			}
		}
	}
}

