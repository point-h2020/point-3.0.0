/*
 * mcast.hh
 *
 *  Created on: 15 Apr 2017
 *      Author: Xenofon Vasilakos <xvas-at-aueb.gr>
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

#include <thread>

#include "mcast.hh"
#include "ip.hh"

using namespace std;
using namespace namespaces::mcast;
using namespace log4cxx;

struct timespec spec;

LoggerPtr MCast::logger(Logger::getLogger("namespaces.mcast"));

MCast::MCast(Blackadder *icnCore, Configuration &configuration,
		Transport &transport, Statistics &statistics, bool *run)
: _icnCore(icnCore),
  _configuration(configuration),
  _transport(transport),
  _statistics(statistics),
  _run(run)
{

	// Initialising IP buffer cleaner with this ugly void caster
	IpBufferCleaner ipBufferCleaner((void *) &_packetBuffer,
			(void *) &_mutexBuffer, _configuration, _statistics, _run);
	std::thread *mcastBufferThread = new std::thread(ipBufferCleaner);
	mcastBufferThread->detach();
	delete mcastBufferThread;

	isClientNAP = true;
	isServerNAP = true;
	MESSAGE_PROCESSING_LAG = 100000; //ns
	LEAVE_TIMER = 100000; //ms
	DROM_AFTER_QUER_NUM = 0;
	_rootScope_ptr = NULL;
	_controlScope_ptr = NULL;
	_sNapToIPTVServerTimer = 0;
	_dataScope_ptr = NULL;
	_runDetachedThds = false;
	REDUNDANT_IGMP_NUM = 0;
	_mcastBufferThread = NULL;
	GENERAL_QUERY_TIMER = _configuration.igmp_genQueryTimer();
}

MCast::~MCast() {

	_runDetachedThds = false;

	delete _rootScope_ptr;
	delete _controlScope_ptr;
	delete _dataScope_ptr;

	_dataScopeIds.clear();
	_ctrlScopeIds.clear();

	_mutexMcastDB_cNap.lock();
	_mcastDB_cNap.clear(); // TODO do the same for file descriptors and sockets
	_mutexMcastDB_cNap.unlock();

	//clear server counter
	_mutexSNapCounter.lock();
	_sNAPCounterOfCNapIDsIt = _sNAPCounterOfCNapIDs.begin();
	while (_sNAPCounterOfCNapIDsIt != _sNAPCounterOfCNapIDs.end()) {
		delete _sNAPCounterOfCNapIDsIt->second;
		_sNAPCounterOfCNapIDs.erase(_sNAPCounterOfCNapIDsIt);
		++_sNAPCounterOfCNapIDsIt;
	}
	_mutexSNapCounter.unlock();
	//clear client counter
	//_mutexClientCounter.lock();
	//_clientCounterSrcAddrsIt = _clientCounterSrcAddrs.begin();
	//while (_clientCounterSrcAddrsIt != _clientCounterSrcAddrs.end()) {

	//delete _clientCounterSrcAddrsIt->second;
	//_clientCounterSrcAddrs.erase(_clientCounterSrcAddrsIt);
	//++_clientCounterSrcAddrsIt;
	//}
	//_clientCounter.clear();
	//_mutexClientCounter.unlock();
	_dataFwdState.clear();
}

void MCast::handle(IpAddress &sourceIpAddress, IpAddress &destinationIpAddress,
		uint8_t * packet)
{
	//YT: handler is disabled
	if (!_configuration.igmpHandler()) {
		LOG4CXX_DEBUG(logger, "handle: IGMP handler is disabled");
		return;
	}

	//YT: prevent looped back packets
	if (find(_ignoreIGMPFrom.begin(), _ignoreIGMPFrom.end(),
			sourceIpAddress.str()) != _ignoreIGMPFrom.end())
	{
		LOG4CXX_DEBUG(logger, "handle: dropping packet, ignored source: "
				<< sourceIpAddress.str() <<", dst: "
				<< destinationIpAddress.str());
		return;
	}

	struct igmp *igmpHdr;
	igmpHdr = (struct igmp *) packet;

	switch (igmpHdr->igmp_type) {

	case IGMP_MEMBERSHIP_QUERY:
		if (isServerNAP)
			_handleIGMPQuery(sourceIpAddress, destinationIpAddress, igmpHdr);
		else
			LOG4CXX_DEBUG(logger, "Dropping memebership query message, "
					"serverNAP is disabled, source: "<< sourceIpAddress.str()
					<<", dst: " << destinationIpAddress.str());
		break;
	case IGMP_V1_MEMBERSHIP_REPORT:
	case IGMP_V2_MEMBERSHIP_REPORT:
		_handleIGMPMemReport(sourceIpAddress, destinationIpAddress, igmpHdr);
		break;

	case IGMP_LEAVE_GROUP:
		_handleIGMPLeave(sourceIpAddress, destinationIpAddress, igmpHdr);
		break;

	default:
		LOG4CXX_DEBUG(logger, "IGMP Header type *not* handled - Type: "
				<< ntohs(igmpHdr->igmp_type) << " |  Code: "
				<< ntohs(igmpHdr->igmp_code) << " | for group address: "
				<< destinationIpAddress.str());
		break;
	}
}

void MCast::handleMcastDataAtSNap(IpAddress & sourceIpAddress,
		IpAddress &multicastIpAddress, uint8_t *packet, uint16_t & packetSize) {

	if (isServerNAP == false) {
		LOG4CXX_DEBUG(logger, "Looped-back packet, group: "
				<< multicastIpAddress.str() << ", src: "
				<< sourceIpAddress.str());
		return;
	}

	if (!_ignoreMCastDataFrom.empty())
		if (find(_ignoreMCastDataFrom.begin(), _ignoreMCastDataFrom.end(),
				sourceIpAddress.str()) != _ignoreMCastDataFrom.end()) {
			LOG4CXX_DEBUG(logger, "Dropping packet, ignored source "
					<< sourceIpAddress.str()<< ", group address: "
					<< multicastIpAddress.str());
			return;
		}
	_statistics.rxIGMPBytes(packetSize);

	list<string>* cNapNids = NULL;
	// <editor-fold defaultstate="collapsed" desc="check for possible packet drop">
	_mutexSNapCounter.lock();
	_sNAPCounterOfCNapIDsIt = _sNAPCounterOfCNapIDs.find(
			multicastIpAddress.uint());

	// if former registration  for ths multicast IP exists
	if (_sNAPCounterOfCNapIDsIt != _sNAPCounterOfCNapIDs.end()) {
		cNapNids = _sNAPCounterOfCNapIDsIt->second;
	} else {
		LOG4CXX_DEBUG(logger, "Dropping packet, unknown group : "
				<< multicastIpAddress.str() << ", src: "
				<< sourceIpAddress.str());
		_mutexSNapCounter.unlock();
		return;
	}
	if (cNapNids->empty()) {
		LOG4CXX_DEBUG(logger, "Dropping packet, no registered cNAPs, group: "
				<< multicastIpAddress.str() << ", src: "
				<< sourceIpAddress.str());
		_mutexSNapCounter.unlock();
		return;
	}
	_mutexSNapCounter.unlock(); // </editor-fold>

	LOG4CXX_DEBUG(logger,"Handling multicast data packet destined to multicast "
			"address" << " " << multicastIpAddress.str() << "" );

	IcnId* dataCID_ptr = IcnId::createIGMPDataScopeId(&multicastIpAddress);

	_mutexSNapCounter.lock();
	_sNAPCounterOfCNapIDsIt = _sNAPCounterOfCNapIDs.find(
			multicastIpAddress.uint());
	// if former registration  for ths multicast IP exists
	if (_sNAPCounterOfCNapIDsIt == _sNAPCounterOfCNapIDs.end()) {
		_mutexSNapCounter.unlock();
		LOG4CXX_ERROR(logger, "Dropping packet, unknown group, group: "
				<< multicastIpAddress.str());
		return;
	}
	list<string> * _cNapNids = _sNAPCounterOfCNapIDsIt->second;
	_mutexSNapCounter.unlock();
	if (_cNapNids->empty()) {
		LOG4CXX_ERROR(logger, "Dropping packet, no registered cNAPs, group: "
				<< multicastIpAddress.str());

		delete dataCID_ptr;
		return;
	}
	_mutexDataFwdState.lock();
	_dataFwdStateIt = _dataFwdState.find(dataCID_ptr->uint());
	if (!_dataFwdStateIt->second) // If multicast packet has to be hold back (pause)
	{
		_mutexDataFwdState.unlock();
		_bufferPacket(*dataCID_ptr, packet, packetSize);
		LOG4CXX_DEBUG(logger, "Re-advertising information item, DID: "
				<< dataCID_ptr->printPrefixId()<<"/"<<dataCID_ptr->id());
		_icnCore->publish_info(dataCID_ptr->binId(), dataCID_ptr->binPrefixId(),
				DOMAIN_LOCAL, NULL, 0);
		delete dataCID_ptr;
		return;
	}
	_mutexDataFwdState.unlock();
	_tryPublishData(*dataCID_ptr, packet, packetSize, _cNapNids,
			&multicastIpAddress);
	delete dataCID_ptr;
}

void MCast::handlePUBLISHED_DATA(IcnId icnId, Event * event) {
	int cntrlOrData = _cntrl1_data2(icnId);
	if (cntrlOrData == 1) {
		LOG4CXX_WARN(logger, "Dropping request, incompatible <CTRL> scope type,"
				" CID: " << icnId.print());
	} else if (cntrlOrData == 2) {
		LOG4CXX_DEBUG(logger, " <PUBLISHED_DATA> to <DATA> ID "
				<< icnId.print() << "");
		_handleMCastDataAtCNap(event);
	} else
		LOG4CXX_ERROR(logger, "Dropping request, unknown subscope, CID: "
				<< icnId.print());
}

void MCast::handlePUBLISHED_DATA_iSUB(IcnId rCId, Event * event) {
	int cntrlOrData = _cntrl1_data2(rCId);
	if (cntrlOrData == 1) {
		LOG4CXX_WARN(logger, "Dropping request, incompatible scope type, "
				"<CTRL> CID: " << rCId.print());
	} else if (cntrlOrData == 2) {
		if (isServerNAP == false) {
			LOG4CXX_DEBUG(logger, "Dropping request, server mode disabled, "
					"CID: " << rCId.print());
			return;
		}
		LOG4CXX_DEBUG(logger, "Scope type: <DATA>, ID: " << rCId.print());
		_handleCtrlPub_dataISub(event);
		_forwardingData(rCId, true);
	} else
		LOG4CXX_ERROR(logger, "Dropping request, unknown scope (neither <CTRL> "
				"or <DATA> scope), CID: " << rCId.print());
}

void MCast::handleSTART_PUBLISH(IcnId icnId) {
	int cntrlOrData = _cntrl1_data2(icnId);
	if (cntrlOrData == 1) {
		LOG4CXX_DEBUG(logger, "<CTRL> scope: "<< icnId.print());
		_startPublishFromCtrlBuffer(icnId);
	} else if (cntrlOrData == 2) {
		LOG4CXX_ERROR(logger, "Dropping request, incompatible subscope type, "
				"<DATA> scope: " << icnId.print());
	} else
		LOG4CXX_ERROR(logger, "Dropping request, unknown subscope type, ID: "
				<< icnId.print());
}

void MCast::handleSTART_PUBLISH_iSUB(IcnId rCId) {
	int cntrlOrData = _cntrl1_data2(rCId);
	if (cntrlOrData == 1) {
		LOG4CXX_WARN(logger, "Dropping request, incompatible type of scope, "
				"<CTRL> scope: " << rCId.print() << "");
	} else if (cntrlOrData == 2) {
		if (isServerNAP == false) {
			LOG4CXX_DEBUG(logger, "Dropping request, server mode is not "
					"enabled, DID: " << rCId.print());
			return;
		}
		LOG4CXX_DEBUG(logger, "<DATA> ID: " << rCId.print());
		_forwardingData(rCId, true);
		_publishFromDataBuffer(rCId);
	} else
		LOG4CXX_ERROR(logger, "Dropping request, unknown type (neither <CTRL> "
				"or <DATA> scope), ID: " << rCId.print());
}
/*
 * YT: Most probably a call-back method for the corresponding Blackadder function
 * I do not think it is used in IGMP handler though.
 * */
