/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
 * Copyright (C) 2015-2018  Mays AL-Naday
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

#include "localproxy.hh"
#include <../lib/blackadder_enums.hpp>
#include "ba_bitvector.hh"

CLICK_DECLS

LocalProxy::LocalProxy() {
}

LocalProxy::~LocalProxy() {
	click_chatter("LocalProxy: destroyed!");
}

int LocalProxy::configure(Vector<String> &conf, ErrorHandler */*errh*/) {
	gc = (GlobalConf *) cp_element(conf[0], this);
	//click_chatter("LocalProxy: configured!");
	return 0;
}

int LocalProxy::initialize(ErrorHandler */*errh*/) {
	//click_chatter("LocalProxy: initialized!");
	return 0;
}

void LocalProxy::cleanup(CleanupStage stage) {
	int size = 0;
	if (stage >= CLEANUP_ROUTER_INITIALIZED) {
		size = local_pub_sub_Index.size();
		PubSubIdxIter it1 = local_pub_sub_Index.begin();
		for (int i = 0; i < size; i++) {
			delete (*it1).second;
			it1 = local_pub_sub_Index.erase(it1);
		}
		size = activePublicationIndex.size();
		ActivePubIter it2 = activePublicationIndex.begin();
		for (int i = 0; i < size; i++) {
			delete (*it2).second;
			it2 = activePublicationIndex.erase(it2);
		}
		size = activeSubscriptionIndex.size();
		ActiveSubIter it3 = activeSubscriptionIndex.begin();
		for (int i = 0; i < size; i++) {
			delete (*it3).second;
			it3 = activeSubscriptionIndex.erase(it3);
		}
	}
	click_chatter("LocalProxy: Cleaned Up!");
}

void LocalProxy::push(int in_port, Packet * p) {
	int descriptor, index;
	int type_of_publisher;
	bool forward;
	unsigned char type, APItype, numberOfIDs, IDLength /*in fragments of PURSUIT_ID_LEN each*/, prefixIDLength /*in fragments of PURSUIT_ID_LEN each*/, strategy, numberofNodeIDs;//, Apptype /*notification sent by application to blackadder */;
	Vector<String> IDs;
	LocalHost *_localhost;
	BABitvector RVFID;
	BABitvector FID_to_subscribers;
	String ID, prefixID, nodeID;
	StringSet nodeIDs;
	index = 0;
	if (in_port == 2) {
		/*from port 2 I receive publications from the network*/
		index = 0;
		/*read the "header"*/
		numberOfIDs = *(p->data());
		/*Read all the identifiers*/
		for (int i = 0; i < (int) numberOfIDs; i++) {
			IDLength = *(p->data() + sizeof (numberOfIDs) + index);
			IDs.push_back(String((const char *) (p->data() + sizeof (numberOfIDs) + sizeof (IDLength) + index), (int) IDLength * PURSUIT_ID_LEN));
			index = index + sizeof (IDLength) + IDLength * PURSUIT_ID_LEN;
		}
		p->pull(sizeof (numberOfIDs) + index);
		// handle CPR notifications to apps
		if ((IDs.size() == 1) && (IDs[0].compare(gc->controlReliabilityScope) == 0)){
				//click_chatter("LocalProxy, Link change : ID: %s", IDs[0].quoted_hex().c_str());
				handleControlReliabilityNotification(p);
				return;
		}
		type = *(p->data());
		if (type == PUBLISH_DATA) {
			APItype = PUBLISHED_DATA;
			/*pull the type out as the application does not need it*/
			p->pull(sizeof (type));
			if ((IDs.size() == 1) && (IDs[0].compare(gc->notificationIID) == 0)) {
				/*a special case here: Got back an RV/TM event...it was published using the ID /FFFFFFFFFFFFFFFD/MYNODEID*/
				/*remove the header, plus the outer type of PUBLISHED_DATA */
				handleRVNotification(p);
				p->kill();
			} else if ((IDs.size() == 1) && (IDs[0].compare(gc->pathMgmtIID) == 0)){
				//click_chatter("LocalProxy, Link change : ID: %s", IDs[0].quoted_hex().c_str());
				processTMNotification(p);
			}
			else {
				/*a regular network publication..I will look for local subscribers*/
				/*Careful: I will not kill the packet - I will reuse it one way or another, so....get rid of everything except the data*/
				/*remove the header*/
				handleNetworkPublication(IDs, p, APItype);
			}
		}
		else if (type == PUBLISH_DATA_iSUB) {
			APItype = PUBLISHED_DATA_iSUB;
			/*pull the type out as the application does not need it*/
			p->pull(sizeof (type));
			String nodeID = String((const char *)p->data(), NODEID_LEN);
			nodeIDs.find_insert(StringSetItem(nodeID));
			/*doing nothing with the return bool in forward, but might use it later on*/
			forward = storeActiveNode(nodeID, IDs);
            click_chatter("LocalProxy: node %s registered implicit subscriptions", nodeID.quoted_hex().c_str());
            if(forward) {
                click_chatter("LocalProxy: new Node requesting Implicist Subscription %s, I will request LocalRV assistance", nodeID.quoted_hex().c_str());
                publishiReqToRV(nodeIDs);
            }
			handleNetworkPublication(IDs, p, APItype);
		} else {
			/*Pub/Sub types of packets (niether PUBLISH_DATA, nor PUBLISH_DATA_iSUB*/
			/*a regular network publication..I will look for local subscribers*/
			/*keep the message type as the application needs it*/
			APItype = PUBLISHED_DATA;
			handleNetworkPublication(IDs, p, APItype);
		}
	}
    /*Management traffic through MAPI*/
    else if (in_port == 3) {
#if !CLICK_NS
        /*The packet came from the FromMGMNetlink Element. An application sent it*/
        descriptor = p->anno_u32(0);
        type_of_publisher = LOCAL_PROCESS;
#else
        /*The packet came from the FromMGMNetlink Element. An application sent it*/
        memcpy(&descriptor, p->data(), sizeof (descriptor));
        p->pull(sizeof (descriptor));
        type_of_publisher = LOCAL_PROCESS;
#endif
        _localhost = getLocalHost(type_of_publisher, descriptor);
//        click_chatter("LocalProxy: packet from a MApp using MAPI with PID: %u", _localhost->id);
        handleMappRequest(p, _localhost);
    }
	else {
		/*the request comes from the IPC element or from a click Element. The descriptor here may be the netlink ID of an application or the click port of an Element*/
#if !CLICK_NS
		if (in_port == 0) {
			/*The packet came from the FromNetlink Element. An application sent it*/
			descriptor = p->anno_u32(0);
			type_of_publisher = LOCAL_PROCESS;
		} else {
			/*anything else is from a Click Element (e.g. the LocalRV Element)*/
			descriptor = in_port;
			type_of_publisher = CLICK_ELEMENT;
		}
#else
		if (in_port == 0) {
			/*The packet came from the FromNetlink Element. An application sent it*/
			memcpy(&descriptor, p->data(), sizeof (descriptor));
			p->pull(sizeof (descriptor));
			type_of_publisher = LOCAL_PROCESS;
		} else {
			/*anything else is from a Click Element (e.g. the LocalRV Element)*/
			descriptor = in_port;
			type_of_publisher = CLICK_ELEMENT;
		}
#endif
		_localhost = getLocalHost(type_of_publisher, descriptor);
		type = *(p->data());
		switch (type) {
				/*decide an action based on the type of the message recieved from the application*/
				case DISCONNECT:
			{
				disconnect(_localhost);
				p->kill();
				return;
			}
				break;
				case PUBLISH_DATA:
			{
				/*this is a publication coming from an application or a click element*/
				IDLength = *(p->data() + sizeof (type));
				ID = String((const char *) (p->data() + sizeof (type) + sizeof (IDLength)), IDLength * PURSUIT_ID_LEN);
				strategy = *(p->data() + sizeof (type) + sizeof (IDLength) + ID.length());
				APItype = PUBLISHED_DATA;
				switch (strategy) {
						case IMPLICIT_RENDEZVOUS:
					{
						FID_to_subscribers = BABitvector(FID_LEN * 8);
						memcpy(FID_to_subscribers._data, p->data() + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy), FID_LEN);
						/*Careful: I will not kill the packet - I will reuse it one way or another, so....get rid of everything except the data and will see*/
						p->pull(sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy) + FID_LEN);
						if ((ID.compare(gc->notificationIID) == 0)) {
							/*A special case here: The locaRV element published data using the blackadder API. This data is an RV notification*/
							handleRVNotification(p);
							p->kill();
						}
						/*A special case here: The localRV elemented published a request to the TM using blackadder API. This data is an RV request for topology formation*/
						/*This packet already has a request_type = MATCH_PUB_SUBs from the RV and an APITYPE of PUBLISH_DATA, so we need to keep the request_type*/
						else if ((ID.compare(gc->nodeTMScope) == 0) || (ID.compare(gc->nodeTMScopeUC) == 0)){
							handleUserPublication(ID, FID_to_subscribers, p, _localhost, APItype);
						}
						else if (ID.compare(gc->pathMgmtIID) == 0){
							//click_chatter("LocalProxy: Link ID change");
							handleUserTMPublication(ID, type, FID_to_subscribers, p, _localhost);
						}
						else {
							handleUserPublication(ID, type, FID_to_subscribers, p, _localhost, APItype);
						}
					}
						break;
						case LINK_LOCAL:
					{
						FID_to_subscribers = BABitvector(FID_LEN * 8);
						memcpy(FID_to_subscribers._data, p->data() + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy), FID_LEN);
						//click_chatter("publish link_local using LID %s", FID_to_subscribers.to_string().c_str());
						/*Careful: I will not kill the packet - I will reuse it one way or another, so....get rid of everything except the data and will see*/
						p->pull(sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy) + FID_LEN);
						/*push packet of <type> to remote subscribers and of <APItype> to local subscribers*/
						handleUserPublication(ID, type, FID_to_subscribers, p, _localhost, APItype);
					}
						break;
						case BROADCAST_IF:
					{
						FID_to_subscribers = BABitvector(FID_LEN * 8);
						FID_to_subscribers.negate();
						/*Careful: I will not kill the packet - I will reuse it one way or another, so....get rid of everything except the data and will see*/
						p->pull(sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy));
						/*push packet of <type> to remote subscribers and of <APItype> to local subscribers*/
						handleUserPublication(ID, type, FID_to_subscribers, p, _localhost, APItype);
					}
						break;
						case NODE_LOCAL:
						/*Notification from the app to blackadder, currently only used for path management communications by the LinkStateMonitor*/
					{
							/*Careful: I will not kill the packet - I will reuse it one way or another, so....get rid of everything except the data*/
							p->pull(sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy));
							handleUserPublication(ID, type, p, _localhost, APItype);
					}
						break;
					default:
					{
						/*Careful: I will not kill the packet - I will reuse it one way or another, so....get rid of everything except the data*/
						p->pull(sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy));
						handleUserPublication(ID, type, p, _localhost, APItype);
					}
						break;
				}
			}
				break;
				case PUBLISH_DATA_iSUB:
			{
				/*isub message has a single node ID*/
				IDLength = *(p->data() + sizeof (type));
				ID = String((const char *) (p->data() + sizeof (type) + sizeof (IDLength)), IDLength * PURSUIT_ID_LEN);
				strategy = *(p->data() + sizeof (type) + sizeof (IDLength) + ID.length());
				APItype = PUBLISHED_DATA_iSUB;
				RVFID = BABitvector(FID_LEN * 8);
				switch (strategy) {
						case IMPLICIT_RENDEZVOUS:
						/*Do nothing now...just a place holder*/
						break;
						case DOMAIN_LOCAL:
					{
						p->pull(sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy));
						handleUserPublication(ID, type, p, _localhost, APItype);
						/*also store the subscription state as the cNAP wont make explicit subscription, it needs to store the state here*/
						RVFID = gc->defaultRV_dl;
						/*Careful, the ID and prefixID of the isubscription are in the payload of the packet - they are not in ID, which is the publishing ID. because when cNAP makes iSub,\
						 it will send the hash(URL) of the subscription in the payload while the publishing ID is of the hash(FQDN)*/
						/*therefore, reparse the packet to get the ID of subscription not ID of publication*/
						numberOfIDs = *(p->data());
						ID = String((const char *) (p->data() + sizeof (numberOfIDs)), (int) numberOfIDs * PURSUIT_ID_LEN);
						prefixID = ID.substring(0, ID.length() - PURSUIT_ID_LEN);
						forward = handleLocalRequest(type, _localhost, ID, prefixID, strategy, RVFID);
						click_chatter("LocalProxy: store the implicit subscription of ID: %s of prefixID: %s", ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str());
                        /*kill original but obselete packet, a new packet got sent so must kill this one here as we still need it*/
                        p->kill();
					}
						break;
					default:
						break;
				}
			}
				break;
				case PUBLISH_DATA_iMULTICAST:
				/*This is a publication requesting a multicast FID for a set of nodes - supporting concidental multicast*/
			{
				IDLength = *(p->data() + sizeof (type));
				ID = String((const char *) (p->data() + sizeof (type) + sizeof (IDLength)), IDLength * PURSUIT_ID_LEN);
				strategy = *(p->data() + sizeof (type) + sizeof (IDLength) + ID.length());
				numberofNodeIDs = *(p->data() + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy));
				/********Get the Set of Node Identifiers of all iSubscribers***************/
				for (int i = 0; i < (int) numberofNodeIDs; i++) {
					nodeID = String((const char *)(p->data() + sizeof (type) + sizeof(IDLength) + ID.length() + sizeof (strategy) + sizeof(numberofNodeIDs) + (i * NODEID_LEN)), NODEID_LEN);
					//click_chatter("LocalProxy: iSubscriber: %s", nodeID.quoted_hex().c_str());
					nodeIDs.find_insert(StringSetItem(nodeID));
				}
				/**************************************************************************/
				/*This is a request to form a considental multicast tree from multiple unicast FIDs*/
				p->pull(sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy) + sizeof (numberofNodeIDs) /*number of isubscribers*/ + (int) numberofNodeIDs * NODEID_LEN /*isubscribers*/);
				type = PUBLISH_DATA;
				handleUserMulticastRequest(ID, type, p, nodeIDs);
			}
				break;
			default:
			{
				/*This is a pub/sub request*/
				/*read user request*/
				IDLength = *(p->data() + sizeof (type));
				ID = String((const char *) (p->data() + sizeof (type) + sizeof (IDLength)), IDLength * PURSUIT_ID_LEN);
				prefixIDLength = *(p->data() + sizeof (type) + sizeof (IDLength) + ID.length());
				prefixID = String((const char *) (p->data() + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength)), prefixIDLength * PURSUIT_ID_LEN);
				strategy = *(p->data() + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength) + prefixID.length());
				RVFID = BABitvector(FID_LEN * 8);
				
				switch (strategy) {
						case NODE_LOCAL:
						break;
						case LINK_LOCAL:
						/*don't do anything here..just a placeholder to remind us about that strategy...subscriptions will recorded only locally. No publication will be sent to the RV (wherever that is)*/
						break;
						case BROADCAST_IF:
						/*don't do anything here..just a placeholder to remind us about that strategy...subscriptions will recorded only locally. No publication will be sent to the RV (wherever that is)*/
						break;
						case DOMAIN_LOCAL:
						RVFID = gc->defaultRV_dl;
						break;
						case IMPLICIT_RENDEZVOUS:
						/*don't do anything here..just a placeholder to remind us about that strategy...subscriptions will recorded only locally. No publication will be sent to the RV (wherever that is)*/
						break;
					default:
						click_chatter("LocalProxy: a weird strategy that I don't know of --- FATAL");
						break;
				}
				forward = handleLocalRequest(type, _localhost, ID, prefixID, strategy, RVFID);
				if (forward) {
					publishReqToRV(p, RVFID);
				} else {
					p->kill();
				}
			}
				break;
		}
	}
}

