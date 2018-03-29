/*
 * ip.cc
 *
 *  Created on: 4 May 2016
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

#include "ipsocket.hh"

using namespace ipsocket;

LoggerPtr IpSocket::logger(Logger::getLogger("ipsocket"));

IpSocket::IpSocket(Configuration &configuration, Statistics &statistics)
	: _configuration(configuration),
	  _statistics(statistics)
{
	// IPv4 socket
	if ((_socketRaw4 = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
	{
		LOG4CXX_FATAL(logger, "Failed to get IPv4 socket descriptor ");
	}

	if (setuid(getuid()) < 0)//no special permissions needed anymore
	{
		LOG4CXX_ERROR(logger, "Could not set UID of to " << getuid());
	}
	else
	{
		LOG4CXX_DEBUG(logger, "UID set to " << getuid());
	}

	//set option to send pre-made IP packets over raw socket
	const int on = 1;

	if (setsockopt(_socketRaw4, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
	{
		LOG4CXX_ERROR(logger, "Socket option IP_HDRINCL could not be set for "
				"raw IPv4 socket");
	}
	else
	{
		LOG4CXX_DEBUG(logger, "Socket option IP_HDRINCL enabled");
	}

	// ICMP socket
	if ((_socketIcmp = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
	{
		LOG4CXX_FATAL(logger, "Failed to get ICMP socket descriptor ");
	}

	// Libnet socket for packet fragmentation
	_libnetPtag = 0;
	_libnetRaw4 = libnet_init(LIBNET_RAW4,
			_configuration.networkDevice().c_str(), _errbuf);

	if (_libnetRaw4 == NULL)
	{
		LOG4CXX_FATAL(logger, "Libnet Raw4 socket could not be created: "
				<< _errbuf);
	}
	else
	{
		IpAddress ipAddressNic =  libnet_get_ipaddr4(_libnetRaw4);
		LOG4CXX_DEBUG(logger, "Libnet Raw4 socket created on interface with IP "
				<< ipAddressNic.str());
	}

	/* Getting max payload size */
	_maxIpPacketPayloadSize = _configuration.mtu() - LIBNET_IPV4_H;
	/* making it a multiple of 8 */
	_maxIpPacketPayloadSize -= (_maxIpPacketPayloadSize % 8);
	// Set hints for IPv4 raw socket
	bzero(&_hints, sizeof(struct addrinfo));
	_hints.ai_flags = AI_CANONNAME;  /* always return canonical name */
	_hints.ai_family = 0;//AF_INET;
	_hints.ai_socktype = 0;//IPPROTO_RAW;
}

IpSocket::~IpSocket()
{
	_mutexLibnetRaw4.lock();
	libnet_destroy(_libnetRaw4);
	_mutexLibnetRaw4.unlock();
	LOG4CXX_DEBUG(logger, "Libnet Raw4 socket destroyed");
	close(_socketRaw4);
	LOG4CXX_DEBUG(logger, "Raw IPv4 socket closed");
	close(_socketIcmp);
	LOG4CXX_DEBUG(logger, "ICMP (IPv4) socket closed");
}