void MCast::subscribeScope(IcnId & icnId) {
	int cntrlOrData = _cntrl1_data2(icnId);
	if (cntrlOrData == 1) {
		_icnCore->subscribe_scope(icnId.binId(), icnId.binPrefixId(),
				DOMAIN_LOCAL, NULL, 0);
		LOG4CXX_DEBUG(logger, "<CTRL> subscriber scope: " << icnId.print());
	} else if (cntrlOrData == 2) {
		_icnCore->subscribe_scope(icnId.binId(), icnId.binPrefixId(),
				DOMAIN_LOCAL, NULL, 0);
		LOG4CXX_DEBUG(logger, "<DATA> subscribe scope " << icnId.print());
	} else
		LOG4CXX_WARN(logger, "Unknown subscope (neither <CTRL> or <DATA> "
				"scope): " << icnId.print());
}

/*
 * "load" props from configuration object 
 *  performs some checks 
 * */
void MCast::_loadSNapProps()
{
	if (GENERAL_QUERY_TIMER < 1 || GENERAL_QUERY_TIMER > 999999) {
		GENERAL_QUERY_TIMER = 120000;
		LOG4CXX_WARN(logger, "GENERAL_QUERY_TIMER out of bounds (1-999,999ms), "
				"set to " << GENERAL_QUERY_TIMER);
	}
	LEAVE_TIMER = _configuration.igmp_genLeaveTimer();
	if (LEAVE_TIMER < 1 || LEAVE_TIMER > 100000) {
		LEAVE_TIMER = 10000;
		LOG4CXX_WARN(logger, "LEAVE_TIMER out of bounds (1-100,000ms), set to "
				<< LEAVE_TIMER);
	}
	GENERAL_QUERY_IP = _configuration.igmp_genQueryIP();
	_groupLeaveIP = _configuration.igmp_groupLeaveIP();
	REDUNDANT_IGMP_NUM = _configuration.igmp_redundantIGMP();
	if (REDUNDANT_IGMP_NUM < 1 || REDUNDANT_IGMP_NUM > 10) {
		REDUNDANT_IGMP_NUM = 2;
		LOG4CXX_WARN(logger, "REDUNDANT_IGMP_NUM out of bounds (1-10), set to "
				<< REDUNDANT_IGMP_NUM);
	}
	DROM_AFTER_QUER_NUM = _configuration.igmp_dropAfterQueriesNum();
	if (DROM_AFTER_QUER_NUM < 1 || DROM_AFTER_QUER_NUM > 10) {
		DROM_AFTER_QUER_NUM = 3;
		LOG4CXX_WARN(logger, "DROM_AFTER_QUER_NUM out of bounds (1-10), set to "
				<< DROM_AFTER_QUER_NUM);
	}
	MESSAGE_PROCESSING_LAG = _configuration.igmp_message_processing_lag();
	if (MESSAGE_PROCESSING_LAG < 10000 || MESSAGE_PROCESSING_LAG > 5000000) {
		MESSAGE_PROCESSING_LAG = 500000;
		LOG4CXX_WARN(logger, "MESSAGE_PROCESSING_LAG out of bounds (10000-5000,"
				"000ns), set to " << MESSAGE_PROCESSING_LAG);
	}
	if (_configuration.igmp_sNapMCastIPs().length() > 0) {
		istringstream iss(_configuration.igmp_sNapMCastIPs());
		vector<string> tmp_sNapMCastIPs;
		copy(istream_iterator<string>(iss), istream_iterator<string>(),
				back_inserter(tmp_sNapMCastIPs));
		for (auto it = tmp_sNapMCastIPs.begin(); it != tmp_sNapMCastIPs.end();
				it++) {
			string s = *it;
			string delimiter = ":";
			string grp = s.substr(0, s.find(delimiter));
			string port = s.substr(s.find(delimiter) + 1);
			_sNapMCastIPs[grp] = atoi(port.c_str());
		}
	}

	if (_configuration.igmp_ignoreIGMPFrom().length() > 0) {
		istringstream iss(_configuration.igmp_ignoreIGMPFrom());
		vector<string> tmmp;
		copy(istream_iterator<string>(iss), istream_iterator<string>(),
				back_inserter(tmmp));
		for (auto it = tmmp.begin(); it != tmmp.end(); it++) {
			_ignoreIGMPFrom.push_back(*it);
		}
	}

	if (_configuration.igmp_ignoreMCastDataFrom().length() > 0) {
		LOG4CXX_INFO(logger, "Parser: drop data messages from: ");
		istringstream iss(_configuration.igmp_ignoreMCastDataFrom());
		vector<string> tmmp;
		copy(istream_iterator<string>(iss), istream_iterator<string>(),
				back_inserter(tmmp));
		for (auto it = tmmp.begin(); it != tmmp.end(); it++) {
			LOG4CXX_INFO(logger, "\t" << *it << "");
			_ignoreMCastDataFrom.push_back(*it);
		}
	}
	//YT:: get server operation mode to address loopedback packets
	if (_configuration.igmp_napOperationMode().compare("clientNAP") == 0) {
		isClientNAP = true;
		isServerNAP = false;
	} else if (_configuration.igmp_napOperationMode().compare("serverNAP")
			== 0) {
		isClientNAP = false;
		isServerNAP = true;
	}
}

