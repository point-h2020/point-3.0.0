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

#include <iostream>
#include <cstdlib>
#include <boost/asio.hpp>
#include "network.hpp"

#ifndef DEPLOYMENT_SERVER_HPP
#define	DEPLOYMENT_SERVER_HPP

using boost::asio::ip::tcp;

/**@brief The external domain object.
 *
 * It is initialized upon first deployment and is updated by the dynamic deployment tool.
 *
 */
extern Domain dm;

/**@brief The session class which handles the sessions.
 *
 */
class session {
public:
	/**@brief The constructor.
	 *
	 * @param io_service The IO service to be used in all sessions.
	 *
	 */
	session(boost::asio::io_service& io_service);
	/**@brief It returns the created socket.
	 *
	 * @return The socket.
	 */
	tcp::socket& socket();
	/**@brief The method which starts a session.
	 *
	 */
	void start();

private:
	/**@brief The method which reads the received bytes.
	 *
	 * It's actually the method in which all the dynamic deployment tool logic resides.
	 * It calls the appropriate methods to merge the new node to the existing topology,
	 * generate the TM/RV IDs, the click files for the attached nodes, the new graphml
	 * topology file, and finally send these files to the respective nodes.
	 *
	 * @param error The error code.
	 * @param bytes_transferred The received bytes.
	 *
	 */
	void handle_read(const boost::system::error_code& error,
			size_t bytes_transferred);
	/**@brief The method which writes back to the client.
	 *
	 * Currently, it's only a placeholder.
	 *
	 * @param error The error code.
	 *
	 */
	void handle_write(const boost::system::error_code& error);
	/**@brief The created socket.
	 *
	 */
	tcp::socket socket_;
	/**@brief The maximum data length.
	 *
	 */
	enum {
		max_length = 10240
	};
	/**@brief The received data.
	 *
	 */
	char data_[max_length];
};

/**@brief The server class which initializes and accepts new deployment requests.
 * 
 */
class server {
public:
	/**@brief The constructor.
	 *
	 *@param io_service The IO service to be used.
	 *@param port The listening port.
	 */
	server(boost::asio::io_service& io_service, short port);
	/**@brief The method which initializes the tcp server.
	 *
	 */
	void init();

private:
	/**@brief The method which creates a socket and waits for new connections.
	 *
	 */
	void start_accept();
	/**@brief The method which handles a new client request.
	 *
	 * @param new_session The pointer to the new session.
	 * @param error The error code.
	 *
	 */
	void handle_accept(session* new_session,
			const boost::system::error_code& error);

	/**@brief A boost::asio::io_service object.
	 *
	 * It provides IO services, e.g. sockets, to the server.
	 *
	 */
	boost::asio::io_service& io_service_;
	/**@brief A tcp::acceptor object.
	 *
	 * It is used for accepting new sockets.
	 *
	 */
	tcp::acceptor acceptor_;
	/**@brief The listening port.
	 *
	 */
	short port_;
};

#endif	/* DEPLOYMENT_SERVER_HPP */
