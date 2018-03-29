/*
 * enumerations.hh
 *
 *  Created on: 17 Apr 2015
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

#ifndef ENUMERATIONS_HH_
#define ENUMERATIONS_HH_

#include <blackadder_enums.hpp>

/*!
 * \brief Definition of exit error code across the NAP
 *
 * When a FATAL message occurred the NAP terminates ungracefully and throws one
 * of the error codes below which can be read out by typing 'echo $?' in the
 * console right after the NAP terminated.
 */
enum NapExitErrorCodes
{
	CONFIGURATION_FILE,/*!< The file nap.cfg cannot be found at the given
	location, i.e., either /etc/nap/nap.cfg or the location provided with the -c
	option*/
	CONFIGURATION_FILE_PARSER,/*!< There are issues with the content of nap.cfg.
	Enable at least FATAL for 'log4j.logger.configuration' in /etc/nap/nap.l4j
	for further information*/
	PROXY_XML_PARSER
};

/*!
 * \brief Message/primitive types for NAP_SA API
 */
enum nap_sa_commands_t
{
	NAP_SA_ACTIVATE,
	NAP_SA_DEACTIVATE
};

/*!
 * \brief Port number declaration
 *
 * This enumeration list follows the IANA service port specification:
 * https://www.iana.org/assignments/service-names-port-numbers/service-names-
 * port-numbers.xhtml
 */
enum PortNumbers {
	PORT_UNKNOWN=0,			/*!< Only if transport layer protocol is unknown or
	raw IP packet has been received */
	PORT_FTP=21,			/*!< File Transfer Protocol */
	PORT_SSH=22,			/*!< The Secure Shell Protocol */
	PORT_TELNET=23,			/*!< Telnet */
	PORT_DNS=53,			/*!< Domain Name Server */
	PORT_DHCP_SERVER=67,	/*!< Dynamic Host Configuration Protocol Server */
	PORT_DHCP_CLIENT=68,	/*!< Dynamic Host Configuration Protocol Client */
	PORT_HTTP=80,			/*!< World Wide Web HTTP */
	PORT_NETBIOS=137,		/*!< NETBIOS name service */
	PORT_HTTPS=443,			/*!< HTTP over TLS/SSL */
	PORT_COMMPLEX=5001,		/*!< IPERF */
	PORT_RTP_MEDIA=5004,	/*!< RTP media data */
	PORT_RTP_CONTROL=5005,	/*!< RPT control protocol */
	PORT_COAP=5683,			/*!< Constrained Application Protocol */
	PORT_COAPS=5684,		/*!< DTLS-secured CoAP */
	PORT_ICMP=5813			/*!< Internet Control Message Protocol */
};
/*!
 * \brief logging levels
 *
 * Re-enumeration from the Log4cxx library
 */
enum Log4cxxLevels {
	LOG4CXX_LEVELS_FATAL=50000,	/*!< fatal logging */
	LOG4CXX_LEVELS_ERROR=40000,	/*!< error logging and above */
	LOG4CXX_LEVELS_WARN=30000,	/*!< warning logging and above */
	LOG4CXX_LEVELS_INFO=20000,	/*!< info logging and above */
	LOG4CXX_LEVELS_DEBUG=10000,	/*!< debug logging and above */
	LOG4CXX_LEVELS_TRACE=5000	/*!< trace logging and above */
};

enum local_surrogacy_method_t
{
	LOCAL_SURROGACY_METHOD_KERNEL,/*!< use iptables to forward any TCP packet */
	LOCAL_SURROGACY_METHOD_NAP,/*!< use the NAP to forward HTTP packets*/
	LOCAL_SURROGACY_METHOD_UNDEFINED
};
/*!
 * \brief LTP message types
 */
enum ltp_message_types_t
{
	LTP_DATA,
	LTP_CONTROL
};
/*!
 * \brief LTP control message types
 */
