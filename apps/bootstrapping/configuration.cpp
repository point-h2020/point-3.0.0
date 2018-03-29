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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <linux/if_link.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <map>
#include <stdlib.h>
#include <getopt.h>

using namespace std;

configuration::configuration() {

}

configuration::configuration(bool sdn, bool overlay, bool mininet, bool tm):
		sdn_flag(sdn), overlay_flag(overlay), mininet_flag(mininet), tm_flag(tm) {

}

configuration::~configuration() {
	// TODO Auto-generated destructor stub
}

vector<network_interface*> configuration::read_interfaces() {
	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	char ip_address[NI_MAXHOST];

	//get available machine interfaces
	if (getifaddrs(&ifaddr) == -1)
	{
		cerr << "Error while reading interfaces." << endl;
		return interfaces;
	}

	//iterate the list of interfaces and create the network interfaces list
	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
	{
		network_interface* ni = new network_interface();
		if (ifa->ifa_addr == NULL)
			continue;
		family = ifa->ifa_addr->sa_family;
		if (family == AF_INET)
		{
			s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
					ip_address,
					NI_MAXHOST,
					NULL, 0, NI_NUMERICHOST);
			if (s != 0)
				continue;
			//extract the relevant parameters
			ni->name = ifa->ifa_name;
			ni->ip_address = ip_address;
			//for sdn deployment, assume *eth0 interfaces
			if (sdn_flag && ni->name.find("eth0") != string::npos)
				ni->sdn_enabled = true;
			//if lo then continue
			if (ni->name.find("lo") != string::npos)
				continue;
			//if lo then continue
			if (ni->name.find("eth") != string::npos)
				add_interface_to_vector(ni);
		}
		else if (family == AF_PACKET)
		{
			struct sockaddr_ll *s = (struct sockaddr_ll*) ifa->ifa_addr;
			char macp[19];
			int len = 0;
			for (int i = 0; i < 6; i++)
				len += sprintf(macp + len, "%02X%s", s->sll_addr[i],
						i < 5 ? ":" : "");
			ni->name = ifa->ifa_name;
			ni->mac_address = macp;
			//for sdn deployment, assume *eth0 interfaces
			if (sdn_flag && ni->name.find("eth0") != string::npos)
				ni->sdn_enabled = true;
			//if lo then continue
			if (ni->name.find("lo") != string::npos)
				continue;
			//if lo then continue
			if (ni->name.find("eth") != string::npos)
				add_interface_to_vector(ni);
		}
	}

	freeifaddrs(ifaddr);
	return interfaces;
}

void configuration::add_interface_to_vector(network_interface* ni) {
	bool found = false;
	for (int i = 0; i < interfaces.size(); i++)
	{
		if (interfaces[i]->name.compare(ni->name) == 0)
		{
			if (interfaces[i]->mac_address.empty())
				interfaces[i]->mac_address = ni->mac_address;
			if (interfaces[i]->ip_address.empty())
				interfaces[i]->ip_address = ni->ip_address;
			found = true;
			break;
		}
	}
	if (!found) {
		if (ni->name.find("eth0") != string::npos)
			interfaces.insert(interfaces.begin(), ni);
		else
			interfaces.push_back(ni);
	}
}

