/*
 * typedef.hh
 *
 *  Created on: 17 Jun 2017
 *      Author: user
 */

#ifndef NAP_TYPEDEF_HH_
#define NAP_TYPEDEF_HH_

#include <boost/date_time.hpp>

#define ENIGMA 23 // https://en.wikipedia.org/wiki/23_enigma

typedef uint32_t cid_t;/*!< Content Identifier */
typedef uint32_t enigma_t; /*!< Proxy rule identifier */
typedef uint16_t sk_t; /*!< Session key */
typedef uint32_t nid_t;/*!< Node Identifier */
typedef uint16_t seq_t;/*!< Sequence number in LTP*/
typedef int socket_fd_t;/*!< Socket file descriptor*/

struct packet_t
{
	uint8_t *packet; /*!< Pointer to the actual IP packet */
	uint16_t packetSize;/*!< Length of IP packet */
	boost::posix_time::ptime timestamp;/*!< Timestamp of when this packet
	was added to buffer*/
};

#endif /* NAP_TYPEDEF_HH_ */