enum ltp_ctrl_control_types_t
{
	LTP_CONTROL_NACK,
	LTP_CONTROL_RESET,
	LTP_CONTROL_RESETTED,
	LTP_CONTROL_SESSION_END,
	LTP_CONTROL_SESSION_ENDED,
	LTP_CONTROL_WINDOW_END,
	LTP_CONTROL_WINDOW_ENDED,
	LTP_CONTROL_WINDOW_UPDATE,
	LTP_CONTROL_WINDOW_UPDATED
};
/*!
 * \brief Transport protocol states
 *
 * When handling an incoming data packet stateful transport protocols, e.g.,
 * LTP, require to return their state so that the ICN handler thread can act
 * accordingly. More information about this implementation design choice can be
 * found in the LaTeX document in /doc/tex
 */
enum tp_states_t
{
	TP_STATE_FRAGMENTS_OUTSTANDING,
	TP_STATE_ALL_FRAGMENTS_RECEIVED,
	TP_STATE_NO_TRANSPORT_PROTOCOL_USED,// Neither UTP nor LTP used
	TP_STATE_SESSION_ENDED,// session end signal over LTP
	TP_STATE_NO_ACTION_REQUIRED
};
/*!
 * \brief Enumeration of HTTP methods
 *
 * Those enumerations are taken from RFC 2616 and represent the different
 * methods used in requests and responses of the HTTP protocol.
 */
enum http_methods_t
{
	HTTP_METHOD_UNKNOWN,
	/* Taken from section 9 in RFC 2616 */
	HTTP_METHOD_REQUEST_OPTIONS,
	HTTP_METHOD_REQUEST_GET,
	HTTP_METHOD_REQUEST_HEAD,
	HTTP_METHOD_REQUEST_POST,
	HTTP_METHOD_REQUEST_PUT,
	HTTP_METHOD_REQUEST_DELETE,
	HTTP_METHOD_REQUEST_TRACE,
	HTTP_METHOD_REQUEST_CONNECT,
	HTTP_METHOD_REQUEST_EXTENSION,
	/* Taken from section 10 in RFC 2616 */
	HTTP_METHOD_RESPONSE_CONTINUE=100,
	HTTP_METHOD_RESPONSE_SWITCHINGPROTOCOLS,
	HTTP_METHOD_RESPONSE_OK=200,
	HTTP_METHOD_RESPONSE_CREATED,
	HTTP_METHOD_RESPONSE_ACCEPTED,
	HTTP_METHOD_RESPONSE_NOCONTENT,
	HTTP_METHOD_RESPONSE_MOVED=302,/* Google uses this to move you from
	google.com to google.co.uk */
	HTTP_METHOD_RESPONSE_NOTMODIFIED=304,
	HTTP_METHOD_RESPONSE_NOTFOUND=404,
	HTTP_METHOD_RESPONSE_REQUESTTIMEOUT=408,
	HTTP_METHOD_RESPONSE_INTERNALERROR=500,
	HTTP_METHOD_RESPONSE_NOT_IMPLEMENTED,/*501*/
	HTTP_METHOD_RESPONSE_BADGATEWAY,/*502*/
	HTTP_METHOD_RESPONSE_SERVICEUNAVAILABLE,/*503*/
	HTTP_METHOD_RESPONSE_GATEWAYTIMEOUT/*504*/
};
/*!
 * Preferred socket type to communicate with IP endpoints when using the IP
 * handler
 */
enum SocketType
{
	RAWIP,
	LIBNET
};

/*!
 * always use type TRANSPORT_STATE
 */
enum TransportStates {
	TRANSPORT_STATE_START,
	TRANSPORT_STATE_FRAGMENT,
	TRANSPORT_STATE_FINISHED,
	TRANSPORT_STATE_SINGLE_PACKET,
	TRANSPORT_STATE_UNKNOWN
};
/*!
 * \brief Transport protocol message types
 */
enum TransportProtocolTypes
{
	TRANSPORT_DATA,
	TRANSPORT_CONTROL_PROXY_SOCKET_CLOSED,
	TRANSPORT_MESSAGE_TYPE_UNKNOWN
};
#endif /* ENUMERATIONS_HH_ */
