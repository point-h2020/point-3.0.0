/*
 * httprequestsfqdn.hh
 *
 *  Created on: 2 Jun 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the MOnitoring LibrarY (MOLY).
 *
 * MOLY is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOLY is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOLY. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MOLY_PRIMITIVES_HTTPREQUESTSFQDN_HH_
#define MOLY_PRIMITIVES_HTTPREQUESTSFQDN_HH_

#include "../typedef.hh"

/*!
 * \brief Implementation of the HTTP_REQUESTS_FQDN primitive
 */
class HttpRequestsFqdn {
public:
	/*!
	 * \brief Constructor for applications
	 */
	HttpRequestsFqdn(string fqdn, uint32_t httpRequests);
	/*!
	 * \brief Constructor for MAs
	 */
	HttpRequestsFqdn(uint8_t *pointer, uint32_t size);
	/*!
	 * \brief Destructor
	 */
	~HttpRequestsFqdn();
	/*!
	 * \brief Obtain the FQDN for which the number of HTTP requests are reported
	 *
	 * \return String of the FQDN
	 */
	string fqdn();
	/*!
	 * \brief Obtain the reported number of HTTP requests
	 *
	 * \return Integer representation of the number
	 */
	uint32_t httpRequests();
	/*!
	 * \brief Pointer to the packet
	 *
	 * \return Pointer to packet pointer
	 */
	uint8_t * pointer();
	/*!
	 * \brief Print out the content of the UPDATE_CMC_GROUP_SIZE primitive
	 *
	 * The print out order is determined by the MOLY specification
	 *
	 * \return The primitive's content in human readable format
	 */
	string print();
	/*!
	 * \brief Size
	 *
	 * \return Return the size of the pointer returned by
	 * HttpRequestsFqdn::pointer()
	 */
	uint32_t size();
private:
	string _fqdn; /*!< The FQDN for which the  number of requests are reported*/
	uint32_t _httpRequests;/*!< The number of HTTP requests over a pre-defined
	time interval*/
	uint8_t * _pointer; /*!< Pointer to the generated packet */
	uint32_t _size; /*!< Size of the generated packet */
	/*!
	 * \brief Compose packet
	 *
	 * This method composes the packet according to the specification.
	 */
	void _composePacket();
	/*!
	 * \brief Decompose packet
	 *
	 * This method decomposes the packet according to the specification.
	 */
	void _decomposePacket();
};

#endif /* MOLY_PRIMITIVES_HTTPREQUESTSFQDN_HH_ */
