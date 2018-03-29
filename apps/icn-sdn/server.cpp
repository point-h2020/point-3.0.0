/*
 * Copyright (C) 2017  George Petropoulos
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 3 as published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of
 * the BSD license.
 *
 * See LICENSE and COPYING for more details.
 */


#include <iostream>
#include "tm_resource.h"
#include "resource_manager.h"
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <time.h>
#include <moly/moly.hh>
#include "messages/asio_rw.h"
#include "messages/messages.pb.h"
#include "messages/protobuf_rw.h"
#include <sstream>
#include <fstream>
#include <string>

using boost::asio::ip::tcp;
namespace posix = boost::asio::posix;
using namespace std;
using namespace eu::point::tmsdn::impl;

/**@brief The server class, implementing the server side of the ICN-SDN interface,
 * listening for bootstrapping protocol, link state monitoring, traffic, group
 * and flow monitoring messages.
 * Depending on the type of messages, it calls relevant modules to perform the
 * required functions.
 *
 * @author George Petropoulos
 * @version cycle-2
 *
 */
class server {
public:
	/**@brief The constructor with arguments.
	 *
	 */
	server(boost::asio::io_service& io_service, short port, Blackadder *ba,
			ResourceManager* rm) :
			io_service_(io_service), rm_(rm), ba_(ba), acceptor_(io_service,
					tcp::endpoint(tcp::v4(), port)) {

		cout << currentDateTime() << ": Starting server at port " << port << endl;
	}

	/**@brief The method which returns the current date and time
	 * for logging purposes
	 *
	 * @return The current date and time as string.
	 *
	 */
	const std::string currentDateTime() {
		timeval tp;
		gettimeofday(&tp, 0);
		time_t curtime = tp.tv_sec;
		tm *t = localtime(&curtime);
		char currentTime[84] = "";
		sprintf(currentTime, "[%02d-%02d-%02d %02d:%02d:%02d:%03d]",
				t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
				t->tm_hour, t->tm_min, t->tm_sec, tp.tv_usec/1000);
		return currentTime;
	}

