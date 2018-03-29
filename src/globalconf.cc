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

#include "globalconf.hh"

CLICK_DECLS

GlobalConf::GlobalConf() {
	
}

GlobalConf::~GlobalConf() {
	click_chatter("GlobalConf: destroyed!");
}

int GlobalConf::configure(Vector<String> &conf, ErrorHandler *errh) {
    String types;
    String mode;
    String defRVFID;
    String internalLID;
    String defFromRVFID;
    String TMFID_str = String();
    int offset = 0;
    int delim_pos = 0;
    unsigned char delim = ':';
    click_chatter("*******************************************************GLOBAL CONFIGURATION*******************************************************");
    if (cp_va_kparse(conf, this, errh,
            "TYPE", cpkM, cpString, &types,
            "MODE", cpkM, cpString, &mode,
            "NODEID", cpkM, cpString, &nodeID,
            "DEFAULTRV", cpkM, cpString, &defRVFID,
            "DEFAULTFromRV", cpkM, cpString, &defFromRVFID,
            "iLID", cpkM, cpString, &internalLID,
            "TMFID", cpkN, cpString, &TMFID_str,
            cpEnd) < 0) {
        return -1;
    }
    status = CONNECTED;
    type[FW] = false;
    type[RV] = false;
    type[TM] = false;
    while (offset < types.length()) {
        delim_pos = (types.find_left(delim) != -1) ? types.find_left(delim) : types.length();/*if -1 means no delim and therefor should return the last pos in the string*/
        String node_type = types.substring(offset, delim_pos); /*find the delim*/
        click_chatter("GlobalConf: Node Type: %s", node_type.c_str());
        if((node_type.compare(String("FW")) == 0)) {
            type[FW] = true;
        }
        if((node_type.compare(String("RV")) == 0)) {
            type[RV] = true;
        }
        if((node_type.compare(String("TM")) == 0)) {
            type[TM] = true;
        }
        offset += delim_pos;
        offset ++; /*skip the delim position to the next*/
        
    }
    if ((mode.compare(String("mac")) == 0)
        || (mode.compare(String("mac_ml")) == 0)
        || (mode.compare(String("mac_qos")) == 0)) {
        use_mac = true;
        //click_chatter("Forwarder will run using ethernet frames");
    } else if (mode.compare(String("ip")) == 0) {
        use_mac = false;
        //click_chatter("Forwarder will run using ip raw sockets");
    } else {
        errh->fatal("wrong MODE argument");
        return -1;
    }
    if (nodeID.length() != NODEID_LEN) {
        errh->fatal("NodeID should be %d bytes long...it is %d bytes", NODEID_LEN, nodeID.length());
        return -1;
    }
    if (defRVFID.length() != FID_LEN * 8) {
        errh->fatal("defaultRV_dl should be %d bits...it is %d bits", FID_LEN * 8, defRVFID.length());
        return -1;
    }
    defaultRV_dl = BABitvector(FID_LEN * 8);
    for (int j = 0; j < defRVFID.length(); j++) {
        if (defRVFID.at(j) == '1') {
            defaultRV_dl[defRVFID.length() - j - 1] = true;
        } else {
            defaultRV_dl[defRVFID.length() - j - 1] = false;
        }
    }
     if (defFromRVFID.length() != FID_LEN * 8) {
        errh->fatal("defaultFromRV_dl should be %d bits...it is %d bits", FID_LEN * 8, defFromRVFID.length());
        return -1;
    }
    defaultFromRV_dl = BABitvector(FID_LEN * 8);
    for (int j = 0; j < defFromRVFID.length(); j++) {
        if (defFromRVFID.at(j) == '1') {
            defaultFromRV_dl[defFromRVFID.length() - j - 1] = true;
        } else {
            defaultFromRV_dl[defFromRVFID.length() - j - 1] = false;
        }
    }
    iLID = BABitvector(FID_LEN * 8);
    if (internalLID.length() != FID_LEN * 8) {
        errh->fatal("Internal LID should be %d bits...it is %d bits", FID_LEN * 8, internalLID.length());
        return -1;
    }
    for (int j = 0; j < internalLID.length(); j++) {
        if (internalLID.at(j) == '1') {
            iLID[internalLID.length() - j - 1] = true;
        } else {
            iLID[internalLID.length() - j - 1] = false;
        }
    }
    /*create the RV root scope: /FFFFFFFF.....depending on the PURSUIT_ID_LEN*/
    RVScope = String();
	RVScopeUC = String();
	TMScope = String();
	TMScopeUC = String();
	notificationIID = String();
	pathMgmtScope = String();
	pathMgmtIID = String();
	controlReliabilityScope = String();

	for (int j = 0; j < PURSUIT_ID_LEN; j++) {
		RVScope += (char) 255;
	}
	for (int j = 0; j < PURSUIT_ID_LEN -1; j++) {
		RVScopeUC += 255;
		TMScope += (char) 255;
		TMScopeUC += 255;
		notificationIID += (char) 255;
		pathMgmtScope += (char) 255;
		controlReliabilityScope += (char) 255;

	}
	RVScopeUC += 253;
	TMScope += (char) 254;
	controlReliabilityScope += (char) 252;
	TMScopeUC += (char) 253;
	notificationIID += (char) 253;
	notificationIID += nodeID;
	pathMgmtScope += (char) 249;
	pathMgmtIID = pathMgmtScope + pathMgmtScope; //ID: FFFFFFFFFFFFFFF9/FFFFFFFFFFFFFFF9
	click_chatter("GlobalConf: NodeID: %s", nodeID.c_str());
	if (TMFID_str.length() != 0) {
		if (TMFID_str.length() != FID_LEN * 8) {
			errh->fatal("TMFID LID should be %d bits...it is %d bits", FID_LEN * 8, TMFID_str.length());
			return -1;
		}
		TMFID = BABitvector(FID_LEN * 8);
		for (int j = 0; j < TMFID_str.length(); j++) {
			if (TMFID_str.at(j) == '1') {
				TMFID[TMFID_str.length() - j - 1] = true;
			} else {
				TMFID[TMFID_str.length() - j - 1] = false;
			}
		}
		click_chatter("GlobalConf: FID to the TM: %s", TMFID.to_string().c_str());
	}
	click_chatter("GlobalConf: RV Scope: %s", RVScope.quoted_hex().c_str());
	click_chatter("GlobalConf: TM scope: %s", TMScope.quoted_hex().c_str());
	nodeRVScope = RVScope + nodeID;
	nodeTMScope = TMScope + nodeID;
	nodeTMScopeUC = TMScope + TMScopeUC + nodeID;
	click_chatter("GlobalConf: Information ID to publish RV requests: %s", nodeRVScope.quoted_hex().c_str());
	click_chatter("GlobalConf: Information ID to publish TM requests: %s", nodeTMScope.quoted_hex().c_str());
	click_chatter("GlobalConf: Information ID to publish TM requests for unicast paths of *-over-ICN: %s", nodeTMScopeUC.quoted_hex().c_str());
	click_chatter("GlobalConf: Information ID to subscribe for receiving all explicit pub/sub notifications: %s", notificationIID.quoted_hex().c_str());
	click_chatter("GlobalConf: Information ID that marks reliable control pub/sub messages: %s", controlReliabilityScope.quoted_hex().c_str());
	click_chatter("GlobalConf: default FID for RendezVous node %s", defaultRV_dl.to_string().c_str());
	if (defaultRV_dl == iLID) {
		click_chatter("GlobalConf: I am the RV node for this domain");
	}
	/******** *-over-ICN Well-Known Root Scopes****/
	IP_ROOTSCOPE = String();
	HTTP_ROOTSCOPE = String();
    HTTP_DEFAULT_IID = String();
	for (int j = 0 ; j < PURSUIT_ID_LEN - 1 ; j++){
		HTTP_ROOTSCOPE += (char) 0;
		IP_ROOTSCOPE += (char) 0;
        HTTP_DEFAULT_IID += (char) 0;
	}
	IP_ROOTSCOPE += (char) NAMESPACE_IP;
	HTTP_ROOTSCOPE += (char) NAMESPACE_HTTP	;
    HTTP_DEFAULT_IID = HTTP_ROOTSCOPE + HTTP_DEFAULT_IID + (char) DEFAULT_HTTP_IID;
	
	//click_chatter("GlobalConf: configured!");
	return 0;
}

int GlobalConf::initialize(ErrorHandler */*errh*/) {
	//click_chatter("GlobalConf: initialized!");
	return 0;
}

void GlobalConf::cleanup(CleanupStage /*stage*/) {
	click_chatter("GlobalConf: Cleaned Up!");
}

CLICK_ENDDECLS
EXPORT_ELEMENT(GlobalConf)