void IpSocket::icmp(IpAddress sourceIpAddress, IpAddress destinationIpAddress,
		uint16_t ipHeaderLength, uint16_t code)
{
	if (_configuration.socketType() == RAWIP)
	{
		ostringstream additionalInformation;
		struct icmphdr icmpHeader;
		size_t bytesWritten = 0 ;
		bzero(&icmpHeader, sizeof(icmpHeader));

		switch (code)
		{
		case ICMP_FRAG_NEEDED:
			icmpHeader.type = ICMP_DEST_UNREACH;
			icmpHeader.code = code;
			icmpHeader.un.frag.mtu = ntohs(_configuration.mitu()
					- ipHeaderLength);
			additionalInformation << "set MTU to " << _configuration.mitu()
					- ipHeaderLength << " octets";
			break;
		default:
			LOG4CXX_WARN(logger, "ICMP message codes other then "
					"ICMP_FRAG_NEEDED currently not supported");
			return;
		}

		icmpHeader.checksum = _checksum(&icmpHeader, sizeof(icmpHeader));
		struct sockaddr_in dst = destinationIpAddress.socketAddress();
		_mutexSocketRaw4.lock();
		bytesWritten = sendto(_socketIcmp, &icmpHeader, sizeof(icmpHeader), 0,
				(struct sockaddr *)&dst, sizeof(dst));
		_mutexSocketRaw4.unlock();

		if (bytesWritten < 0)
		{
			LOG4CXX_WARN(logger, "ICMP packet to " << destinationIpAddress.str()
					<< ", type " << (int)icmpHeader.type << " and code "
					<< (int)code << " could not be sent: " << strerror(errno));
			return;
		}

		LOG4CXX_TRACE(logger, "ICMP packet with type " << (int)icmpHeader.type
				<< " and code " << (int)icmpHeader.code << " ("
				<< additionalInformation.str() << ") sent to "
				<< destinationIpAddress.str());
	}
	else if (_configuration.socketType() == LIBNET)
	{
		LOG4CXX_INFO(logger, "Sending ICMP messages via libnet socket is "
				"currently not supported. Please switch to raw IP in nap.cfg");
	}
}

bool IpSocket::sendPacket(uint8_t *data, uint16_t &dataSize)
{
	struct ip *ipHeader;
	IpAddress destinationIpAddress;
	ipHeader = (struct ip *)data;
	destinationIpAddress = ipHeader->ip_dst.s_addr;

	// single packet, but choose socket
	if (_configuration.socketType() == RAWIP)
	{
		return _sendPacketRawIp(data, dataSize);
	}
	else if (_configuration.socketType() == LIBNET)
	{
		uint16_t ipHeaderLength = 4 * ipHeader->ip_hl;

		if (_maxIpPacketPayloadSize < (dataSize  - ipHeaderLength))
		{
			LOG4CXX_DEBUG(logger, "IP fragmentation required, as packet of"
					"length " << dataSize << " is larger than MTU of "
					<< _maxIpPacketPayloadSize <<". As the IP header gets "
					"rewritten, this could cause issues with already fragmented"
					" IP packets");
			return _sendFragments(data, dataSize);
		}

		return _sendPacketLibnet(data, dataSize);
	}

	return false;
}