void MCast::_bufferPacket(IcnId &cId, uint8_t *packet, uint16_t & packetSize) {
	_mutexBuffer.lock();
	_bufferIt = _packetBuffer.find(cId.uint());
	// CID unknown
	if (_bufferIt == _packetBuffer.end()) {
		pair<IcnId, packet_t> value;
		value.first = cId;
		value.second.packet = (uint8_t *) malloc(packetSize);
		memcpy(value.second.packet, packet, packetSize);
		value.second.packetSize = packetSize;
		value.second.timestamp =
				boost::posix_time::microsec_clock::local_time();
		_packetBuffer.insert(
				pair<uint32_t, pair<IcnId, packet_t>>(cId.uint(), value));
		LOG4CXX_DEBUG(logger, "ID " << cId.print());
	} else {
		free((*_bufferIt).second.second.packet); // Deleting memory in heap
		(*_bufferIt).second.second.packet = (uint8_t *) malloc(packetSize);
		memcpy((*_bufferIt).second.second.packet, packet, packetSize);
		(*_bufferIt).second.second.packetSize = packetSize;
		(*_bufferIt).second.second.timestamp =
				boost::posix_time::microsec_clock::local_time();
		LOG4CXX_DEBUG(logger, "Packet update, ID " << cId.print());
	}
	_mutexBuffer.unlock();
}

void MCast::_deleteBufferedPacket(IcnId & cid) {
	_mutexBuffer.lock();
	_bufferIt = _packetBuffer.find(cid.uint());
	if (_bufferIt == _packetBuffer.end()) {
		LOG4CXX_DEBUG(logger, "local packet buffer has no packet for CID "
				<< cid.print() );
		_mutexBuffer.unlock();
		return;
	}
	_packetBuffer.erase(_bufferIt);
	LOG4CXX_DEBUG(logger,"packet for CID " << cid.print() << " deleted from "
			"local packet buffer" );
	_mutexBuffer.unlock();
}

int MCast::_cntrl1_data2(IcnId & cId) {
	string dataOrCtrlScope = cId.scopeId(1);
	if (dataOrCtrlScope.compare(_controlScope_ptr->id()) == 0) {
		return 1;
	}
	if (dataOrCtrlScope.compare(_dataScope_ptr->id()) == 0) {
		return 2;
	}

	return -1;
}
/*
 * YT: are buffered packets cleared..?
 * */
/**
 * YT: sleeps "leave response period" seconds and then checks is the counter of a multicast group is equal to max value  (DROM_AFTER_QUER_NUM).
 * 
 * It is executed by a dedicated thread created by the main thread upon the receipt of a group leave message.
 * The thread ends after method's return.
 * 
 * Returns "true" if group's counter is refreshed, "false" if group is not found or counter is not refreshed 
 * */
void MCast::_removeGroupAfterLeave(IpAddress groupAddr) {
	// YT: sleep until the leave response period is over
	this_thread::sleep_for(chrono::milliseconds(LEAVE_TIMER));

	_mutexMcastDB_cNap.lock();
	_mcastDB_cNapIt = _mcastDB_cNap.find(groupAddr.uint());
	//YT: group is registered
	if (_mcastDB_cNapIt != _mcastDB_cNap.end()) {
		uint32_t counter = _mcastDB_cNapIt->second;
		_mutexMcastDB_cNap.unlock();
		//YT: counter is not refreshed, then delete group and notify sNAP
		if (counter >= DROM_AFTER_QUER_NUM) {
			LOG4CXX_DEBUG(logger, "Deleting group, counter not refreshed, "
					"group: "<<groupAddr.str());
			_mcastDB_cNapErase(&groupAddr);
			uint16_t packetSize = sizeof(IGMP_LEAVE_GROUP)
					+ sizeof(groupAddr.uint());
			uint8_t * packet = (uint8_t *) malloc(packetSize);
			_prepareIgmpPacket(groupAddr, IGMP_LEAVE_GROUP, packet);
			IcnId ctrlCID = _ctrlScopeIds[groupAddr.uint()];
			_mutexCtrlFwdState.lock();
			_ctrlFwdStateIt = _ctrlFwdState.find(ctrlCID.uint());
			if (_ctrlFwdStateIt != _ctrlFwdState.end()
					&& !_ctrlFwdStateIt->second) // If ctrl packet has to be hold back (pause)
			{
				_mutexCtrlFwdState.unlock();
				_bufferPacket(ctrlCID, packet, packetSize);
				LOG4CXX_DEBUG(logger, "<CTRL> packet buffered because the FWD "
						"state of ctrlCID " << ctrlCID.print()
						<< " is disabled. Re-advertising <CTRL> CID "
						<< ctrlCID.id() << " under father scope "
						<< ctrlCID.printPrefixId());
				_icnCore->publish_info(ctrlCID.binId(), ctrlCID.binPrefixId(),
						DOMAIN_LOCAL, NULL, 0);
				return;
			}
			_mutexCtrlFwdState.unlock();
			IcnId* dataCID_ptr = IcnId::createIGMPDataFromCtrlScopeId(ctrlCID);
			LOG4CXX_DEBUG(logger, "dataCID_ptr: "<<dataCID_ptr->print());
			_publishIGMPCtrl_data_isub(ctrlCID, *dataCID_ptr, packet,
					packetSize);
			_tryClearCNapStateFor(packet, &ctrlCID, &groupAddr);
			free(packet);
			delete dataCID_ptr;
		} else {
			//YT: group is alive, other clients exist despite received group leave 
			LOG4CXX_DEBUG(logger, "Keeping group, counter was refreshed, "
					"group: "<<groupAddr.str());
		}
	} else {
		LOG4CXX_WARN(logger, "Leaving, group not found, group "
				<< groupAddr.str() <<" (Either this is a duplicate group leave "
				"or there is a problem in cNAP's state.)");
		_mutexMcastDB_cNap.unlock();
	}
	return;
}

/**
 * YT: creates a query packet and send its to the attached (?) multicast clients
 * Usually triggers by "_cNapGeneralQueriesTimer" which is ran by dedicated thread.
 * Can also be called by main thread when handing group leave message in "_handleIGMPLeave"
 * */
void MCast::_cNAP2IPClientQueries(IpAddress mcast) {

	bool isGnrl = mcast.str().compare("0.0.0.0") == 0;

	int fd; // socket file descriptor
	string str =
			isGnrl ?
					"*general* query" :
					"group specific query, group: " + mcast.str() + "";
	/* this is protocol num 2 (i.e. IGMP) socket */
	if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP)) < 0) {
		LOG4CXX_ERROR(logger, "Socket failure while trying to " << " send"
				<< str << " to IP clients");
		return;
	}
	/* set up destination address */
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(0);
	addr.sin_addr.s_addr =
			isGnrl ?
					inet_addr(GENERAL_QUERY_IP.c_str()) :
					inet_addr(mcast.str().c_str());
	int packetSz = 8;
	u_int8_t igmpPayload[packetSz], checksum[packetSz];
	struct igmp *igmpHdrOut;
	igmpHdrOut = (struct igmp *) igmpPayload;
	igmpHdrOut->igmp_type = IGMP_MEMBERSHIP_QUERY; //8bit
	igmpHdrOut->igmp_code = isGnrl ? 0x64 : 0xA; //8bit Max Time Response
	//for general 10 seconds (0x64=100 x 0.1 sec))
	//for group specific 1 second (0xA=10 x 0.1 sec))
	igmpHdrOut->igmp_cksum = 0; //16bit
	igmpHdrOut->igmp_group.s_addr = mcast.inAddress().s_addr; //32bit
	/* Copy the packet to the tmp checksum array, then compute and set the checksum. */
	memcpy(checksum, igmpHdrOut, sizeof(struct igmp));
	igmpHdrOut->igmp_cksum = _cksum((u_int16_t *) checksum,
			sizeof(struct igmp)); // sizeof (struct igmp) should be 8 bytes
	/* now just sendto() our mcast group! */
	const struct sockaddr * addrPtr = (struct sockaddr *) &addr;
	LOG4CXX_DEBUG(logger, "sock address "
			<< inet_ntoa(((struct sockaddr_in *)addrPtr)->sin_addr));
	// XG: If general query, send one, otherwise send multiple
	int times = isGnrl ? 1 : REDUNDANT_IGMP_NUM; // send redundant queries only for the case of group-specific
	while (times--) {
		if (sendto(fd, igmpPayload, packetSz, 0, addrPtr,
				sizeof(struct sockaddr)) < 0) {
			LOG4CXX_ERROR(logger, "Socket \"sendto\" failure while trying to "
					"send " << str << " message from cNAP. GENERAL_QUERY_IP = "
					<< GENERAL_QUERY_IP << " Error code: " << strerror(errno));
		} else {
 			LOG4CXX_DEBUG(logger, "sent " << str << " message from fd "<< fd);
		}
	}
	close(fd);

}
/**
 * YT: method runs by dedicated thread 
 * 1. increases unanswered query counter per group 
 * 2. removes groups with expired counters
 * 3. triggers emition of generic queries 
 * */
