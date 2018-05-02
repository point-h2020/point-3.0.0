/*
 * configuration.hh
 *
 *  Created on: 17 Apr 2016
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

#ifndef NAP_CONFIGURATION_HH_
#define NAP_CONFIGURATION_HH_

#include <list>
#include <libconfig.h++>
#include <linux/socket.h>
#include <log4cxx/logger.h>
#include <map>
#include <mutex>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <unordered_map>

#include <types/enumerations.hh>
#include <types/icnid.hh>
#include <types/nodeid.hh>
#include <types/routingprefix.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace libconfig;
using namespace std;

namespace configuration {
	/*!
	 * \brief
	 */
	class Configuration {
		static log4cxx::LoggerPtr logger;
	public:
		/*!
		 * Constructor
		 */
		Configuration();
		/*!
		 * \brief Add/update an FQDN to the list of known FQDN served by this
		 * NAP
		 *
		 * \param cid The CID holding the FQDN
		 * \param ipAddress The IP address of the IP endpoint serving the FQDN
		 * \param port The port on which the HTTP service is expecting traffic
		 */
		void addFqdn(IcnId &cid, IpAddress &ipAddress, uint16_t port);
		/*!
		 * \brief The set TCP backlog
		 *
		 * \return TCP backlog
		 */
		int backlog();
		/*!
		 * \brief The interval in which buffer cleaners wake up
		 *
		 * All handlers have a buffer in case a packet cannot be published under
		 * its CID due to an outstanding START_PUBLISH notification. In case
		 * there's no subscriber available within a given interval the buffer
		 * cleaner per handler cleans aged packets from the buffer.
		 *
		 * \return The interval in seconds
		 */
		uint32_t bufferCleanerInterval();
		/*!
		 * \brief Obtain if this NAP runs as a client-side NAP
		 *
		 * \return Boolean indicating whether or not this NAP acts as a cNAP
		 */
		bool cNap();
		/*!
		 * \brief Delete an FQDN entry from the list of known FQDNs and their IP
		 * addresses
		 *
		 * \param cid The CID (FQDN) which shall be deleted
		 */
		void deleteFqdn(IcnId &cid);
		/*!
		 * \brief Obtain the IP address of the IP endpoint this NAP is serving
		 *
		 * In case the NAP has been configured in a host-based environment
		 *
		 * \return The IP address of the endpoint
		 */
		IpAddress endpointIpAddress();
		/*!
		 * \brief Obtain the list of registered FQDNs this NAP should serve
		 *
		 * \return The list of currently known FQDNs and the IP addresses of the
		 * endpoints serving them
		 */
		list<pair<IcnId, pair<IpAddress, uint16_t>>> fqdns();
		/*!
		 * \brief Obtain if HTTP handler is supposed to be turned off
		 *
		 * \return Boolean indicating if HTTP handler should be activated
		 */
		bool httpHandler();
		/*!
		 * \brief The port used to forward HTTP traffic
		 *
		 * \return The PID the transparent HTTP proxy should use to listen for
		 * incoming TCP sessions
		 */
		uint16_t httpProxyPort();
		/*!
		 * \brief Returns in this NAP has been configured for a host-based
		 * environment
		 *
		 * \return Boolean
		 */
		bool hostBasedNap();
		/*!
		 * \brief Obtain the host's routing prefix
		 *
		 * \return The host's routing prefix
		 */
		RoutingPrefix hostRoutingPrefix();
		/*!
		 * \brief Obtain the list of routing prefixes
		 *
		 * \return Routing prefix map
		 */
		map<uint32_t, RoutingPrefix> *hostRoutingPrefixes();
		/*!
		 * \brief Returns if this NAP was configured to be an ICN GW (HTTP)
		 *
		 * \return Boolean
		 */
		bool icnGatewayHttp();
		/*!
		 * \brief Returns if this NAP was configured to be an ICN GW (IP)
		 *
		 * \return Boolean
		 */
		bool icnGatewayIp();
		/*!
		 * \brief Obtain the ICN GW's routing prefix in case the NAP was
		 * configured to be an ICN GW
		 *
		 * \return The ICN GW's routing prefix as a RoutingPrefix class
		 * reference
		 */
		RoutingPrefix icnGatewayRoutingPrefix();
		/*!
		 * \brief Return the ICN header length
		 *
		 * This method returns the ICN header length in octets without the
		 * lengths of CIDs
		 *
		 * \return The ICN header length
		 */
		uint32_t icnHeaderLength();
		/*!
		 * \brief TODO
		 *
		 * \return TODO
		 */
		bool igmpHandler();
		/*!
		 * \brief TODO
		 *
		 * \return TODO
		 */
		string igmp_genQueryIP();
		/*!
		 * \brief TODO
		 *
		 * \return TODO
		 */
		string igmp_groupLeaveIP();
		/*!
		 * \brief TODO
		 *
		 * \return TODO
		 */
		uint32_t igmp_genQueryTimer();
		/*!
		 * \brief TODO
		 *
		 * \return TODO
		 */
		uint32_t igmp_genLeaveTimer();
		/*!
		 * \brief TODO
		 *
		 * \return TODO
		 */
		uint16_t igmp_redundantIGMP();
		/*!
		 * \brief TODO
		 *
		 * \return TODO
		 */
		uint32_t igmp_dropAfterQueriesNum();
		/*!
		 * \brief TODO
		 *
		 * \return TODO
		 */
		string igmp_sNapMCastIPs();
		/*!
		 * \brief TODO
		 *
		 * \return TODO
		 */
		string igmp_ignoreIGMPFrom();
		/*!
		 * \brief TODO
		 *
		 * \return TODO
		 */
		string igmp_ignoreMCastDataFrom();
		/*!
		 * \brief TODO
		 *
		 * \return TODO
		 */
		string igmp_napOperationMode();
		/*!
		 * \brief TODO
		 *
		 * \return TODO
		 */
		uint32_t igmp_message_processing_lag();
		/*!
		 * \param ipAddress The IP address in human readable format (dot
		 * notation, e.g. 172.16.23.1)
		 */
		void ipAddress(char *ipAddress);
		/*!
		 * \brief Obtian the IP address the NAP read from the device it has been
		 * allocated
		 *
		 * \return The IP address of _device
		 */
		IpAddress ipAddress();
		/*!
		 * \brief Initial LTP credit
		 *
		 * \return The initial credit for each LTP session
		 */
		uint32_t ltpInitialCredit();
		/*!
		 * \brief Obtain the desired size of the LTP RTT size
		 *
		 * \return List size
		 */
		uint32_t ltpRttListSize();
		/*!
		 * \brief Obtain if local surrogacy has been enabled
		 *
		 * \return Boolean if local surrogacy has been enabled
		 */
		bool localSurrogacy();
		/*!
		 * \brief Obtain FQDN configured for local surrogate
		 *
		 * \return FQDN which should be always relayed to localhost
		 */
		string localSurrogateFqdn();
		/*!
		 * \brief In case the local surrogate method is set to kernel this
		 * method returns the cofnigured IP address of the surrogate server
		 * which must be excluded from the (v)DEMUX
		 */
		IpAddress localSurrogateIpAddress();
		/*!
		 * \brief Obtain the method used to let UEs communicate with the local
		 * surrogate
		 *
		 * \brief The method of local_surrogacy_method_t (enumerations.hh)
		 */
		local_surrogacy_method_t localSurrogateMethod();
		/*!
		 * \brief Obtain port number configured for local surrogate
		 *
		 * \return Port number of the local surrogate server
		 */
		uint32_t localSurrogatePort();
		/*!
		 * \brief Obtain the Maximum ICN Transmission Unit (MITU)
		 *
		 * This is provided through nap.cfg
		 *
		 * \return The MITU in octets
		 */
		uint16_t mitu();
		/*!
		 * \brief Obtain the reporting interval to report to monitoring agent
		 *
		 * \return The reporting interval in seconds
		 */
		uint32_t molyInterval();
		/*!
		 * \brief Obtain the Maximum Transmission Unit (MTU)
		 *
		 * This value is read directly from the NIC which was configured in
		 * nap.cfg
		 *
		 * \return The MTU in octets
		 */
		uint16_t mtu();
		/*!
		 * \brief Obtain the network device on which the NAP communicates with
		 * its IP endpoints
		 *
		 * \return The network device in the format eth0 or wlan0
		 */
		string networkDevice();
		/*!
		 * \brief Obtain the node ID
		 *
		 * \return The node ID
		 */
		NodeId &nodeId();
		/*!
		 * \brief Obtain the node ID
		 *
		 * \param nodeId The NID obtained through MAPI
		 */
		void nodeId(uint32_t nodeId);
		/*!
		 * \brief Parse the configuration file and populate the information
		 *
		 * \return Boolean indicating if the parsing has been successfully
		 * finished
		 */
		bool parse(string file);
		/*!
		 * \brief Obtain the configured timeout multiplier
		 *
		 * \return The multiplier
		 */
		uint16_t ltpRttMultiplier();
		/*!
		 * \brief Retrieve the configured socekt type to communicate with IP
		 * endpoints
		 *
		 * \return The socket type using SocketType enumeration
		 */
		SocketType socketType();
		/*!
		 * \brief Retrieve whether surrogacy has been enabled or not
		 */
		bool surrogacy();
		/*!
		 * \brief Retrieve the configured traffic control drop rate
		 *
		 * \return The drop rate or -1
		 */
		int tcDropRate();
		/*!
		 * \brief Obtain the TCP client socket buffer size
		 *
		 * \return TCP client socket buffer size
		 */
		uint16_t tcpClientSocketBufferSize();
		/*!<
		 * \brief Obtain the TCP interceptipon port
		 *
		 * The default value is 80
		 *
		 * \return The TCP interception port
		 */
		int tcpInterceptionPort();
		/*!
		 * \brief Obtain the TCP server socket buffer size
		 *
		 * \return TCP server socket buffer size
		 */
		uint16_t tcpServerSocketBufferSize();
		/*!
		 * \brief The timeout after which LTP session state should be cleaned up
		 *
		 * \return The timeout value
		 */
		uint16_t timeout();
		/*!
		 * \brief Obtain the virtual interface name
		 *
		 * \return The name of the local interface treated as the virtual
		 * interface
		 */
		string virtualInterface();
	private:
		int _backlog;/*!< TCP backlog */
		uint32_t _bufferCleanerInterval;/*!< The interval in seconds all buffer
		cleaners wake up and clean the handler buffer from timed out packets*/
		bool _cNap;/*!< If the configuration file has FQDNs or surrogacy
		enabled, this boolean becomes false */
		uint32_t _ltpInitialCredit;/*!< The initial credit each LTP session
		starts with */
		RoutingPrefix _hostRoutingPrefix; /*!< The routing prefix this NAP is
		acting in */
		IpAddress _endpointIpAddress; /*!< If host-based deployment configured,
		this variable holds the IP endpoint's IP address */
		bool _httpHandler;/*!< Boolean to turn off HTTP handler */
		uint32_t _httpProxyPort; /*!< The HTTP proxy port the NAP is listening
		for new	incoming TCP connections. Default: 3127 */
		bool _icnGatewayHttp; /*!< Is this NAP running as an ICN GW and has
		Internet access. HTTP-over-ICN will have the wildcard */
		bool _icnGatewayIp; /*!< Is this NAP running as an ICN GW and has
		Internet access. IP-over-ICN will have the wildcard */
		RoutingPrefix _icnGatewayRoutingPrefix;/*!< Routing prefix of the ICN GW
		if set */
		bool _igmpHandler;/*!< TODO */
		string _igmp_genQueryIP;/*!< TODO */
		string _igmp_groupLeaveIP;/*!< TODO */
		unsigned int _igmp_genQueryTimer;/*!< TODO */
		unsigned int _igmp_genLeaveTimer;/*!< TODO */
		unsigned int _igmp_redundantIGMP;/*!< TODO */
		unsigned int _igmp_dropAfterQueriesNum;/*!< TODO */
		string _igmp_sNapMCastIPs; /*!< TODO */
		string _igmp_ignoreIGMPFrom; /*!< TODO */
		string _igmp_ignoreMCastDataFrom;/*!< TODO */
		string _igmp_napOperationMode;/*!< TODO */
		unsigned int _igmp_message_processing_lag;/*!< TODO */
		IpAddress _ipAddress;/*!< The IP address of _device*/
		bool _hostBasedNap; /*!< Is this NAP configured as in host-based
		scenario */
		list<pair<IcnId, pair<IpAddress, uint16_t>>> _fqdns; /*!< Holding all
		FQDNs this NAP is registered to. Note, surrogacy is provided through
		NAP-SA interface and won't be added to this list*/
		std::mutex _fqdnsMutex;/*!< Mutex for transaction safe operations on
		list of known FQDNs*/
		uint32_t _ltpRttListSize; /*!< The size of the list of measured RTT
		values*/
		uint32_t _ltpRttMultiplier;/*!< Multiplier for LTP timeout counter using
		a multipler of the measured RTT */
		SocketType _socketType;/*!< The socket type to communicate with IP
		endpoints when IP handler is used*/
		bool _localSurrogacy; /*!< Supporting surrogates on localhost */
		IpAddress _localSurrogateIpAddress;/*!< IP address of the local
		surrogate enabled via kernel method*/
		local_surrogacy_method_t _localSurrogateMethod;/*!< NAP or iptables */
		string _localSurrogateFqdn; /*!< If local surrogacy was enabled in the
		NAP config file this string holds the respective FQDN*/
		uint32_t _localSurrogatePort; /*!< If local surrogacy was enabled in the
		NAP config file this unsigned int holds the corresponding port */
		uint32_t _mitu; /*!< Maximum ICN Transmission Unit (MITU). Default: 1300
		bytes */
		uint32_t _molyInterval; /*!< The interval in seconds to report
		monitoring data to monitoring agent */
		uint32_t _mtu; /*!< Maximum Transmission Unit (MTU). Default: 1500
		bytes */
		string _networkDevice; /*!< The network device name on which the NAP is
		communicating with its IP endpoint(s) */
		NodeId _nodeId; /*!< The identifier of the node this NAP is running on*/
		map<uint32_t, RoutingPrefix> _routingPrefixes; /*!< Holding
		all	declared routing prefixes */
		bool _surrogacy;/*!< Enable eNAP functionality (NAP-SA) */
		int _tcDropRate;/*!< The traffic control drop rate */
		uint32_t _tcpClientSocketBufferSize;/*!< Size of the socket buffer for
		the TCP client towards servers (HTTP responses)*/
		int _tcpInterceptionPort;/*!< TCP interception port for HTTP-over-
		ICN. This port is used when inserting iptables rule*/
		uint32_t _tcpServerSocketBufferSize;/*!< Size of the socket buffer for
		the TCP server towards clients (HTTP requests)*/
		uint16_t _timeout;/*!< Timeout when LTP state sessions should be cleaned
		up in case they have not been correctly closed*/
	};
}
#endif /* NAP_CONFIGURATION_HH_ */
