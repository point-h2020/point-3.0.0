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

#include "forwarder.hh"

CLICK_DECLS

ForwardingEntry::ForwardingEntry() {
    src = NULL;
    dst = NULL;
    src_ip = NULL;
    dst_ip = NULL;
    LID = NULL;
    proto_type = 0;
}

ForwardingEntry::~ForwardingEntry() {
    if (src != NULL) {
        delete src;
    }
    if (dst != NULL) {
        delete dst;
    }
    if (src_ip != NULL) {
        delete src_ip;
    }
    if (dst_ip != NULL) {
        delete dst_ip;
    }
    if (LID != NULL) {
        delete LID;
    }
}

Forwarder::Forwarder() {

}

Forwarder::~Forwarder() {
    click_chatter("Forwarder: destroyed!");
}

int Forwarder::configure(Vector<String> &conf, ErrorHandler */*errh*/) {
    int port;
    gc = (GlobalConf *) cp_element(conf[0], this);
    _id = 0;
    click_chatter("*****************************************************FORWARDER CONFIGURATION*****************************************************");
    click_chatter("Forwarder: internal LID: %s", gc->iLID.to_string().c_str());
    if (gc->use_mac == true) {
        cp_integer(conf[1], &number_of_links);
        click_chatter("Forwarder: Number of Links: %d", number_of_links);
        for (int i = 0; i < number_of_links; i++) {
            int reverse_proto;
            cp_integer(conf[2 + 5 * i], &port);
            EtherAddress * src = new EtherAddress();
            EtherAddress * dst = new EtherAddress();
            cp_ethernet_address(conf[3 + 5 * i], src, this);
            cp_ethernet_address(conf[4 + 5 * i], dst, this);
            cp_integer(conf[5 + 5 * i], 16, &reverse_proto);
            ForwardingEntry *fe = new ForwardingEntry();
            fe->src = src;
            fe->dst = dst;
            fe->port = port;
            fe->LID = new BABitvector(FID_LEN * 8);
            fe->proto_type = htons(reverse_proto);
            for (int j = 0; j < conf[6 + 5 * i].length(); j++) {
                if (conf[6 + 5 * i].at(j) == '1') {
                    (*fe->LID)[conf[6 + 5 * i].length() - j - 1] = true;
                } else {
                    (*fe->LID)[conf[6 + 5 * i].length() - j - 1] = false;
                }
            }
            fwTable.push_back(fe);
            if (port != 0) {
                click_chatter("Forwarder: Added forwarding entry: port %d - source MAC: %s - destination MAC: %s - protocol type: Ox0%x - LID: %s", fe->port, fe->src->unparse().c_str(), fe->dst->unparse().c_str(), ntohs(fe->proto_type), fe->LID->to_string().c_str());
            } else {
                click_chatter("Forwarder: Added forwarding entry for the internal LINK ID: %s", fe->LID->to_string().c_str());
            }
        }
    } else {
        cp_integer(conf[1], &number_of_links);
        click_chatter("Forwarder: Number of Links: %d", number_of_links);
        for (int i = 0; i < number_of_links; i++) {
            cp_integer(conf[2 + 4 * i], &port);
            IPAddress * src_ip = new IPAddress();
            IPAddress * dst_ip = new IPAddress();
            cp_ip_address(conf[3 + 4 * i], src_ip, this);
            cp_ip_address(conf[4 + 4 * i], dst_ip, this);
            ForwardingEntry *fe = new ForwardingEntry();
            fe->src_ip = src_ip;
            fe->dst_ip = dst_ip;
            fe->port = port;
            fe->LID = new BABitvector(FID_LEN * 8);
            for (int j = 0; j < conf[5 + 4 * i].length(); j++) {
                if (conf[5 + 4 * i].at(j) == '1') {
                    (*fe->LID)[conf[5 + 4 * i].length() - j - 1] = true;
                } else {
                    (*fe->LID)[conf[5 + 4 * i].length() - j - 1] = false;
                }
            }
            fwTable.push_back(fe);
            click_chatter("Forwarder: Added forwarding entry: port %d - source IP: %s - destination IP: %s - LID: %s", fe->port, fe->src_ip->unparse().c_str(), fe->dst_ip->unparse().c_str(), fe->LID->to_string().c_str());
        }
    }
    click_chatter("*********************************************************************************************************************************");
    //click_chatter("Forwarder: Configured!");
    return 0;
}