void MCast::_cNapGeneralQueriesTimer(uint32_t period) {
	//    this is the function task to be called, i.e. the query sender
	//    function<typename result_of < callable(arguments...)>::type() >
	//    task(bind(forward<callable>(mthd), forward<arguments>(args)...));
	this_thread::sleep_for(chrono::milliseconds(1000));

	_mutexMcastDB_cNap.lock();
	bool keepRunning = _runDetachedThds;
	_mutexMcastDB_cNap.unlock();

	while (keepRunning && isClientNAP == true) {
		LOG4CXX_DEBUG(logger, "General query timer expired");
		_mutexMcastDB_cNap.lock();
		LOG4CXX_DEBUG(logger, "DB contains #" << _mcastDB_cNap.size());
		//  - check the membership reports to confirm that the groups are still active
		// for each group..
		_mcastDB_cNapIt = _mcastDB_cNap.begin();
		//YT: fix inf loop on user implicit leave
		while (_mcastDB_cNapIt != _mcastDB_cNap.end()
				&& _mcastDB_cNap.size() > 0) {
			IpAddress mcast(_mcastDB_cNapIt->first);
			uint32_t counter = _mcastDB_cNapIt->second;
			// -- drop any group not reported after 2 - 3 queries with no answer
			if (counter >= DROM_AFTER_QUER_NUM) {
				LOG4CXX_DEBUG(logger, "Removing group, max unanswered queries, "
						"group: "<<mcast.str());
				_mutexMcastDB_cNap.unlock(); //need to unlock due to internal locking in method _forceLeave..
				// must also update iterator _mcastDB_cNapIt
				_forceLeave(mcast);
				_mutexMcastDB_cNap.lock();
				_mcastDB_cNapIt = _mcastDB_cNap.begin();
				//reset iterator
			}
			//update iterator
			_mcastDB_cNapIt++;
		}
		//YT: created second loop because single loop with removals got weird results  
		_mcastDB_cNapIt = _mcastDB_cNap.begin();
		while (_mcastDB_cNapIt != _mcastDB_cNap.end()) {
			IpAddress mcast(_mcastDB_cNapIt->first);
			uint32_t counter = _mcastDB_cNapIt->second;
			if (counter >= DROM_AFTER_QUER_NUM) {
				_mcastDB_cNap[_mcastDB_cNapIt->first] = ++counter;
				LOG4CXX_DEBUG(logger, "Increased counter-to-leave to "
						<< to_string(counter)<< ", group: " << mcast.str());
			}
			//update iterator
			_mcastDB_cNapIt++;
		}
		// Now send the general queries
		IpAddress mcast("0.0.0.0");
		_cNAP2IPClientQueries(mcast);

		keepRunning = _runDetachedThds;
		_mutexMcastDB_cNap.unlock();

		// sleep my precious
		this_thread::sleep_for(chrono::milliseconds(period));
	}
}

int MCast::_incSNapCounterFor(IpAddress &multicastIpAddress,
		string & cNapNodeId) {
	_mutexSNapCounter.lock();
	_sNAPCounterOfCNapIDsIt = _sNAPCounterOfCNapIDs.find(
			multicastIpAddress.uint());
	list<string>* cNapNids;
	// if former registration for ths multicast IP exists
	if (_sNAPCounterOfCNapIDsIt != _sNAPCounterOfCNapIDs.end()) {
		cNapNids = _sNAPCounterOfCNapIDsIt->second;

		auto cNapNidsIt = find(cNapNids->begin(), cNapNids->end(), cNapNodeId);
		if (cNapNidsIt == cNapNids->end()) {
			cNapNids->push_back(cNapNodeId);
			int num = cNapNids->size();
			_mutexSNapCounter.unlock();
			LOG4CXX_DEBUG(logger, "Increased counter, counter: " << num
					<< ", new cNAP: " << cNapNodeId << ", group: "
					<< multicastIpAddress.str());
			return num;
		} else {
			LOG4CXX_DEBUG(logger, "Did not increase counter, registered cNAP,"
					" counter: " << cNapNids->size() << ", known cNAP: "
					<< cNapNodeId << ", group: " << multicastIpAddress.str());
			_mutexSNapCounter.unlock();
			return -1;
		}
	}
	//otherwise this is ther first registration ever for this multicast address in the sNAP
	LOG4CXX_DEBUG(logger, "Did not increase counter, unknown group, counter: - "
			", known cNAP: " << cNapNodeId << ", group: "
			<< multicastIpAddress.str());
	_mutexSNapCounter.unlock();
	return -2;
}

int MCast::_decrSNapCounterFor(IpAddress &multicastIpAddress,
		string & cNapNodeId) {

	_mutexSNapCounter.lock();
	_sNAPCounterOfCNapIDsIt = _sNAPCounterOfCNapIDs.find(
			multicastIpAddress.uint());

	list<string>* cNapNids;

	// if this is a valid multicast IP address in this sNAP
	if (_sNAPCounterOfCNapIDsIt != _sNAPCounterOfCNapIDs.end()) {
		cNapNids = _sNAPCounterOfCNapIDsIt->second;
		int num = cNapNids->size();

		auto cNapNidsIt = find(cNapNids->begin(), cNapNids->end(), cNapNodeId);
		if (cNapNidsIt != cNapNids->end()) {
			cNapNids->erase(cNapNidsIt);
			num = cNapNids->size();
			_mutexSNapCounter.unlock();
			LOG4CXX_DEBUG(logger, "Descreased counter, counter: " << num
					<< ", dropped cNAP: " << cNapNodeId << ", group: "
					<< multicastIpAddress.str());
			return num;
		}

		_mutexSNapCounter.unlock();
		LOG4CXX_DEBUG(logger, "Did not descrease counter, not registered cNAP, "
				"counter: " << num << ", cNAP: " << cNapNodeId << ", group: "
				<< multicastIpAddress.str());
		return -1;
	}

	_mutexSNapCounter.unlock();
	return -2;
}

void MCast::_forwardingData(IcnId &dataCID, bool state) {
	_mutexDataFwdState.lock();
	_dataFwdStateIt = _dataFwdState.find(dataCID.uint());
	// must be initialised by initilise()
	_dataFwdStateIt->second = state;
	_mutexDataFwdState.unlock();
	LOG4CXX_DEBUG(logger, "" << (state ? "En" : "Dis")
			<< "abled  <DATA> FWD state for DID " << dataCID.print());
}

void MCast::_forwardingCtrl(IcnId &ctrlCID, bool state) {
	_mutexCtrlFwdState.lock();
	_ctrlFwdStateIt = _ctrlFwdState.find(ctrlCID.uint());
	// FWD Must be initialised by mem_report or leave
	_ctrlFwdStateIt->second = state;
	_mutexCtrlFwdState.unlock();
	LOG4CXX_DEBUG(logger, ""<<(state ? "En" : "Dis") << "abled <CTRL> FWD state"
			" for CID " << ctrlCID.print());
}

void MCast::_tryClearCNapStateFor(uint8_t *packet, IcnId* ctrlCID_ptr,
	IpAddress * multicastIpAddress_ptr)
{
	u_int8_t igmp_type;
	memcpy(&igmp_type, packet, sizeof(u_int8_t));

	if (igmp_type == IGMP_LEAVE_GROUP) {

		usleep(MESSAGE_PROCESSING_LAG);
		LOG4CXX_DEBUG(logger, "Deleting state, group: "
				<< multicastIpAddress_ptr->str() << ", unpublishing CID: "
				<< ctrlCID_ptr->print() << "");

		if (multicastIpAddress_ptr == NULL)
			(*multicastIpAddress_ptr) = IcnId::extractIPAddrFromCId(
					*ctrlCID_ptr);
		// delete fwd state for this control message
		_mutexCtrlFwdState.lock();
		_ctrlFwdStateIt = _ctrlFwdState.find(ctrlCID_ptr->uint());
		_ctrlFwdState.erase(_ctrlFwdStateIt);
		_mutexCtrlFwdState.unlock();
		_mcastDB_cNapErase(multicastIpAddress_ptr);
		_mutexCtrlScopeIds.lock();
		_ctrlScopeIds.erase(multicastIpAddress_ptr->uint());
		_mutexCtrlScopeIds.unlock();
		_icnCore->unpublish_info(ctrlCID_ptr->binId(),
				ctrlCID_ptr->binPrefixId(), DOMAIN_LOCAL, NULL, 0);
	}
}

void MCast::_extractFromPacket(uint8_t *packet, unsigned int packetSize,
		IpAddress & sourceIpAddress, IpAddress &multicastIpAddress, char** data,
		u_int16_t * dataSz) {

	struct ip * iph = (struct ip *) packet; // ip header
	unsigned short ipHdrlen = iph->ip_hl * 4; // header length

	sourceIpAddress = iph->ip_src.s_addr;
	multicastIpAddress = iph->ip_dst.s_addr;

	struct udphdr *udph = (struct udphdr*) (packet + ipHdrlen); // udp header
	unsigned short udpHdrlen = sizeof(udph); // header length

	if (data != NULL) {
		*data = (char *) packet + ipHdrlen + udpHdrlen;
		*dataSz = packetSize - udpHdrlen - ipHdrlen;
	}
}