void configuration::write_click_file(vector<network_interface*> interfaces) {
	ofstream click_conf;
	ofstream write_TMFID;

	label = (tm_flag) ? tm_label : label;
	type = (tm_flag) ? "FW:RV:TM" : type;
	rv_fid = (tm_flag) ? tm_internal_lid : rv_fid;
	tm_fid = (tm_flag) ? tm_internal_lid : tm_fid;
	internal_lid = (tm_flag) ? tm_internal_lid : internal_lid;

	click_conf.open((write_conf + label + ".conf").c_str());
	write_TMFID.open((write_conf + label + "_TMFID.txt").c_str());

	/*Blackadder Elements First*/
	click_conf << "require(blackadder);" << endl << endl;
	click_conf << "globalconf::GlobalConf(TYPE " << type << ", MODE "
			<< overlay_mode << ", NODEID " << label << "," << endl;
	click_conf << "DEFAULTRV " << rv_fid << "," << endl;
	click_conf << "TMFID     " << tm_fid << "," << endl;
	write_TMFID << rv_fid << endl;
	click_conf << "iLID      " << internal_lid << ");" << endl << endl;


	click_conf << "localRV::LocalRV(globalconf);" << endl;
	click_conf << "netlink::Netlink();" << endl
			<< "tonetlink::ToNetlink(netlink);" << endl
			<< "fromnetlink::FromNetlink(netlink);" << endl << endl;
	click_conf << "proxy::LocalProxy(globalconf);" << endl << endl;
	click_conf << "fw::Forwarder(globalconf," << interfaces.size() << ","
			<< endl;

	for (size_t j = 0; j < interfaces.size(); j++)
	{
		string proto_type = (interfaces[j]->sdn_enabled) ? "0x86dd" : "0x080a";
		string dst_mac = "";
		if (interfaces[j]->attached_mac_address.empty())
			dst_mac = (interfaces[j]->sdn_enabled) ? "00:00:00:00:00:00" : "ff:ff:ff:ff:ff:ff";
		else
			dst_mac = interfaces[j]->attached_mac_address;
		if (tm_flag)
			interfaces[j]->lid = string(j+1, '0') + '1' + string(255-j-1, '0');
		string src_lid = "";
		if (!interfaces[j]->lid.empty())
			src_lid = interfaces[j]->lid;
		else
			src_lid = link_id;
		click_conf << j+1 << "," << interfaces[j]->mac_address << "," << dst_mac
				<< "," << proto_type << "," << src_lid;
		if (j < interfaces.size() - 1)
			click_conf << "," << endl;
	}
	click_conf << ");" << endl << endl;

	for (size_t j = 0; j < interfaces.size(); j++)
	{
		click_conf << "tsf" << j << "::ThreadSafeQueue(1000);" << endl;
		string promisc = (interfaces[j]->sdn_enabled) ? ", PROMISC true" : "";

		click_conf << "fromdev" << j << "::FromDevice("
			<< interfaces[j]->name << promisc << ", METHOD LINUX";
		click_conf << ");" << endl << "todev" << j << "::ToDevice("
			<< interfaces[j]->name << ");" << endl;
	}

	click_conf << endl << endl << "proxy[0]->tonetlink;" << endl
			<< "fromnetlink->[0]proxy;" << endl
			<< "localRV[0]->[1]proxy[1]->[0]localRV;" << endl
			<< "proxy[2]-> [0]fw[0] -> [2]proxy;" << endl;

	for (size_t j = 0; j < interfaces.size(); j++)
	{
		string classifier = (interfaces[j]->sdn_enabled) ? "12/86dd" : "12/080a";
		click_conf << endl << "classifier" << j << "::Classifier(" << classifier << ");"
				<< endl;
		click_conf << "fw[" << (j + 1) << "]->tsf" << j << "->todev" << j << ";"
				<< endl;
		click_conf << "fromdev" << j << "->classifier" << j << "[0]->["
				<< (j + 1) << "]fw;" << endl;
	}

	click_conf.close();
	write_TMFID.close();
}

void configuration::start_click() {
	FILE *ssh_command;
	string command = "sudo " + click_home + "bin/click " + write_conf + label
			+ ".conf > " + write_conf + "ba_log 2>&1 &";
	cout << command << endl;
	ssh_command = popen(command.c_str(), "r");
	if (ssh_command == NULL)
	{
		cerr << "Failed to start click. " << endl;
	}
	pclose(ssh_command);
	return;
}

void configuration::stop_click() {
	FILE *ssh_command;
	string command= "sudo pkill -9 click";
    cout << command << endl;
    ssh_command = popen(command.c_str(), "r");
    if (ssh_command == NULL)
    {
        cerr << "Failed to stop click." << endl;
    }
    pclose(ssh_command);
}


void configuration::update_configuration(string new_label, string new_rv_fid,
		string new_tm_fid, string new_i_lid) {
	if (!new_label.empty())
		label = new_label;
	if (!new_rv_fid.empty())
		rv_fid = new_rv_fid;
	if (!new_tm_fid.empty())
		tm_fid = new_tm_fid;
	if (!new_i_lid.empty())
		internal_lid = new_i_lid;

	write_click_file(interfaces);
	sleep(1);
}

