/*
 * demux.hh
 *
 *  Created on: 19 Apr 2015
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

#ifndef NAP_DEMUX_HH_
#define NAP_DEMUX_HH_

#include <arpa/inet.h>
#include <log4cxx/logger.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h> /* includes net/ethernet.h */
#include <netinet/ether.h>
#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <configuration.hh>
#include <types/enumerations.hh>
#include <sockets/ipsocket.hh>
#include <namespaces/namespaces.hh>
#include <monitoring/statistics.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace log4cxx;
using namespace monitoring::statistics;

namespace demux {
/*!
 * \brief Implementation of the Demux class
 *
 * Class for listening on socket for any incoming packet and to demultiplex them
 * based on Ethernet type, IP protocol type and transport protocol.
 */
class Demux
{
	static LoggerPtr logger;
public:
	/*!
	 * \brief Demux Constructor
	 */
	Demux(Namespaces &namespaces, Configuration &configuration,
			Statistics &statistics, IpSocket &ipSocket, bool *run);
	/*!
	 * \brief Destructor
	 */
	~Demux();
	/*!
	 * \brief Starting the demuxer
	 *
	 * Functor to call this method in a thread
	 */
	void operator()();
private:
	Namespaces &_namespaces;/*!< Reference to namespaces */
	Configuration &_configuration;/*!< Reference to configueration */
	Statistics &_statistics;/*!< Reference to statistics class */
	IpSocket &_ipSocket; /*!< Reference to IpSocket class */
	bool *_run;/*!< from main thread is any SIG* had been caught */
	pcap_t *_pcapSocket;/*!< PCAP pointer for listening socket */
	IpAddress _napIpAddress;/*!< IP address of the NAP */
	uint32_t _linkHdrLen;/*!< Ethernet header length of the NAP's interface */
	/*!
	 * \brief Obtain the IP address of the NAP using the interface name provided
	 * in nap.cfg
	 *
	 * \return Boolean indicating whether or not the IP address has been
	 * obtained
	 */
	bool _getNapIpAddress();
	/*!
	 * \brief Process an incoming packet
	 *
	 * \param header Pointer to the pcap header
	 * \param packet Pointer to the entire packet
	 */
	void _processPacket(const struct pcap_pkthdr *header, const u_char *packet);
};

} /* namespace demux */

#endif /* NAP_DEMUX_HH_ */