void MCast::_prepareIgmpPacket(IpAddress &multicastIpAddress,
		const unsigned char igmpHdrType, uint8_t * packet) {
	uint8_t offset = 0;
	// [1] include node LEAVE type
	memcpy(packet, &igmpHdrType, sizeof(igmpHdrType));
	offset += sizeof(uint8_t);
	// [2] include multicast IP Address
	memcpy(packet + offset, multicastIpAddress.uintPointer(), sizeof(uint32_t));
	stringstream ss;
	ss << igmpHdrType;
	LOG4CXX_DEBUG(logger, "<IGMP type=" << (unsigned) igmpHdrType
			<< "; mcast=" << multicastIpAddress.str() << ">");
}

void MCast::_startPublishFromCtrlBuffer(IcnId ctrlCID) {
	_mutexBuffer.lock();
	_bufferIt = _packetBuffer.find(ctrlCID.uint());
	if (_bufferIt == _packetBuffer.end()) {
		LOG4CXX_DEBUG(logger, "*NO* <CTRL> packet for CID " << ctrlCID.print());
		_mutexBuffer.unlock();
		return;
	}
	_mutexBuffer.unlock();

	LOG4CXX_DEBUG(logger, "buffer has <CTRL> packet for CID "
			<< ctrlCID.print());

	_mutexCtrlFwdState.lock();
	_ctrlFwdStateIt = _ctrlFwdState.find(ctrlCID.uint());

	if (_ctrlFwdStateIt->second) {
		_mutexCtrlFwdState.unlock();
		_mutexBuffer.lock();
		uint8_t *packet = (*_bufferIt).second.second.packet;
		uint16_t packetSize = (*_bufferIt).second.second.packetSize;
		_mutexBuffer.unlock();

		//otherwise, normally publish ctrl
		IcnId* dataCID_ptr = IcnId::createIGMPDataFromCtrlScopeId(ctrlCID);
		_publishIGMPCtrl_data_isub(ctrlCID, *dataCID_ptr, packet, packetSize);
		LOG4CXX_DEBUG(logger, "<CTRL> CID "<< ctrlCID.print() << " <DATA> CID "
				<< dataCID_ptr->print() << "");

		_tryClearCNapStateFor(packet, &ctrlCID);

		delete dataCID_ptr;
		_deleteBufferedPacket(ctrlCID);
	} else {
		LOG4CXX_ERROR(logger, "<CTRL> FWD disabled for "<< ctrlCID.print());
	}
	_mutexCtrlFwdState.unlock();
}

void MCast::_publishIGMPCtrl_data_isub(IcnId &ctrlCID, IcnId &dataCID,
		uint8_t * packet, unsigned int packetSize) {

	//otherwise, normally publish ctrl
	if (_icnCore->publish_data_isub(ctrlCID.binIcnId(), DOMAIN_LOCAL, NULL, 0,
			dataCID.binIcnId(), packet, packetSize)) {
		LOG4CXX_DEBUG(logger, "CID: " << ctrlCID.print() << ", DID: "
				<< dataCID.print());
		return;
	}
	LOG4CXX_ERROR(logger, "FAILED to sent, CID: " << ctrlCID.print()
			<< ", DID: " << dataCID.print());
}

void MCast::_publishFromDataBuffer(IcnId & dataCID) {
	_mutexBuffer.lock();
	_bufferIt = _packetBuffer.find(dataCID.uint());
	if (_bufferIt == _packetBuffer.end()) {
		LOG4CXX_DEBUG(logger, "leaving, no buffered packet, CID "
				<< dataCID.print());
		_mutexBuffer.unlock();
		return;
	}
	_mutexDataFwdState.lock();
	if (_dataFwdStateIt->second) {
		_mutexBuffer.lock();
		uint8_t *packet = (*_bufferIt).second.second.packet;
		uint16_t packetSize = (*_bufferIt).second.second.packetSize;
		_mutexBuffer.unlock();
		_tryPublishData(dataCID, packet, packetSize);
		_deleteBufferedPacket(dataCID);

	} else {
		LOG4CXX_DEBUG(logger, "Dropping request, <FWD> state is disabled, "
				"<DATA> CID " << dataCID.print());
	}
	_mutexDataFwdState.unlock();
	_mutexBuffer.unlock();
}

void MCast::_tryPublishData(IcnId &dataCID, uint8_t * packet,
		unsigned int packetSize, list<string> * _cNapNids,
		IpAddress * multicastIpAddress) {
	if (_cNapNids == NULL) {
		_mutexSNapCounter.lock();
		_sNAPCounterOfCNapIDsIt = _sNAPCounterOfCNapIDs.find(
				multicastIpAddress->uint());
		// if former registration  for ths multicast IP exists
		if (_sNAPCounterOfCNapIDsIt == _sNAPCounterOfCNapIDs.end()) {
			_mutexSNapCounter.unlock();
			LOG4CXX_ERROR(logger, "Dropping packet, unknown group: "
					<< multicastIpAddress->str());
			return;
		}
		_cNapNids = _sNAPCounterOfCNapIDsIt->second;
		_mutexSNapCounter.unlock();
	}
	if (multicastIpAddress == NULL)
		IcnId::extractIPAddrFromCId(dataCID);

	if (!_cNapNids->empty()) {
		if (_icnCore->publish_data(dataCID.binIcnId(), DOMAIN_LOCAL, NULL, 0,
				(*_cNapNids), packet, packetSize)) {
			LOG4CXX_DEBUG(logger, "<DATA> CID: " << dataCID.print()
					<< ", packet size: "<<packetSize);
		} else {
			LOG4CXX_ERROR(logger, "Dropping packet, Blackadder return error, "
					"DID: " << dataCID.print());
		}
		return;
	}
	LOG4CXX_ERROR(logger, "Dropping packet, no registered cNAPs, <DATA> CID "
			<< dataCID.print());
}

void MCast::_handleMCastDataAtCNap(Event * event) {
	uint8_t *packet = (uint8_t*) event->data;
	unsigned int packetSize = event->data_len;

	char* data;
	u_int16_t dataSz;

	IpAddress sourceIpAddress;
	IpAddress multicastIpAddress;

	_extractFromPacket(packet, packetSize, sourceIpAddress, multicastIpAddress,
			&data, &dataSz);

	pair<int, struct sockaddr_in *> fd_sockAddr;
	_mutexMcastDB_cNap.lock(); // <- ΥΤ changed this to "lock()" from "unlock()"
	_mcastDB_cNapIPSocketsIt = _mcastDB_cNapIPSockets.find(
			multicastIpAddress.uint());
	if (_mcastDB_cNapIPSocketsIt == _mcastDB_cNapIPSockets.end()) {
		_mutexMcastDB_cNap.unlock();
		LOG4CXX_DEBUG(logger, "Multicast address "<< multicastIpAddress.str()
				<< " is unknown  multicast to clients has failed");
		return;
	}
	fd_sockAddr = _mcastDB_cNapIPSocketsIt->second;
	_mutexMcastDB_cNap.unlock();

	int fd = fd_sockAddr.first;
	const struct sockaddr * addrPtr = (struct sockaddr *) fd_sockAddr.second;
	//addrPtr->sin_port=htons(15555);
	int res_ = sendto(fd, data, dataSz, 0, addrPtr, sizeof(struct sockaddr));
	if (res_ < 0) {
		LOG4CXX_ERROR(logger, "sendto socket operation to multicast clients has"
				" failed, res_: " << res_ << " fd: " << fd << " addrPtr: "
				<< inet_ntoa(((struct sockaddr_in *)addrPtr)->sin_addr)
				<< " address.str(): " << multicastIpAddress.str()
				<< " dataSz: " << dataSz << " errno: " << strerror(errno));
	} else {
		LOG4CXX_DEBUG(logger, "successfully send content to local app via "
				"socket " << fd);
		string msg = string(data);
		_statistics.txIGMPBytes(dataSz);
		//iff this is the first packet received after joining.
		if (_statJoin2FirstDatumMap[multicastIpAddress.uint()] > 0) {
			clock_gettime(CLOCK_MONOTONIC_RAW, &spec);
			long now = spec.tv_sec * 1.0e3 + spec.tv_nsec / 1.0e6; // get time in milliseconds
			if (now > _statJoin2FirstDatumMap[multicastIpAddress.uint()]) { // prevent clock rollover
				_statistics.channelAcquisitionTime(multicastIpAddress.uint(),
						now	- _statJoin2FirstDatumMap[multicastIpAddress.uint()]);
				LOG4CXX_ERROR(logger, "Channel join time "
						<< now - _statJoin2FirstDatumMap[multicastIpAddress.uint()]
						 << " now " << now);
				_statJoin2FirstDatumMap[multicastIpAddress.uint()] = 0; // mark it as being counted

			}
		}
	}
}

