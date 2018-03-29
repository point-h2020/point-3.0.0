/*
 * unreliable.hh
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

#ifndef NAP_TRANSPORT_UNRELIABLE_HH_
#define NAP_TRANSPORT_UNRELIABLE_HH_

#include <blackadder.hpp>
#include <log4cxx/logger.h>
#include <map>
#include <mutex>

#include <configuration.hh>
#include <types/enumerations.hh>
#include <sockets/ipsocket.hh>
#include <transport/unreliabletypedef.hh>
#include <types/icnid.hh>
#include <types/typedef.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace configuration;
using namespace ipsocket;
using namespace log4cxx;

namespace transport
{
	namespace unreliable
	{
		/*!
		 * \brief Implementation of an unreliable transport protocol
		 *
		 * This transport protocol basically does packet fragmentation and
		 * packet sequencing only
		 */
		class Unreliable
		{
			static LoggerPtr logger;
		public:
			/*!
			 * \brief Constructor
			 */
			Unreliable(Blackadder *icnCore, Configuration &configuration,
					IpSocket &ipSocket, std::mutex &icnCoreMutex);
			/*!
			 * \brief Destructor
			 */
			~Unreliable();
			/*!
			 * \brief Publish data to the ICN core
			 *
			 * \param icnId Reference to the content identifier
			 * \param packet Pointer to the data
			 * \param dataSize Pointer to the length of the packet
			 */
			void publish(IcnId &icnId, uint8_t *data, uint16_t &dataSize);
			/*!
			 * \brief Reassemble an IP packet received over ICN
			 *
			 * \param icnId The CID under which the packet has been published
			 * \param data Pointer to the data
			 * \param dataSize Pointer to the length of the received data
			 *
			 * \return void method
			 */
			void handle(IcnId &icnId, uint8_t *data, uint16_t &dataSize);
		private:
			Blackadder *_icnCore; /*!< Pointer to the Blackadder instance */
			Configuration &_configuration;
			IpSocket &_ipSocket;/*!< Reference to IpSocket class*/
			std::mutex &_icnCoreMutex;/*!< Reference to ICN core mutex */
			reassembly_packet_buffer_t _reassemblyBuffer; /*!< Buffer for
			incoming UTP chunks in order to reassemble the actual IP packet */
			reassembly_packet_buffer_t::iterator _reassemblyBufferIt; /*!<
			Iterator for _reassemblyMap */
			std::mutex _reassemblyBufferMutex; /*!< Mutex to lock/unlock
			reassembly buffer*/
			map<uint8_t, pair<utp_header_t, uint8_t*>>::iterator _packetIt;
		};
	}
}

#endif /* NAP_TRANSPORT_UNRELIABLE_HH_ */
