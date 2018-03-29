/*
 * Copyright (C) 2015-2016  George Petropoulos
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 3 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * See LICENSE and COPYING for more details.
 */

#include "deployment_server.hpp"
#include "network.hpp"
#include "parser.hpp"
#include "graph_representation.hpp"
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

using boost::asio::ip::tcp;

Domain dm;

server::server(boost::asio::io_service& io_service, short port):
    						io_service_(io_service),
							acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
{
	port_ = port;
	start_accept();
}

void server::start_accept()
{
	session* new_session = new session(io_service_);
	acceptor_.async_accept(new_session->socket(),
			boost::bind(&server::handle_accept, this, new_session,
					boost::asio::placeholders::error));
}

void server::handle_accept(session* new_session, 
		const boost::system::error_code& error)
{
	if (!error)
		new_session->start();
	else
		delete new_session;

	start_accept();
}

void server::init()
{
	std::cout << "Starting deployment server at port " << port_ << "." << std::endl;
	io_service_.run();
}

session::session(boost::asio::io_service& io_service): socket_(io_service)
{  
}

tcp::socket& session::socket()
{
	return socket_;
}

void session::start()
{
	socket_.async_read_some(boost::asio::buffer(data_, max_length),
			boost::bind(&session::handle_read, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}

void session::handle_read(const boost::system::error_code& error,
		size_t bytes_transferred)
{
	if (!error)
	{
		std::cout << "Received deployment request. " << std::endl;
        /*pass an empty extension to the TM*/
        string extension = "";
		/**create random file with received request
		 */
		string file_name = "request"
				+ boost::lexical_cast<string>(rand() % 10000 + 1) + ".req";
		std::ofstream ofs(file_name.c_str());
		ofs.write( (char*)data_, bytes_transferred );
		ofs.close();

		/**parse request to network node and network connections
		 */
		Domain new_dm = Domain();
		Parser parser((char*)file_name.c_str(), &new_dm);
		int parsing_result = parser.buildNetworkDomain();

		if (parsing_result < 0) {
			std::cerr << "Error while parsing received deployment request." << std::endl;
			std::remove((char*)file_name.c_str());
			return;
		}
		/**update domain and assign LIDs to new nodes
		 */
		MergingResult merging_result = dm.mergeDomains(new_dm);

		/**update graph
		 */
		GraphRepresentation graph = GraphRepresentation(&dm, false);
		graph.buildIGraphTopology();

		/**calculate RV/TM IDs
		 */
		graph.calculateRVFIDs();
		graph.calculateTMFIDs();

		switch(merging_result) {
		case NODE_ADDED:
		{
			std::cout << "Node successfully added." << std::endl;
			/**discover mac addresses for new and attachment nodes
			 */
			new_dm.discoverMacAddresses(false);

			/**prepare click files for new and attachment nodes
			 */
			new_dm.writeClickFiles(false, false, false);

			/**send them to new node and attachment nodes
			 */
			new_dm.scpClickFiles();

			/**start click in new and attachment nodes
			 */
			new_dm.startClick(false, false);
			break;
		}
		case NODE_DELETED:
		{
			std::cout << "Node successfully deleted." << std::endl;
			/**discover mac addresses for all nodes
			 */
			dm.discoverMacAddresses(false);

			/**prepare click files for all nodes
			 */
			dm.writeClickFiles(false, false, false);

			/**send them to all nodes
			 */
			dm.scpClickFiles();

			/**start click in all nodes
			 */
			dm.startClick(false, false);
			break;
		}
		case ERROR:
		{
			std::cout << "Error while processing deployment request." << std::endl;
			std::remove((char*)file_name.c_str());
			return;
		}
		}

		/**prepare new graphml file
		 */
		igraph_cattribute_GAN_set(&graph.igraph, "FID_LEN", dm.fid_len);
		igraph_cattribute_GAS_set(&graph.igraph, "TM", dm.TM_node->label.c_str());
		igraph_cattribute_GAS_set(&graph.igraph, "RV", dm.RV_node->label.c_str());
		igraph_cattribute_GAS_set(&graph.igraph, "TM_MODE", dm.TM_node->running_mode.c_str());
		FILE * outstream_graphml = fopen(string(dm.write_conf + "topology.graphml").c_str(), "w");
#if IGRAPH_V >= IGRAPH_V_0_7
		igraph_write_graph_graphml(&graph.igraph, outstream_graphml,true);
#else
		igraph_write_graph_graphml(&graph.igraph, outstream_graphml);
#endif
		fclose(outstream_graphml);

		/**send the graphml file to TM and restart (assuming it's already running)
		 */
		dm.scpTMConfiguration("topology.graphml");
		dm.startTM(false, extension);

		/**delete request file
		 */
		std::remove((char*)file_name.c_str());

	}
	else
		delete this;
}

void session::handle_write(const boost::system::error_code& error)
{
	if (!error)
	{
		socket_.async_read_some(boost::asio::buffer(data_, max_length),
				boost::bind(&session::handle_read, this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
	}
	else
		delete this;
}