void MCast::_handleCtrlPub_dataISub(Event * event) {
	stringstream ss;

	string nodeId = event->nodeId;
	uint8_t *packet = (uint8_t*) event->data;

	uint8_t offset = 0;

	u_int8_t igmp_type;
	memcpy(&igmp_type, packet + offset, sizeof(u_int8_t));
	offset += sizeof(u_int8_t);

	uint32_t mcastIp_uint;
	memcpy(&mcastIp_uint, packet + offset, sizeof(uint32_t));

	IpAddress multicastIpAddress(mcastIp_uint);

	ss << igmp_type;
	LOG4CXX_DEBUG(logger, "<IGMP type=" << (unsigned)igmp_type <<"; mcast="
			<< multicastIpAddress.str() << ">");

	_mutexSNapCounter.lock();
	_sNAPCounterOfCNapIDsIt = _sNAPCounterOfCNapIDs.find(mcastIp_uint);
	// if former registration  for ths multicast IP exists
	if (_sNAPCounterOfCNapIDsIt == _sNAPCounterOfCNapIDs.end()) {
		_mutexSNapCounter.unlock();
		LOG4CXX_WARN(logger, "Dropping request, unknown group, group: "
				<< multicastIpAddress.str());
		return;
	}
	_mutexSNapCounter.unlock();

	switch (igmp_type) {
	case IGMP_V1_MEMBERSHIP_REPORT:
	case IGMP_V2_MEMBERSHIP_REPORT: {
		if (_incSNapCounterFor(multicastIpAddress, nodeId) == 1) {
			LOG4CXX_DEBUG(logger, "forwarding <MEMBERSHIP_REPORT> to server, "
					"group: "<< multicastIpAddress.str());
			thread snd2IPSrvrThd = thread(&MCast::_sNapToIPTVServer, this,
					multicastIpAddress, IGMP_V2_MEMBERSHIP_REPORT);
			snd2IPSrvrThd.detach();
		};
		return;
	}
	case IGMP_LEAVE_GROUP:
		// case IGMP_V2_LEAVE_GROUP: not needed; both are equal to 0x16 :
	{
		if (_decrSNapCounterFor(multicastIpAddress, nodeId) == 0) {
			int fd; // socket file descriptor
			/* this is protocol num 2 (i.e. IGMP) socket */
			if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP)) < 0) {
				LOG4CXX_ERROR(logger, "Dropping <IGMP_LEAVE_GROUP>, socket "
						"failure, group "<< multicastIpAddress.str());
				return;
			}
			LOG4CXX_DEBUG(logger, "Forwarding <IGMP_LEAVE_GROUP> to server, "
					"group: "<< multicastIpAddress.str());
			_sNapToIPTVServer(multicastIpAddress, IGMP_LEAVE_GROUP);
			// try to delete possibly buffered mcast data
			// no need to keep them bufferred anymore ..
			string rCIdStr = chararray_to_hex(event->isubID);
			IcnId dataCID;
			dataCID = rCIdStr;
			_mutexBuffer.lock();
			_bufferIt = _packetBuffer.find(dataCID.uint());
			if (_bufferIt != _packetBuffer.end()) {
				_packetBuffer.erase(_bufferIt);
				LOG4CXX_DEBUG(logger, "Dropping buffered packets, no registered"
						" cNAPs, <DATA> CID: " << dataCID.print());
			}
			_mutexBuffer.unlock();
			close(fd); //YT
		}
		return;
	}
	default: {
		ss.str(string());
		ss << igmp_type;
		LOG4CXX_WARN(logger, "Dropping request, unknown IGMP type "
				<< ss.str().c_str());
		return;
	}
	} //switch
}

/* Copying the IGMP header to the checksum.
 *
 * According to RFC 2236:
 * 2.3.  Checksum
 *    The checksum is the 16-bit one's complement of the one's complement
 *    sum of the whole IGMP message (the entire IP payload). For computing
 *    the checksum, the checksum field is set to zero.  When transmitting
 *    packets, the checksum MUST be computed and inserted into this field.
 *    When receiving packets, the checksum MUST be verified before
 *    processing a packet.
 */
u_int16_t MCast::_cksum(u_int16_t * data, int32_t length) {
	int32_t nleft = length;
	u_int16_t * w = data;
	int32_t sum = 0;
	u_int16_t answer = 0;

	/* with help from https://github.com/yersinia/junkcode/blob/master/tool/t50/t50-2.45r-H2HC/src/cksum.c*/
	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}
	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_int8_t *) (&answer) = *(u_int8_t *) w;
		sum += answer;
	}
	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
	sum += (sum >> 16); /* add carry */
	answer = ~sum; /* truncate to 16 bits */
	return (answer);
}

void MCast::_sNapToIPTVServer(IpAddress mcast, uint8_t type) {

	string typeStr =
			type == IGMP_LEAVE_GROUP ? "<LEAVE>" :
			type == IGMP_V2_MEMBERSHIP_REPORT ?
					"<MEMBERSHIP_REPORT>" : "< ? ? ? >";

	int fd; // socket file descriptor
	/* this is protocol num 2 (i.e. IGMP) socket */
	if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP)) < 0) {
		LOG4CXX_ERROR(logger, "Socket failure while trying to send "
				<< typeStr << " to IPTV server");
		return;
	}
	/* set up destination address */
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;

	if (type == IGMP_LEAVE_GROUP) { //YT: send leave to leave-group mcast address
		addr.sin_addr.s_addr = inet_addr(_groupLeaveIP.c_str());
	} else
		addr.sin_addr.s_addr = inet_addr(mcast.str().c_str());

	int packetSz = 8;
	u_int8_t igmpPayload[packetSz], checksum[packetSz];

	struct igmp *igmpHdrOut;
	igmpHdrOut = (struct igmp *) igmpPayload;

	igmpHdrOut->igmp_type = type; //8bit, e.g. IGMP_V2_MEMBERSHIP_REPORT
	igmpHdrOut->igmp_code = 0; //8bit
	igmpHdrOut->igmp_cksum = 0; //16bit
	igmpHdrOut->igmp_group.s_addr = mcast.inAddress().s_addr; //32bit

	/* Copy the packet to the tmp checksum array, then compute and set the checksum. */
	memcpy(checksum, igmpHdrOut, sizeof(struct igmp));
	igmpHdrOut->igmp_cksum = _cksum((u_int16_t *) checksum,
			sizeof(struct igmp)); // sizeof (struct igmp) should be 8 bytes

	/* now just sendto() our mcast group! */
	const struct sockaddr * addrPtr = (struct sockaddr *) &addr;
	int times = 1 + REDUNDANT_IGMP_NUM;
	while (times--) {
		if (sendto(fd, igmpPayload, packetSz, 0, addrPtr,
				sizeof(struct sockaddr)) < 0) {
			LOG4CXX_ERROR(logger, "Socket \"sendto\" failure while trying to "
					"send " << typeStr << " message");
		} else {
			LOG4CXX_DEBUG(logger, "Message type: " << typeStr << ", dst: "
					<< inet_ntoa(((sockaddr_in *)addrPtr)->sin_addr));
		}
	}
	close(fd);
}
/**
 * ΥΤ: This method handles queries sent by upstream routers to the sNAP
 * */
void MCast::_handleIGMPQuery(IpAddress & sourceIpAddress,
		IpAddress &multicastIpAddress, struct igmp * igmpHdrIn) {
	int fd; // socket file descriptor
	/* this is protocol num 2 (i.e. IGMP) socket */
	if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP)) < 0) {
		LOG4CXX_ERROR(logger, "Socket failure while trying to respond to "
				"general query from source address " << sourceIpAddress.str());
		return;
	}
	// General Query - All hosts
	// Group-Specific Query - The group being queried
	IpAddress gnrlQ = IpAddress(GENERAL_QUERY_IP);
	// case this is a general query
	if (multicastIpAddress.uint() == gnrlQ.uint()) {
		LOG4CXX_DEBUG(logger, "General <IGMP_MEMBERSHIP_QUERY> received from "
				"host address: " << sourceIpAddress.str());
		// try to join to every active mcast group
		_mutexSNapCounter.lock();
		_sNAPCounterOfCNapIDsIt = _sNAPCounterOfCNapIDs.begin();
		while (_sNAPCounterOfCNapIDsIt != _sNAPCounterOfCNapIDs.end()) {
			IpAddress nxtMcast(_sNAPCounterOfCNapIDsIt->first);
			// if at least one cNAP for ths multicast IP exists
			if (_sNAPCounterOfCNapIDsIt->second->size() > 0) {
				_sNapToIPTVServer(nxtMcast, IGMP_V2_MEMBERSHIP_REPORT);
			} else {
				LOG4CXX_DEBUG(logger, "no active cNAPs for this multicast group"
						" address "<< nxtMcast.str()
						<< ". No <IGMP_V2_MEMBERSHIP_REPORT> to be sent to IP "
						"Multicast server");
			}
			++_sNAPCounterOfCNapIDsIt;
		}    //while
		_mutexSNapCounter.unlock();

		return;
	}    // if general query

	// Otherwise, this is a group specific query
	LOG4CXX_DEBUG(logger, "Group-specific <IGMP_MEMBERSHIP_QUERY> received"
			<< " for group address: " << "" << multicastIpAddress.str() << "" << " from host address: " << "" << sourceIpAddress.str() << "");

	_mutexSNapCounter.lock();
	_sNAPCounterOfCNapIDsIt = _sNAPCounterOfCNapIDs.find(
			multicastIpAddress.uint());
	if (_sNAPCounterOfCNapIDsIt == _sNAPCounterOfCNapIDs.end()
			|| _sNAPCounterOfCNapIDsIt->second->size() < 1) {
		LOG4CXX_DEBUG(logger, "Group-specific <IGMP_MEMBERSHIP_QUERY> ignored: "
				<< " Group inactive (no cNAPs registered)");
		_mutexSNapCounter.unlock();
		return;
	}
	_mutexSNapCounter.unlock();

	IpAddress nxtMcast(_sNAPCounterOfCNapIDsIt->first);
	_sNapToIPTVServer(nxtMcast, IGMP_V2_MEMBERSHIP_REPORT);
}

