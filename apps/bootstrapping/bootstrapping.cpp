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
#include "configuration.h"
#include "discovery.h"
#include "attachment.h"
#include <boost/thread/thread.hpp>

using namespace std;

configuration *config;
Blackadder *ba;
attachment *att;
discovery *d;

/**@brief The method which starts the attachment process for the specified interface.
 *
 */
void start_attachment(network_interface* ni) {
	cout << "Starting attachment thread for interface " << ni->name << "."
			<< endl;
	att->init(ni);
}

/**@brief The main method initializing the ICN bootstrapping process.
 * It will first read all interfaces, then start the discovery process,
 * and if there are more interfaces to be attached to an ICN,
 * it will start the attachment process for each of them.
 * When all are completed, it will generate the final click configuration
 * file.
 *
 */
int main(int argc, char* argv[]) {
	cout << "Starting Bootstrapping process." << endl;
	bool sdn = false;
	bool overlay = false;
	bool mininet = false;
	bool tm = false;
	char opt;
	while ((opt = getopt(argc, argv, "somt")) != -1) {
		switch (opt) {
		//is the first interface attached to sdn network? If yes, use -s flag.
		case 's':
			sdn = true;
			break;
			//is it an overlay deployment? If yes, use -o flag.
		case 'o':
			overlay = true;
			break;
			//is it a mininet deployment? If yes, use -m flag.
		case 'm':
			mininet = true;
			break;
			//what's the type of the node? If yes use -t flag.
		case 't':
			tm = true;
			break;
		case '?':
			if (isprint(optopt))
				fprintf(stderr, "Bootstrapping: Unknown option `-%c'.\n",
						optopt);
			else
				fprintf(stderr,
						"Bootstrapping: Unknown option character `\\x%x'.\n",
						optopt);
			return 1;
		default:
			cout << "Bootstrapping: something went wrong, aborting..." << endl;
			abort();
		}
	}

	ba = Blackadder::Instance(true);
	//create the configuration object
	config = new configuration(sdn, overlay, mininet, tm);
	//read the available interfaces
	vector<network_interface*> interfaces = config->read_interfaces();
	//the first contains only the first one (and should be the main one)
	vector<network_interface*> first;
	first.push_back(interfaces[0]);
	//bootstrap with the first interface first
	if (!mininet) {
		config->write_click_file(first);
		config->stop_click();
		config->start_click();
		sleep(1);
	}
	d = new discovery(config, ba);
	att = new attachment(config, ba);
	//if not TM, then requires discovery
	if (!tm) {
		d->start_discovery();
		if (!mininet) {
			config->stop_click();
			config->start_click();
			sleep(1);
		}
	}

	boost::thread_group attachment_threads;
	//if sdn or not TM then assume that the first interface is already bootstrapped.
	int i = (sdn || !tm) ? 1 : 0;
	//for the avaialble interfaces, start the attachment thread
	for (; i < interfaces.size(); i++) {
		attachment_threads.add_thread(
				new boost::thread(&start_attachment, interfaces[i]));
	}
	attachment_threads.join_all();
	//when done, update the click file configuration
	config->write_click_file(interfaces);

	//stop, start click and finish
	if (!mininet) {
		config->stop_click();
		config->start_click();
		sleep(1);
	}
}