int Forwarder::initialize(ErrorHandler */*errh*/) {
    //click_chatter("Forwarder: Initialized!");
    return 0;
}

void Forwarder::cleanup(CleanupStage stage) {
    if (stage >= CLEANUP_CONFIGURED) {
        for (int i = 0; i < number_of_links; i++) {
            ForwardingEntry *fe = fwTable.at(i);
            delete fe;
        }
    }
    click_chatter("Forwarder: Cleaned Up!");
}

void Forwarder::push(int in_port, Packet *p) {
//  click_chatter("Forwarder::push");
    WritablePacket *newPacket;
    WritablePacket *payload = NULL;
    ForwardingEntry *fe;
    Vector<ForwardingEntry *> out_links;
    BABitvector FID(FID_LEN * 8);
    BABitvector andVector(FID_LEN * 8);
    Vector<ForwardingEntry *>::iterator out_links_it;
    int counter = 1;
    bool pushLocally = false;
    click_ip *ip;
    click_udp *udp;
    int reverse_proto;
    /**length of data for IP header
    * does not include MAC (14) or BF (32)*/
    unsigned short payload_len=p->length()-14-32;
    if (in_port == 0) {
        memcpy(FID._data, p->data(), FID_LEN);
        /*Check all entries in my forwarding table and forward appropriately*/
        for (int i = 0; i < fwTable.size(); i++) {
            fe = fwTable[i];
            andVector = (FID)&(*fe->LID);
            if (andVector == (*fe->LID)) {
                out_links.push_back(fe);
            }
        }
        if (out_links.size() == 0) {
            /*I can get here when an app or a click element did publish_data with a specific FID
             *Note that I never check if I can push back the packet above if it matches my iLID
             * the upper elements should check before pushing*/
            p->kill();
        }
        for (out_links_it = out_links.begin(); out_links_it != out_links.end(); out_links_it++) {
            if (counter == out_links.size()) {
                payload = p->uniqueify();
            } else {
                payload = p->clone()->uniqueify();
            }
            fe = *out_links_it;
            if (gc->use_mac) {
                reverse_proto = ntohs(fe->proto_type);
                /*0x080a == 2058*/
                if(reverse_proto == 2058){
                    newPacket = payload->push_mac_header(14);
                    /*prepare the mac header*/
                    /*destination MAC*/
                    memcpy(newPacket->data(), fe->dst->data(), MAC_LEN);
                    /*source MAC*/
                    memcpy(newPacket->data() + MAC_LEN, fe->src->data(), MAC_LEN);
                    /*protocol type*/
                    memcpy(newPacket->data() + MAC_LEN + MAC_LEN, &fe->proto_type, 2);
                    /*push the packet to the appropriate ToDevice Element*/
                    output(fe->port).push(newPacket);
                }
                /*0x86dd == 34525, this is the protocol type used for IPv6 in the SDN implementation*/
                if (reverse_proto == 34525){
                    newPacket = payload->push_mac_header(14 + 8);
                    /*prepare the mac header*/
                    /*destination MAC*/
                    memcpy(newPacket->data(), fe->dst->data(), MAC_LEN);
                    /*source MAC*/
                    memcpy(newPacket->data() + MAC_LEN, fe->src->data(), MAC_LEN);
                    /*protocol type 0x080a*/
                    memcpy(newPacket->data() + MAC_LEN + MAC_LEN, &fe->proto_type, 2);
                    /*add 8 byte to complete IPv6 header for SDN switching, including:*/
                    /*version number */
                    memset(newPacket->data() + MAC_LEN + MAC_LEN + 2, 0x60, 1);
                    /*zero for not used IP feilds*/
                    memset(newPacket->data() + MAC_LEN + MAC_LEN + 3, 0x00, 3);
                    /*actualy payload length*/
                    memcpy(newPacket->data() + MAC_LEN + MAC_LEN + 6,(void*)&payload_len, 2);
                    /*IPv6 value for 'no next header'*/
                    memset(newPacket->data() + MAC_LEN + MAC_LEN + 8, 59, 1);
                    /*hop limit =255*/
                    memset(newPacket->data() + MAC_LEN + MAC_LEN + 9, 0xff, 1);
                    /*push the packet to the appropriate ToDevice Element*/
                    output(fe->port).push(newPacket);
                }
            } else {
                newPacket = payload->push(sizeof (click_udp) + sizeof (click_ip));
                ip = reinterpret_cast<click_ip *> (newPacket->data());
                udp = reinterpret_cast<click_udp *> (ip + 1);
                // set up IP header
                ip->ip_v = 4;
                ip->ip_hl = sizeof (click_ip) >> 2;
                ip->ip_len = htons(newPacket->length());
                ip->ip_id = htons(_id.fetch_and_add(1));
                ip->ip_p = IP_PROTO_UDP;
                ip->ip_src = fe->src_ip->in_addr();
                ip->ip_dst = fe->dst_ip->in_addr();
                ip->ip_tos = 0;
                ip->ip_off = 0;
                ip->ip_ttl = 250;
                ip->ip_sum = 0;
                ip->ip_sum = click_in_cksum((unsigned char *) ip, sizeof (click_ip));
                newPacket->set_ip_header(ip, sizeof (click_ip));
                // set up UDP header
                udp->uh_sport = htons(55555);
                udp->uh_dport = htons(55555);
                uint16_t len = newPacket->length() - sizeof (click_ip);
                udp->uh_ulen = htons(len);
                udp->uh_sum = 0;
                unsigned csum = click_in_cksum((unsigned char *) udp, len);
                udp->uh_sum = click_in_cksum_pseudohdr(csum, ip, len);
                output(fe->port).push(newPacket);
            }
            counter++;
        }
    } else if (in_port >= 1) {
        /**a packet has been pushed by the underlying network.**/
        /*check if it needs to be forwarded*/
        int p_proto_type;
        if (gc->use_mac) {
            /*check if the packet is coming from a SDN switch or a BA forwarder*/
            memcpy(&p_proto_type, p->data() + 12, 2);
            p_proto_type = ntohs(p_proto_type);
            /*click_chatter("Forwarder: Incoming Packet with Ethernet type %x", p_proto_type);*/
            /*if from BA node, the FID is placed imidiatly after the MAC header*/
            /*if from SDN node, the FID is after 8 bytes + MAC header*/
            if (p_proto_type == 34525) {
                memcpy(FID._data, p->data() + 14 + 8, FID_LEN);
            } else {
                /*Carefull, this assumes any packets that are not IPv6 (SDN) to be of Blackadder packets*/
                memcpy(FID._data, p->data() + 14, FID_LEN);
            }
        } else {
            memcpy(FID._data, p->data() + 28, FID_LEN);
        }
        BABitvector testFID(FID);
        testFID.negate();
        if (!testFID.zero()) {
            /*Check all entries in my forwarding table and forward appropriately*/
            for (int i = 0; i < fwTable.size(); i++) {
                fe = fwTable[i];
                andVector = (FID)&(*fe->LID);
                if (andVector == (*fe->LID)) {
                    if (gc->use_mac) {
                        EtherAddress src(p->data() + MAC_LEN);
                        EtherAddress dst(p->data());
                        /*click_chatter("Forwarder: network packet, src MAC: %s, dst MAC: %s", (src.unparse()).c_str(), (dst.unparse()).c_str());*/
                        if ((src.unparse().compare(fe->dst->unparse()) == 0) && (dst.unparse().compare(fe->src->unparse()) == 0)) {
                            click_chatter("MAC: a loop in %u from positive..I am not forwarding to the interface I received the packet from", i);
                            continue;
                        }
                        if ((src.unparse().compare(fe->src->unparse()) == 0) || (dst.unparse().compare(fe->dst->unparse()) == 0)) {
                            click_chatter("MAC: a looped packet in %u from positive, potentialy SDN..I am not forwarding to the interface I received the packet from", i);
                            continue;
                        }
                    } else {
                        click_ip *ip = reinterpret_cast<click_ip *> ((unsigned char *)p->data());
                        if ((ip->ip_src.s_addr == fe->dst_ip->in_addr().s_addr) && (ip->ip_dst.s_addr == fe->src_ip->in_addr().s_addr)) {
                            click_chatter("IP: a loop from positive..I am not forwarding to the interface I received the packet from");
                            continue;
                        }
                    }
                    out_links.push_back(fe);
                }
            }
        } else {
            /*all bits were 1 - probably from a link_broadcast strategy--do not forward*/
        }
        /*check if the packet must be pushed locally*/
        andVector = FID & gc->iLID;
        if (andVector == gc->iLID) {
            pushLocally = true;
        }
        if (!testFID.zero()) {
            for (out_links_it = out_links.begin(); out_links_it != out_links.end(); out_links_it++) {
                if ((counter == out_links.size()) && (pushLocally == false)) {
                    payload = p->uniqueify();
                } else {
                    payload = p->clone()->uniqueify();
                }
                fe = *out_links_it;
                if (gc->use_mac) {
                    /*BA->BA or SDN->SDN, only write the src/dst MAC addrs - no need to write the protocol type or tamper with the packet*/
                    if (p_proto_type == ntohs(fe->proto_type)){
                        /*prepare the mac header*/
                        /*destination MAC*/
                        memcpy(payload->data(), fe->dst->data(), MAC_LEN);
                        /*source MAC*/
                        memcpy(payload->data() + MAC_LEN, fe->src->data(), MAC_LEN);
                        /*push the packet to the appropriate ToDevice Element*/
                        output(fe->port).push(payload);
                    }else{
                        /*BA -> SDN*/
                        if (p_proto_type == 2058 && ntohs(fe->proto_type) == 34525){
                            /*add 8 bytes between the FID and MAC header to complete the IPv6 header for ICN-SDN FW*/
                            payload->push_mac_header(8);
                            /*destination MAC*/
                            memcpy(payload->data(), fe->dst->data(), MAC_LEN);
                            /*source MAC*/
                            memcpy(payload->data() + MAC_LEN, fe->src->data(), MAC_LEN);
                            /*protocol type*/
                            memcpy(payload->data() + MAC_LEN + MAC_LEN, &fe->proto_type, 2);
                            /*add 8 byte to complete IPv6 header for SDN switching, including:*/
                            /*version number */
                            memset(payload->data() + MAC_LEN + MAC_LEN + 2, 0x60, 1);
                            /*zero for not used IP feilds*/
                            memset(payload->data() + MAC_LEN + MAC_LEN + 3, 0x00, 3);
                            /*actualy payload length*/
                            memcpy(payload->data() + MAC_LEN + MAC_LEN + 6,(void*)&payload_len, 2);
                            /*IPv6 value for 'no next header'*/
                            memset(payload->data() + MAC_LEN + MAC_LEN + 8, 59, 1);
                            /*hop limit =255*/
                            memset(payload->data() + MAC_LEN + MAC_LEN + 9, 0xff, 1);
                            output(fe->port).push(payload);
                        }
                        /*SDN -> BA*/
                        else if (p_proto_type == 34525 && ntohs(fe->proto_type) == 2058){
                            /*take out the 8 bytes between the FID and MAC header to restore the BA packet, requires also to take the 14 byte current MAC header*/
                            payload->pull(14 + 8);
                            /*reinstate the MAC header*/
                            payload->push_mac_header(14);
                            /*destination MAC*/
                            memcpy(payload->data(), fe->dst->data(), MAC_LEN);
                            /*source MAC*/
                            memcpy(payload->data() + MAC_LEN, fe->src->data(), MAC_LEN);
                            /*protocol type*/
                            memcpy(payload->data() + MAC_LEN + MAC_LEN, &fe->proto_type, 2);
                            output(fe->port).push(payload);
                        }
                        /**Carefull, I will assume every packet not matching the above to be a blackadder packet
                         *This is particularlry to solve NS3 issue of using IPv4 for Blackadder packets.
                         *When NS3 issue is resolved, this should be corrected
                         */
                        else {
                            /*prepare the mac header*/
                            /*destination MAC*/
                            memcpy(payload->data(), fe->dst->data(), MAC_LEN);
                            /*source MAC*/
                            memcpy(payload->data() + MAC_LEN, fe->src->data(), MAC_LEN);
                            /*push the packet to the appropriate ToDevice Element*/
                            output(fe->port).push(payload);
                          click_chatter ("Forwarder: unknown ethernet packet type: %x !", p_proto_type);
                          /*payload->kill();*/
                        }
                    }
                } else {
                    click_ip *ip = reinterpret_cast<click_ip *> (payload->data());
                    ip->ip_src = fe->src_ip->in_addr();
                    ip->ip_dst = fe->dst_ip->in_addr();
                    ip->ip_tos = 0;
                    ip->ip_off = 0;
                    ip->ip_ttl = 250;
                    ip->ip_sum = 0;
                    ip->ip_sum = click_in_cksum((unsigned char *) ip, sizeof (click_ip));
                    click_udp *udp = reinterpret_cast<click_udp *> (ip + 1);
                    //uint16_t len = udp->uh_ulen;
                    uint16_t len = payload->length() - sizeof (click_ip);
                    udp->uh_sum = 0;
                    unsigned csum = click_in_cksum((unsigned char *) udp, len);
                    udp->uh_sum = click_in_cksum_pseudohdr(csum, ip, len);
                    output(fe->port).push(payload);
                }
                counter++;
            }
		} else {
            /*all bits were 1 - probably from a link_broadcast strategy--do not forward*/
        }
        if (pushLocally) {
            if (gc->use_mac) {
                if(p_proto_type == 2058){
                    /*BA packet: take out the MAC header + FID*/
                    //p->pull(14 + FID_LEN);
                    p->pull(14);
                }
                else if(p_proto_type == 34525){
                    /*SDN packet: take out the MAC header + 8 byte SDN + FID*/
                    //p->pull(14 + 8 + FID_LEN);
//click_chatter("Forwrder::Pushing SDN packet to proxy ");
                    p->pull(14 + 8);
                }else {
                    /*carefull, I will assume every other packet type to also be blackadder packet
                     *This is to solve NS3 issue of having IPv4 ethertype to represent Blackadder packets
                     *Once NS3 is extended to support blackadder as a protocol_type, this pull should be omitted*/
                    //p->pull(14 + FID_LEN);
                    p->pull(14);
                    /*click_chatter("Forwarder: Unknown Ethernet type in packet destined locally!");
                    p->kill();*/
                }
                output(0).push(p);
            } else {
                //p->pull(20 + 8 + FID_LEN);
                p->pull(20 + 8);
                output(0).push(p);
            }
        }

        if ((out_links.size() == 0) && (!pushLocally)) {
            p->kill();
        }
    }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Forwarder)
ELEMENT_PROVIDES(ForwardingEntry)