void MCast::_handleIGMPMemReport(IpAddress & sourceIpAddress,
		IpAddress &multicastIpAddress, struct igmp * igmpHdr) {

	if (!_ignoreIGMPFrom.empty())
		if (find(_ignoreIGMPFrom.begin(), _ignoreIGMPFrom.end(),
				sourceIpAddress.str()) != _ignoreIGMPFrom.end()) {
			LOG4CXX_DEBUG(logger, "*IGNORING* <MEMBERSHIP_REPORT> for group "
					"address: " << multicastIpAddress.str()
					<< " from ignored source address "
					<< sourceIpAddress.str());
			return;
		}
	LOG4CXX_DEBUG(logger, "group: " << multicastIpAddress.str()<< ", sender: "
			<< sourceIpAddress.str());

	_mutexMcastDB_cNap.lock();
	_mcastDB_cNapIt = _mcastDB_cNap.find(multicastIpAddress.uint());

	IcnId* dataCID_ptr = IcnId::createIGMPDataScopeId(&multicastIpAddress);
	IcnId* ctrlCID_ptr = IcnId::createIGMPCtrlScopeId(&multicastIpAddress);

	_mcastDB_cNap[multicastIpAddress.uint()] = 0; //set or reset the cNAP DB counter to zero

	LOG4CXX_DEBUG(logger, "Setting or resetting the cNAP DB counter to zero "
			"for " << multicastIpAddress.str());

	// if no state existing in registry yet
	if (_mcastDB_cNapIt == _mcastDB_cNap.end()) {
		LOG4CXX_DEBUG(logger, "handleIGMPMemReport: creating new group: "
				<< multicastIpAddress.str());
		//ΥΤ: registers the group to the cNAP
		clock_gettime(CLOCK_MONOTONIC_RAW, &spec);
		long now = spec.tv_sec * 1.0e3 + spec.tv_nsec / 1.0e6;
		_statJoin2FirstDatumMap[multicastIpAddress.uint()] = now;
		struct sockaddr_in * addr = (struct sockaddr_in *) malloc(
				sizeof(struct sockaddr_in));
		int fd;
		/* create what looks like an ordinary UDP socket */
		if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
			LOG4CXX_ERROR(logger, "Dropping request, socket for multicast "
					"clients was not created.");
			_mutexMcastDB_cNap.unlock();
			return;
		}
		/* set up destination address */
		memset(addr, 0, sizeof(struct sockaddr_in));
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = inet_addr(multicastIpAddress.str().c_str());
		uint16_t destPortNum = _sNapMCastIPs[multicastIpAddress.str()];
		addr->sin_port = htons(destPortNum);
		_mcastDB_cNapIPSockets[multicastIpAddress.uint()] = pair<int,
				struct sockaddr_in *>(fd, addr);

		_mutexMcastDB_cNap.unlock();
		_mutexCtrlScopeIds.lock();

		LOG4CXX_DEBUG(logger, "New DB entry for " << multicastIpAddress.str()
				<< " fd: "<<fd);

		_ctrlScopeIds.insert(
				pair<uint32_t, IcnId>(multicastIpAddress.uint(), *ctrlCID_ptr));
		_mutexCtrlScopeIds.unlock();

		_mutexCtrlFwdState.lock();
		_ctrlFwdStateIt = _ctrlFwdState.find(ctrlCID_ptr->uint());
		if (_ctrlFwdStateIt == _ctrlFwdState.end()) {
			_ctrlFwdState.insert(
					pair<uint32_t, bool>(ctrlCID_ptr->uint(), false));
		}
		_mutexCtrlFwdState.unlock();
		//YT: register cNAP as publisher in the related ICN group-name
		_icnCore->publish_info(ctrlCID_ptr->binId(), ctrlCID_ptr->binPrefixId(),
				DOMAIN_LOCAL, NULL, 0);
		LOG4CXX_DEBUG(logger, "published_info id: " << ctrlCID_ptr->binId()
				<< " scope: "<< ctrlCID_ptr->printPrefixId());

		//YT: group registration completed!
		//YT: create and send packet to sNAP
		uint8_t *packet;
		uint16_t packetSize = sizeof(igmpHdr->igmp_type)
				+ sizeof(multicastIpAddress.uint());
		packet = (uint8_t *) malloc(packetSize);
		_prepareIgmpPacket(multicastIpAddress, igmpHdr->igmp_type, packet);

		_mutexCtrlFwdState.lock();
		_ctrlFwdStateIt = _ctrlFwdState.find(ctrlCID_ptr->uint());
		if (!_ctrlFwdStateIt->second) // If ctrl packet has to be hold back (pause)
		{
			_mutexCtrlFwdState.unlock();
			_bufferPacket(*ctrlCID_ptr, packet, packetSize);
			_icnCore->publish_info(ctrlCID_ptr->binId(),
					ctrlCID_ptr->binPrefixId(), DOMAIN_LOCAL, NULL, 0);
			delete ctrlCID_ptr;
			delete dataCID_ptr;
			return;
		}
		_mutexCtrlFwdState.unlock();
		//YT: send message to sNAP
		_publishIGMPCtrl_data_isub(*ctrlCID_ptr, *dataCID_ptr, packet,
				packetSize);
		free(packet);
	} else {
		_mutexMcastDB_cNap.unlock();
		LOG4CXX_DEBUG(logger, "group exists");
	}

	delete ctrlCID_ptr;
	delete dataCID_ptr;
}

void MCast::_handleIGMPLeave(IpAddress & sourceIpAddress,
		IpAddress &multicastIpAddress, struct igmp * igmpHdr) {
	//YT: If its from ignored source then drop it
	if (!_ignoreIGMPFrom.empty())
		if (find(_ignoreIGMPFrom.begin(), _ignoreIGMPFrom.end(),
				sourceIpAddress.str()) != _ignoreIGMPFrom.end()) {
			LOG4CXX_DEBUG(logger,"Dropping request, ignored source: "
					<< sourceIpAddress.str()<< " group: "
					<< multicastIpAddress.str());
			return;
		}
	IpAddress groupAddress(&igmpHdr->igmp_group);
	_mutexMcastDB_cNap.lock();
	//YT: if it concerns a valid-known group address
	if (multicastIpAddress.str().compare(_groupLeaveIP) == 0) {
		LOG4CXX_DEBUG(logger, "Group: "<<groupAddress.str() << ", sender: "
				<< sourceIpAddress.str());
		_mcastDB_cNapIt = _mcastDB_cNap.find(groupAddress.uint());
		if (_mcastDB_cNapIt == _mcastDB_cNap.end()) {
			LOG4CXX_DEBUG(logger, "Dropping request, unregistered group: "
					<< groupAddress.str());
			_mutexMcastDB_cNap.unlock();
			return;
		}
	}
	LOG4CXX_TRACE(logger, "Setting counter to max value");
	_mcastDB_cNap[_mcastDB_cNapIt->first] = DROM_AFTER_QUER_NUM;
	_mutexMcastDB_cNap.unlock();
	LOG4CXX_TRACE(logger, "[cNAP]: sending last query to users");
	_cNAP2IPClientQueries(groupAddress);
	LOG4CXX_TRACE(logger,"Starting thread to check for membership report");
	thread(&MCast::_removeGroupAfterLeave, this, groupAddress).detach();
}
/*
 * YT: are buffered packets cleared..?
 * */
