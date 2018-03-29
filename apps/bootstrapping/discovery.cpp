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

#include <bitvector.hpp>
#include <stdlib.h>
#include <signal.h>
#include <map>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <getopt.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <set>
#include <arpa/inet.h>
#include <algorithm>
#include <bitset>
#include <sstream>
#include <iomanip>
#include "configuration.h"
#include "discovery.h"

using namespace std;

discovery::discovery(configuration *conf, Blackadder *ba):
		config(conf), badder(ba) {

}

discovery::~discovery() {
	// TODO Auto-generated destructor stub
}

/**@brief The method which ORs a given LID to a TMFID.
 *
 *@param tmfid The given TMFID.
 *@param lid The given LID.
 *@return The merged TMFID.
 *
 */
string mergeTmfidLid(string tmfid, string lid) {
	string merged(256, ' ');
	for (int i = 0; i < 256; i++) {
		if (tmfid[i] == '1' || lid[i] == '1')
			merged[i] = '1';
		else
			merged[i] = '0';
	}
	return merged;
}

void discovery::start_discovery() {
	string nid, lid, ilid, TMFID;
	//create the resource request message
	char* resource_request = (char *) malloc(256 + 1 + config->interfaces.size() * 17 + 1 + 17);
	Bitvector *FID_to_TM;

	//create the advertisement message
	int advertisement_size = 8 + 256;
	char *advertisement = (char *) malloc(advertisement_size);

	//subscribe to discovery/offer scope to receive discovery offers by attached nodes.
	//use BROADCAST strategy in order to listen for such messages from all interfaces.
	badder->subscribe_scope(bin_offer_id, bin_discovery_id, BROADCAST_IF, NULL, 0);
	cout << "Subscribed to "
			<< chararray_to_hex(bin_discovery_id + bin_offer_id)
			<< " scope and waiting for offers." << endl;

	//publish to discovery/ scope to receive discovery offers.
	//publish them with BROADCAST strategy to send such messages to all interfaces.
	badder->publish_data(bin_discovery_id, BROADCAST_IF, NULL, 0, advertisement,
			advertisement_size);
	cout << "Published to " << chararray_to_hex(bin_discovery_id) << " scope."
			<< endl;
	bool operation = true;
	//loop for blackadder events
	while (operation) {
		Event ev;
		badder->getEvent(ev);
		switch (ev.type) {
		//if received published data
		case PUBLISHED_DATA:
			string full_id = chararray_to_hex(ev.id);
			cout << "Published data " << full_id << endl;

			//if received discovery offer publications
			if (full_id.find("dddd") != string::npos) {
				cout << "Received discovery offer publication." << endl;
				cout << "Received data length: " << ev.data_len << endl;
				//the first 256 chars are the attached node's TMFID
				TMFID = string((char *) ev.data, 0, 256);
				//the next 17 chars are the attached node's MAC address
				config->interfaces[0]->attached_mac_address = string(
						(char *) ev.data, 256, 17);
				cout << "Attached MAC address: "
						<< config->interfaces[0]->attached_mac_address << endl;
				cout << "Attached TMFID: " << TMFID << endl;

				//if no MAC address received, then it means that the attached node
				//is an SDN switch, hence assign the all-zeros MAC address
				string attached_mac_address = (config->interfaces[0]->attached_mac_address.empty()) ?
						"00:00:00:00:00:00" : config->interfaces[0]->attached_mac_address;

				//unsubscibing from discovery/offer scope, not needed anymore.
				badder->unsubscribe_scope(bin_offer_id, bin_discovery_id,
				BROADCAST_IF,
				NULL, 0);

				//initialize resource pub/sub process
				//subscribe to resource/offer scope to receive resource offer message
				//use BROADCAST strategy to receive them by all interfaces
				badder->subscribe_scope(bin_offer_id, bin_resource_id, BROADCAST_IF,
				NULL, 0);
				cout << "Subscribed to "
						<< chararray_to_hex(bin_resource_id + bin_offer_id)
						<< " scope and waiting for offers." << endl;

				//add the tmfid and the corresponding mac address in resource request
				string resource_request_string = TMFID + config->interfaces[0]->mac_address
						+ attached_mac_address;

				//if only one interface, then request no additional resources
				if (config->interfaces.size() == 1)
					resource_request_string = resource_request_string + "0";
				//otherwise add the available MAC addresses to the list of requests
				else {
					resource_request_string = resource_request_string +
							to_string(config->interfaces.size() - 1);
					for (int i = 1; i < config->interfaces.size(); i++) {
						resource_request_string = resource_request_string
							+ config->interfaces[i]->mac_address;
					}
				}

				snprintf(resource_request, 256 + 1 + config->interfaces.size() * 17 + 1 + 17,
						resource_request_string.c_str());

				cout << "Resource request " << resource_request << endl;
				cout << "Published data " << TMFID << endl;
				FID_to_TM = new Bitvector(TMFID);
				cout << "fid to tm " << FID_to_TM->to_string() << endl;

				//publish to resource/request scope to send a resource request message
				//use IMPLICIT RENDEZVOUS to send the request directly to the TM.
				badder->publish_data(bin_resource_id + bin_request_id,
						IMPLICIT_RENDEZVOUS, (char *) FID_to_TM->_data,
						FID_LEN, resource_request, 256 + 1 + config->interfaces.size() * 17 + 1 + 17);
				cout << "Published to "
						<< chararray_to_hex(bin_resource_id + bin_request_id)
						<< " scope." << endl;
				break;

			}
			//if received resource offer publication
			else if (full_id.find("4e4e") != string::npos) {
				cout << "Received resource offer publication." << endl;
				char* data = (char *) ev.data;
				cout << "Received data: " << data << endl;
				cout << "Received data length: " << ev.data_len << endl;
				//if does not have a node identifier, then assign the received one.
				if (nid.empty()) {
					//the first 8 chars of the resource offer are the node id
					nid = string(data, 0, 8);
					cout << "Node ID: " << nid << endl;
					//the first 256 chars of the resource offer are the internal link id
					ilid = string(data, 8, 256);
					cout << "Internal LID: " << ilid << endl;
					//iterate the list of interfaces
					for (int i=0; i<config->interfaces.size(); i++) {
						//each 256 chars are the assigned link id for the interface
						lid = string(data, 8 + 256 * (i+1), 256);
						cout << "LID: " << lid << endl;
						config->interfaces[i]->lid = lid;
					}
					//create the new TM/RVFID using the assigned LID.
					string RVFID = mergeTmfidLid(TMFID, config->interfaces[0]->lid);

					//unsubscibe from resource/offer scope, not needed anymore.
					badder->unsubscribe_scope(bin_offer_id, bin_resource_id,
					BROADCAST_IF, NULL, 0);

					//update click configuration
					config->update_configuration(nid, RVFID, RVFID, ilid);
					operation = false;
					break;
				}
			}
		}
	}
	cout << "Finished here " << endl;
	free(resource_request);
	free(advertisement);
	return;
}
