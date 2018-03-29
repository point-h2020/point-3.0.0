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

#include "attachment.h"

using namespace std;

attachment::attachment(configuration *conf, Blackadder *ba):
				config(conf), badder(ba) {

}

attachment::~attachment() {
	// TODO Auto-generated destructor stub
}

void attachment::init(network_interface* ni) {

	std::cout << "Initializing attachment for network interface "
			<< ni->mac_address << endl;

	//subscribe to discovery/ scope to receive discovery requests
	//subscribe with LINK_LOCAL strategy to receive such messages,
	//only from the specified link, with LID.
	Bitvector LID = Bitvector(ni->lid);
	badder->subscribe_scope(bin_discovery_id, "", LINK_LOCAL,
			(char*) LID._data, FID_LEN);
	cout << "Subscribed to " << chararray_to_hex(bin_discovery_id)
			<< " scope and waiting for resource requests." << endl;

	string full_id;
	Bitvector RVTMFID(FID_LEN * 8);
	char *update_offer = (char *) malloc(17 + 1);
	char *discovery_offer = (char *) malloc(256 + 17 + 1);
	bool operation = true;

	//loop waiting for blackadder events
	while (operation) {
		Event ev;
		badder->getEvent(ev);
		switch (ev.type) {
		//if received published data
		case PUBLISHED_DATA:
			string full_id = chararray_to_hex(ev.id);
			cout << "Published data " << full_id << endl;

			//if received discovery request publication
			if (full_id.find("dddd") != string::npos) {
				cout << "Received discovery request publication." << endl;
				RVTMFID = Bitvector(config->tm_fid);
				//create the discovery offer message
				//it contains the TMFID of the node, and the MAC address
				//from which it received the discovery request
				snprintf(discovery_offer, 256 + 17 + 1,
						(config->tm_fid + ni->mac_address).c_str());

				//subscribe to update/request scope for update request by TM
				//use LINK_LOCAL strategy to receive them only from the
				//specified link with LID.
				badder->subscribe_scope(bin_request_id, bin_update_id, LINK_LOCAL,
						(char*) LID._data, FID_LEN);
				cout << "Subscribed to "
						<< chararray_to_hex(bin_update_id + bin_request_id)
						<< " scope and waiting for offers." << endl;

				//publish to discovery/offer scope to new node with
				//BROADCAST strategy
				badder->publish_data(bin_discovery_id + bin_offer_id,
						BROADCAST_IF, NULL, 0, discovery_offer, 256 + 17 + 1);
				break;
			}
			//if received update request message
			else if (full_id.find("0404") != string::npos) {
				cout << "Received update request publication "
						<< (char *) ev.data << endl;

				//extract the received parameters of the new node.
				char* data = (char *) ev.data;
				//the first 8 chars are the node id of the node
				cout << "Received data: " << data << endl;
				string attached_node_id = string(data, 0, 8);
				cout << "Attached node id: " << attached_node_id << endl;
				//the next 17 chars are the MAC address of the node
				string attached_mac_address = string(data, 8, 17);
				cout << "Attached MAC Address: " << attached_mac_address << endl;
				ni->attached_mac_address = attached_mac_address;

				//unsubscribe from update request scope
				badder->unsubscribe_scope(bin_request_id, bin_update_id, LINK_LOCAL,
						(char*) LID._data, FID_LEN);
				cout << "Unubscribed from "
						<< chararray_to_hex(bin_update_id + bin_request_id)
						<< " scope." << endl;
				sleep(1);
				//create update offer with own mac address
				snprintf(update_offer, 17 + 1, ni->mac_address.c_str());
				//publish update offer to TM to scope update/offer/own_node_id/attached_node_id
				badder->publish_data(bin_update_id + bin_offer_id + config->label + attached_node_id,
								IMPLICIT_RENDEZVOUS, (char*) RVTMFID._data, FID_LEN,
								update_offer, 17 + 1);

				operation = false;
				free(discovery_offer);
				free(update_offer);
				break;
			}
		}
	}
}

