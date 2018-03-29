/*
 * unreliable.cc
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

#include "unreliable.hh"

using namespace transport::unreliable;

LoggerPtr Unreliable::logger(Logger::getLogger("transport.unreliable"));

Unreliable::Unreliable(Blackadder *icnCore, Configuration &configuration,
		IpSocket &ipSocket, std::mutex &icnCoreMutex)
	: _icnCore(icnCore),
	  _configuration(configuration),
	  _ipSocket(ipSocket),
	  _icnCoreMutex(icnCoreMutex)
{}

Unreliable::~Unreliable(){}

void Unreliable::publish(IcnId &icnId, uint8_t *data, uint16_t &dataSize)
{
	utp_header_t header;
	uint8_t *packet;
	uint16_t maxPayloadLength =
			_configuration.mitu() /* maximal ICN payload*/
			/*** Calculating header length ***/
			- icnId.length() - 20 /* additional header fields */
			- sizeof(utp_header_t);
	header.key = icnId.uint() + rand();

	// Fragmentation required
	if (maxPayloadLength < dataSize)
	{
		uint16_t bytesSent = 0;
		header.payloadLength = maxPayloadLength;
		header.state = TRANSPORT_STATE_START;
		header.sequence = 0;

		while (bytesSent < dataSize)
		{
			// Last fragment reached
			if ((bytesSent + header.payloadLength) > dataSize)
			{
				header.state = TRANSPORT_STATE_FINISHED;
 				header.payloadLength = dataSize - bytesSent;
			}

			header.sequence++;
			packet = (uint8_t *)malloc(sizeof(utp_header_t) +
					header.payloadLength);
			memcpy(packet, &header, sizeof(utp_header_t));
			memcpy(packet + sizeof(utp_header_t), data, header.payloadLength);
			_icnCoreMutex.lock();
			_icnCore->publish_data(icnId.binIcnId(), DOMAIN_LOCAL, NULL, 0,
					packet,	sizeof(utp_header_t) + header.payloadLength);
			_icnCoreMutex.unlock();
			bytesSent += header.payloadLength;
			data += header.payloadLength;
			LOG4CXX_TRACE(logger, "Fragment of length "
					<< header.payloadLength + sizeof(utp_header_t) << ", state "
					<< (uint16_t)header.state << ", sequence "
					<< (uint16_t)header.sequence << " and key " << header.key
					<< " published under CID " << icnId.str() << " ("
					<< bytesSent << "/" << dataSize
					<< " bytes have been sent so far)");
			header.state = TRANSPORT_STATE_FRAGMENT;
			free(packet);
		}
	}
	// Single packet
	else
	{
		header.state = TRANSPORT_STATE_SINGLE_PACKET;
		header.sequence = 1;
		header.payloadLength = dataSize;
		packet = (uint8_t *)malloc(sizeof(utp_header_t) + header.payloadLength);
		memcpy(packet, &header, sizeof(utp_header_t));
		memcpy(packet + sizeof(utp_header_t), data, header.payloadLength);
		LOG4CXX_TRACE(logger, "Single fragment of length " << dataSize
				<< " with additional UTP header (key " << header.key
				<< ", sequence " << (uint16_t)header.sequence << ") of "
				<< sizeof(utp_header_t) << " bytes published using CID "
				<< icnId.print());
		_icnCoreMutex.lock();
		_icnCore->publish_data(icnId.binIcnId(), DOMAIN_LOCAL, NULL, 0, packet,
				header.payloadLength + sizeof(utp_header_t));
		_icnCoreMutex.unlock();
		free(packet);
	}
}