LocalHost * LocalProxy::getLocalHost(int type, int id) {
	LocalHost *_localhost;
	String ID;
	_localhost = local_pub_sub_Index.get(id);
	if (_localhost == local_pub_sub_Index.default_value()) {
		_localhost = new LocalHost(type, id);
		local_pub_sub_Index.set(id, _localhost);
	}
	return _localhost;
}

void LocalProxy::disconnect(LocalHost *_localhost) {
	/*there is a bug here...I have to rethink how to correctly delete all entries in the right sequence*/
	if (_localhost != NULL) {
		click_chatter("LocalProxy: Entity %s disconnected...cleaning...", _localhost->localHostID.c_str());
		/*I know whether we talk about a scope or an information item from the isScope boolean value*/
		deleteAllActiveInformationItemPublications(_localhost);
		deleteAllActiveInformationItemSubscriptions(_localhost);
		deleteAllActiveScopePublications(_localhost);
		deleteAllActiveScopeSubscriptions(_localhost);
		local_pub_sub_Index.erase(_localhost->id);
		delete _localhost;
	}
}

/*Handle application or click element request..the RVFID is NULL except from link-local cases where the application has specified one*/
bool LocalProxy::handleLocalRequest(unsigned char &type, LocalHost *_localhost, String &ID, String &prefixID, unsigned char &strategy, BABitvector &RVFID) {
	bool forward = false;
	String fullID;
	/*create the fullID*/
	if (ID.length() == PURSUIT_ID_LEN) {
		/*a single fragment*/
		fullID = prefixID + ID;
	} else {
		/*multiple fragments*/
		fullID = prefixID + ID.substring(ID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
	}
	switch (type) {
			case PUBLISH_SCOPE:
			//click_chatter("LocalProxy: received PUBLISH_SCOPE request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
			forward = storeActivePublication(_localhost, fullID, strategy, RVFID, true);
			break;
			case PUBLISH_INFO:
			//click_chatter("LocalProxy: received PUBLISH_INFO request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
			forward = storeActivePublication(_localhost, fullID, strategy, RVFID, false);
			break;
			case UNPUBLISH_SCOPE:
			//click_chatter("LocalProxy: received UNPUBLISH_SCOPE request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
			forward = removeActivePublication(_localhost, fullID, strategy);
			break;
			case UNPUBLISH_INFO:
			//click_chatter("LocalProxy: received UNPUBLISH_INFO request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
			forward = removeActivePublication(_localhost, fullID, strategy);
			break;
			case SUBSCRIBE_SCOPE:
			//click_chatter("LocalProxy: received SUBSCRIBE_SCOPE request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
			forward = storeActiveSubscription(_localhost, fullID, strategy, RVFID, true);
			break;
			case SUBSCRIBE_INFO:
			//click_chatter("LocalProxy: received SUBSCRIBE_INFO request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
			forward = storeActiveSubscription(_localhost, fullID, strategy, RVFID, false);
			break;
			case UNSUBSCRIBE_SCOPE:
			//click_chatter("LocalProxy: received UNSUBSCRIBE_SCOPE request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
			forward = removeActiveSubscription(_localhost, fullID, strategy);
			break;
			case UNSUBSCRIBE_INFO:
			//click_chatter("LocalProxy: received UNSUBSCRIBE_INFO request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
			forward = removeActiveSubscription(_localhost, fullID, strategy);
			break;
			case PUBLISH_DATA_iSUB:
			//click_chatter("LocalProxy: received PUBLISH_DATA_iSUB request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
			//Carefull, this assumes all implicit subscriptions are made for information items - no implicit subscriptions for scopes
			//\TODO: consolidate this assumption and modify the code accordingly...
			forward = storeActiveSubscription(_localhost, fullID, strategy, RVFID, false);
			break;
		default:
			//click_chatter("LocalProxy: unknown request - skipping request - this should be something FATAL!");
			break;
	}
	return forward;
}

/*store the remote scope for the _publisher..forward the message to the RV point only if this is the first time the scope is published.
 If not, the RV point already knows about this node's publication...Note that RV points know only about network nodes - NOT for processes or click modules*/
bool LocalProxy::storeActivePublication(LocalHost *_publisher, String &fullID, unsigned char strategy, BABitvector &RVFID, bool isScope) {
	ActivePublication *ap;
	if ((strategy == NODE_LOCAL) || (strategy == DOMAIN_LOCAL)) {
		ap = activePublicationIndex.get(fullID);
		if (ap == activePublicationIndex.default_value()) {
			/*create the active scope's publication entry*/
			ap = new ActivePublication(fullID, strategy, isScope);
			ap->RVFID = RVFID;
			/*add the active scope's publication to the index*/
			activePublicationIndex.set(fullID, ap);
			/*update the local publishers of that active scope's publication*/
			ap->publishers.find_insert(_publisher, STOP_PUBLISH);
			/*update the active scope publications for this publsher*/
			_publisher->activePublications.find_insert(StringSetItem(fullID));
			//click_chatter("LocalProxy: store Active Scope Publication %s for local publisher %s", fullID.quoted_hex().c_str(), _publisher->publisherID.c_str());
			return true;
		}
		else {
			/*the ap->FID has been invalidated - likely due to link/node failure - hence requires republish request to Domain RV*/
			if ((ap->publishers.get(_publisher) == RE_PUBLISH) || (ap->publishers.get(_publisher) == RESUME_PUBLISH)) {
                click_chatter("PUBLISH_INFO have been recieved as result of RE_PUBLISH or RESUME_PUBLISH");
				return true;
			}
			else if (ap->strategy == strategy) {
				/*update the publishers of that remote scope*/
                ap->publishers.find_insert(_publisher, STOP_PUBLISH);
				/*update the published remote scopes for this publsher*/
				_publisher->activePublications.find_insert(StringSetItem(fullID));
				//click_chatter("LocalProxy: Active Scope Publication %s exists...updated for local publisher %s", fullID.quoted_hex().c_str(), _publisher->publisherID.c_str());
			} else {
				//click_chatter("LocalProxy: LocalRV: error while trying to update list of publishers for active publication %s..strategy mismatch", ap->fullID.quoted_hex().c_str());
			}
		}
	} else {
		//click_chatter("I am not doing anything with %s..the strategy is not NODE_LOCAL or DOMAIN_LOCAL", fullID.quoted_hex().c_str());
	}
	return false;
}

/*delete the remote publication for the _publisher..forward the message to the RV point only if there aren't any other publishers or subscribers for this scope*/
bool LocalProxy::removeActivePublication(LocalHost *_publisher, String &fullID, unsigned char strategy) {
	ActivePublication *ap;
	if ((strategy == NODE_LOCAL) || (strategy == DOMAIN_LOCAL)) {
		ap = activePublicationIndex.get(fullID);
		if (ap != activePublicationIndex.default_value()) {
			if (ap->strategy == strategy) {
				_publisher->activePublications.erase(fullID);
				ap->publishers.erase(_publisher);
				//click_chatter("LocalProxy: deleted publisher %s from Active Scope Publication %s", _publisher->publisherID.c_str(), fullID.quoted_hex().c_str());
				if (ap->publishers.size() == 0) {
					//click_chatter("LocalProxy: delete Active Scope Publication %s", fullID.quoted_hex().c_str());
					delete ap;
					activePublicationIndex.erase(fullID);
					return true;
				}
			} else {
				//click_chatter("LocalProxy: error while trying to delete active publication %s...strategy mismatch", ap->fullID.quoted_hex().c_str());
			}
		} else {
			//click_chatter("LocalProxy:%s is not an active publication", fullID.quoted_hex().c_str());
		}
	} else {
		//click_chatter("I am not doing anything with %s..the strategy is not NODE_LOCAL or DOMAIN_LOCAL", fullID.quoted_hex().c_str());
	}
	return false;
}

/*store the active scope for the _subscriber..forward the message to the RV point.
 Previously this only announced if this is the first subscription for this scope,
 but new behaviour announces if another subscriber also joins. This is so that
 all new START_PUBLISH events are forwarded.
 If not, the RV point already knows about this node's subscription...Note that RV points know only about network nodes - NOT about processes or click modules*/
bool LocalProxy::storeActiveSubscription(LocalHost *_subscriber, String &fullID, unsigned char strategy, BABitvector &RVFID, bool isScope) {
	ActiveSubscription *as;
    ActiveSubscription *pas;
    String prefixID;
	as = activeSubscriptionIndex.get(fullID);
	if (as == activeSubscriptionIndex.default_value()) {
		as = new ActiveSubscription(fullID, strategy, isScope);
		as->RVFID = RVFID;
		/*add the remote scope to the index*/
		activeSubscriptionIndex.set(fullID, as);
		/*update the subscribers of that remote scope*/
		as->subscribers.find_insert(LocalHostSetItem(_subscriber));
		/*update the subscribed remote scopes for this publsher*/
		_subscriber->activeSubscriptions.find_insert(StringSetItem(fullID));
		click_chatter("LocalProxy: store Active Subscription %s for local subscriber %s", fullID.quoted_hex().c_str(), _subscriber->localHostID.c_str());
        if(fullID.compare(gc->HTTP_DEFAULT_IID) == 0) {
            prefixID = fullID.substring(0, fullID.length() - PURSUIT_ID_LEN);
            click_chatter("LocalProxy: ICNGW, also store Active Subscription %s for local subscriber %s", prefixID.quoted_hex().c_str(), _subscriber->localHostID.c_str());
            pas = new ActiveSubscription(prefixID, strategy, true);
            pas->RVFID = RVFID;
            /*add the remote scope to the index*/
            activeSubscriptionIndex.set(prefixID, pas);
            /*update the subscribers of that remote scope*/
            pas->subscribers.find_insert(LocalHostSetItem(_subscriber));
            /*update the subscribed remote scopes for this publsher*/
            _subscriber->activeSubscriptions.find_insert(StringSetItem(prefixID));
        }
		if ((strategy != IMPLICIT_RENDEZVOUS) && (strategy != LINK_LOCAL) && (strategy != BROADCAST_IF)) {
			return true;
		} else {
			//click_chatter("I am not forwarding subscription for %s...strategy is %d", fullID.quoted_hex().c_str(), (int) strategy);
		}
	} else {
		if (as->strategy == strategy) {
			/*update the subscribers of that remote scope*/
			as->subscribers.find_insert(LocalHostSetItem(_subscriber));
			/*update the subscribed remote scopes for this publsher*/
			_subscriber->activeSubscriptions.find_insert(StringSetItem(fullID));
			click_chatter("LocalProxy: Active Subscription %s exists...updated for local subscriber %s", fullID.quoted_hex().c_str(), _subscriber->localHostID.c_str());
            if(fullID.compare(gc->HTTP_DEFAULT_IID) == 0) {
                prefixID = fullID.substring(0, fullID.length() - PURSUIT_ID_LEN);
                click_chatter("LocalProxy: ICNGW Active Subscription %s exists...updated for local subscriber %s", prefixID.quoted_hex().c_str(), _subscriber->localHostID.c_str());
                pas = activeSubscriptionIndex.get(prefixID);
                pas->subscribers.find_insert(LocalHostSetItem(_subscriber));
            }
			return true;
		} else {
			click_chatter("LocalProxy: error while trying to update list of subscribers for Active Subscription %s..strategy mismatch", as->fullID.quoted_hex().c_str());
		}
	}
	return false;
}

/*delete the remote scope for the _subscriber..forward the message to the RV point only if there aren't any other publishers or subscribers for this scope*/
bool LocalProxy::removeActiveSubscription(LocalHost *_subscriber, String &fullID, unsigned char strategy) {
	ActiveSubscription *as;
	as = activeSubscriptionIndex.get(fullID);
	if (as != activeSubscriptionIndex.default_value()) {
		if (as->strategy == strategy) {
			_subscriber->activeSubscriptions.erase(fullID);
			as->subscribers.erase(_subscriber);
			//click_chatter("LocalProxy: deleted subscriber %s from Active Subscription %s", _subscriber->localHostID.c_str(), fullID.quoted_hex().c_str());
			if (as->subscribers.size() == 0) {
				//click_chatter("LocalProxy: delete Active Subscription %s", fullID.quoted_hex().c_str());
				delete as;
				activeSubscriptionIndex.erase(fullID);
				if ((strategy != IMPLICIT_RENDEZVOUS) && (strategy != LINK_LOCAL) && (strategy != BROADCAST_IF)) {
					return true;
				} else {
					//click_chatter("I am not forwarding subscription for %s...strategy is %d", fullID.quoted_hex().c_str(), (int) strategy);
				}
			}
		} else {
			//click_chatter("LocalProxy: error while trying to delete Active Subscription %s...strategy mismatch", as->fullID.quoted_hex().c_str());
		}
	} else {
		//click_chatter("LocalProxy: no active subscriptions %s", fullID.quoted_hex().c_str());
	}
	return false;
}

/*store the nodeIDs of implicit subscribers*/
bool LocalProxy::storeActiveNode(String &_isubscriberID, Vector<String> &IDs) {
	ActiveNode *an;
	LocalHostStringHashMap localSubscribers;
	//	click_chatter("received data for ID: %s", IDs[0].quoted_hex().c_str());
	bool foundLocalSubscribers = findLocalSubscribers(IDs, localSubscribers);
	an = activeNodeIndex.get(_isubscriberID);
	if (an == activeNodeIndex.default_value()) {
		an = new ActiveNode(_isubscriberID);
		activeNodeIndex.set(_isubscriberID, an);
		an->FID_to_node = BABitvector(FID_LEN * 8);
		if (foundLocalSubscribers) {
			for (LocalHostStringHashMapIter localSubscribers_it = localSubscribers.begin(); localSubscribers_it != localSubscribers.end(); localSubscribers_it++) {
//			pushDataToLocalSubscriber((*localSubscribers_it).first, (*localSubscribers_it).second, p, APItype);
			an->publishers.find_insert((*localSubscribers_it).first, STOP_PUBLISH_iSUB);
			}
		}
		return true;
	}
    /*the node is stored but with all-zero FID (e.g. due to isolation from the ICN)*/
    else if (an->FID_to_node.zero()) {
        return true;
    }
//    click_chatter("Existing iSubscriber with FID: %s: ", an->FID_to_node.to_string().c_str());
	return false;
}


/**
 * Delivers control reliability notifications to the correlated local apps
 * */
void LocalProxy::handleControlReliabilityNotification(Packet *p){
	click_chatter("LocalProxy::handleControlReliabilityNotification");
	// incoming structure:: || CONTROL_REQ | DELIVERY_STATUS | PAYLOAD ||  
	// PAYLOAD  structure:: || numOfIDs | IdLen | ID | TYPE | PAYLOADD ||
	unsigned char type = *(p->data());	// this is "CONTROL_REQ"
	unsigned index = sizeof(char);
	//unsigned char status = *(p->data() + index); // this is the status of CPR request
	index +=  sizeof(char);
	/// here starts the original payload of initial request
	unsigned char numberOfIDs = *(p->data() + index);
	click_chatter("CPR numberOfIDs %d",numberOfIDs);
	index += sizeof (numberOfIDs);
	Vector <String> IDs;
	unsigned char IDLength;
	for (int i = 0; i < (int) numberOfIDs; i++) {
		IDLength = *(p->data() + index);
		index += sizeof (IDLength);
		IDs.push_back(String((const char *) (p->data() + index), IDLength * PURSUIT_ID_LEN));
		click_chatter("CPR id %s", IDs[i].c_str());
		index += IDLength * PURSUIT_ID_LEN;
	}
	unsigned char type2 =  *(p->data() + index); // this is the type of the initial request
	index += sizeof (char);
	click_chatter("CPR type2 %d",(unsigned)type2);

	//click_chatter("LocalProxy::handleControlReliabilityNotification | Payload type %d ", type2);
	String ID, prefixID="";
	char prefixIDLength, strategy=0;
	// if this is a PUB-SUB req to RV, then read RV_ID to find the correlated local apps
	if ((IDs.size() == 1) && (IDs[0].compare(gc->nodeRVScope) == 0)) {
		IDLength = *(p->data() + index);
		index += sizeof(IDLength);
		ID = String((const char *) (p->data() + index), IDLength * PURSUIT_ID_LEN);
		index += IDLength * PURSUIT_ID_LEN;
		prefixIDLength = *(p->data() + index);
		index+= sizeof(prefixIDLength);
		prefixID = String((const char *) (p->data() + index), prefixIDLength * PURSUIT_ID_LEN);
		index  += prefixIDLength * PURSUIT_ID_LEN;
		strategy = *(p->data() + index);
		index+= sizeof(strategy);
	}	
	String fullID = prefixID + ID;
	unsigned char fullIDLen = fullID.length() / PURSUIT_ID_LEN;
	
	if (type2 == MATCH_PUB_SUBS) {
		//click_chatter("LocalProxy::handleControlReliabilityNotification | RV's message to TM could not be delivered..");
		/*** Dont really know what to do in this case.. ***/
		 p->kill();
		 return;
		}
	
	ActivePublication *ap = 0;	
	ActiveSubscription *as = 0;	
	switch (type2) {
		case PUBLISH_SCOPE:
		case UNPUBLISH_SCOPE:
		case PUBLISH_INFO:
		case UNPUBLISH_INFO:
			ap = activePublicationIndex.get(fullID);
			if (ap == activePublicationIndex.default_value() || ap->strategy != strategy || ap->publishers.size() == 0) {
				ap = 0;
				//click_chatter("LocalProxy::handleControlReliabilityNotification did not find active publishers of ID %s", fullID.c_str());
			}
			break;
		case SUBSCRIBE_SCOPE:
		case UNSUBSCRIBE_SCOPE:
		case SUBSCRIBE_INFO:
		case UNSUBSCRIBE_INFO:
			as = activeSubscriptionIndex.get(fullID);
			if (as == activeSubscriptionIndex.default_value() || as->strategy != strategy || as->subscribers.size() == 0) {
				as = 0;
				//click_chatter("LocalProxy::handleControlReliabilityNotification did not find active subscribers to ID %s", fullID.c_str());
			}
			
			break;
		case START_PUBLISH:
		case STOP_PUBLISH:
		
			break;
		default:
			click_chatter("LocalProxy::handleControlReliabilityNotification -> unkown strategy, dropping packet");
			if (gc->type[TM]) click_chatter("LocalProxy::handleControlReliabilityNotification -> Most probably a TYM's notification could not be delivered");
			break;
	}
	
	
	WritablePacket *pp = Packet::make(30, NULL, sizeof (char) /*type*/ + sizeof (char) /*id length*/ + fullID.length() /*id*/+ sizeof (char) /*type2*/ 
		+ sizeof (char) /*strategy*/, 0);
	type = CONTROL_REQ_FAILURE;
	memcpy(pp->data(), &type, sizeof (char));
	memcpy(pp->data() + sizeof (unsigned char), &fullIDLen, sizeof (unsigned char));
	memcpy(pp->data() + sizeof (unsigned char) + sizeof (unsigned char), fullID.c_str(), fullIDLen * PURSUIT_ID_LEN);
	memcpy(pp->data() + sizeof (unsigned char) + sizeof (unsigned char)+ fullIDLen * PURSUIT_ID_LEN, &type2, sizeof(char));
	memcpy(pp->data() + sizeof (unsigned char) + sizeof (unsigned char)+ fullIDLen * PURSUIT_ID_LEN + sizeof(char), &strategy, sizeof(char));
	
	if (ap != 0)
		for (HashTable<LocalHost*, unsigned char>::iterator set_it = ap->publishers.begin(); set_it != ap->publishers.end(); set_it++) {
			click_chatter("Found publisher of request with id %s %d - packet size %d", (set_it)->first->localHostID.c_str(), (set_it)->first->id, pp->length());
			pp->set_anno_u32(0, (set_it)->first->id);
			output(0).push(pp);		
		}
	else if (as != 0)
		for (HashTable<LocalHostSetItem>::iterator set_it = as->subscribers.begin(); set_it != as->subscribers.end(); set_it++) {
			//click_chatter("Found publisher of request with id %s %d - packet size %d", (set_it)->first->localHostID.c_str(), (set_it)->first->id, pp->length());
			pp->set_anno_u32(0, (set_it)->_lhpointer->id);
			output(0).push(pp);		
		}	
	
	return;
}

void LocalProxy::handleRVNotification(Packet *p) {
	unsigned char type, numberOfIDs, IDLength/*in fragments of PURSUIT_ID_LEN each*/;
	unsigned int index = 0;
	Vector<String> IDs;
	ActivePublication *ap;
	BABitvector FID;
	unsigned char numberOfiSubscribers;
	String isubscriber;
	ActiveNode *an;
	BABitvector FID_to_isubscriber;
	type = *(p->data());
	if (type != UPDATE_FID_iSUB) {
		numberOfIDs = *(p->data() + sizeof (type));
		for (int i = 0; i < (int) numberOfIDs; i++) {
			IDLength = *(p->data() + sizeof (type) + sizeof (numberOfIDs) + index);
			IDs.push_back(String((const char *) (p->data() + sizeof (type) + sizeof (numberOfIDs) + sizeof (IDLength) + index), IDLength * PURSUIT_ID_LEN));
			index = index + sizeof (IDLength) + IDLength*PURSUIT_ID_LEN;
		}
	}
	switch (type) {
			case SCOPE_PUBLISHED:
			//click_chatter("Received notification about new scope");
			/*Find the applications to forward the notification*/
			for (int i = 0; i < (int) numberOfIDs; i++) {
				/*check the active scope subscriptions for that*/
				/*prefix-match checking here*/
				/*I should create a set of local subscribers to notify*/
				LocalHostSet local_subscribers_to_notify;
				findActiveSubscriptions(IDs[i], local_subscribers_to_notify);
				for (LocalHostSetIter set_it = local_subscribers_to_notify.begin(); set_it != local_subscribers_to_notify.end(); set_it++) {
					//click_chatter("LocalProxy: notifying Subscriber: %s", (*set_it)._lhpointer->localHostID.c_str());
					/*send the message*/
					sendNotificationLocally(SCOPE_PUBLISHED, (*set_it)._lhpointer, IDs[i]);
				}
			}
			break;
			case SCOPE_UNPUBLISHED:
			//click_chatter("Received notification about a deleted scope");
			/*Find the applications to forward the notification*/
			for (int i = 0; i < (int) numberOfIDs; i++) {
				/*check the active scope subscriptions for that*/
				/*prefix-match checking here*/
				/*I should create a set of local subscribers to notify*/
				LocalHostSet local_subscribers_to_notify;
				findActiveSubscriptions(IDs[i], local_subscribers_to_notify);
				for (LocalHostSetIter set_it = local_subscribers_to_notify.begin(); set_it != local_subscribers_to_notify.end(); set_it++) {
					//click_chatter("LocalProxy: notifying Subscriber: %s", (*set_it)._lhpointer->localHostID.c_str());
					/*send the message*/
					sendNotificationLocally(SCOPE_UNPUBLISHED, (*set_it)._lhpointer, IDs[i]);
				}
			}
			break;
			case START_PUBLISH:
			FID = BABitvector(FID_LEN * 8);
			memcpy(FID._data, p->data() + sizeof (type) + sizeof (numberOfIDs) + index, FID_LEN);
			//click_chatter("LocalProxy: RECEIVED FID: %s, number of IDs: %u\n", FID.to_string().c_str(), numberOfIDs);
			for (int i = 0; i < (int) numberOfIDs; i++) {
				ap = activePublicationIndex.get(IDs[i]);
				if (ap != activePublicationIndex.default_value()) {
					/*copy the IDs vector to the allKnownIDs vector of the ap*/
					ap->allKnownIDs = IDs;
					/*this item exists*/
					ap->FID_to_subscribers = FID;
				}
			}
			/*notify the first publisher you find*/
			for (int i = 0; i < (int) numberOfIDs; i++) {
				ap = activePublicationIndex.get(IDs[i]);
				if (ap != activePublicationIndex.default_value()) {
					/*iterate once to see if any of the publishers for this item (which may be represented by many ids) is already notified*/
					for (PublisherHashMapIter publishers_it = ap->publishers.begin(); publishers_it != ap->publishers.end(); publishers_it++) {
						(*publishers_it).second = START_PUBLISH;
						//click_chatter("LocalProxy: IDs[i] is: %s\n", IDs[i].quoted_hex().c_str());
						sendNotificationLocally(START_PUBLISH, (*publishers_it).first, IDs[i]);
						break;
					}
				}
			}
			break;
			case UPDATE_FID:
		{
			FID = BABitvector(FID_LEN * 8);
			memcpy(FID._data, p->data() + sizeof (type) + sizeof (numberOfIDs) + index, FID_LEN);
			//click_chatter("LocalProxy: RECEIVED FID, but only update: %s, number of IDs: %u\n", FID.to_string().c_str(), numberOfIDs);
			for (int i = 0; i < (int) numberOfIDs; i++) {
				ap = activePublicationIndex.get(IDs[i]);
				if (ap != activePublicationIndex.default_value()) {
					/*copy the IDs vector to the allKnownIDs vector of the ap*/
					ap->allKnownIDs = IDs;
					/*this item exists*/
					ap->FID_to_subscribers = FID;
				}
			}
            /*notify the first publisher you find*/
            for (int i = 0; i < (int) numberOfIDs; i++) {
                ap = activePublicationIndex.get(IDs[i]);
                if (ap != activePublicationIndex.default_value()) {
                    /*iterate once to see if any of the publishers for this item (which may be represented by many ids) is already notified*/
                    for (PublisherHashMapIter publishers_it = ap->publishers.begin(); publishers_it != ap->publishers.end(); publishers_it++) {
                        if ((*publishers_it).second == START_PUBLISH) {
                            //click_chatter("LocalProxy: IDs[i] is: %s\n", IDs[i].quoted_hex().c_str());
                            sendNotificationLocally(UPDATE_PUBLISH, (*publishers_it).first, IDs[i]);
                        }
                        
                        break;
                    }
                }
            }
			break;
		}
			/*Update RV and TM FIDs as part of path management and resilience*/
			case UPDATE_RVFID:
		{
			BABitvector up_RVFID;
			up_RVFID = BABitvector(FID_LEN * 8);
			memcpy(up_RVFID._data, p->data() + sizeof (type), FID_LEN);
			gc->defaultRV_dl = up_RVFID;
			click_chatter("LocalProxy: UPDATE RVFID :%s \n\n", gc->defaultRV_dl.to_string().c_str());
			break;
		}
			case UPDATE_TMFID:
		{
			BABitvector up_TMFID;
			up_TMFID = BABitvector(FID_LEN * 8);
			memcpy(up_TMFID._data, p->data() + sizeof (type), FID_LEN);
			gc->TMFID = up_TMFID;
			click_chatter("LocalProxy: UPDATE TMFID  :%s \n", gc->TMFID.to_string().c_str());
			break;
		}
			case STOP_PUBLISH:
		{
			for (int i = 0; i < (int) numberOfIDs; i++) {
				ap = activePublicationIndex.get(IDs[i]);
				if (ap != activePublicationIndex.default_value()) {
					ap->allKnownIDs = IDs;
					/*update the FID to the all zero FID*/
					ap->FID_to_subscribers = BABitvector(FID_LEN * 8);
					/*iterate once to see if any the publishers for this item (which may be represented by many ids) is already notified*/
					for (PublisherHashMapIter publishers_it = ap->publishers.begin(); publishers_it != ap->publishers.end(); publishers_it++) {
						if ((*publishers_it).second == START_PUBLISH) {
							(*publishers_it).second = STOP_PUBLISH;
							sendNotificationLocally(STOP_PUBLISH, (*publishers_it).first, IDs[i]);
						}
					}
				}
			}
			break;
		}
			case UPDATE_FID_iSUB:
		{
			numberOfiSubscribers = *(p->data() + sizeof (type));
			click_chatter("LocalProxy: recieved an FID for %u implicit Subscriptions", (int)numberOfiSubscribers);
			index = 0;
			for (int i = 0; i < (int) numberOfiSubscribers; i++) {
				isubscriber = String((const char *)(p->data() + sizeof(type) + sizeof (numberOfiSubscribers) + index), NODEID_LEN);
				index += NODEID_LEN;
				FID_to_isubscriber = BABitvector(FID_LEN * 8);
				memcpy(FID_to_isubscriber._data, p->data() + sizeof (type) + sizeof (numberOfiSubscribers) + index, FID_LEN);
				index += FID_LEN;
				click_chatter("LocalProxy: recieved FID: %s, for isubscriber: %s\n", FID_to_isubscriber.to_string().c_str(), isubscriber.quoted_hex().c_str());
				an = activeNodeIndex.get(isubscriber);
				if (an != activeNodeIndex.default_value()) {
					an->FID_to_node = FID_to_isubscriber;
                    /*the TM return a non-zero FID (i.e. there is a path to the isubscriber)*/
                    if (!FID_to_isubscriber.zero()) {
                        click_chatter("LocalProxy: non-zero FID added to ActiveNode : %s", isubscriber.quoted_hex().c_str());
                        for (PublisherHashMapIter publishers_it = an->publishers.begin(); publishers_it != an->publishers.end(); publishers_it++) {
                            (*publishers_it).second = START_PUBLISH_iSUB;
                            //click_chatter("LocalProxy: IDs[i] is: %s\n", IDs[i].quoted_hex().c_str());
                            //\TODO: lib introduce new message type and make sure notification is correct.
                            sendNotificationLocally(START_PUBLISH_iSUB, (*publishers_it).first, isubscriber);
                            break;
                        }
                    }
                    /*all-zero FID means there is no working path from publisher to isubscriber*/
                    else {
                        click_chatter("LocalProxy: zero-FID added to ActiveNode : %s", isubscriber.quoted_hex().c_str());
                        for (PublisherHashMapIter publishers_it = an->publishers.begin(); publishers_it != an->publishers.end(); publishers_it++) {
                            (*publishers_it).second = STOP_PUBLISH_iSUB;
                            //click_chatter("LocalProxy: IDs[i] is: %s\n", IDs[i].quoted_hex().c_str());
                            //\TODO: lib introduce new message type and make sure notification is correct.
                            sendNotificationLocally(STOP_PUBLISH_iSUB, (*publishers_it).first, isubscriber);
                            break;
                        }
                    }
				}
			}
			break;
		}
		default:
		{
			click_chatter("LocalProxy: FATAL - didn't understand the RV notification");
			break;
		}
	}
}
void LocalProxy::pushDataToLocalSubscriber(LocalHost *_localhost, String &ID, Packet *p /*p contains only the data and has some headroom as well*/, unsigned char &APItype) {
	unsigned char IDLength;
	unsigned char type = APItype;
	WritablePacket *newPacket;
	IDLength = ID.length() / PURSUIT_ID_LEN;
	//click_chatter("pushing data to subscriber %s", _localhost->localHostID.c_str());
#if !CLICK_NS
	/*no need to push the size of type as type is reintroduced in the publish_data packet*/
	newPacket = p->push(sizeof (unsigned char) + sizeof (unsigned char) +ID.length());
	memcpy(newPacket->data(), &type, sizeof (unsigned char));
	memcpy(newPacket->data() + sizeof (unsigned char), &IDLength, sizeof (unsigned char));
	memcpy(newPacket->data() + sizeof (unsigned char) + sizeof (unsigned char), ID.c_str(), ID.length());
	if (_localhost->type == CLICK_ELEMENT) {
		output(_localhost->id).push(newPacket);
	} else {
		newPacket->set_anno_u32(0, _localhost->id);
		output(0).push(newPacket);
	}
#else
	if (_localhost->type == CLICK_ELEMENT) {
		newPacket = p->push(sizeof (unsigned char) + sizeof (unsigned char) +ID.length());
		memcpy(newPacket->data(), &type, sizeof (unsigned char));
		memcpy(newPacket->data() + sizeof (unsigned char), &IDLength, sizeof (unsigned char));
		memcpy(newPacket->data() + sizeof (unsigned char) + sizeof (unsigned char), ID.c_str(), ID.length());
		output(_localhost->id).push(newPacket);
	} else {
		newPacket = p->push(sizeof (_localhost->id) + sizeof (unsigned char) + sizeof (unsigned char) +ID.length());
		memcpy(newPacket->data(), &_localhost->id, sizeof (_localhost->id));
		memcpy(newPacket->data() + sizeof (_localhost->id), &type, sizeof (unsigned char));
		memcpy(newPacket->data() + sizeof (_localhost->id) + sizeof (unsigned char), &IDLength, sizeof (unsigned char));
		memcpy(newPacket->data() + sizeof (_localhost->id) + sizeof (unsigned char) + sizeof (unsigned char), ID.c_str(), ID.length());
		output(0).push(newPacket);
        click_chatter("LocalProxy: Pushed packet to Subscriber");
	}
	/*mfhaln: TODO, check if you need to kill the original (p) packet here.*/
#endif
}
/* This is a legacy method that has been overriden by the next method, will be removed in subsequent releases
 this method is quite different from the one above
 it will forward the data using the provided FID
 Here, we only know about a single ID.We do not care if there are multiple IDs*/
void LocalProxy::pushDataToRemoteSubscribers(Vector<String> &IDs, BABitvector &FID_to_subscribers, Packet *p) {
	WritablePacket *newPacket;
	unsigned char IDLength = 0;
	int index;
	unsigned char numberOfIDs;
	int totalIDsLength = 0;
	Vector<String>::iterator it;
	numberOfIDs = (unsigned char) IDs.size();
	for (it = IDs.begin(); it != IDs.end(); it++) {
		totalIDsLength = totalIDsLength + (*it).length();
	}
	newPacket = p->push(FID_LEN + sizeof (numberOfIDs) /*number of ids*/+((int) numberOfIDs) * sizeof (unsigned char) /*id length*/ +totalIDsLength);
	memcpy(newPacket->data(), FID_to_subscribers._data, FID_LEN);
	memcpy(newPacket->data() + FID_LEN, &numberOfIDs, sizeof (numberOfIDs));
	index = 0;
	it = IDs.begin();
	for (int i = 0; i < (int) numberOfIDs; i++) {
		IDLength = (unsigned char) (*it).length() / PURSUIT_ID_LEN;
		memcpy(newPacket->data() + FID_LEN + sizeof (numberOfIDs) + index, &IDLength, sizeof (IDLength));
		memcpy(newPacket->data() + FID_LEN + sizeof (numberOfIDs) + index + sizeof (IDLength), (*it).c_str(), (*it).length());
		index = index + sizeof (IDLength) + (*it).length();
		it++;
	}
	output(2).push(newPacket);
	/*mfhaln: TODO, check if you need to kill the original (p) packet here.*/
}
/*push data to remote subscribers with the approperiate type, i.e. PUBLISH_DATA or PUBLISH_DATA_iSUB*/
void LocalProxy::pushDataToRemoteSubscribers(Vector<String> &IDs, unsigned char &type, BABitvector &FID_to_subscribers, Packet *p) {
	WritablePacket *newPacket;
	unsigned char IDLength = 0;
	int index;
	unsigned char numberOfIDs;
	int totalIDsLength = 0;
	Vector<String>::iterator it;
	numberOfIDs = (unsigned char) IDs.size();
	for (it = IDs.begin(); it != IDs.end(); it++) {
		totalIDsLength = totalIDsLength + (*it).length();
	}
	switch (type) {
			case PUBLISH_DATA:
		{
			newPacket = Packet::make(30, NULL, FID_LEN + sizeof (numberOfIDs) + ((int) numberOfIDs) * sizeof (unsigned char) /*id length*/ +totalIDsLength + sizeof (unsigned char) /*request type*/ + p->length(), 30);
			//	newPacket = p->push(FID_LEN + sizeof (numberOfIDs) /*number of ids*/+((int) numberOfIDs) * sizeof (unsigned char) /*id length*/ +totalIDsLength + sizeof (unsigned char) /*request type*/);
			memcpy(newPacket->data(), FID_to_subscribers._data, FID_LEN);
			memcpy(newPacket->data() + FID_LEN, &numberOfIDs, sizeof (numberOfIDs));
			index = 0;
			it = IDs.begin();
			for (int i = 0; i < (int) numberOfIDs; i++) {
				IDLength = (unsigned char) (*it).length() / PURSUIT_ID_LEN;
				memcpy(newPacket->data() + FID_LEN + sizeof (numberOfIDs) + index, &IDLength, sizeof (IDLength));
				memcpy(newPacket->data() + FID_LEN + sizeof (numberOfIDs) + index + sizeof (IDLength), (*it).c_str(), (*it).length());
				index = index + sizeof (IDLength) + (*it).length();
				it++;
			}
			/* copy the request type */
			memcpy(newPacket->data() + FID_LEN + sizeof (numberOfIDs) + index, &type, sizeof (unsigned char));
			memcpy(newPacket->data() + FID_LEN + sizeof (numberOfIDs) + index + sizeof (type), p->data(), p->length());
            /*p is an obselete packet now, so kill it to free its memory, carefull, this is only fine here in the publish_data clause. for isub the packet is still needed in the push function and it will be killed there, after completion*/
            p->kill();
		}
			break;
			case PUBLISH_DATA_iSUB:
		{
			newPacket = Packet::make(30, NULL, FID_LEN + sizeof (numberOfIDs) + ((int) numberOfIDs) * sizeof (unsigned char) /*id length*/ +totalIDsLength + sizeof (unsigned char) /*request type*/ + NODEID_LEN /*gc->nodeID*/ + p->length(), 30);
			//	newPacket = p->push(FID_LEN + sizeof (numberOfIDs) /*number of ids*/+((int) numberOfIDs) * sizeof (unsigned char) /*id length*/ +totalIDsLength + sizeof (unsigned char) /*request type*/);
			memcpy(newPacket->data(), FID_to_subscribers._data, FID_LEN);
			memcpy(newPacket->data() + FID_LEN, &numberOfIDs, sizeof (numberOfIDs));
			index = 0;
			it = IDs.begin();
			for (int i = 0; i < (int) numberOfIDs; i++) {
				IDLength = (unsigned char) (*it).length() / PURSUIT_ID_LEN;
				memcpy(newPacket->data() + FID_LEN + sizeof (numberOfIDs) + index, &IDLength, sizeof (IDLength));
				memcpy(newPacket->data() + FID_LEN + sizeof (numberOfIDs) + index + sizeof (IDLength), (*it).c_str(), (*it).length());
				index = index + sizeof (IDLength) + (*it).length();
				it++;
			}
			/* copy the request type */
			memcpy(newPacket->data() + FID_LEN + sizeof (numberOfIDs) + index, &type, sizeof (unsigned char));
			memcpy(newPacket->data() + FID_LEN + sizeof (numberOfIDs) + index + sizeof (type), gc->nodeID.c_str(), NODEID_LEN);
			memcpy(newPacket->data() + FID_LEN + sizeof (numberOfIDs) + index + sizeof (type) + NODEID_LEN, p->data(), p->length());
		}
			break;
		default:
			break;
	}
	/*push the new packet to the network*/
	output(2).push(newPacket);
    /*don't kill the original packet as it is still needed in the push function to store the isubscription, it will be killed there after completion*/
}
void LocalProxy::handleNetworkPublication(Vector<String> &IDs, Packet *p /*the packet has some headroom and only the data which hasn't been copied yet*/, unsigned char &APItype) {
	LocalHostStringHashMap localSubscribers;
	int counter = 1;
//	click_chatter("received data for ID: %s", IDs[0].quoted_hex().c_str());
	bool foundLocalSubscribers = findLocalSubscribers(IDs, localSubscribers);
	int localSubscribersSize = localSubscribers.size();
	if (foundLocalSubscribers) {
//        click_chatter("LocalProxy: found Subscribers");
		for (LocalHostStringHashMapIter localSubscribers_it = localSubscribers.begin(); localSubscribers_it != localSubscribers.end(); localSubscribers_it++) {
			if (counter == localSubscribersSize) {
				/*don't clone the packet since this is the last subscriber*/
				pushDataToLocalSubscriber((*localSubscribers_it).first, (*localSubscribers_it).second, p, APItype);
			} else {
				pushDataToLocalSubscriber((*localSubscribers_it).first, (*localSubscribers_it).second, p->clone()->uniqueify(), APItype);
			}
			counter++;
		}
	} else {
		p->kill();
	}
}
/*the method handles user publications where it needs to pass the APItype only*/
void LocalProxy::handleUserPublication(String &ID, Packet *p /*the packet has some headroom and only the data which hasn't been copied yet*/, LocalHost *__localhost, unsigned char APItype) {
	int localSubscribersSize;
	int counter = 1;
	bool remoteSubscribersExist = true;
	bool useFatherFID = false;
	unsigned char notification;
	Vector<String> IDs;
	LocalHostStringHashMap localSubscribers;
	ActivePublication *ap = activePublicationIndex.get(ID);
	if (ap == activePublicationIndex.default_value()) {
		/*check a FID is assigned to the father item - used for fragmentation*/
		ap = activePublicationIndex.get(ID.substring(0, ID.length() - PURSUIT_ID_LEN));
		useFatherFID = true;
		IDs.push_back(ID);
	}
	if ((ap != activePublicationIndex.default_value()) && (ap->FID_to_subscribers != NULL)) {
		if ((ap->FID_to_subscribers.zero()) || (ap->FID_to_subscribers == gc->iLID)) {
			remoteSubscribersExist = false;
		}
		/*I have to find any subscribers that exist locally*/
		/*Careful: I will use all known IDs of the ap and check for each one (findLocalSubscribers() does that)*/
		if (useFatherFID == true) {
			findLocalSubscribers(IDs, localSubscribers);
		} else {
			findLocalSubscribers(ap->allKnownIDs, localSubscribers);
		}
		/*the next line is because a publisher of publication cannot be subscriber of it*/
		localSubscribers.erase(__localhost);
		localSubscribersSize = localSubscribers.size();
		
		/*Now I know if I should send the packet to the Network and how many local subscribers exist*/
		/*I should be able to minimise packet copy*/
		if ((localSubscribersSize == 0) && (!remoteSubscribersExist)) {
			p->kill();
		} else if ((localSubscribersSize == 0) && (remoteSubscribersExist)) {
			/*no need to clone..packet will be sent only to the network*/
			if (useFatherFID == true) {
				pushDataToRemoteSubscribers(IDs, ap->FID_to_subscribers, p);
			} else {
				pushDataToRemoteSubscribers(ap->allKnownIDs, ap->FID_to_subscribers, p);
			}
		} else if ((localSubscribersSize > 0) && (!remoteSubscribersExist)) {
			/*only local subscribers exist*/
			for (LocalHostStringHashMapIter localSubscribers_it = localSubscribers.begin(); localSubscribers_it != localSubscribers.end(); localSubscribers_it++) {
				LocalHost *_localhost = (*localSubscribers_it).first;
				if (counter == localSubscribersSize) {
					/*don't clone the packet since this is the last subscriber*/
					pushDataToLocalSubscriber(_localhost, ID, p, APItype);
				} else {
					pushDataToLocalSubscriber(_localhost, ID, p->clone()->uniqueify(), APItype);
				}
				counter++;
			}
		} else {
			/*local and remote subscribers exist*/
			if (useFatherFID == true) {
				pushDataToRemoteSubscribers(IDs, ap->FID_to_subscribers, p->clone()->uniqueify());
			} else {
				pushDataToRemoteSubscribers(ap->allKnownIDs, ap->FID_to_subscribers, p->clone()->uniqueify());
			}
			for (LocalHostStringHashMapIter localSubscribers_it = localSubscribers.begin(); localSubscribers_it != localSubscribers.end(); localSubscribers_it++) {
				LocalHost *_localhost = (*localSubscribers_it).first;
				if (counter == localSubscribersSize) {
					/*don't clone the packet since this is the last subscriber*/
					pushDataToLocalSubscriber(_localhost, ID, p, APItype);
				} else {
					pushDataToLocalSubscriber(_localhost, ID, p->clone()->uniqueify(), APItype);
				}
				counter++;
			}
		}
	}
	else if ((ap != activePublicationIndex.default_value()) && (ap->FID_to_subscribers == NULL)){
		//click_chatter("LocalProxy: No valid FID for ActivePublication %s", ID.quoted_hex().c_str());
		notification = ap->publishers.get(__localhost);
		switch (notification) {
				case RE_PUBLISH:
				//click_chatter("LocalProxy: RE_PUBLISH");
				sendNotificationLocally(RE_PUBLISH, __localhost, ID);
				break;
				case PAUSE_PUBLISH:
				//click_chatter("LocalProxy: PAUSE_PUBLISH");
				sendNotificationLocally(PAUSE_PUBLISH, __localhost, ID);
				break;
			default:
				click_chatter("LocalProxy: HUP without type: unknown state: %x", notification);
                p->kill();
				break;
		}
	}
	else {
		p->kill();
	}
}
/*the method handles user publications where it needs to pass both the type and APItype*/
void LocalProxy::handleUserPublication(String &ID, unsigned char &type, Packet *p /*the packet has some headroom and only the data which hasn't been copied yet*/, LocalHost *__localhost, unsigned char APItype) {
	int localSubscribersSize;
	int counter = 1;
	bool remoteSubscribersExist = true;
	bool useFatherFID = false;
	unsigned char notification;
	Vector<String> IDs;
	LocalHostStringHashMap localSubscribers;
	ActivePublication *ap = activePublicationIndex.get(ID);
	if (ap == activePublicationIndex.default_value()) {
		/*check a FID is assigned to the father item - used for fragmentation*/
		ap = activePublicationIndex.get(ID.substring(0, ID.length() - PURSUIT_ID_LEN));
		useFatherFID = true;
		IDs.push_back(ID);
	}
	if ((ap != activePublicationIndex.default_value()) && (ap->FID_to_subscribers != NULL)) {
        if (ap->FID_to_subscribers.zero()) {
			remoteSubscribersExist = false;
		}
		/*I have to find any subscribers that exist locally*/
		/*Careful: I will use all known IDs of the ap and check for each one (findLocalSubscribers() does that)*/
		if (useFatherFID == true) {
			findLocalSubscribers(IDs, localSubscribers);
		} else {
			findLocalSubscribers(ap->allKnownIDs, localSubscribers);
		}
		/*the next line is because a publisher of publication cannot be subscriber of it*/
		localSubscribers.erase(__localhost);
		localSubscribersSize = localSubscribers.size();
		
		/*Now I know if I should send the packet to the Network and how many local subscribers exist*/
		/*I should be able to minimise packet copy*/
		if ((localSubscribersSize == 0) && (!remoteSubscribersExist)) {
			p->kill();
		} else if ((localSubscribersSize == 0) && (remoteSubscribersExist)) {
			/*no need to clone..packet will be sent only to the network*/
			if (useFatherFID == true) {
				pushDataToRemoteSubscribers(IDs, type, ap->FID_to_subscribers, p);
			} else {
				pushDataToRemoteSubscribers(ap->allKnownIDs, type, ap->FID_to_subscribers, p);
			}
		} else if ((localSubscribersSize > 0) && (!remoteSubscribersExist)) {
			/*only local subscribers exist*/
			for (LocalHostStringHashMapIter localSubscribers_it = localSubscribers.begin(); localSubscribers_it != localSubscribers.end(); localSubscribers_it++) {
				LocalHost *_localhost = (*localSubscribers_it).first;
				if (counter == localSubscribersSize) {
					/*don't clone the packet since this is the last subscriber*/
					pushDataToLocalSubscriber(_localhost, ID, p, APItype);
				} else {
					pushDataToLocalSubscriber(_localhost, ID, p->clone()->uniqueify(), APItype);
				}
				counter++;
			}
		} else {
			/*local and remote subscribers exist*/
			if (useFatherFID == true) {
				pushDataToRemoteSubscribers(IDs, type, ap->FID_to_subscribers, p->clone()->uniqueify());
			} else {
				pushDataToRemoteSubscribers(ap->allKnownIDs, type, ap->FID_to_subscribers, p->clone()->uniqueify());
			}
			for (LocalHostStringHashMapIter localSubscribers_it = localSubscribers.begin(); localSubscribers_it != localSubscribers.end(); localSubscribers_it++) {
				LocalHost *_localhost = (*localSubscribers_it).first;
				if (counter == localSubscribersSize) {
					/*don't clone the packet since this is the last subscriber*/
					pushDataToLocalSubscriber(_localhost, ID, p, APItype);
				} else {
					pushDataToLocalSubscriber(_localhost, ID, p->clone()->uniqueify(), APItype);
				}
				counter++;
			}
		}
	}
	else if ((ap != activePublicationIndex.default_value()) && (ap->FID_to_subscribers == NULL)) {
		//click_chatter("LocalProxy: No valid FID for ActivePublication %s", ID.quoted_hex().c_str());
		notification = ap->publishers.get(__localhost);
		switch (notification) {
            case RE_PUBLISH:
				//click_chatter("LocalProxy: RE_PUBLISH");
				sendNotificationLocally(RE_PUBLISH, __localhost, ID);
				break;
            case PAUSE_PUBLISH:
				//click_chatter("LocalProxy: PAUSE_PUBLISH");
				sendNotificationLocally(PAUSE_PUBLISH, __localhost, ID);
				break;
            case STOP_PUBLISH:
                click_chatter("LocalProxy: IID set to STOP_PUBLISH, either set by the App because of duplicate publish_info or waiting for FID from the TM, so do nothing for the moment");
                break;
			default:
				click_chatter("LocalProxy: HUP with type: unknown state: %x", notification);
                click_chatter("LocalProxy: HUP with API type: unknown state: %x", APItype);
                click_chatter("LocalProxy: IID of unknown packet is: %s", ID.quoted_hex().c_str());
                p->kill();
				break;
		}
	}
	else {
		p->kill();
	}
}
/*this method is quite different...I will check if there is any active subscription for that item.
 * if there is one, I will also forward the data to the local subscribers
 * If not I will forward the data to the network using the application provided FID
 *the method handle user publications where it needs to pass the APItype only
 */
void LocalProxy::handleUserPublication(String &ID, BABitvector &FID_to_subscribers, Packet *p, LocalHost *__localhost, unsigned char APItype) {
	int counter = 1;
	int localSubscribersSize;
	LocalHostStringHashMap localSubscribers;
	Vector<String> IDs;
	ActivePublication *ap = activePublicationIndex.get(ID.substring(0, ID.length() - PURSUIT_ID_LEN));
	/*i will augment the IDs vector using my father publication*/
	if (ap != activePublicationIndex.default_value()) {
		for (int i = 0; i < ap->allKnownIDs.size(); i++) {
			String knownID = ap->allKnownIDs[i] + ID.substring(ID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
			if (knownID.compare(ID) != 0) {
				IDs.push_back(knownID);
			}
		}
	}
	IDs.push_back(ID);
	/*I have to find any subscribers that exist locally*/
	findLocalSubscribers(IDs, localSubscribers);
	localSubscribers.erase(__localhost);
	localSubscribersSize = localSubscribers.size();
	if (localSubscribersSize == 0) {
		/*no need to clone..packet will be sent only to the network*/
		pushDataToRemoteSubscribers(IDs, FID_to_subscribers, p);
	} else {
		/*local and remote subscribers exist*/
		if (gc->TMFID != NULL) {
			/*\TODO: justify or edit this condition, as it prevents sending data to the TM node (as subscriber) if local subscribers exists - question is why?*/
			if (FID_to_subscribers != gc->TMFID) {
				pushDataToRemoteSubscribers(IDs, FID_to_subscribers, p->clone()->uniqueify());
			}
		} else {
			pushDataToRemoteSubscribers(IDs, FID_to_subscribers, p->clone()->uniqueify());
		}
		for (LocalHostStringHashMapIter localSubscribers_it = localSubscribers.begin(); localSubscribers_it != localSubscribers.end(); localSubscribers_it++) {
			LocalHost *_localhost = (*localSubscribers_it).first;
			if (counter == localSubscribersSize) {
				//click_chatter("/*don't clone the packet since this is the last subscriber*/");
				pushDataToLocalSubscriber(_localhost, ID, p, APItype);
			} else {
				pushDataToLocalSubscriber(_localhost, ID, p->clone()->uniqueify(), APItype);
			}
			counter++;
		}
	}
}
/*handle user publications where it needs to pass both the type and APItype*/
void LocalProxy::handleUserPublication(String &ID, unsigned char &type,  BABitvector &FID_to_subscribers, Packet *p, LocalHost *__localhost, unsigned char APItype) {
	/*find better approach/function to passing the API type - API type is need to distiguish between publish_data and publish_data_isub*/
	int counter = 1;
	int localSubscribersSize;
	LocalHostStringHashMap localSubscribers;
	Vector<String> IDs;
	ActivePublication *ap = activePublicationIndex.get(ID.substring(0, ID.length() - PURSUIT_ID_LEN));
	/*i will augment the IDs vector using my father publication*/
	if (ap != activePublicationIndex.default_value()) {
		for (int i = 0; i < ap->allKnownIDs.size(); i++) {
			String knownID = ap->allKnownIDs[i] + ID.substring(ID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
			if (knownID.compare(ID) != 0) {
				IDs.push_back(knownID);
			}
		}
	}
	IDs.push_back(ID);
	/*I have to find any subscribers that exist locally*/
	findLocalSubscribers(IDs, localSubscribers);
	localSubscribers.erase(__localhost);
	localSubscribersSize = localSubscribers.size();
	if (localSubscribersSize == 0) {
		/*no need to clone..packet will be sent only to the network*/
		pushDataToRemoteSubscribers(IDs, type, FID_to_subscribers, p);
	} else {
		//TODO: the below conditions require revisiting...
		/*local and remote subscribers exist*/
		if (gc->TMFID != NULL) {
			/*\TODO: justify or edit this condition, as it prevents sending data to the TM node (as subscriber) if local subscribers exists - question is why?*/
			if (FID_to_subscribers != gc->TMFID) {
				pushDataToRemoteSubscribers(IDs, type, FID_to_subscribers, p->clone()->uniqueify());
			}
		} else {
			pushDataToRemoteSubscribers(IDs, type, FID_to_subscribers, p->clone()->uniqueify());
		}
		for (LocalHostStringHashMapIter localSubscribers_it = localSubscribers.begin(); localSubscribers_it != localSubscribers.end(); localSubscribers_it++) {
			LocalHost *_localhost = (*localSubscribers_it).first;
			if (counter == localSubscribersSize) {
				//click_chatter("/*don't clone the packet since this is the last subscriber*/");
				pushDataToLocalSubscriber(_localhost, ID, p, APItype);
			} else {
				pushDataToLocalSubscriber(_localhost, ID, p->clone()->uniqueify(), APItype);
			}
			counter++;
		}
	}
}
/*The method handles management notifications published by the TM*/
void LocalProxy::handleUserTMPublication(String &ID, unsigned char &type,  BABitvector &FID_to_subscribers, Packet *p, LocalHost *__localhost) {
	int localSubscribersSize;
	LocalHostStringHashMap localSubscribers;
	Vector<String> IDs;
	ActivePublication *ap = activePublicationIndex.get(ID.substring(0, ID.length() - PURSUIT_ID_LEN));
	/*augment the IDs vector using my father publication*/
	if (ap != activePublicationIndex.default_value()) {
		for (int i = 0; i < ap->allKnownIDs.size(); i++) {
			String knownID = ap->allKnownIDs[i] + ID.substring(ID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
			if (knownID.compare(ID) != 0) {
				IDs.push_back(knownID);
			}
		}
	}
	IDs.push_back(ID);
	/*find any subscribers that exist locally*/
	findLocalSubscribers(IDs, localSubscribers);
	localSubscribers.erase(__localhost);
	localSubscribersSize = localSubscribers.size();
	if (localSubscribersSize == 0) {
		/*no need to clone..packet will be sent only to the network*/
		pushDataToRemoteSubscribers(IDs, type, FID_to_subscribers, p);
	} else {
		//TODO: the below conditions require revisiting...
		/*local and remote subscribers exist*/
		if (gc->TMFID != NULL) {
			if (FID_to_subscribers != gc->TMFID) {
				pushDataToRemoteSubscribers(IDs, type, FID_to_subscribers, p->clone()->uniqueify());
			}
		} else {
			pushDataToRemoteSubscribers(IDs, type, FID_to_subscribers, p->clone()->uniqueify());
		}
	}
	processTMNotification(p);
}
/*The method processes management notifications published by the TM*/
void LocalProxy::processTMNotification(Packet * p) {
	bool match = false;
	unsigned int numberOfActivePublications = activePublicationIndex.size();
	unsigned int numberOfActiveNodes = activeNodeIndex.size();
	ActivePubIter ap_it = activePublicationIndex.begin();
	ActiveNodMapIter an_it = activeNodeIndex.begin();
	StringSet affectedNodeIDs;
	BABitvector affected_LID = BABitvector(FID_LEN * 8);
	BABitvector AndResult = BABitvector(FID_LEN * 8);
	memcpy(affected_LID._data, p->data(), FID_LEN);
	click_chatter("LocalProxy: affected LID: %s", affected_LID.to_string().c_str());
	p->kill();
	/*first check the ActivePublication state (i.e. ICNID -> FID) mainly in cNAP to see if any FIDs are affected*/
	for (unsigned int i = 0; i < numberOfActivePublications; i++){
		/*only information publications will require republishing, scopes are not affected becasue only when information is placed under them that an FID will be provided*/
		if (!(*ap_it).second->isScope) {
			/*the publication has been assign an FID*/
			if ((*ap_it).second->FID_to_subscribers != NULL) {
				AndResult = affected_LID & (*ap_it).second->FID_to_subscribers;
				//click_chatter("LocalProxy: AndResult: %s", AndResult.to_string().c_str());
				match = (AndResult == affected_LID);
				/*Affected FID - collect the publication information*/
				if(match){
					/*Carefull here we are assuming that only one identifier of the publication is requried to trigger a FID update for all IDs, since they all belong to the same activepublication*/
					(*ap_it).second->FID_to_subscribers = BABitvector();
					for (PublisherHashMapIter publisher_it = (*ap_it).second->publishers.begin(); publisher_it != (*ap_it).second->publishers.end(); publisher_it++) {
						if ((*publisher_it).second == START_PUBLISH) {
							(*publisher_it).second = RE_PUBLISH;
						}
					}
					//click_chatter("LocalProxy: invalidate FID");
				}
			}
		}
		ap_it++;
	}
	/*************************************************************************************************************/
	/*next check the ActiveNode state (i.e. NodeID -> FID of implicit subscriptions) mainly in sNAP to see if any FIDs are affected*/
	for (unsigned int j = 0; j < numberOfActiveNodes; j++) {
		AndResult = affected_LID & (*an_it).second->FID_to_node;
		match = (AndResult == affected_LID);
		if (match) {
			affectedNodeIDs.find_insert(StringSetItem((*an_it).second->nodeID));
		}
		an_it++;
	}
	if (affectedNodeIDs.size() > 0) {
		/*there are affected paths to isubscribers*/
		publishiReqToRV(affectedNodeIDs);
	}
}
void LocalProxy::handleMappRequest(Packet *p, LocalHost *_localhost) {
    uint8_t type, _type;
    unsigned char status;
    uint32_t length;
    WritablePacket *_p;
    String sent_bytes;
    _type = *(p->data());
    switch (_type) {
        case GET_NODEID:
            type = NODEID_IS;
            length = NODEID_LEN;
            _p = Packet::make(30, NULL, /*response type*/ sizeof (type) + /*length of nodeID*/ sizeof (NODEID_LEN) +  NODEID_LEN/*nodeID*/, 0);
            /************ fill the fields ************/
            memcpy(_p->data(), &type, sizeof (type));
            memcpy(_p->data() + sizeof(type), &length, sizeof (length));
            memcpy(_p->data() + sizeof (type) + sizeof (length), gc->nodeID.c_str(), length);
            /*set the annotation for the to_netlink element*/
            _p->set_anno_u32(0, _localhost->id);
//            click_chatter("LocalProxy: give NodeID to Mapp %s", gc->nodeID.quoted_hex().c_str());
            output(3).push(_p);
            break;
        case GET_RVTMFID:
            type = RVTMFID_IS;
            length = FID_LEN;
            _p = Packet::make(30, NULL, /*response type*/ sizeof (type) + /*length of RVTMFID - up to 256*/ sizeof (FID_LEN) + FID_LEN/*nodeID*/, 0);
            memcpy(_p->data(), &type, sizeof (type));
            memcpy(_p->data() + sizeof (type), &length, sizeof (length));
            memcpy(_p->data() + sizeof (type) + sizeof (length), (char*) gc->defaultRV_dl._data, FID_LEN);
            /*set the annotation for the to_netlink element*/
            _p->set_anno_u32(0, _localhost->id);
//            click_chatter("LocalProxy: give RVTMFID of length %u bytes to Mapp: %s", length, gc->defaultRV_dl.to_string().c_str());
            output(3).push(_p);
            break;
        case SET_CONNECTION_STATUS:
            status = *(p->data() + sizeof (_type));
            gc->status = status;
            click_chatter("LocalProxy: Status changed to %u", status);
            /*This takes similar actions to thaat of handleAppNotification*/
            updateStates(gc->status);
            /*nothing at the moment, just a place holder*/
            break;
        case DISCONNECT:
            disconnect(_localhost);
            p->kill();
            return;
        default:
            click_chatter("LocalProxy: Unknown management type: %x", _type);
            break;
    }
	/*kill the origianl request packet*/
	p->kill();
}

/*handles any required change to the state of node, depending on the connectivity change*/
void LocalProxy::updateStates(unsigned char &state){
    /*current states*/
    unsigned int numberOfActivePublications = activePublicationIndex.size();
    unsigned int numberOfActiveNodes = activeNodeIndex.size();
    StringSet NodeIDs;
    String ap_ID;
    ActivePubIter ap_it = activePublicationIndex.begin();
    ActiveNodMapIter an_it = activeNodeIndex.begin();
    switch (state) {
        case DISCONNECTED:
            click_chatter("LocalProxy: Disconnected from network");
            gc->defaultRV_dl = BABitvector();
            for (unsigned int i = 0; i < numberOfActivePublications; i++) {
                if (!(*ap_it).second->isScope) {
                    (*ap_it).second->FID_to_subscribers = BABitvector();
                    for (PublisherHashMapIter publisher_it = (*ap_it).second->publishers.begin(); publisher_it != (*ap_it).second->publishers.end(); publisher_it++) {
                        if ((*publisher_it).second != STOP_PUBLISH) {
                            /*i.e. either START_PUBLISH or RE_PUBLISH*/
                            (*publisher_it).second = PAUSE_PUBLISH;
                        }
                    }
                }
                ap_it++;
            }
            for (unsigned int j = 0; j < numberOfActiveNodes; j++) {
                (*an_it).second->FID_to_node = BABitvector();
                an_it++;
            }
            break;
        case RECONNECTED:
            click_chatter("LocalProxy: Reconnected to the network");
            /*recovery caused the node to reconnect to the network - notify paused Apps to resume their operation*/
            for (unsigned int i = 0; i < numberOfActivePublications; i++) {
                if (!(*ap_it).second->isScope) {
                    for (PublisherHashMapIter publisher_it = (*ap_it).second->publishers.begin(); publisher_it != (*ap_it).second->publishers.end(); publisher_it++) {
                        if ((*publisher_it).second == PAUSE_PUBLISH) {
                            /*i.e. either START_PUBLISH or RE_PUBLISH*/
                            (*publisher_it).second = RESUME_PUBLISH;
                            ap_ID = (*ap_it).second->allKnownIDs[0];
                            click_chatter("LocalProxy: resume operation with ID: %s using FID: %s", ap_ID.quoted_hex().c_str(), (*ap_it).second->FID_to_subscribers.to_string().c_str());
                            sendNotificationLocally(RESUME_PUBLISH, (*publisher_it).first, ap_ID);
                        }
                    }
                }
                ap_it++;
            }
            /*request new FIDs for all implicit subscribers*/
            for (unsigned int j = 0; j < numberOfActiveNodes; j++) {
                NodeIDs.find_insert(StringSetItem((*an_it).second->nodeID));
                an_it++;
            }
            if (numberOfActiveNodes > 0) {
                /*there are affected paths to isubscribers*/
                publishiReqToRV(NodeIDs);
            }
            break;
        default:
            click_chatter("Unkown connectivity state %c", state);
            break;
    }

}
/*the method handles implicit multicast requests, whereby set of nodes is provided by the app (mainly the NAP)*/
void LocalProxy::handleUserMulticastRequest(String &ID, unsigned char &type, Packet *p /*the packet has some headroom and only the data which hasn't been copied yet*/, StringSet &isubscriberIDs) {
	bool useFatherFID = false;
	Vector<String> IDs;
	String isubscribers;
	ActivePublication *ap = activePublicationIndex.get(ID);
	ActiveNode *an;
	BABitvector FID = BABitvector(FID_LEN * 8);
	/*need to set ap->allKnownIDs anyway with IDs, so took the below line outisde the default_value() condition*/
	IDs.push_back(ID);
	if (ap == activePublicationIndex.default_value()) {
		/*check a FID is assigned to the father item - used for fragmentation*/
		ap = activePublicationIndex.get(ID.substring(0, ID.length() - PURSUIT_ID_LEN));
		if (ap != activePublicationIndex.default_value()) {
			useFatherFID = true;
		} else {
			click_chatter("LocalProxy: Error, father scope does not exist.");
			p->kill();
		}
	}
	if (ap != activePublicationIndex.default_value()) {
		/*set ap->allKnownIDs to IDs because Multicast is called only when there is implicit subscription of published info - i.e. equivalent to START_PUBLISH signal from the RV*/
		ap->allKnownIDs = IDs;
		/*find all unicast FIDs of the Set of iSubscribers and OR them in a multicast FID*/
		for (StringSetIter it = isubscriberIDs.begin(); it != isubscriberIDs.end(); it++) {
			an = activeNodeIndex.get((*it)._strData);
			if (an != activeNodeIndex.default_value()) {
				FID = FID | an->FID_to_node;
				isubscribers +=  " " + an->nodeID;
			}
		}
		/*Now I know if I should send the packet to the Network*/
		/*I should be able to minimise packet copy*/
		/*no need to clone..packet will be sent only to the network*/
		if (useFatherFID == true) {
//			click_chatter("LocalProxy: Multicast FID: %s for scope ID %s to nodes: %s", FID.to_string().c_str(), IDs[0].quoted_hex().c_str(), isubscribers.c_str());
			pushDataToRemoteSubscribers(IDs, type, FID, p);
		} else {
//			click_chatter("LocalProxy: Multicast FID: %s for ID: %s", FID.to_string().c_str(), ID.quoted_hex().c_str());
			pushDataToRemoteSubscribers(ap->allKnownIDs, type, FID, p);
		}
	} else {
		p->kill();
	}
}

/*sends the pub/sub request to the local or remote RV*/
void LocalProxy::publishReqToRV(Packet *p, BABitvector &RVFID) {
	WritablePacket *p1, *p2;
	if ((RVFID.zero()) || (gc->type[RV])) {
		/*this should be a request to the RV element running locally*/
		/*This node is the RV point for this request*/
		/*interact using the API - differently than below*/
		/*these events are going to be PUBLISHED_DATA*/
		unsigned char typeOfAPIEvent = PUBLISHED_DATA;
		unsigned char IDLengthOfAPIEvent = gc->nodeRVScope.length() / PURSUIT_ID_LEN;
		/***********************************************************/
		p1 = p->push(sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + gc->nodeRVScope.length());
		memcpy(p1->data(), &typeOfAPIEvent, sizeof (typeOfAPIEvent));
		memcpy(p1->data() + sizeof (typeOfAPIEvent), &IDLengthOfAPIEvent, sizeof (IDLengthOfAPIEvent));
		memcpy(p1->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent), gc->nodeRVScope.c_str(), gc->nodeRVScope.length());
		output(1).push(p1);
	} else {
		/*wrap the request to a publication to /FFFFFFFF/NODE_ID  */
		/*Format: requesttype = (unsigned char) PUBLISHED_DATA, numberOfIDs = (unsigned char) 1, numberOfFragments1 = (unsigned char) 2, ID1 = /FFFFFFFF/NODE_ID*/
		/*this should be a request to the RV element running in some other node*/
		//click_chatter("I will send the request to the domain RV using the FID: %s", RVFID.to_string().c_str());
		unsigned char numberOfIDs = 1;
		unsigned char numberOfFragments = 2;
		/*push the "header" - see above*/
		p1 = p->push(sizeof (unsigned char) + 1 * sizeof (unsigned char) + 2 * PURSUIT_ID_LEN);
		memcpy(p1->data(), &numberOfIDs, sizeof (unsigned char));
		memcpy(p1->data() + sizeof (unsigned char), &numberOfFragments, sizeof (unsigned char));
		memcpy(p1->data() + sizeof (unsigned char) + sizeof (unsigned char), gc->nodeRVScope.c_str(), gc->nodeRVScope.length());
		p2 = p1->push(FID_LEN);
		memcpy(p2->data(), RVFID._data, FID_LEN);
		output(2).push(p2);
	}
}
/*sends the implicit subscription request to the local RV, which will request domain TM assistance*/
void LocalProxy::publishiReqToRV(StringSet &nodeIDs) {
	WritablePacket *p;
	unsigned char numberofNodeIDs = nodeIDs.size();
	int index = 0;
	/*this should be a request to the RV element running locally*/
	/*This node is the RV point for this request*/
	unsigned char typeOfAPIEvent = PUBLISHED_DATA_iSUB;
	unsigned char IDLengthOfAPIEvent = gc->nodeRVScope.length() / PURSUIT_ID_LEN;
	p = Packet::make(50, NULL, /*API*/ sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + gc->nodeRVScope.length() /*payload*/+ sizeof (numberofNodeIDs) + (int) numberofNodeIDs * NODEID_LEN /*nodeIDs*/, 50);
	/***********************************************************/
	memcpy(p->data(), &typeOfAPIEvent, sizeof (typeOfAPIEvent));
	memcpy(p->data() + sizeof (typeOfAPIEvent), &IDLengthOfAPIEvent, sizeof (IDLengthOfAPIEvent));
	memcpy(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent), gc->nodeRVScope.c_str(), gc->nodeRVScope.length());
	memcpy(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + gc->nodeRVScope.length(), &numberofNodeIDs, sizeof (numberofNodeIDs));
	for (StringSetIter it = nodeIDs.begin(); it != nodeIDs.end(); it++) {
		memcpy(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + gc->nodeRVScope.length() + sizeof (numberofNodeIDs) + index, (*it)._strData.c_str(), NODEID_LEN);
		index += NODEID_LEN;
	}
	output(1).push(p);
}

void LocalProxy::findActiveSubscriptions(String &ID, LocalHostSet &local_subscribers_to_notify) {
	ActiveSubscription *as;
	LocalHostSetIter set_it;
	as = activeSubscriptionIndex.get(ID.substring(0, ID.length() - PURSUIT_ID_LEN));
	if (as != activeSubscriptionIndex.default_value()) {
		if (as->isScope) {
			for (set_it = as->subscribers.begin(); set_it != as->subscribers.end(); set_it++) {
				local_subscribers_to_notify.find_insert(*set_it);
			}
		}
	}
}

bool LocalProxy::findLocalSubscribers(Vector<String> &IDs, LocalHostStringHashMap & _localSubscribers) {
	bool foundSubscribers;
	String knownID;
    String prefixID;
	LocalHostSetIter set_it;
	Vector<String>::iterator id_it;
	ActiveSubscription *as;
	foundSubscribers = false;
	/*prefix-match checking here for all known IDS of aiip*/
	for (id_it = IDs.begin(); id_it != IDs.end(); id_it++) {
		knownID = *id_it;
//        click_chatter("LocalProxy: finding subscribers for ID: %s", knownID.quoted_hex().c_str());
		/*check for local subscription for the specific information item*/
		as = activeSubscriptionIndex.get(knownID);
		if (as != activeSubscriptionIndex.default_value()) {
			for (set_it = as->subscribers.begin(); set_it != as->subscribers.end(); set_it++) {
				_localSubscribers.set((*set_it)._lhpointer, knownID);
				foundSubscribers = true;
			}
		}
        prefixID = knownID.substring(0, knownID.length() - PURSUIT_ID_LEN);
//        click_chatter("LocalProxy: finding subscribers for PrefixID: %s", prefixID.quoted_hex().c_str());
		as = activeSubscriptionIndex.get(knownID.substring(0, knownID.length() - PURSUIT_ID_LEN));
		if (as != activeSubscriptionIndex.default_value()) {
			for (set_it = as->subscribers.begin(); set_it != as->subscribers.end(); set_it++) {
				_localSubscribers.set((*set_it)._lhpointer, knownID);
				foundSubscribers = true;
			}
//            click_chatter("LocalProxy: local subscriber found for ID: %s", knownID.quoted_hex().c_str());
		}
	}
	return foundSubscribers;
}

void LocalProxy::sendNotificationLocally(unsigned char type, LocalHost *_localhost, String ID) {
	WritablePacket *p;
	unsigned char IDLength;
#if !CLICK_NS
	p = Packet::make(30, NULL, sizeof (unsigned char) /*type*/ + sizeof (unsigned char) /*id length*/ +ID.length() /*id*/, 0);
	IDLength = ID.length() / PURSUIT_ID_LEN;
	memcpy(p->data(), &type, sizeof (char));
	memcpy(p->data() + sizeof (unsigned char), &IDLength, sizeof (unsigned char));
	memcpy(p->data() + sizeof (unsigned char) + sizeof (unsigned char), ID.c_str(), IDLength * PURSUIT_ID_LEN);
	if (_localhost->type == CLICK_ELEMENT) {
		output(_localhost->id).push(p);
	} else {
		/*set the annotation for the to_netlink element*/
		p->set_anno_u32(0, _localhost->id);
		//click_chatter("setting annotation: %d", _localhost->id);
		output(0).push(p);
	}
#else
	if (_localhost->type == CLICK_ELEMENT) {
		p = Packet::make(30, NULL, sizeof (unsigned char) /*type*/ + sizeof (unsigned char) /*id length*/ +ID.length() /*id*/, 0);
		IDLength = ID.length() / PURSUIT_ID_LEN;
		memcpy(p->data(), &type, sizeof (char));
		memcpy(p->data() + sizeof (unsigned char), &IDLength, sizeof (unsigned char));
		memcpy(p->data() + sizeof (unsigned char) + sizeof (unsigned char), ID.c_str(), IDLength * PURSUIT_ID_LEN);
		output(_localhost->id).push(p);
	} else {
		p = Packet::make(30, NULL, sizeof (_localhost->id) + sizeof (unsigned char) /*type*/ + sizeof (unsigned char) /*id length*/ +ID.length() /*id*/, 0);
		IDLength = ID.length() / PURSUIT_ID_LEN;
		memcpy(p->data(), &_localhost->id, sizeof (_localhost->id));
		memcpy(p->data() + sizeof (_localhost->id), &type, sizeof (char));
		memcpy(p->data() + sizeof (_localhost->id) + sizeof (unsigned char), &IDLength, sizeof (unsigned char));
		memcpy(p->data() + sizeof (_localhost->id) + sizeof (unsigned char) + sizeof (unsigned char), ID.c_str(), IDLength * PURSUIT_ID_LEN);
		output(0).push(p);
	}
#endif
}

void LocalProxy::createAndSendPacketToRV(unsigned char type, unsigned char IDLength /*in fragments of PURSUIT_ID_LEN each*/, String &ID, unsigned char prefixIDLength /*in fragments of PURSUIT_ID_LEN each*/, String &prefixID, BABitvector &RVFID, unsigned char strategy) {
	WritablePacket *p;
	unsigned char numberOfIDs = 1;
	unsigned char numberOfFragments = 2;
	if ((RVFID.zero()) || (gc->type[RV])) {
		unsigned char typeOfAPIEvent = PUBLISHED_DATA;
		unsigned char IDLengthOfAPIEvent = gc->nodeRVScope.length() / PURSUIT_ID_LEN;
		/***********************************************************/
		p = Packet::make(50, NULL, sizeof (numberOfIDs) + 1 * sizeof (numberOfFragments) + 2 * PURSUIT_ID_LEN + sizeof (type) + sizeof (IDLength) + IDLength * PURSUIT_ID_LEN + sizeof (prefixIDLength) + prefixIDLength * PURSUIT_ID_LEN + sizeof (strategy), 50);
		/*the local RV should always be in output port 1*/
		memcpy(p->data(), &typeOfAPIEvent, sizeof (typeOfAPIEvent));
		memcpy(p->data() + sizeof (typeOfAPIEvent), &IDLengthOfAPIEvent, sizeof (IDLengthOfAPIEvent));
		memcpy(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent), gc->nodeRVScope.c_str(), gc->nodeRVScope.length());
		memcpy(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + gc->nodeRVScope.length(), &type, sizeof (type));
		memcpy(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + gc->nodeRVScope.length() + sizeof (type), &IDLength, sizeof (IDLength));
		memcpy(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + gc->nodeRVScope.length() + sizeof (type) + sizeof (IDLength), ID.c_str(), ID.length());
		memcpy(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + gc->nodeRVScope.length() + sizeof (type) + sizeof (IDLength) + ID.length(), &prefixIDLength, sizeof (prefixIDLength));
		memcpy(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + gc->nodeRVScope.length() + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength), prefixID.c_str(), prefixID.length());
		memcpy(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + gc->nodeRVScope.length() + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength) + prefixID.length(), &strategy, sizeof (strategy));
		output(1).push(p);
	} else {
		p = Packet::make(50, NULL, FID_LEN + sizeof (numberOfIDs) + 1 * sizeof (numberOfFragments) + 2 * PURSUIT_ID_LEN + sizeof (type) + sizeof (IDLength) + IDLength * PURSUIT_ID_LEN + sizeof (prefixIDLength) + prefixIDLength * PURSUIT_ID_LEN + sizeof (strategy), 0);
		memcpy(p->data(), RVFID._data, FID_LEN);
		memcpy(p->data() + FID_LEN, &numberOfIDs, sizeof (unsigned char));
		memcpy(p->data() + FID_LEN + sizeof (unsigned char), &numberOfFragments, sizeof (unsigned char));
		memcpy(p->data() + FID_LEN + sizeof (unsigned char) + sizeof (unsigned char), gc->nodeRVScope.c_str(), 2 * PURSUIT_ID_LEN);
		memcpy(p->data() + FID_LEN + sizeof (unsigned char) + sizeof (unsigned char) + 2 * PURSUIT_ID_LEN, &type, sizeof (type));
		memcpy(p->data() + FID_LEN + sizeof (unsigned char) + sizeof (unsigned char) + 2 * PURSUIT_ID_LEN + sizeof (type), &IDLength, sizeof (IDLength));
		memcpy(p->data() + FID_LEN + sizeof (unsigned char) + sizeof (unsigned char) + 2 * PURSUIT_ID_LEN + sizeof (type) + sizeof (IDLength), ID.c_str(), ID.length());
		memcpy(p->data() + FID_LEN + sizeof (unsigned char) + sizeof (unsigned char) + 2 * PURSUIT_ID_LEN + sizeof (type) + sizeof (IDLength) + ID.length(), &prefixIDLength, sizeof (prefixIDLength));
		memcpy(p->data() + FID_LEN + sizeof (unsigned char) + sizeof (unsigned char) + 2 * PURSUIT_ID_LEN + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength), prefixID.c_str(), prefixID.length());
		memcpy(p->data() + FID_LEN + sizeof (unsigned char) + sizeof (unsigned char) + 2 * PURSUIT_ID_LEN + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength) + prefixID.length(), &strategy, sizeof (strategy));
		output(2).push(p);
	}
}

