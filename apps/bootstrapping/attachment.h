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

#ifndef APPS_BOOTSTRAPPING_ATTACHMENT_H_
#define APPS_BOOTSTRAPPING_ATTACHMENT_H_

#include <blackadder.hpp>
#include <signal.h>
#include "configuration.h"
#include <bitvector.hpp>

/**@brief The class for any attachment node to handle resource requests by new nodes.
 *
 * @author George Petropoulos
 * @version cycle-2
 *
 */
class attachment {
public:

	/**@brief The constructor.
	 *
	 */
	attachment(configuration *conf, Blackadder *ba);

	virtual ~attachment();

	/**@brief The method which returns the substring of a given string.
	 *
	 * @param input The given string as input.
	 * @param start The starting position of the substring.
	 * @param len The length of the substring.
	 *
	 * @return The calculated substring.
	 *
	 */
	char *substring(char const *input, size_t start, size_t len);

	/**@brief The method which initializes the class.
	 *
	 * It listens for resource requests at a given scope.
	 * When it receives one, it then extracts the received parameters,
	 * including the TMFID and MAC address, and requests by the resource
	 * manager unique node id and link id and publishes them as
	 * resource offer.
	 *
	 */
	void init(network_interface* ni);

private:
	/**@brief The blackadder instance.
	 *
	 */
	Blackadder *badder;

	/**@brief The configuration object.
	 *
	 */
	configuration *config;

	/**@brief The discovery scope.
	 *
	 */
	string discovery_id = "DDDDDDDDDDDDDDDD"; // DISCOVERY scope: "DDD..DD";
	string bin_discovery_id = hex_to_chararray(discovery_id);

	/**@brief The update scope.
	 *
	 */
	string update_id = "0404040404040404"; // UPDATE scope: "0404..04";
	string bin_update_id = hex_to_chararray(update_id);

	/**@brief The offer scope.
	 *
	 */
	string offer_id = "4545454545454545"; // OFFER scope: "4545..45";
	string bin_offer_id = hex_to_chararray(offer_id);

	/**@brief The request scope.
	 *
	 */
	string request_id = "5555555555555555"; // REQUEST scope: "5555..55";
	string bin_request_id = hex_to_chararray(request_id);
};

#endif /* APPS_BOOTSTRAPPING_ATTACHMENT_H_ */
