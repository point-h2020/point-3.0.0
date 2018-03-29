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

#ifndef APPS_RESOURCE_MANAGER_TM_RESOURCE_H_
#define APPS_RESOURCE_MANAGER_TM_RESOURCE_H_

#include <blackadder.hpp>
#include <signal.h>
#include "../../deployment/bitvector.hpp"
#include "resource_manager.h"

/**@brief The class to handle resource requests and update requests by new ICN nodes.
 * It resides at the Topology Manager, and utilizes ICN semantics (pub/sub) to receive
 * requests by new ICN nodes. It then contacts the resource manager module, to assign
 * unique resources and update the topology graph, as part of the bootstrapping protocol.
 *
 * @author George Petropoulos
 * @version cycle-2
 *
 */
class TMResource {
public:

	/**@brief The constructor.
	 *
	 */
	TMResource();

	/**@brief The constructor with argument.
	 *
	 * @param rm_ The pointer to the resource manager object.
	 *
	 */
	TMResource(ResourceManager *rm_);
	virtual ~TMResource();

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
	void init();

private:
	/**@brief The blackadder instance.
	 *
	 */
	Blackadder *ba;

	/**@brief The resource manager instance.
	 *
	 */
	ResourceManager *rm;

	/**@brief The resource scope.
	 *
	 */
	string resource_id = "4E4E4E4E4E4E4E4E"; // RESOURCE scope: "4E4E..4E";
	string bin_resource_id = hex_to_chararray(resource_id);

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

#endif /* APPS_RESOURCE_MANAGER_TM_RESOURCE_H_ */
