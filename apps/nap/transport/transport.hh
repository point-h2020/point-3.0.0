/*
 * transport.hh
 *
 *  Created on: 22 Apr 2016
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

#ifndef NAP_TRANSPORT_HH_
#define NAP_TRANSPORT_HH_

#include <blackadder.hpp>
#include <mutex>

#include <configuration.hh>
#include <sockets/ipsocket.hh>
#include <monitoring/statistics.hh>
#include <transport/lightweight.hh>
#include <transport/unreliable.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace monitoring::statistics;
using namespace ipsocket;
using namespace std;
using namespace transport::lightweight;
using namespace transport::unreliable;

/*!
 * \brief Implementation of all transport protocols the NAP supports
 *
 * This class inherits all transport protocols the NAP supports namely
 * unreliable and lightweight
 */
class Transport: public Lightweight, public Unreliable
{
public:
	/*!
	 * \brief Constructor
	 */
	Transport(Blackadder *icnCore, Configuration &configuration,
			IpSocket &ipSocket, std::mutex &icnCoreMutex,
			Statistics &statistics, bool *run);
	/*!
	 * \brief Handle an incoming PUBLISH_DATA events
	 *
	 * In case the data to be handled has been an LTP CTRL-WE message and all
	 * segments have been received this method returns a TP state indicating
	 * that the parameters enigma and sessioKey have been filled so that
	 * the correct session can be checked afterwards.
	 *
	 * \param cId The CID under which the data has been received
	 * \param packet Pointer to the data which has been received
	 * \param packetSize The size of the data which has been received
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The session key required to send off the packet after
	 * this method returns the corresponding TP state
	 *
	 * \return The LTP state using tp_states_t enumeration
	 */
	tp_states_t handle(IcnId &cId, void *packet, uint16_t &packetSize,
			enigma_t &enigma, sk_t &sessionKey);
	/*!
	 * \brief Handle an incoming PUBLISH_DATA_iSUB events
	 *
	 * \param cId The CID under which it has been received
	 * \param rCId The iSub CID which must be used to publish the response
	 * \param nodeId The NID which must be used when publish the response
	 * \param packet Pointer to the ICN payload
	 * \param packetSize The length of the ICN payload
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 * \param sessionKey The session key needed if all fragments have been
	 * received and retrievePacket method is being called afterwards
	 *
	 * \return The LTP state using tp_states_t enumeration
	 */
	tp_states_t handle(IcnId &cId, IcnId &rCId, string &nodeId, void *packet,
			uint16_t &packetSize, enigma_t &enigma, sk_t &sessionKey);
	/*!
	 * \brief Retrieve a packet from ICN buffer
	 *
	 * At the moment this is only meant for LTP (HTTP-over-ICN). The UTP does
	 * not require any additional input and can directly send off IP traffic.
	 *
	 * TODO params
	 *
	 * \param enigma https://en.wikipedia.org/wiki/23_enigma
	 */
	bool retrieveIcnPacket(IcnId &rCId, enigma_t &enigma, string &nodeId,
			sk_t &sessionKey, uint8_t *packet, uint16_t &packetSize);
private:
	tp_states_t _tpState;
};
#endif /* NAP_TRANSPORT_HH_ */