void Unreliable::handle(IcnId &icnId, uint8_t *data, uint16_t &dataSize)
{
	pair<utp_header_t, uint8_t *> packet;
	memcpy(&packet.first, data, sizeof(utp_header_t));
	data += sizeof(utp_header_t);
	dataSize -= sizeof(utp_header_t);

	switch(packet.first.state)
	{
	// Fragmented piece
	case TRANSPORT_STATE_START:
	case TRANSPORT_STATE_FRAGMENT:
	case TRANSPORT_STATE_FINISHED:
	{
		LOG4CXX_TRACE(logger, "Packet fragment received for unique UTP key "
				<< packet.first.key << ", State "
				<< (uint16_t)packet.first.state	<< ", Sequence "
				<< (uint16_t)packet.first.sequence);
		_reassemblyBufferMutex.lock();
		_reassemblyBufferIt = _reassemblyBuffer.find(packet.first.key);
		packet.second = (uint8_t *)malloc(packet.first.payloadLength);
		memcpy(packet.second, data, packet.first.payloadLength);

		// First packet with that key
		if (_reassemblyBufferIt == _reassemblyBuffer.end())
		{
			map<uint8_t, pair<utp_header_t, uint8_t*>> packets;
			packets.insert(pair<uint8_t, pair<utp_header_t, uint8_t*>>(
					packet.first.sequence, packet));
			_reassemblyBuffer.insert(pair<uint32_t, map<uint8_t,
					pair<utp_header_t, uint8_t*>>>(packet.first.key, packets));
			LOG4CXX_TRACE(logger, "New entry in packet buffer created");
		}
		// Key is known, add this packet to the buffer
		else
		{
			(*_reassemblyBufferIt).second.insert(pair<uint8_t,
					pair<utp_header_t, uint8_t*>>(packet.first.sequence,
							packet));
			LOG4CXX_TRACE(logger, "Packet added to existing packet buffer");
		}

		_reassemblyBufferIt = _reassemblyBuffer.find(packet.first.key);
		size_t previousSequenceNumber = 0;
		uint8_t transportStateFinished = TRANSPORT_STATE_UNKNOWN;
		uint16_t reassembledPacketLength = 0;

		// Iterator over all received packet fragments
		for (_packetIt = (*_reassemblyBufferIt).second.begin();
				_packetIt != (*_reassemblyBufferIt).second.end(); _packetIt++)
		{
			// First write the lowest sequence number
			if (previousSequenceNumber == 0)
			{
				previousSequenceNumber = (*_packetIt).first;

				if ((*_packetIt).second.first.state !=
						TRANSPORT_STATE_START)
				{
					LOG4CXX_TRACE(logger, "First fragment is missing");
					_reassemblyBufferMutex.unlock();
					return;
				}
			}
			else
			{
				// if a fragment is still missing, end here
				if ((*_packetIt).first != (previousSequenceNumber + 1))
				{
					LOG4CXX_TRACE(logger, "Fragment " << previousSequenceNumber
							+ 1 << " is still missing");
					_reassemblyBufferMutex.unlock();
					return;
				}

				previousSequenceNumber = (*_packetIt).first;
				transportStateFinished = (*_packetIt).second.first.state;
			}

			reassembledPacketLength += (*_packetIt).second.first.payloadLength;
		}

		// Last fragment has not been received
		if (transportStateFinished != TRANSPORT_STATE_FINISHED)
		{
			LOG4CXX_TRACE(logger, "Final fragment " << previousSequenceNumber
					+ 1 << " is still missing");
			_reassemblyBufferMutex.unlock();
			return;
		}

		LOG4CXX_TRACE(logger, "All " << previousSequenceNumber << " fragments "
				"received to reassemble IP packet");
		// Get length of reassembled packet
		uint8_t *reassembledPacket = (uint8_t *)malloc(reassembledPacketLength);
		uint16_t offset = 0;

		for (_packetIt = (*_reassemblyBufferIt).second.begin();
						_packetIt != (*_reassemblyBufferIt).second.end();
						_packetIt++)
		{
			memcpy(reassembledPacket + offset, (*_packetIt).second.second,
					(*_packetIt).second.first.payloadLength);
			offset += (*_packetIt).second.first.payloadLength;
			// and delete packet in reassembled buffer
			free(_packetIt->second.second);
		}

		_ipSocket.sendPacket(reassembledPacket, reassembledPacketLength);
		free(reassembledPacket);
		_reassemblyBuffer.erase(_reassemblyBufferIt);
		_reassemblyBufferMutex.unlock();
		break;
	}
	case TRANSPORT_STATE_SINGLE_PACKET:
	{
		LOG4CXX_TRACE(logger, "Single packet received for unique UTP key "
				<< packet.first.key << ", Sequence: "
				<< (uint16_t)packet.first.sequence << ", Length " << dataSize);
		_ipSocket.sendPacket(data, dataSize);
		break;
	}
	default:
		LOG4CXX_ERROR(logger, "Unknown transport protocol state "
				<< (uint16_t)packet.first.state << ", sequence "
				<< (uint16_t)packet.first.sequence << ", key "
				<< packet.first.key	<< ", packet size " << dataSize);
	}
}
