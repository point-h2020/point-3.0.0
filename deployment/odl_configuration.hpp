/*
 * Copyright (C) 2017  George Petropoulos
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 3 as published by the Free Software Foundation.
 *
 * See LICENSE and COPYING for more details.
 */

#ifndef ODL_CONFIGURATION_HPP_
#define ODL_CONFIGURATION_HPP_
#include <string>
using namespace std;

/**@brief The class for configuring the ABM flow rules to the ODL controller.
 *
 * @author George Petropoulos
 * @version cycle-2
 *
 */
class OdlConfiguration {
public:
	/**@brief The constructor.
	 */
	OdlConfiguration();
	/**@brief The constructor with argument.
	 *
	 * @param odl The IP address of the ODL controller.
	 */
	OdlConfiguration(string odl);
	virtual ~OdlConfiguration();
	/**@brief The method which configures a provided flow to a specific openflow switch.
	 *
	 * @param openflow_id The openflow identifier of the switch to be configured.
	 * @param table The table identifier to be configured.
	 * @param flow The flow to be configured in JSON format.
	 * @param flow_id The flow identifier to be configured.
	 *
	 */
	void configureFlow(string openflow_id, int table, string flow, int flow_id);
	/**@brief The method which configures a provided fast failover group
	 * to a specific openflow switch.
	 *
	 * @param openflow_id The openflow identifier of the switch to be configured.
	 * @param group The group to be configured in JSON format.
	 * @param group_id The group identifier to be configured.
	 *
	 */
	void configureGroup(string openflow_id, string group, int group_id);

	/**@brief The method which configures the POINT-enabled SDN controller
	 * node connector registry.
	 *
	 * @param connector_registry_entry The node connector registry entry.
	 *
	 */
	void configureNodeConnectorRegistry(string connector_registry_entry);

	/**@brief The method which configures the POINT-enabled SDN controller
	 * node registry.
	 *
	 * @param connector_registry_entry The node connector registry entry.
	 *
	 */
	void configureNodeRegistry(string node_registry_entry);
	/**@brief The IP address of the ODL controller.
	 */
	string odl_ip_address;
};

#endif /* ODL_CONFIGURATION_HPP_ */