uint16_t IpSocket::_checksum(const void *buffer, size_t headerLength)
{
	unsigned long sum = 0;
	const uint16_t *ipHeaderField;

	ipHeaderField = (const uint16_t *)buffer;

	while (headerLength > 1)
	{
		sum += *ipHeaderField++;

		if (sum & 0x80000000)
		{
			sum = (sum & 0xFFFF) + (sum >> 16);
		}

		headerLength -= 2;
	}

	while (sum >> 16)
	{
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	return(~sum);
}

bool IpSocket::_sendFragments(uint8_t *data, uint16_t &dataSize)
{
	/*headerOffset = fragmentation flags + offset (in bytes) divided by 8*/
	uint16_t payloadOffset = 0;
	uint16_t headerOffset = 0; /* value of the offset */
	int bytesWritten, ipPacketPayloadSize;
	uint32_t ipHeaderId = libnet_get_prand(LIBNET_PR16);
	ipPacketPayloadSize = _maxIpPacketPayloadSize;
	struct ip *ipHeader = (struct ip *)data;
	IpAddress destinationIpAddress = ipHeader->ip_dst.s_addr;
	IpAddress sourceIpAddress = ipHeader->ip_src.s_addr;

	// This packet is already a fragmented one... discard
	if (ipHeader->ip_off == IP_MF)
	{
		LOG4CXX_WARN(logger, "IP packet towards " << destinationIpAddress.str()
				<< " is already a fragmented one. Unsupported feature! Dropping"
						"it");
		return false;
	}

	uint8_t *payload = data + (4 * ipHeader->ip_hl);
	uint16_t payloadSize = dataSize - (4 * ipHeader->ip_hl);
	_mutexLibnetRaw4.lock();
	_libnetPtag = libnet_build_ipv4((LIBNET_IPV4_H + _maxIpPacketPayloadSize),
			ipHeader->ip_tos, ipHeaderId, IP_MF, ipHeader->ip_ttl,
			ipHeader->ip_p, 0, ipHeader->ip_src.s_addr, ipHeader->ip_dst.s_addr,
			payload, _maxIpPacketPayloadSize, _libnetRaw4, 0);

	if (_libnetPtag == -1 )
	{
		LOG4CXX_ERROR(logger, "Error building fragmented IP header: "
				<< libnet_geterror(_libnetRaw4));
		libnet_clear_packet(_libnetRaw4);
		_mutexLibnetRaw4.unlock();
		return false;
	}

	bytesWritten = libnet_write(_libnetRaw4);

	if (bytesWritten == -1)
	{
		LOG4CXX_ERROR(logger, "First IP fragment with ID " << ipHeaderId
				<< "to IP endpoint" << destinationIpAddress.str() << " has not "
						"been sent. Error: " << libnet_geterror(_libnetRaw4));
		libnet_clear_packet(_libnetRaw4);
		_mutexLibnetRaw4.unlock();
		return false;
	}

	LOG4CXX_TRACE(logger, "IP fragment with ID " << ipHeaderId << ", offset "
			<< headerOffset << " and size " << ipPacketPayloadSize << " sent "
			"to IP endpoint " << destinationIpAddress.str() << " ("
			<< ipPacketPayloadSize << "/" << payloadSize << ")");
	libnet_clear_packet(_libnetRaw4);
	_libnetPtag = LIBNET_PTAG_INITIALIZER;
	/* Now send off the remaining fragments */
	payloadOffset += ipPacketPayloadSize;

	while (payloadSize > payloadOffset)
	{
		/* Building IP header */
		/* checking if there will be more fragments */
		if ((payloadSize - payloadOffset) > _maxIpPacketPayloadSize)
		{
			headerOffset = IP_MF + (payloadOffset)/8;
			ipPacketPayloadSize = _maxIpPacketPayloadSize;
		}
		else
		{
			headerOffset = payloadOffset/8;
			ipPacketPayloadSize = payloadSize - payloadOffset;
		}

		_libnetPtag = libnet_build_ipv4((LIBNET_IPV4_H + ipPacketPayloadSize),
				0, ipHeaderId, headerOffset, ipHeader->ip_ttl, ipHeader->ip_p,
				0, sourceIpAddress.uint(), destinationIpAddress.uint(), payload
				+ payloadOffset, ipPacketPayloadSize, _libnetRaw4, _libnetPtag);

		if (_libnetPtag == -1)
		{
			LOG4CXX_ERROR(logger, "Error building IP header for destination "
					<< destinationIpAddress.str() << ": "
					<< libnet_geterror(_libnetRaw4));
			libnet_clear_packet(_libnetRaw4);
			_mutexLibnetRaw4.unlock();
			return false;
		}

		bytesWritten = libnet_write(_libnetRaw4);

		if (bytesWritten == -1)
		{
			LOG4CXX_ERROR(logger, "Error writing packet for IP destination "
					<< destinationIpAddress.str() << ": "
					<< libnet_geterror(_libnetRaw4));
			libnet_clear_packet(_libnetRaw4);
			_mutexLibnetRaw4.unlock();
			return false;
		}

		LOG4CXX_TRACE(logger, "IP fragment with ID " << ipHeaderId
				<< ", offset " << headerOffset << " and size "
				<< ipPacketPayloadSize << " sent to "
				<< destinationIpAddress.str() << " ("
				<< payloadOffset + ipPacketPayloadSize << "/" << payloadSize
				<< " bytes sent)");
		/* Updating the offset */
		payloadOffset += ipPacketPayloadSize;
		//libnet_clear_packet(_libnetRaw4);// if it doesn't get cleared, the
		//second IP fragment cannot be sent: libnet_write_link(): only -1 bytes
		// written (Message too long)
	}

	libnet_clear_packet(_libnetRaw4);
	_mutexLibnetRaw4.unlock();
	return true;
}

bool IpSocket::_sendPacketLibnet(uint8_t *data, uint16_t &dataSize)
{
	int bytesWritten;
	struct ip *ipHeader = (struct ip *)data;
	IpAddress destinationIpAddress = ipHeader->ip_dst.s_addr;
	IpAddress sourceIpAddress = ipHeader->ip_src.s_addr;
	uint16_t payloadSize = dataSize - (4 * ipHeader->ip_hl);
	uint8_t *payload = data + (4 * ipHeader->ip_hl);
	_mutexLibnetRaw4.lock();
	// calculate the payload of the transport
	_libnetPtag = libnet_build_ipv4((LIBNET_IPV4_H + payloadSize),
			ipHeader->ip_tos, ipHeader->ip_id, ipHeader->ip_off,
			ipHeader->ip_ttl, ipHeader->ip_p, 0, sourceIpAddress.uint(),
			destinationIpAddress.uint(), payload, payloadSize, _libnetRaw4, 0);

	if (_libnetPtag == -1)
	{
		LOG4CXX_ERROR(logger, "Error building fragmented IP header: "
				<< libnet_geterror(_libnetRaw4));
		libnet_clear_packet(_libnetRaw4);
		_mutexLibnetRaw4.unlock();
		return false;
	}

	bytesWritten = libnet_write(_libnetRaw4);

	if (bytesWritten == -1)
	{
		LOG4CXX_ERROR(logger, "IP packet with ID " << ipHeader->ip_id
				<< "to IP endpoint" << destinationIpAddress.str() << " has not "
						"been sent. Error: " << libnet_geterror(_libnetRaw4));
		libnet_clear_packet(_libnetRaw4);
		_mutexLibnetRaw4.unlock();
		return false;
	}

	LOG4CXX_TRACE(logger, "IP packet of size " << LIBNET_IPV4_H + payloadSize
			<< " from " << sourceIpAddress.str() << " sent to IP endpoint "
			<< destinationIpAddress.str());
	libnet_clear_packet(_libnetRaw4);
	_mutexLibnetRaw4.unlock();
	return true;
}

bool IpSocket::_sendPacketRawIp(uint8_t *data, uint16_t &dataSize)
{
	struct ip *ipHeader;
	int bytesWritten;
	ipHeader = (struct ip *)data;
	IpAddress destinationIpAddress = ipHeader->ip_dst.s_addr;
	// optional way START
	struct hostent *hp;

	if ((hp = gethostbyname(destinationIpAddress.str().c_str())) == NULL)
	{
		LOG4CXX_ERROR(logger, "Could not resolve DST IP "
				<< destinationIpAddress.str());
		return false;
	}

	bcopy(hp->h_addr_list[0], &ipHeader->ip_dst.s_addr, hp->h_length);
	struct sockaddr_in dst = destinationIpAddress.socketAddress();
	_mutexSocketRaw4.lock();
	bytesWritten = sendto(_socketRaw4, data, dataSize, 0,
			(struct sockaddr *)&dst, sizeof(dst));

	if (bytesWritten == -1)
	{
		if (errno == EMSGSIZE)
		{
			LOG4CXX_DEBUG(logger, "IP packet of length " << dataSize << " could"
					" not be sent to " << destinationIpAddress.str() << " via "
					"raw socket: "	<< strerror(errno));
			_mutexSocketRaw4.unlock();
			LOG4CXX_DEBUG(logger, "Using libnet to send it as IP fragments");
			return _sendFragments(data, dataSize);
		}
		else
		{
			LOG4CXX_WARN(logger, "IP packet of length " << dataSize << " could "
					"not be sent to " << destinationIpAddress.str() << ": "
					<< strerror(errno));
			_mutexSocketRaw4.unlock();
		}
		return false;
	}

	_mutexSocketRaw4.unlock();
	LOG4CXX_TRACE(logger, "IP packet of length " << dataSize << " sent to IP "
			"endpoint " << destinationIpAddress.str());
	_statistics.txIpBytes(&bytesWritten);
	return true;
}
