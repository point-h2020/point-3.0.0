/*
 * blackadder_enum.hpp
 *
 * Copyright (C) 2016-2017  Sebastian Robitzsch, Mohammed AL-Khalidi and Mays AL-Naday
 * All rights reserved.
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
 * Blackadder. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BLACKADDER_ENUM_HPP_
#define BLACKADDER_ENUM_HPP_


/*!
 * \brief Commonly used information items for the management namespace
 */
enum information_items_t
{
	MANAGEMENT_II_DNS_LOCAL=0, /*!< Enable surrogate support in all NAPs */
	MANAGEMENT_II_MONITORING, /*!< Sending trigger to agents */
	EXPERIMENTATION_BANDWIDTH_DATA=0,/*!< For the puyblisher to push traffic*/
	EXPERIMENTATION_BANDWIDTH_FEEDBACK,/*!< For subscribers to provide feedback*/
    DEFAULT_HTTP_IID=0, /*!< default HTTP IID for wildcard match */
};

/*!
 * \brief Level 1 scopes for experimentation scopes
 */
enum experimentation_scopes_t
{
	EXPERIMENTAION_BANDWIDTH,/*!< Bandwidth (IPERF-like UDP) tests */
	EXPERIMENTAION_PING, /*!< Ping-like tests */
};

/*!
 * \brief Port Identifiers for Inter Process Communication
 *
 * The ports specified in this enumeration are used for IPCs between userspace
 * applications. In order to not interfere with any standardised PID (based on
 * IANA specification), the unassigned PID range 39682 - 39999 is used here.
 */
enum port_identifiers_t
{
	PID_BLACKADDER=9999,
	// Monitoring
	PID_MOLY_LISTENER,/*!< PID of monitoring agent listening for data from
	applications through MOLY*/
	PID_MOLY_BOOTSTRAP_LISTENER,/*!< PID for monitoring agent to listen for
	trigger command*/
	PID_NAP_SA_LISTENER,/*!< PID for NAP-SA interface (receiver) */
	PID_NAP_SA_SENDER,/*!< PID for NAP-SA interface (sender) */
	PID_MAPI/* PID for communciation between blackadder and MApps through
                    MAPI */
};

/*!
 * \brief Root scope IDs (namespaces)
 *
 * This enumeration is used across all Blackadder core and application modules
 * to reduce the number of Level 1 scope IDs
 */
enum root_namespaces_t {
        NAMESPACE_IP,                   /*!< IP namespace */
        NAMESPACE_HTTP,                 /*!< HTTP namespace */
        NAMESPACE_COAP,                 /*!< COAP namespace */
        NAMESPACE_MONITORING,   /*!< Monitoring namespace */
        NAMESPACE_MANAGEMENT,   /*!< Management namespace */
        NAMESPACE_SURROGACY,            /*!< Surrogacy namespace */
        NAMESPACE_LINK_STATE,/*!< Link state monitoring for non SDN links. FIXME
        still hardcoded to FFFFFFFFFFFFFFFB  */
        NAMESPACE_PATH_MANAGEMENT,/*!< Path management. FIXME still hardcoded to
        FFFFFFFFFFFFFFFA */
        NAMESPACE_EXPERIMENTATION, /*!< Experimentation for various purposes */
        NAMESPACE_IGMP_CTRL, /*!< IGMP control namespace */
        NAMESPACE_IGMP_DATA, /*!< IGMP data namespace */
        NAMESPACE_APP_RV,/*!< Communications from applications to RV. FIXME
        still hardocded to FFFFFFFFFFFFFFFF */
        NAMESPACE_RV_TM,/*!< Communications from RV to TM. FIXME still hardcoded to
        FFFFFFFFFFFFFFFE  */
        NAMESPACE_TM_APP,/*!< Communications from TM to applications. FIXME still
        hardcoded to FFFFFFFFFFFFFFFD  */
        NAMESPACE_UNKNOWN
};

enum suffix {
    SUFFIX_NAMESPACE = 0,         /*!< suffix for all the namespaces above*/
    SUFFIX_NOTIFICATION = 255,  /*!< notification scopes FF*/
};

