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

#ifndef APPS_BOOTSTRAPPING_CONFIGURATION_H_
#define APPS_BOOTSTRAPPING_CONFIGURATION_H_

#include <vector>
#include <string>
#include <algorithm>
#include <blackadder.hpp>
#include <nb_blackadder.hpp>
#include <bitvector.hpp>
#include <signal.h>

using namespace std;

class network_interface;

/**@brief The class for creating and updating click configuration.
 *
 * @author George Petropoulos
 * @version cycle-2
 *
 */
class configuration {
public:
	/**@brief The constructor.
	 *
	 */
	configuration();

	/**@brief The constructor with argument.
	 *
	 * @param flag A boolean flag indicating whether it's native ICN or ICN-SDN configuration.
	 *
	 */
	configuration(bool sdn, bool overlay, bool mininet, bool tm);
	virtual ~configuration();

	/**@brief The method which updates the click configuration with the new parameters.
	 *
	 * @param new_node_id The new node identifier.
	 * @param new_rv_fid The new RV FID.
	 * @param new_tm_fid The new TM FID.
	 *
	 */
	void update_configuration(string new_node_id, string new_rv_fid, string new_tm_fid,
			string new_i_lid);

	/**@brief The method which reads the available network interfaces.
	 *
	 * It works only in linux.
	 * It currently identifies only eth0 interfaces. To be changed.
	 *
	 */
	vector<network_interface*> read_interfaces();

	/**@brief The method which generates the click configuration file.
	 *
	 */
	void write_click_file(vector<network_interface*> interfaces);

	/**@brief The method which starts the click process.
	 *
	 */
	void start_click();

	/**@brief The method which stops the click process.
	 *
	 */
	void stop_click();
	/**@brief The vector of the available network interfaces.
	 *
	 */
	vector<network_interface*> interfaces;

	bool sdn_flag, overlay_flag, mininet_flag, tm_flag;
	/**@brief The default TM FID.
	 *
	 */
	string tm_fid = string(256, '0');
	/**@brief The default node ID for a new node.
	 *
	 */
	string label = "99999999";
	/**@brief The default node ID for the TM.
	 *
	 */
	string tm_label = "00000001";

private:

	/**@brief The method which adds a network interface to the vector of network interfaces.
	 *
	 */
	void add_interface_to_vector(network_interface* ni);

	bool sudo = true;
	string rv_fid = string(256, '0');
	string internal_lid = string(256, '0');
	string tm_internal_lid = '1' + string(255, '0');
	string link_id = string(256, '0');
	string click_home = "/usr/local/";
	string write_conf = "/tmp/";
	string overlay_mode = "mac";
	string type = "FW";

};

/**@brief The network interface supporting class.
 *
 */
class network_interface {
public:
	string name;
	string mac_address;
	string ip_address;
	string lid;
	string attached_mac_address;
	bool sdn_enabled;
	friend bool operator ==(const network_interface &n1,
			const network_interface &n2) {
		return n1.name == n2.name;
	}
	friend bool operator !=(const network_interface &n1,
			const network_interface &n2) {
		return !(n1 == n2);
	}
};

#endif /* APPS_BOOTSTRAPPING_CONFIGURATION_H_ */