	/**@brief The method which listens for incoming ICN-SDN messages and acts accordingly.
	 *
	 * It accepts incoming messages, and checks their type.
	 * If it is a traffic monitoring message, it integrates with MOLY to report them.
	 * If it is a flow monitoring message, it just prints them in the console.
	 * If it is a group monitoring message, it just prints them in the console.
	 * If it is a link status message, it publishes to the LSN scope, and also integrates
	 * with MOLY to report them.
	 * If it is a resource request message, it then requests by the resource manager for
	 * unique resources and responds with the resource offer.
	 *
	 */
	void start_accept() {
		//list to hold the reported ports through ADD_PORT_M, so that they are reported only once
		std::list<string> reportedPortsList;

		cout << "Register with MONA and wait until trigger arrives to start "
				"reporting" << endl;

		if (!moly.startReporting())//blocking method until MOOSE becomes available
		{
			cout << "MONA rejected registration. Closing here" << endl;
			return;
		}

		cout << "Trigger received from MONA. Starting server and awaiting ODL "
				"reports" << endl;

		//loop waiting for messsages from the ICN-SDN socket.
		while (true) {
			tcp::socket socket_(io_service_);
			acceptor_.accept(socket_);
			AsioInputStream<tcp::socket> ais(socket_);
			CopyingInputStreamAdaptor cis_adp(&ais);
			TmSdnMessage message;
			//read the buffer and if receive message, create the protobuf message
			bool read = google::protobuf::io::readDelimitedFrom(&cis_adp,
					&message);

			if (read) {
				switch (message.type()) {
				//if traffic monitoring message
				case TmSdnMessage::TM: {
					std::cout << currentDateTime() << ": Received traffic monitoring message. "
							<< std::endl;
					const TmSdnMessage::TrafficMonitoringMessage &tm =
							message.trafficmonitoringmessage();
					std::cout << tm.nodeid1() << ", " << tm.nodeid2() << ", " << tm.nodeconnector()
							<< ": [packets rec/tra] "
							<< tm.packetsreceived() << "/" << tm.packetstransmitted()
							<< ", [bytes rec/tra] "
							<< tm.bytesreceived() << "/" << tm.bytestransmitted()
							<< ", [errors rec/tra] "
							<< tm.receiveerrors() << "/" << tm.transmiterrors()
							<< ", [drops rec/tra] "
							<< tm.receivedrops() << "/" << tm.transmitdrops()
							<< std::endl;

					//update node statistics to moly
					moly.nodeState(NODE_ROLE_FN, atol(tm.nodeid1().c_str()), NODE_STATE_UP);
					moly.nodeState(NODE_ROLE_FN, atol(tm.nodeid2().c_str()), NODE_STATE_UP);

					//update link statistics to moly
					//find link ID by reading the file that the TM has written
					size_t position;
					ifstream infile("/tmp/lids");
					if (infile.is_open()) {
							string line;
							while (getline(infile, line))
							{
									string line;
									while (getline(infile, line)) {
											if (line.find("Source") != std::string::npos) {
													istringstream iss(line);
													string source, dest, linkid;
													string sourceNode, destNode, lid;
													if (!(iss >> source >> sourceNode >> dest >> destNode >> linkid >> lid))
													{
															cout << "Problem with lines read." << endl;
													}
													if (tm.nodeid1() == sourceNode && tm.nodeid2() == destNode){
															position = lid.find("1");
															if (position != string::npos){
																	cout << "LID for " << sourceNode << " --> " << destNode << " is " << position << endl;
															}
													}
											}
									}

							}
					}
					else{
							cout << "Could not open TM log to get LIDs." << endl;
					}
					infile.close();

					moly.linkState(position, atol(tm.nodeid2().c_str()), atol(tm.nodeid1().c_str()),
													tm.nodeconnector(), LINK_TYPE_SDN_802_3_AE, LINK_STATE_UP);

					//report each port once through ADD_PORT_M, then just update its statistics through PORT_STATE_M
					std::list<string>::iterator it;
					string portToFind = tm.portname();
					it = find (reportedPortsList.begin(), reportedPortsList.end(), portToFind);
					if (it != reportedPortsList.end()){
							cout << "Just updating statistics for port " << tm.nodeconnector() << " of switch " << atol(tm.nodeid1().c_str()) << endl;
							moly.portState(atol(tm.nodeid1().c_str()), tm.nodeconnector(), PORT_STATE_UP);
					}
					else{
							cout << " Adding for the first time port " << tm.nodeconnector() << " of switch " << atol(tm.nodeid1().c_str()) << endl;
							moly.addPort(atol(tm.nodeid1().c_str()), tm.nodeconnector(), tm.portname());
							reportedPortsList.push_back(portToFind);
					}

					//update statistics to moly
					moly.rxBytesPort(atol(tm.nodeid1().c_str()), tm.nodeconnector(), tm.bytesreceived());
					moly.txBytesPort(atol(tm.nodeid1().c_str()), tm.nodeconnector(), tm.bytestransmitted());
					moly.packetErrorRate(atol(tm.nodeid1().c_str()), tm.nodeconnector(), tm.receiveerrors());
					moly.packetDropRate(atol(tm.nodeid1().c_str()), tm.nodeconnector(), tm.receivedrops());

					break;
				}
				//if link status message
				case TmSdnMessage::LS: {
					std::cout << currentDateTime() << ":Received link status message. " << std::endl;
					const TmSdnMessage::LinkStatusMessage &ls =
							message.linkstatusmessage();
					std::cout << ls.nodeid1() << ", " << ls.nodeid2() << ", " << ls.nodeconnector()
							<< ": [type] " << ls.lsmtype() << std::endl;
					//check link status type, either ADD (new link) or RMV (removed link)
					switch (ls.lsmtype()) {
					//if ADD
					case TmSdnMessage::LinkStatusMessage::ADD: {
						std::cout << currentDateTime() << ": Received LSM ADD message. " << std::endl;

						//perform relevant publications to LSN ICN application.
						//assuming the added link is bidirectional (A <-> B), you have to publish
						//for both ends of the added link (A -> B and B -> A).

						//publish to /lsn/node1/ scope
						iID = ls.nodeid1();
						lsnID = lsnSID + iID;
						memset(lsn, ADD_LINK, sizeof(unsigned char));
						memset(lsn + sizeof (unsigned char), (unsigned char)bidirectional,
								sizeof (unsigned char));
						memcpy(lsn + sizeof(unsigned char) + sizeof (unsigned char),
								ls.nodeid2().c_str(), (unsigned int) NODEID_LEN);
						ba_->publish_data(lsnID, NODE_LOCAL, NULL, 0, lsn, lsn_size);
						std::cout << "Published ADD LSN to " << lsnID << " scope." << std::endl;

						//publish to /lsn/node2/ scope
						iID = ls.nodeid2();
						lsnID = lsnSID + iID;
						memset(lsn, ADD_LINK, sizeof(unsigned char));
						memset(lsn + sizeof (unsigned char), (unsigned char)bidirectional,
								sizeof (unsigned char));
						memcpy(lsn + sizeof(unsigned char) + sizeof (unsigned char),
								ls.nodeid1().c_str(), (unsigned int) NODEID_LEN);
						//publish to TM assuming it runs locally
						ba_->publish_data(lsnID, NODE_LOCAL, NULL, 0, lsn, lsn_size);
						std::cout << "Published ADD LSN to " << lsnID << " scope." << std::endl;

						//update link state to moly
						moly.linkState(0, atol(ls.nodeid2().c_str()), atol(ls.nodeid1().c_str()),
								ls.nodeconnector(), LINK_TYPE_SDN_802_3_AE, LINK_STATE_UP);

						break;
					}
					//if RMV
					case TmSdnMessage::LinkStatusMessage::RMV: {
						std::cout << currentDateTime() << ": Received LSM RMV message. " << std::endl;

						//perform relevant publications to LSN ICN application.
						//assuming the removed link is bidirectional (A <-> B), you have to publish
						//for both ends of the removed link (A -> B and B -> A).

						//publish to /lsn/node1/ scope
						iID = ls.nodeid1();
						lsnID = lsnSID + iID;
						memset(lsn, REMOVE_LINK, sizeof (unsigned char));
						memset(lsn + sizeof (unsigned char), (unsigned char)bidirectional,
								sizeof(unsigned char));
						memcpy(lsn + sizeof (unsigned char) + sizeof (unsigned char),
								ls.nodeid2().c_str(), (unsigned int) NODEID_LEN);
						//publish to TM assuming it runs locally
						ba_->publish_data(lsnID, NODE_LOCAL, NULL, 0, lsn, lsn_size);
						std::cout << "Published RMV LSN to " << lsnID << " scope." << std::endl;

						//publish to /lsn/node2/ scope
						iID = ls.nodeid2();
						lsnID = lsnSID + iID;
						memset(lsn, REMOVE_LINK, sizeof (unsigned char));
						memset(lsn + sizeof (unsigned char), (unsigned char)bidirectional,
								sizeof(unsigned char));
						memcpy(lsn + sizeof (unsigned char) + sizeof (unsigned char),
								ls.nodeid1().c_str(), (unsigned int) NODEID_LEN);
						//publish to TM assuming it runs locally
						ba_->publish_data(lsnID, NODE_LOCAL, NULL, 0, lsn, lsn_size);
						std::cout << "Published RMV LSN to " << lsnID << " scope." << std::endl;

						//update link state to moly
						moly.linkState(0, atol(ls.nodeid2().c_str()), atol(ls.nodeid1().c_str()),
								ls.nodeconnector(), LINK_TYPE_SDN_802_3_AE, LINK_STATE_DOWN);

						break;
					}

					default: {
						std::cout << "Invalid LSM message type. " << std::endl;
						break;
					}
					}

					break;
				}
				//if resource request message
				case TmSdnMessage::RR: {
					std::cout << currentDateTime() << ": Received resource request message. "
							<< std::endl;
					AsioOutputStream<boost::asio::ip::tcp::socket> aos(socket_);
					CopyingOutputStreamAdaptor cos_adp(&aos);
					TmSdnMessage offer;
					offer.set_type(TmSdnMessage::RO);
					TmSdnMessage::ResourceOfferMessage *resource_offer(
							offer.mutable_resourceoffermessage());

					std::cout << "Size = "
							<< message.resourcerequestmessage().requests_size()
							<< std::endl;
					//for each resource request in the list, get resources
					//and add them to offer
					for (int i = 0;
							i < message.resourcerequestmessage().requests_size();
							i++) {
						const TmSdnMessage::ResourceRequestMessage::RecourceRequest &rr =
								message.resourcerequestmessage().requests(i);
						std::cout << "srcNode/dstNode/srcNodeConnector: " << rr.srcnode() << "/"
								<< rr.dstnode() << "/" << rr.nodeconnector() << std::endl;
						//create the resource offer object
						TmSdnMessage::ResourceOfferMessage::RecourceOffer *ro =
								resource_offer->add_offers();
						//call the resource manager module to respond with a resource offer
						//per request
						rm_->generate_link_id(rr, ro);
						std::cout << currentDateTime() << ": Assigned nid/lid/ilid: " << ro->nid() << "/"
								<< ro->lid() << "/" << ro->ilid() << std::endl;
					}

					//send resource offer back to socket
					google::protobuf::io::writeDelimitedTo(offer, &cos_adp);
					cos_adp.Flush();
					break;
				}
				//if flow monitoring message
				case TmSdnMessage::FM: {
					std::cout << currentDateTime() << ": Received flow monitoring message. "
							<< std::endl;

					const TmSdnMessage::FlowMonitoringMessage &fm =
							message.flowmonitoringmessage();
					std::cout << "node " << fm.nodeid()
							<< ", table " << fm.table() << ", [src/dst]: "
							<< fm.src_ipv6() << "/" << fm.dst_ipv6()
							<< ", [packets/bytes] "
							<< fm.packets() << "/" << fm.bytes() << std::endl;
					//TODO report them to MOLY
					break;
				}
				//if group monitoring message
				case TmSdnMessage::GM: {
					std::cout << currentDateTime() << ": Received group monitoring message. "
							<< std::endl;

					const TmSdnMessage::GroupMonitoringMessage &gm =
							message.groupmonitoringmessage();
					std::cout << "node " << gm.nodeid()
							<< ", group " << gm.group()
							<< ", [packets/bytes] "
							<< gm.packets() << "/" << gm.bytes()
							<< ", buckets size " << gm.buckets_size() << std::endl;

					//TODO report them to MOLY
					break;
				}
				default: {
					std::cout << "Invalid message type. " << std::endl;
					//TODO
					break;
				}
				}
			}

		}
	}

private:
	boost::asio::io_service& io_service_;
	tcp::acceptor acceptor_;
	Blackadder *ba_;
	ResourceManager *rm_;
	bool bidirectional = true;
	string iID;
	string lsnID;
	string lsnSID = string(PURSUIT_ID_LEN - 1, SUFFIX_NOTIFICATION)
			+ (char) LINK_STATE_NOTIFICATION;
	unsigned int lsn_size =  sizeof (unsigned char) + sizeof (unsigned char)
			+ (unsigned int) NODEID_LEN;
	char *lsn = (char *) malloc (lsn_size);
	Process moly;
};

