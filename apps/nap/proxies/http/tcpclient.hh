/*
 * tcpclient.hh
 *
 *  Created on: 28 Jul 2016
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

#ifndef NAP_PROXIES_HTTP_TCPCLIENT_HH_
#define NAP_PROXIES_HTTP_TCPCLIENT_HH_

#include <arpa/inet.h>
#include <list>
#include <log4cxx/logger.h>
#include <map>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include <configuration.hh>
#include <namespaces/namespaces.hh>
#include <proxies/http/dnsresolutions.hh>
#include <proxies/http/headerparser.hh>
#include <proxies/http/httpproxytypedef.hh>
#include <proxies/http/tcpclienthelper.hh>
#include <proxies/http/tcpclientread.hh>
#include <monitoring/statistics.hh>
#include <types/ipaddress.hh>
#include <types/nodeid.hh>
#include <types/typedef.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace log4cxx;
using namespace std;
using namespace proxies::http::dnsresolutions;
using namespace proxies::http::headerparser;

namespace proxies {

namespace http {

namespace tcpclient {
/*
 * \brief
 */
class TcpClient: public HeaderParser, DnsResolutions, TcpClientHelper
{
	static LoggerPtr logger;
public:
	/*
	 * \brief Constructor.
	 *
	 * \param configuration Reference to the class Configuration
	 * \param namespaces Reference to the class Namespaces
	 * \param statistics Reference to statistics class
	 */
	TcpClient(Configuration &configuration, Namespaces &namespaces,
			Statistics &statistics, reverse_lookup_t *reverseLookup,
			socket_fds_t *socketFds, socket_state_t *socketState,
			mutex &tcpClientMutex, bool *run);
	/*
	 * \brief Destructor
	 */
	~TcpClient();
	/*!
	 * \brief Functor
	 *
	 * Only used if new TCP client session is required. If so, the sendData
	 * method returned true to indicate that a new TCP client session is
	 * required.
	 *
	 * This method will always create a new TCP socket towards the registered
	 * IP service endpoint.
	 *
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 */
	void operator()(IcnId cid, IcnId rCid, enigma_t enigma,
			sk_t remoteSocketFd, string nodeIdStr, uint8_t *packet,
			uint16_t packetSize);
private:
	Configuration &_configuration;/*!< Reference to Configuration class */
	Namespaces &_namespaces;/*!< Reference to Namespaces class */
	Statistics &_statistics;/*!< Reference to Statistics class */
	reverse_lookup_t *_reverseLookup;/*!< get
	rCID and Enigma for a particular file descriptor to easier delete it
	eventually from _openedTcpSessions if required. */
	reverse_lookup_t::iterator _reverseLookupIt;/*!< Iterator for
	_reverseLookup map */
	socket_fds_t *_socketFds;/*!<
	map<NID, map<remote SK, local SK*/
	socket_fds_t::iterator _socketFdsIt;/*!< Iterator for _socketFdMap */
	socket_state_t *_socketState;/*!< u_map<SFD, writeable> make
	socket closure states available to all threads*/
	socket_state_t::iterator _socketStateIt;/*!< Iterator for
	_socketState map*/
	std::mutex &_mutex; /*!< Mutex for all shared maps operations*/
	IcnId _wildcardCid;/*!< The CID for HTTP wildcards*/
	bool *_run;
	/*!
	 * \brief Add rCID and Enigma to reverse look-up table based on FD
	 *
	 * \param socketFd The socket FD which should be added as a key to the
	 * reverse look-up map
	 * \param rCId The rCID which should be added as a key to the look-up map
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 */
	void _addReverseLookup(socket_fd_t &socketFd, IcnId &rCId,
			enigma_t &enigma);
	/*!
	 * \brief Set the status of a socket file descriptor
	 *
	 * \param socketFd The socket FD for which the status is supposed to be set
	 * \param state The writeable state of the socketFd
	 */
	void _setSocketState(socket_fd_t &socketFd, bool state);
	/*!
	 * \brief Obtain socket state
	 *
	 * \param socketFd The FD for which the state should be retrieved
	 *
	 * \return The state for the FD stored in the map
	 */
	bool _socketWriteable(socket_fd_t &socketFd);
	/*!
	 * \brief Create/obtain TCP socket to send off HTTP request to server
	 *
	 * The CID is required here because it is used to look up the IP address
	 * from the FQDN registration
	 *
	 * \param cId The CID used to store IP endpoint configuration (sNAP/eNAP)
	 * \param nodeId The NID which has sent the HTTP request
	 * \param remoteSocketFd The session key under which the HTTP reqeust has
	 * been received (which corresponds to the socket FD)
	 * \param callRead Reference to boolean indicating of the functor must call
	 * read() or if there's another functor which is already reading from the FD
	 * \param packet Pointer to packet in case this is a wildcard match and the
	 * HTTP request must be parsed to obtain the FQDN for DNS resolution
	 * \param packetSize The size of the pointer packet
	 *
	 * \return The socket FD to which the main thread can write. -1 if socket
	 * could not be obtained/created
	 */
	int _tcpSocket(IcnId &cId, NodeId &nodeId, sk_t &remoteSocketFd,
			bool &callRead, uint8_t *packet, uint16_t &packetSize);
};

} /* namsepace tcpclient */

} /* namsepace http */

} /* namespace proxies */

#endif /* NAP_PROXIES_HTTP_TCPCLIENT_HH_ */
