/*
 * headerparser.hh
 *
 *  Created on: 1 Feb 2017
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of Blackadder.
 *
 * Blackadder is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Blackadder is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Blackadder.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NAP_PROXIES_HTTP_HEADERPARSER_HH_
#define NAP_PROXIES_HTTP_HEADERPARSER_HH_

#include <algorithm>
#include <log4cxx/logger.h>
#include <string.h>

#include <types/enumerations.hh>

using namespace log4cxx;
using namespace std;

namespace proxies
{
namespace http
{
namespace headerparser
{
/*!
 * \brief Implementation of all methods to parse HTTP headers
 */
class HeaderParser {
	static LoggerPtr logger;
public:
protected:
	/*!
	 * \brief Constructor
	 */
	HeaderParser();
	/*!
	 * \brief Destructor
	 */
	~HeaderParser();
	/*!
	 * \brief Parse the packet if this is an HTTP request for an MPEG DASH MPD
	 *
	 * \param packet Pointer to the packet
	 * \param packetSize The size of the packet
	 *
	 * \return Boolean indicating whether or not this HTTP request asks for an
	 * MPD resource
	 */
	bool mpdRequest(uint8_t *packet, uint16_t &packetSize);
	/*!
	 * \brief Parse the packet if this is an HTTP response for an MPEG DASH MPD
	 *
	 * \param packet Pointer to the packet
	 * \param packetSize The size of the packet
	 *
	 * \return Boolean indicating whether or not this HTTP request asks for an
	 * MPD resource
	 */
	bool mpdResponse(uint8_t *packet, uint16_t &packetSize);
	/*!
	 * \brief Wrapper for string readFqdn(char *, uint16_t &);
	 *
	 * \param packet Pointer to the HTTP packet
	 * \param packetSize Length of the HTTP packet
	 *
	 * \return FQDN of the HTTP packet provided
	 */
	string readFqdn(uint8_t *packet, uint16_t &packetSize);
	/*!
	 * \brief Obtain the FQDN from an HTTP header
	 *
	 * This method parses the HTTP message until \r\r appears and extracts the
	 * FQDN provided after 'Host: '
	 *
	 * \param packet Pointer to the HTTP packet
	 * \param packetSize Length of the HTTP packet
	 *
	 * \return FQDN of the HTTP packet provided
	 */
	string readFqdn(char *packet, uint16_t &packetSize);
	/*!
	 * \brief Wrapper for readHttpRequestMethod(char *, uint16_t &)
	 *
	 * \param packet Pointer to the HTTP request packet
	 * \param packetSize Reference to the size of the packet
	 *
	 * \return This method returns the HTTP request method using the enumeration
	 * HttpMethods
	 */
	http_methods_t readHttpRequestMethod(uint8_t *packet, uint16_t &packetSize);
	/*!
	 * \brief Obtain HTTP method from an HTTP request message
	 *
	 * \param packet Pointer to the HTTP request packet
	 * \param packetSize Reference to the size of the packet
	 *
	 * \return This method returns the HTTP request method using the enumeration
	 * HttpMethods
	 */
	http_methods_t readHttpRequestMethod(char *packet, uint16_t &packetSize);
	/*!
	 * \brief Wrapper for readHttpResponseMethod(char *, uint16_t &)
	 *
	 * \param packet Pointer to the HTTP request packet
	 * \param packetSize Reference to the size of the packet
	 *
	 * \return This method returns the HTTP request method using the enumeration
	 * HttpMethods
	 */
	http_methods_t readHttpResponseMethod(uint8_t *packet, uint16_t &packetSize);
	/*!
	 * \brief Obtain HTTP method from an HTTP response message
	 *
	 * \param packet Pointer to the HTTP request packet
	 * \param packetSize Reference to the size of the packet
	 *
	 * \return This method returns the HTTP request method using the enumeration
	 * HttpMethods
	 */
	http_methods_t readHttpResponseMethod(char *packet, uint16_t &packetSize);
	/*!
	 * \brief Wrapper for readResource(char *, uint16_t &)
	 *
	 * \param packet Pointer to the HTTP packet
	 * \param packetSize Length of the HTTP packet
	 *
	 * \return Resource of the HTTP packet provided
	 */
	string readResource(uint8_t *packet, uint16_t &packetSize);
	/*!
	 * \brief Obtain the resource from an HTTP header
	 *
	 * This method parses the HTTP message until \r\r appears and extracts the
	 * resource provided in the header
	 *
	 * \param packet Pointer to the HTTP packet
	 * \param packetSize Length of the HTTP packet
	 *
	 * \return Resource of the HTTP packet provided
	 */
	string readResource(char *packet, uint16_t &packetSize);
};

} /* namespace headerparser */

} /* namespace http */

} /* namespace proxies */
#endif /* NAP_PROXIES_HTTP_HEADERPARSER_HH_ */
