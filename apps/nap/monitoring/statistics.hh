/*
 * statistics.hh
 *
 *  Created on: 2 Sep 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of Blackadder.
 *
 * Blackadder is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
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

#ifndef NAP_MONITORING_STATISTICS_HH_
#define NAP_MONITORING_STATISTICS_HH_

#include <forward_list>
#include <log4cxx/logger.h>
#include <mutex>
#include <unordered_map>

#include <monitoring/monitoringtypedefs.hh>
#include <types/icnid.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace log4cxx;

namespace monitoring {

namespace statistics {

class Statistics {
	static LoggerPtr logger;
public:
	Statistics();
	/*!
	 * \brief Calculate the average CMC group size over the configured reporting
	 * interval
	 *
	 * Note, the average multicast group size is using a base of 0.1 by which it
	 * fits into an unsigned integer and results in a rounded average to one
	 * decimal point (flooring the value though).
	 *
	 * \return The average CMC group size of this NAP as a fitted value
	 */
	uint32_t averageCmcGroupSize();
	/*!
	 * \brief Calculate the average network delay based on the RTT values
	 * measured as reported by LTP
	 *
	 * This method also resets the pair in which the RTT is stored
	 * (_roundTripTimes)
	 *
	 * Note, the average network delay is using a base of 0.1 by which it fits
	 * into an unsigned integer and results in a rounded average to one decimal
	 * point (flooring the value though).
	 *
	 * \return The network delay in milliseconds per FQDN as a fitted value
	 */
	unordered_map<uint32_t, uint16_t> averageNetworkDelayPerFqdn();
	/*!
	 * \brief Obtain the buffer size of the HTTP handler (requests)
	 *
	 * \return The buffer size in number of packets
	 */
	uint32_t bufferSizeHttpHandlerRequests();
	/*!
	 * \brief Update the buffer size for the HTTP handler (requests)
	 *
	 * \param bufferSize The buffer size in number of packets
	 */
	void bufferSizeHttpHandlerRequests(uint32_t bufferSize);
	/*!
	 * \brief Obtain the buffer size of the HTTP handler (responses)
	 *
	 * \return The buffer size in number of packets
	 */
	uint32_t bufferSizeHttpHandlerResponses();
	/*!
	 * \brief Update the buffer size for the HTTP handler (responses)
	 *
	 * \param bufferSize The buffer size in number of packets
	 */
	void bufferSizeHttpHandlerResponses(uint32_t bufferSize);
	/*!
	 * \brief Update the buffer size for the IP handler
	 *
	 * \param bufferSize The buffer size in number of packets
	 */
	void bufferSizeIpHandler(uint32_t bufferSize);
	/*!
	 * \brief Obtain the buffer size of the IP handler
	 *
	 * \return The buffer size in number of packets
	 */
	uint32_t bufferSizeIpHandler();
	/*!
	 * \brief Obtain the buffer size of the LTP packet buffer
	 *
	 * The packets the NAP stores in _ltpPacketBuffer
	 *
	 * \return The buffer size in number of packets
	 */
	uint32_t bufferSizeLtp();
	/*!
	 * \brief Update the buffer size for the LTP packet buffer
	 *
	 * The packets the NAP stores in _ltpPacketBuffer
	 *
	 * \param bufferSize The buffer sizer in number of packets
	 */
	void bufferSizeLtp(uint32_t bufferSize);
	/*!
	 * \brief Update the CMC group size counter
	 *
	 * \param cmcGroupSize The CMC group size which should be recorded
	 */
	void cmcGroupSize(uint32_t cmcGroupSize);
	/*!
	 * \brief Obtain the number of HTTP request and their FQDN which have
	 * traversed the NAP
	 *
	 * \return A map with the values (key: fqdn, value: # of requests)
	 */
	http_requests_per_fqdn_t httpRequestsPerFqdn();
	/*!
	 * \brief Update the number of HTTP request for a particular hashed FQDN
	 *
	 * \param fqdn The FQDN for which the number of requests is
	 * reported
	 * \param numberOfRequests The number of HTTP requests
	 */
	void httpRequestsPerFqdn(string fqdn, uint32_t numberOfRequests);
	/*!
	 * \brief Add an IP endpoint to the list of known endpoints
	 *
	 * When using this method the endpoint reported is going to be an UE
	 *
	 * \param ipAddress The IP address of the endpoint
	 */
	void ipEndpointAdd(IpAddress ipAddress);
	/*!
	 * \brief Add a surrogate server to the list of known endpoints
	 *
	 * When using this method the endpoint reported is going to be a surrogate
	 * server
	 *
	 * \param ipAddress IP address of the server
	 * \param port The port to which HTTP requests will be sent
	 */
	void ipEndpointAdd(IpAddress ipAddress, uint16_t port);
	/*!
	 * \brief Add a server to the list of known endpoints
	 *
	 * When using this method the endpoint reported is going to be a server
	 *
	 * \param ipAddress IP address of the server
	 * \param fqdn The FQDN this endpoint serves
	 * \param port The port to which HTTP requests will be sent
	 */
	void ipEndpointAdd(IpAddress ipAddress, string fqdn, uint16_t port);
	/*!
	 * \brief Obtain a full copy of the list of IP endpoints
	 *
	 * \return The list of IP endpoints
	 */
	ip_endpoints_t ipEndpoints();
	/*!
	 * \brief Update the RTT counter for a particular FQDN
	 *
	 * \param cid The CID which holds the FQDN
	 * \param rtt The RTT which should be recorded
	 */
	void roundTripTime(IcnId cid, uint16_t rtt);
	/*!
	 * \brief Obtain the number of received HTTP bytes and reset the bytes
	 * counter
	 *
	 * \return The numbert of received HTTP bytes
	 */
	uint32_t rxHttpBytes();
	/*!
	 * \brief Update the RX_HTTP byte counter
	 *
	 * \param rxBytes The number of transmitted bytes
	 */
	void rxHttpBytes(int *txBytes);
	/*!
	 * \brief Obtain the number of received IP bytes and reset the bytes
	 * counter
	 *
	 * \return The numbert of received IP bytes
	 */
	uint32_t rxIpBytes();
	/*!
	 * \brief Update the RX_IP byte counter
	 *
	 * \param rxBytes The number of transmitted bytes
	 */
	void rxIpBytes(uint16_t *rxBytes);
	/*!
	 * \brief Update the TCP socket count
	 *
	 * \param sockets The change in the socket count
	 */
	void tcpSocket(int sockets);
	/*!
	 * \brief Obtain the amount of active TCP sockets towards IP endpoints
	 * and reset the internal value
	 *
	 * \return The number of active TCP sockets at the time of calling this
	 * method
	 */
	uint16_t tcpSockets();
	/*!
	 * \brief Obtain the number of transmitted HTTP bytes and reset the bytes
	 * counter
	 *
	 * \return The numbert of transmitted HTTP bytes
	 */
	uint32_t txHttpBytes();
	/*!
	 * \brief Update the TX_HTTP byte counter
	 *
	 * \param txBytes The number of transmitted bytes
	 */
	void txHttpBytes(int *txBytes);
	/*!
	 * \brief Obtain the number of transmitted IP bytes and reset the bytes
	 * counter
	 *
	 * \return The numbert of transmitted IP bytes
	 */
	uint32_t txIpBytes();
	/*!
	 * \brief Update the TX_IP byte counter
	 *
	 * \param txBytes The number of transmitted bytes
	 */
	void txIpBytes(int *txBytes);
