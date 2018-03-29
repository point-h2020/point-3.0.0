/*
 * mysqlconnector.hh
 *
 *  Created on: 13 Feb 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the ICN application MOnitOring SErver (MOOSE) which
 * comes with Blackadder.
 *
 * MOOSE is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOOSE is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOOSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef APPS_MONITORING_SERVER_MYSQLCONNECTOR_HH_
#define APPS_MONITORING_SERVER_MYSQLCONNECTOR_HH_

#include <arpa/inet.h>
#include <cppconn/driver.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <log4cxx/logger.h>
#include <moly/molyhelper.hh>
#include <moly/typedef.hh>
#include <string>

#include "../types/elementtype.hh"
#include "../messagestack/messagestack.hh"
#include "mysql_connection.h"

using namespace std;
/*!
 * \brief MySQL connector class to read the message stack
 */
class MySqlConnector: public MolyHelper
{
public:
	/*!
	 * \brief Constructor
	 */
	MySqlConnector(log4cxx::LoggerPtr logger, string serverIpAddress,
			string serverPort, string username, string password,
			string database, MessageStack &messageStack);
	/*!
	 * \brief Destructor
	 */
	~MySqlConnector();
	/*!
	 * \brief Functor to put this MySQL client into a thread
	 */
	void operator()();
private:
	log4cxx::LoggerPtr _logger; /*!< Logger instance */
	string _serverIpAddress; /*!< IP address of the visualisation server */
	string _serverPort; /*!< PORT on which the visualisation server listens */
	string _username; /*!< username to log in to visualisation server */
	string _password; /*!< password to log in to visualisation server */
	string _database; /*!< Database name of visulisation server */
	MessageStack &_messageStack; /*!< Reference to message stack*/
	sql::Driver *_sqlDriver; /*!< Pointer to SQL driver */
	sql::Connection *_sqlConnection; /*!< Pointer to SQL connection */
	/*!
	 * \brief Send data received through ICN to visualisation server
	 *
	 * \param icnId The ICN ID under which the data has been received
	 * \param data Pointer to the ICN payload field (Event *ev->data)
	 * \param dataLength Length of the ICN data received (Event *ev->data_len)
	 */
	void _send(IcnId &icnId, uint8_t *data, uint32_t dataLength);
	/*!
	 * \brief Obtain state as string (any state!)
	 *
	 * The states that can be used are defined in <moly/enum.hh>, enum States{}
	 *
	 * \param state The state which should be converted into a string
	 *
	 * \return String representation of the state enum
	 */
	string _stateStr(primitive_type_t primitive, uint8_t state);
};

#endif /* APPS_MONITORING_SERVER_MYSQLCONNECTOR_HH_ */
