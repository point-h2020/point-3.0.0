/*
 * httpproxytypedefs.hh
 *
 *  Created on: 2 Oct 2016
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

#ifndef NAP_PROXIES_HTTP_HTTPPROXYTYPEDEFS_HH_
#define NAP_PROXIES_HTTP_HTTPPROXYTYPEDEFS_HH_

#include <unordered_map>

#include <types/icnid.hh>
#include <types/typedef.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/*!
 * \brief Struct for reverse opened TCP session lookup
 */
struct rcid_enigma_t
{
	IcnId rCId;/*!< The rCID  */
	enigma_t enigma;/*!< https://en.wikipedia.org/wiki/23_enigma */
};

typedef std::unordered_map<socket_fd_t, rcid_enigma_t> reverse_lookup_t ;/*!< u_map<SFD,
		rcid_enigma_t>  */

typedef std::unordered_map<nid_t, std::unordered_map<sk_t, socket_fd_t>>
		socket_fds_t ; /*!< u_map<NID, u_map<remote socket FD (SK), local socket
		FD> */

typedef std::unordered_map<socket_fd_t, bool> socket_state_t;/*!< the state of
		a socket (active, deactive)*/

#endif /* NAP_PROXIES_HTTP_HTTPPROXYTYPEDEFS_HH_ */