void MCast::_forceLeave(IpAddress &multicastIpAddress) {

	LOG4CXX_DEBUG(logger, "Group " << multicastIpAddress.str());

	// if no state existing in registry
	if (!_mcastDB_cNapErase(&multicastIpAddress)) {
		LOG4CXX_WARN(logger, "Dropping request, no DB entry for: "
				<< multicastIpAddress.str());
		return;
	}
	// update the sNAP >>>>>>>>>>>>>
	uint8_t *packet;
	uint8_t leave = IGMP_LEAVE_GROUP;
	uint16_t packetSize = sizeof(uint8_t) + sizeof(multicastIpAddress.uint());
	packet = (uint8_t *) malloc(packetSize);
	uint8_t offset = 0;
	// [1] include node LEAVE type
	memcpy(packet, &leave, sizeof(uint8_t));
	offset += sizeof(uint8_t);
	// [2] include multicast IP Address
	memcpy(packet + offset, multicastIpAddress.uintPointer(), sizeof(uint32_t));
	IcnId ctrlCID = _ctrlScopeIds[multicastIpAddress.uint()];
	_mutexCtrlFwdState.lock();
	_ctrlFwdStateIt = _ctrlFwdState.find(ctrlCID.uint());
	if (!_ctrlFwdStateIt->second)  // If ctrl packet has to be hold back (pause)
	{
		_mutexCtrlFwdState.unlock();
		_bufferPacket(ctrlCID, packet, packetSize);

		_icnCore->publish_info(ctrlCID.binId(), ctrlCID.binPrefixId(),
				DOMAIN_LOCAL, NULL, 0);

		return;
	}
	_mutexCtrlFwdState.unlock();
	IcnId* dataCID_ptr = IcnId::createIGMPDataFromCtrlScopeId(ctrlCID);
	_publishIGMPCtrl_data_isub(ctrlCID, *dataCID_ptr, packet, packetSize);
	_tryClearCNapStateFor(packet, &ctrlCID, &multicastIpAddress);
	free(packet);
	delete dataCID_ptr;
}

bool MCast::_mcastDB_cNapErase(IpAddress * multicastIpAddress_ptr) {
	_mutexMcastDB_cNap.lock();
	// update iterator due to erasing element
	//YT: workaround for bug during leave - used to cause infinite loop..
	if (_mcastDB_cNap.find(multicastIpAddress_ptr->uint())
			== _mcastDB_cNap.end()) {
		LOG4CXX_WARN(logger, "Dropping request, no state for group: "
				<< multicastIpAddress_ptr->uint());

		_mutexMcastDB_cNap.unlock();
		return false;
	}
	_mcastDB_cNap.erase(multicastIpAddress_ptr->uint());
	_statJoin2FirstDatumMap.erase(multicastIpAddress_ptr->uint());
	_mcastDB_cNapIPSocketsIt = _mcastDB_cNapIPSockets.find(
			multicastIpAddress_ptr->uint());
	// FIXME this is bug. needs fixing to free sockaddr_in memory
	if (_mcastDB_cNapIPSocketsIt == _mcastDB_cNapIPSockets.end()) {
		LOG4CXX_ERROR(logger, "Dropping request, Cannot free socket memory for "
				"multicast group: " << multicastIpAddress_ptr->str());
		return false;
	}
	close(_mcastDB_cNapIPSocketsIt->first); // close socket for this file descriptor
	struct sockaddr_in * addr = _mcastDB_cNapIPSocketsIt->second.second;
	free(addr);
	_mcastDB_cNapIPSockets.erase(_mcastDB_cNapIPSocketsIt);
	_mutexMcastDB_cNap.unlock();

	LOG4CXX_WARN(logger, "Removed state, group: "
			<< multicastIpAddress_ptr->uint());
	return true;
}

////////////////////////////////////////////////////////////////////////
/////////////////////////////// PROTECTED //////////////////////////////

void MCast::initialise()
{
	
	if (!_configuration.igmpHandler())
	{
		LOG4CXX_DEBUG(logger, "IGMP handler is disabled");
		return;
	}

	// >>>>>>>>>> "load properties and create state, namespace, etc.."" >>>>>>>>
	_loadSNapProps();

	//YT: control scope is used by cNAP to publish reports and leaves
	_controlScope_ptr = IcnId::createIGMPCtrlScopeId(NULL);
	_icnCore->publish_scope(_controlScope_ptr->binId(),
			_controlScope_ptr->binPrefixId(), DOMAIN_LOCAL, NULL, 0);
	LOG4CXX_INFO(logger, "Published <CTRL> scope " << _controlScope_ptr->print());

	_dataScope_ptr = IcnId::createIGMPDataScopeId();

	_icnCore->publish_scope(_dataScope_ptr->binId(),
			_dataScope_ptr->binPrefixId(), DOMAIN_LOCAL, NULL, 0);
	LOG4CXX_INFO(logger, "Published <DATA> scope " << _dataScope_ptr->print());
	//<<<<<<<<<<<<<<<<<<<"publish root and ctrl scopes"/////////////////////////

	if (isServerNAP == true) { //YT: to avoid cNAP receiving messages from other cNAP
		unordered_map<string, uint16_t>::iterator nxtMCastIP =
				_sNapMCastIPs.begin();
		while (nxtMCastIP != _sNapMCastIPs.end()) {

			IpAddress multicastIpAddress(nxtMCastIP->first);
			IcnId* ctrlCID_ptr = IcnId::createIGMPCtrlScopeId(
					&multicastIpAddress);
			IcnId* dataCID_ptr = IcnId::createIGMPDataScopeId(
					&multicastIpAddress);
			LOG4CXX_INFO(logger, "Registering group: "
					<< multicastIpAddress.str()<< ", <DATA> CID: "
					<< dataCID_ptr->print() << ", <CTRL> CID: "
					<< ctrlCID_ptr->print());
			///////// MCAST ADDRESS state /////////
			///(sNAP counter of joined cNAP IDs)///
			_mutexSNapCounter.lock();
			_sNAPCounterOfCNapIDsIt = _sNAPCounterOfCNapIDs.find(
					multicastIpAddress.uint());
			list<string> * cNAPIDs = new list<string>();
			_sNAPCounterOfCNapIDs.insert(
					pair<uint32_t, list<string> *>(multicastIpAddress.uint(),
							cNAPIDs));
			LOG4CXX_INFO(logger, "initialising counter of registered cNAPs");
			_mutexSNapCounter.unlock();
			/////// DATA state //////
			_mutexDataScopeIds.lock();
			_dataScopeIds.insert(
					pair<uint32_t, IcnId>(multicastIpAddress.uint(),
							*dataCID_ptr));
			_mutexDataScopeIds.unlock();
			/////// CTRL state & sub ////// 
			_icnCore->subscribe_info(ctrlCID_ptr->binId(),
					ctrlCID_ptr->binPrefixId(), DOMAIN_LOCAL, NULL, 0);
			LOG4CXX_INFO(logger, "subscribing to <CTRL> CID: "
					<< ctrlCID_ptr->print());
			/////// FWD state ///////
			// for <DATA> CID
			_mutexDataFwdState.lock();
			_dataFwdState.insert(
					pair<uint32_t, bool>(dataCID_ptr->uint(), false));
			LOG4CXX_INFO(logger, "initialising FWD state as disable");
			_mutexDataFwdState.unlock();

			delete ctrlCID_ptr;
			delete dataCID_ptr;

			nxtMCastIP++;
		}			//for
	}
	// >>>>>>>>>> "thread for maintaining cNAP DB state"" >>>>>>>>>>>>>>>>>>>>
	_mutexMcastDB_cNap.lock();
	_runDetachedThds = true;
	_mutexMcastDB_cNap.unlock();

	//YT: stopping thread creation may cause trouble... :/
	if (isClientNAP == true) {
		LOG4CXX_INFO(logger, "Starting thread for maintaining cNAP DB state");
		_snd2IPClientsThd = thread(&MCast::_cNapGeneralQueriesTimer, this,
				GENERAL_QUERY_TIMER);
		_snd2IPClientsThd.detach();
	}
}

void MCast::uninitialise() {

	unordered_map<string, uint16_t>::iterator nxtMCastIP =
			_sNapMCastIPs.begin();
	while (nxtMCastIP != _sNapMCastIPs.end()) {

		IpAddress multicastIpAddress(nxtMCastIP->first);
		IcnId* ctrlCID_ptr = IcnId::createIGMPCtrlScopeId(&multicastIpAddress);

		_icnCore->unsubscribe_info(ctrlCID_ptr->binId(),
				ctrlCID_ptr->binPrefixId(), DOMAIN_LOCAL, NULL, 0);
		LOG4CXX_INFO(logger, "Unsubscribed from <CTRL> CID: "
				<< ctrlCID_ptr->print());

		delete ctrlCID_ptr;
		nxtMCastIP++;
	}			//for

	_mcastBufferThread->interrupt();
	LOG4CXX_INFO(logger, "MCast namespace uninitialised");
}
// YT: callback for start/stop/pause_publish & i_sub, see ../icncore/icn.cc 
void MCast::forwarding(IcnId &cId, bool state) {

	int cntrlOrData = _cntrl1_data2(cId);
	if (cntrlOrData == 1) {
		_forwardingCtrl(cId, state);
		LOG4CXX_DEBUG(logger, ""<<(state ? "En" : "Dis")
				<< "abled  <CTRL> FWD state, CID " << cId.print());
	} else if (cntrlOrData == 2) {
		_forwardingData(cId, state);
		LOG4CXX_DEBUG(logger, ""<<(state ? "En" : "Dis")
				<< "abled  <DATA> FWD state, DID " << cId.print());
	} else
		LOG4CXX_ERROR(logger, "unknown subscope (neither <CTRL> or "
				"<DATA> scope): " << cId.print());
}