enum notification_scopes {
    LINK_STATE_UPDATE=200,
    LINK_STATE_NOTIFICATION,
};

/*!
 * \brief type of core node, all nodes are FW by default and only domain RV/TM assumes the type of RV and/or TM
 */
enum NodeType {
    FW,
    RV,
    TM,
    NUM_TYPEs,
};


/*!
 * \brief length (in bytes) of identifiers used in the platform
 */
enum IDLengths {
	PURSUIT_ID_LEN = 8,
	FID_LEN = 32,
	NODEID_LEN = 8,
};

/*!
 * \brief ICN dissemination strategies, used by blackadder when proccessing pubs/subs
 */
enum Strategies {
	LINK_LOCAL = 1,
	DOMAIN_LOCAL,
	IMPLICIT_RENDEZVOUS,
	BROADCAST_IF,
	NODE_LOCAL,
};

/*!
 * \brief Types of messgages exchanged between ICN APPs, ICN APPs - RV, and ICN Apps - blackdder
 */
enum ClickMessageTypes {
	PUBLISH_SCOPE,
	PUBLISH_INFO,
	UNPUBLISH_SCOPE,
	UNPUBLISH_INFO,
	SUBSCRIBE_SCOPE,
	SUBSCRIBE_INFO,
	UNSUBSCRIBE_SCOPE,
	UNSUBSCRIBE_INFO,
	PUBLISH_DATA,
	PUBLISH_DATA_iSUB,
	CONNECT = 12,
	CONTROL_REC = 9, 
	CONTROL_ACK = 10,
	DISCONNECT,
	ADD_LINK,
	REMOVE_LINK,
	DISCONNECTED,
	RECONNECTED,
	CONNECTED,
	UNDEF_EVENT = 0,
	UPDATE_RVFID = 55,
	UPDATE_TMFID,
	UPDATE_DELIVERY,
	DISCOVER_FAILURE,
	UPDATE_UNICAST_DELIVERY,
	START_PUBLISH = 100,
	STOP_PUBLISH,
	SCOPE_PUBLISHED,
	SCOPE_UNPUBLISHED,
	PUBLISHED_DATA,
	MATCH_PUB_SUBS,
	RV_RESPONSE,
	UPDATE_FID,
	UPDATE_PUBLISH,
	PUBLISHED_DATA_iSUB,
	MATCH_PUB_iSUBS,
	PUBLISH_DATA_iMULTICAST,
	UPDATE_FID_iSUB,
	RE_PUBLISH,
	PAUSE_PUBLISH,
	RESUME_PUBLISH,
	START_PUBLISH_iSUB,
	STOP_PUBLISH_iSUB,
	NETLINK_BADDER = 30,
 CONTROL_REQ_FAILURE = 158,
        CONTROL_REQ_SUCCESS = 159,
};

/*!
 * \brief TODO add brief description
 */
enum RvReturnCodes {
	SUCCESS,
	WRONG_IDS,
	STRATEGY_MISMATCH,
	EXISTS,
	FATHER_DOES_NOT_EXIST,
	INFO_ITEM_WITH_SAME_ID,
	SCOPE_DOES_NOT_EXIST,
	SCOPE_WITH_SAME_ID,
	INFO_DOES_NOT_EXIST,
	DOES_NOT_EXIST,
	UNKNOWN_REQUEST_TYPE,
};


/*!
 * \brief Click element descriptor, LOCAL_PROCESS means an ICN application send the information to blackadder
 */
enum ELEMENTtype {
	LOCAL_PROCESS,
	CLICK_ELEMENT,
	RV_ELEMENT = 1,
};

/*!
 * \brief IPC Socket family
 */
enum Sock_FA {
    AF_BLACKADDER = 134,
    PROTO_BLACKADDER = 1
};
/*!
 * \brief MAPI message types
 */
enum MapiMessageTypes {
    GET_NODEID,
    GET_RVTMFID,
    SET_CONNECTION_STATUS,
};
enum ClickManagementMessageTypes {
    NODEID_IS,
    RVTMFID_IS,
    STATUS_SET_TO,
    UNDEF_RESPONSE
};
#endif /* BLACKADDER_ENUM_HPP_ */
