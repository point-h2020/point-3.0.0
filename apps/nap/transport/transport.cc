/*
 * transport.cc
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

#include "transport.hh"

Transport::Transport(Blackadder *icnCore, Configuration &configuration,
		IpSocket &ipSocket, std::mutex &icnCoreMutex, Statistics &statistics,
		bool *run)
	: Lightweight(icnCore, configuration, icnCoreMutex, statistics, run),
	  Unreliable(icnCore, configuration, ipSocket, icnCoreMutex)
{
	_tpState = TP_STATE_NO_ACTION_REQUIRED;
}

tp_states_t Transport::handle(IcnId &cId, void *packet, uint16_t &packetSize,
		enigma_t &enigma, sk_t &sessionKey)
{
	switch (cId.rootNamespace())
	{
	case NAMESPACE_IP:
		Unreliable::handle(cId, (uint8_t *)packet, packetSize);
		_tpState = TP_STATE_NO_ACTION_REQUIRED;
		break;
	case NAMESPACE_HTTP:
		_tpState = Lightweight::handle(cId, (uint8_t *)packet, enigma,
				sessionKey);
		break;
	case NAMESPACE_MANAGEMENT:
		_tpState = TP_STATE_NO_TRANSPORT_PROTOCOL_USED;
		break;
	default:
		_tpState = TP_STATE_NO_ACTION_REQUIRED;
	}
	return _tpState;
}

tp_states_t Transport::handle(IcnId &cId, IcnId &rCId, string &nodeId,
		void *packet, uint16_t &packetSize, enigma_t &enigma,
		sk_t &sessionKey)
{
	switch (cId.rootNamespace())
	{
	case NAMESPACE_HTTP:
		_tpState = Lightweight::handle(cId, rCId, nodeId, (uint8_t *)packet,
				enigma, sessionKey);
		break;
	default:
		_tpState = TP_STATE_NO_ACTION_REQUIRED;
	}
	return _tpState;
}

bool Transport::retrieveIcnPacket(IcnId &rCId, enigma_t &enigma,
		string &nodeId, sk_t &sessionKey, uint8_t *packet,
		uint16_t &packetSize)
{
	switch (rCId.rootNamespace())
	{
	case NAMESPACE_HTTP:
		return Lightweight::retrieveIcnPacket(rCId, enigma, nodeId,
				sessionKey, packet,	packetSize);
	}
	return false;
}