/**@brief The main method initializing the required threads.
 *
 * It requires the parameters of the TM, specifically its ICN node identifier,
 * its Openflow identifier in the form of host:<mac_address> and
 * the Openflow identifier of its attached SDN switch in the form of openflow:x.
 *
 * Execute the server process by './server' for default Mininet settings, or
 * './server <node_id> host:<mac_address> openflow:X',
 * e.g. './server 00000019 host:af:dd:45:44:22:3e openflow:156786896342474'.
 *
 */
int main(int argc, char* argv[]) {
	string tm_nid, tm_of_id, attached_switch_of_id;
	if (argc == 4)
	{
		//read from cmd
		tm_nid = string(argv[1]);
		tm_of_id = string(argv[2]);
		attached_switch_of_id = string(argv[3]);
	}
	else
	{
		//assign default values
		tm_nid = "00000001";
		tm_of_id = "host:00:00:00:00:00:01";
		attached_switch_of_id = "openflow:1";
	}

	try
	{
		//create the resource manager object
		ResourceManager *rm = new ResourceManager(tm_nid, tm_of_id, attached_switch_of_id);
		//create the blackadder object
		Blackadder *ba = Blackadder::Instance(true);

		//start the tm resource thread.
		TMResource t = TMResource(rm);
		boost::thread* tm_thread = new boost::thread(
				boost::bind(&TMResource::init, &t));

		//start the tm-sdn server thread.
		boost::asio::io_service io_service;
		server s(io_service, 12345, ba, rm);
		boost::thread* s_thread = new boost::thread(
				boost::bind(&server::start_accept, &s));

		s_thread->join();
		tm_thread->join();

	} catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
