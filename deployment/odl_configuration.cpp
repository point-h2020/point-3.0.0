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

#include "odl_configuration.hpp"
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <sstream>

using boost::asio::ip::tcp;
using namespace std;

OdlConfiguration::OdlConfiguration() {
	// TODO Auto-generated constructor stub

}

OdlConfiguration::~OdlConfiguration() {
	// TODO Auto-generated destructor stub
}

OdlConfiguration::OdlConfiguration(string odl) {
	odl_ip_address = odl;
}

void OdlConfiguration::configureFlow(string openflow_id, int table, string flow,
		int flow_id) {

	// Prepare the boost asio connection parameters
	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(odl_ip_address, "8181");
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	ostringstream oss;
	oss << "/restconf/config/opendaylight-inventory:nodes/node/"
			<< openflow_id << "/table/" << table << "/flow/" << flow_id << "/";
	string path = oss.str();

	// Try each endpoint until we successfully establish a connection.
	tcp::socket socket(io_service);
	boost::asio::connect(socket, endpoint_iterator);

	boost::asio::streambuf request;
	std::ostream request_stream(&request);
	// Prepare the HTTP request parameters
	request_stream << "PUT " << path << " HTTP/1.1\r\n";
	request_stream << "Host:http://" << odl_ip_address << ":8181\r\n";
	request_stream << "Authorization:Basic YWRtaW46YWRtaW4=\r\n";
	request_stream << "Content-Type:application/json\r\n";
	request_stream << "Accept:application/json\r\n";
	request_stream << "Content-Length:" << flow.length() << "\r\n";
	request_stream << "Connection:close\r\n\r\n";
	request_stream << flow;

	boost::asio::write(socket, request);

	// Read the response status line.
	boost::asio::streambuf response;
	boost::asio::read_until(socket, response, "\r\n");

	std::cout << "Got HTTP response from ODL controller for flow configuration." << std::endl;
	// Check that response is OK.
	std::istream response_stream(&response);
	std::string http_version;
	response_stream >> http_version;
	unsigned int status_code;
	response_stream >> status_code;
	std::string status_message;
	std::getline(response_stream, status_message);
	if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
		std::cout << "Invalid response" << std::endl;
		return;
	}
	if (status_code != 200) {
		std::cout << "Response with status code " << status_code  << std::endl;
		return;
	}

}

void OdlConfiguration::configureGroup(string openflow_id, string group,
		int group_id) {

	// Prepare the boost asio connection parameters
	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(odl_ip_address, "8181");
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	ostringstream oss;
	oss << "/restconf/config/opendaylight-inventory:nodes/node/"
			<< openflow_id << "/group/" << group_id << "/";
	string path = oss.str();

	// Try each endpoint until we successfully establish a connection.
	tcp::socket socket(io_service);
	boost::asio::connect(socket, endpoint_iterator);

	boost::asio::streambuf request;
	std::ostream request_stream(&request);
	// Prepare the HTTP request parameters
	request_stream << "PUT " << path << " HTTP/1.1\r\n";
	request_stream << "Host:http://" << odl_ip_address << ":8181\r\n";
	request_stream << "Authorization:Basic YWRtaW46YWRtaW4=\r\n";
	request_stream << "Content-Type:application/json\r\n";
	request_stream << "Accept:application/json\r\n";
	request_stream << "Content-Length:" << group.length() << "\r\n";
	request_stream << "Connection:close\r\n\r\n";
	request_stream << group;

	boost::asio::write(socket, request);

	// Read the response status line.
	boost::asio::streambuf response;
	boost::asio::read_until(socket, response, "\r\n");

	std::cout << "Got HTTP response from ODL controller for group configuration." << std::endl;
	// Check that response is OK.
	std::istream response_stream(&response);
	std::string http_version;
	response_stream >> http_version;
	unsigned int status_code;
	response_stream >> status_code;
	std::string status_message;
	std::getline(response_stream, status_message);
	if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
		std::cout << "Invalid response" << std::endl;
		return;
	}
	if (status_code != 200) {
		std::cout << "Response with status code " << status_code  << std::endl;
		return;
	}

}