private:
	std::mutex _mutex;
	pair<uint32_t, uint32_t> _bufferSizeHttpHandlerRequests;/*!< pair<sum of
	buffer sizes, number of measurements> */
	pair<uint32_t, uint32_t> _bufferSizeHttpHandlerResponses;/*!< pair<sum of
	buffer sizes, number of measurements> */
	pair<uint32_t, uint32_t> _bufferSizeIpHandler;/*!< pair<sum of buffer sizes,
	number of measurements> */
	pair<uint32_t, uint32_t> _bufferSizeLtp;/*!< pair<sum of buffer sizes,
	number of measurements> */
	pair<uint32_t, uint32_t> _cmcGroupSizes;/*!< pair<number of CMC reports, sum
	of all CMC group sizes */
	unordered_map<uint32_t, forward_list<uint16_t>> _rttPerFqdn;
	/*!< u_map<hashed FQDN, forward_list<RTT>> The list size is configurable via
	ltpRttListSize in nap.cfg */
	unordered_map<uint32_t, forward_list<uint16_t>>::iterator _rttPerFqdnIt;/*!<
	iterator for _rttPerFqdn*/
	http_requests_per_fqdn_t _httpRequestsPerFqdn; /*!< pair<fqdn, number of HTTP
	requests> */
	ip_endpoints_t _ipEndpoints;/*!< List if IP	endpoints with their IP
	addresses as the key */
	uint32_t _rxHttpBytes;/*!< Counting the number of bytes received at the HTTP
	handler from IP endpoints */
	uint32_t _rxIpBytes;/*!< Counting the number of bytes received at the IP
	handler from IP endpoints */
	uint16_t _tcpSockets;/*!< Counting the number of active TCP sockets*/
	uint32_t _txHttpBytes;/*!< Counting the number of bytes transmitted in the
	HTTP handler from IP endpoints */
	uint32_t _txIpBytes;/*!< Counting the number of bytes transmitted in the IP
	handler from IP endpoints */
};

} /* namespace statistics */

} /* namespace monitoring */

#endif /* NAP_MONITORING_STATISTICS_HH_ */
