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
#ifndef DISCOVERY_H_
#define DISCOVERY_H_

#include <iostream>
#include <blackadder.hpp>
#include <nb_blackadder.hpp>

/**@brief The discovery class, which implements the bootstrapping protocol for a new node.
 * It utilizes ICN semantics to broadcast its presence to the ICN network, and receive
 * the TMFID from the attached node and then contact directly the TM
 * (and hence the tm_resource module) to receive unique resources.
 *
 * @author George Petropoulos
 * @version cycle-2
 *
 */
class discovery {
public:
	/**@brief The constructor with arguments
	 *
	 */
	discovery(configuration *conf, Blackadder *ba);
	virtual ~discovery();
	/**@brief The main method initializing the ICN bootstrapping process.
	 *
	 * It first publishes discovery requests to the discovery scope.
	 * When it receives a discovery offer with the attached node's TMFID,
	 * it then prepares and sends a resource request to the TM.
	 * When it receives the resource offer, it then updates the configuration.
	 *
	 */
	void start_discovery();

private:
	/**@brief The configuration object.
	 *
	 */
	configuration *config;
	/**@brief The blackadder object.
	 *
	 */
	Blackadder *badder;
	/**@brief The discovery scope.
	 *
	 */
	string discovery_id = "DDDDDDDDDDDDDDDD"; // DISCOVERY scope: "DDD..DD";
	string bin_discovery_id = hex_to_chararray(discovery_id);

	/**@brief The resource scope.
	 *
	 */
	string resource_id = "4E4E4E4E4E4E4E4E"; // RESOURCE scope: "4E..4E";
	string bin_resource_id = hex_to_chararray(resource_id);

	/**@brief The request scope.
	 *
	 */
	string request_id = "5555555555555555"; // REQUEST scope: "5555..55";
	string bin_request_id = hex_to_chararray(request_id);

	/**@brief The offer scope..
	 *
	 */
	string offer_id = "4545454545454545"; // OFFER scope: "4545..45";
	string bin_offer_id = hex_to_chararray(offer_id);
};


#endif /* DISCOVERY_H_ */