void OdlConfiguration::configureNodeConnectorRegistry(string connector_registry_entry) {
	// Prepare the boost asio connection parameters
		boost::asio::io_service io_service;
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(odl_ip_address, "8181");
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		ostringstream oss;
		oss << "/restconf/operations/bootstrapping:configureNodeConnectorRegistry";
		string path = oss.str();

		// Try each endpoint until we successfully establish a connection.
		tcp::socket socket(io_service);
		boost::asio::connect(socket, endpoint_iterator);

		boost::asio::streambuf request;
		std::ostream request_stream(&request);
		// Prepare the HTTP request parameters
		request_stream << "POST " << path << " HTTP/1.1\r\n";
		request_stream << "Host:http://" << odl_ip_address << ":8181\r\n";
		request_stream << "Authorization:Basic YWRtaW46YWRtaW4=\r\n";
		request_stream << "Content-Type:application/json\r\n";
		request_stream << "Accept:application/json\r\n";
		request_stream << "Content-Length:" << connector_registry_entry.length() << "\r\n";
		request_stream << "Connection:close\r\n\r\n";
		request_stream << connector_registry_entry;

		boost::asio::write(socket, request);

		// Read the response status line.
		boost::asio::streambuf response;
		boost::asio::read_until(socket, response, "\r\n");

		std::cout << "Got HTTP response from ODL controller for node connector "
				"registry configuration." << std::endl;
		// Check that response is OK.
		std::istream response_stream(&response);
		std::string http_version;
		response_stream >> http_version;
		unsigned int status_code;
		response_stream >> status_code;
		std::string status_message;
		std::getline(response_stream, status_message);
		if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
			std::cout << "Invalid response" << std::endl;
			return;
		}
		if (status_code != 200) {
			std::cout << "Response with status code " << status_code  << std::endl;
			return;
		}
}

void OdlConfiguration::configureNodeRegistry(string node_registry_entry) {
	// Prepare the boost asio connection parameters
		boost::asio::io_service io_service;
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(odl_ip_address, "8181");
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		ostringstream oss;
		oss << "/restconf/operations/bootstrapping:configureNodeRegistry";
		string path = oss.str();

		// Try each endpoint until we successfully establish a connection.
		tcp::socket socket(io_service);
		boost::asio::connect(socket, endpoint_iterator);

		boost::asio::streambuf request;
		std::ostream request_stream(&request);
		// Prepare the HTTP request parameters
		request_stream << "POST " << path << " HTTP/1.1\r\n";
		request_stream << "Host:http://" << odl_ip_address << ":8181\r\n";
		request_stream << "Authorization:Basic YWRtaW46YWRtaW4=\r\n";
		request_stream << "Content-Type:application/json\r\n";
		request_stream << "Accept:application/json\r\n";
		request_stream << "Content-Length:" << node_registry_entry.length() << "\r\n";
		request_stream << "Connection:close\r\n\r\n";
		request_stream << node_registry_entry;

		boost::asio::write(socket, request);

		// Read the response status line.
		boost::asio::streambuf response;
		boost::asio::read_until(socket, response, "\r\n");

		std::cout << "Got HTTP response from ODL controller for node "
				"registry configuration." << std::endl;
		// Check that response is OK.
		std::istream response_stream(&response);
		std::string http_version;
		response_stream >> http_version;
		unsigned int status_code;
		response_stream >> status_code;
		std::string status_message;
		std::getline(response_stream, status_message);
		if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
			std::cout << "Invalid response" << std::endl;
			return;
		}
		if (status_code != 200) {
			std::cout << "Response with status code " << status_code  << std::endl;
			return;
		}
}