void LocalProxy::deleteAllActiveInformationItemPublications(LocalHost * _publisher) {
	unsigned char type, IDLength /*in fragments of PURSUIT_ID_LEN each*/, prefixIDLength /*in fragments of PURSUIT_ID_LEN each*/;
	String ID, prefixID;
	bool shouldNotify = false;
	WritablePacket *newPacket;
	int size = _publisher->activePublications.size();
	StringSetIter it = _publisher->activePublications.begin();
	for (int i = 0; i < size; i++) {
		shouldNotify = false;
		ActivePublication *ap = activePublicationIndex.get((*it)._strData);
		if (!ap->isScope) {
			it = _publisher->activePublications.erase(it);
			if (ap->publishers.get(_publisher) != STOP_PUBLISH) {
				shouldNotify = true;
			}
			ap->publishers.erase(_publisher);
			//click_chatter("LocalProxy: deleted publisher %s from Active Information Item Publication %s", _publisher->localHostID.c_str(), ap->fullID.quoted_hex().c_str());
			if (ap->publishers.size() == 0) {
				//click_chatter("LocalProxy: delete Active Information item Publication %s", ap->fullID.quoted_hex().c_str());
				activePublicationIndex.erase(ap->fullID);
				/*notify the RV Function - depending on strategy*/
				type = UNPUBLISH_INFO;
				IDLength = 1;
				prefixIDLength = (ap->fullID.length() - PURSUIT_ID_LEN) / PURSUIT_ID_LEN;
				ID = ap->fullID.substring(ap->fullID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
				prefixID = ap->fullID.substring(0, ap->fullID.length() - PURSUIT_ID_LEN);
				createAndSendPacketToRV(type, IDLength, ID, prefixIDLength, prefixID, ap->RVFID, ap->strategy);
				delete ap;
			} else {
				/*there are other local publishers...check the state of the deleted local publisher and potentially notify one of the other local publishers*/
				if (shouldNotify) {
					/*None of the available local publishers has been previously notified*/
					(*ap->publishers.begin()).second = START_PUBLISH;
					IDLength = ap->fullID.length() / PURSUIT_ID_LEN;
					type = START_PUBLISH;
#if !CLICK_NS
					newPacket = Packet::make(30, NULL, sizeof (unsigned char) /*type*/ + sizeof (unsigned char) /*id length*/ +ap->fullID.length() /*id*/, 0);
					memcpy(newPacket->data(), &type, sizeof (char));
					memcpy(newPacket->data() + sizeof (unsigned char), &IDLength, sizeof (unsigned char));
					memcpy(newPacket->data() + sizeof (unsigned char) + sizeof (unsigned char), ap->fullID.c_str(), ap->fullID.length());
					if ((*ap->publishers.begin()).first->type == CLICK_ELEMENT) {
						output((*ap->publishers.begin()).first->id).push(newPacket);
					} else {
						newPacket->set_anno_u32(0, (*ap->publishers.begin()).first->id);
						output(0).push(newPacket);
					}
#else
					if ((*ap->publishers.begin()).first->type == CLICK_ELEMENT) {
						newPacket = Packet::make(30, NULL, sizeof (unsigned char) /*type*/ + sizeof (unsigned char) /*id length*/ +ap->fullID.length() /*id*/, 0);
						memcpy(newPacket->data(), &type, sizeof (char));
						memcpy(newPacket->data() + sizeof (unsigned char), &IDLength, sizeof (unsigned char));
						memcpy(newPacket->data() + sizeof (unsigned char) + sizeof (unsigned char), ap->fullID.c_str(), ap->fullID.length());
						output((*ap->publishers.begin()).first->id).push(newPacket);
					} else {
						newPacket = Packet::make(30, NULL, sizeof ((*ap->publishers.begin()).first->id) + sizeof (unsigned char) /*type*/ + sizeof (unsigned char) /*id length*/ +ap->fullID.length() /*id*/, 0);
						memcpy(newPacket->data(), &(*ap->publishers.begin()).first->id, sizeof ((*ap->publishers.begin()).first->id));
						memcpy(newPacket->data() + sizeof ((*ap->publishers.begin()).first->id), &type, sizeof (char));
						memcpy(newPacket->data() + sizeof ((*ap->publishers.begin()).first->id) + sizeof (unsigned char), &IDLength, sizeof (unsigned char));
						memcpy(newPacket->data() + sizeof ((*ap->publishers.begin()).first->id) + sizeof (unsigned char) + sizeof (unsigned char), ap->fullID.c_str(), ap->fullID.length());
						output(0).push(newPacket);
					}
#endif
				}
			}
		} else {
			it++;
		}
	}
}

void LocalProxy::deleteAllActiveInformationItemSubscriptions(LocalHost * _subscriber) {
	unsigned char type, IDLength /*in fragments of PURSUIT_ID_LEN each*/, prefixIDLength /*in fragments of PURSUIT_ID_LEN each*/;
	String ID, prefixID;
	int size = _subscriber->activeSubscriptions.size();
	StringSetIter it = _subscriber->activeSubscriptions.begin();
	for (int i = 0; i < size; i++) {
		ActiveSubscription *as = activeSubscriptionIndex.get((*it)._strData);
		if (!as->isScope) {
			it = _subscriber->activeSubscriptions.erase(it);
			as->subscribers.erase(_subscriber);
			//click_chatter("LocalProxy: deleted subscriber %s from Active Information Item Publication %s", _subscriber->localHostID.c_str(), as->fullID.quoted_hex().c_str());
			if (as->subscribers.size() == 0) {
				//click_chatter("LocalProxy: delete Active Information item Subscription %s", as->fullID.quoted_hex().c_str());
				activeSubscriptionIndex.erase(as->fullID);
				if ((as->strategy != IMPLICIT_RENDEZVOUS) && (as->strategy != LINK_LOCAL) && (as->strategy != BROADCAST_IF)) {
					/*notify the RV Function - depending on strategy*/
					type = UNSUBSCRIBE_INFO;
					IDLength = 1;
					prefixIDLength = (as->fullID.length() - PURSUIT_ID_LEN) / PURSUIT_ID_LEN;
					ID = as->fullID.substring(as->fullID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
					prefixID = as->fullID.substring(0, as->fullID.length() - PURSUIT_ID_LEN);
					createAndSendPacketToRV(type, IDLength, ID, prefixIDLength, prefixID, as->RVFID, as->strategy);
				}
				delete as;
			}
		} else {
			it++;
		}
	}
}

void LocalProxy::deleteAllActiveScopePublications(LocalHost * _publisher) {
	int max_level = 0;
	int temp_level;
	unsigned char type, IDLength/*in fragments of PURSUIT_ID_LEN each*/, prefixIDLength/*in fragments of PURSUIT_ID_LEN each*/;
	String ID, prefixID;
	StringSetIter it;
	for (it = _publisher->activePublications.begin(); it != _publisher->activePublications.end(); it++) {
		ActivePublication *ap = activePublicationIndex.get((*it)._strData);
		
		if (ap->isScope) {
			String temp_id = (*it)._strData;
			temp_level = temp_id.length() / PURSUIT_ID_LEN;
			if (temp_level > max_level) {
				max_level = temp_level;
			}
		}
	}
	for (int i = max_level; i > 0; i--) {
		it = _publisher->activePublications.begin();
		int size = _publisher->activePublications.size();
		for (int j = 0; j < size; j++) {
			String fullID = (*it)._strData;
			it++;
			if (fullID.length() / PURSUIT_ID_LEN == i) {
				ActivePublication *ap = activePublicationIndex.get(fullID);
				if (ap->isScope) {
					_publisher->activePublications.erase(fullID);
					ap->publishers.erase(_publisher);
					//click_chatter("LocalProxy: deleted publisher %s from Active Scope Publication %s", _publisher->localHostID.c_str(), ap->fullID.quoted_hex().c_str());
					if (ap->publishers.size() == 0) {
						//click_chatter("LocalProxy: delete Active Scope Publication %s", ap->fullID.quoted_hex().c_str());
						activePublicationIndex.erase(ap->fullID);
						/*notify the RV Function - depending on strategy*/
						type = UNPUBLISH_SCOPE;
						IDLength = 1;
						prefixIDLength = (ap->fullID.length() - PURSUIT_ID_LEN) / PURSUIT_ID_LEN;
						ID = ap->fullID.substring(ap->fullID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
						prefixID = ap->fullID.substring(0, ap->fullID.length() - PURSUIT_ID_LEN);
						createAndSendPacketToRV(type, IDLength, ID, prefixIDLength, prefixID, ap->RVFID, ap->strategy);
						delete ap;
					}
				}
			}
		}
	}
}

void LocalProxy::deleteAllActiveScopeSubscriptions(LocalHost * _subscriber) {
	int max_level = 0;
	int temp_level;
	unsigned char type, IDLength /*in fragments of PURSUIT_ID_LEN each*/, prefixIDLength /*in fragments of PURSUIT_ID_LEN each*/;
	String ID, prefixID;
	StringSetIter it;
	for (it = _subscriber->activeSubscriptions.begin(); it != _subscriber->activeSubscriptions.end(); it++) {
		ActiveSubscription *as = activeSubscriptionIndex.get((*it)._strData);
		if (as->isScope) {
			String temp_id = (*it)._strData;
			temp_level = temp_id.length() / PURSUIT_ID_LEN;
			if (temp_level > max_level) {
				max_level = temp_level;
			}
		}
	}
	for (int i = max_level; i > 0; i--) {
		it = _subscriber->activeSubscriptions.begin();
		int size = _subscriber->activeSubscriptions.size();
		for (int j = 0; j < size; j++) {
			String fullID = (*it)._strData;
			it++;
			if (fullID.length() / PURSUIT_ID_LEN == i) {
				ActiveSubscription *as = activeSubscriptionIndex.get(fullID);
				if (as->isScope) {
					_subscriber->activeSubscriptions.erase(fullID);
					as->subscribers.erase(_subscriber);
					//click_chatter("LocalProxy: deleted subscriber %s from Active Scope Subscription %s", _subscriber->localHostID.c_str(), as->fullID.quoted_hex().c_str());
					if (as->subscribers.size() == 0) {
						//click_chatter("LocalProxy: delete Active Scope Subscription %s", as->fullID.quoted_hex().c_str());
						activeSubscriptionIndex.erase(as->fullID);
						/*notify the RV Function - depending on strategy*/
						if ((as->strategy != IMPLICIT_RENDEZVOUS) && (as->strategy != LINK_LOCAL) && (as->strategy != BROADCAST_IF)) {
							type = UNSUBSCRIBE_SCOPE;
							IDLength = 1;
							prefixIDLength = (as->fullID.length() - PURSUIT_ID_LEN) / PURSUIT_ID_LEN;
							ID = as->fullID.substring(as->fullID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
							prefixID = as->fullID.substring(0, as->fullID.length() - PURSUIT_ID_LEN);
							createAndSendPacketToRV(type, IDLength, ID, prefixIDLength, prefixID, as->RVFID, as->strategy);
						}
						delete as;
					}
				}
			}
		}
	}
}

CLICK_ENDDECLS
EXPORT_ELEMENT(LocalProxy)
