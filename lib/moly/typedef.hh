/*
 * typedef.hh
 *
 *  Created on: 13 Jan 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the MOnitoring LibrarY (MOLY).
 *
 * MOLY is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOLY is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOLY. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_TYPEDEF_HH_
#define LIB_TYPEDEF_HH_

#include <cstring>
#include "enum.hh"
#include <iostream>
#include <list>
#include <sstream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

using namespace std;

#define MAX_MESSAGE_PAYLOAD 65535

typedef uint32_t bytes_t;
typedef uint32_t buffer_name_t;
typedef uint32_t buffer_size_t;
typedef uint32_t content_identifier_t;
typedef uint32_t file_descriptors_t;
typedef uint16_t file_descriptor_type_t;
typedef uint8_t ip_version_t;
typedef uint32_t list_size_t;
typedef uint32_t matches_t;
typedef uint32_t node_identifier_t;
typedef uint16_t packet_jitter_t;
typedef uint32_t path_calculations_t;
typedef uint16_t port_identifier_t;
typedef uint32_t publishers_t;
typedef uint16_t root_scope_t;
typedef uint32_t subscribers_t;

typedef list< pair<root_scope_t, matches_t> > matches_namespace_t;
typedef list< pair<buffer_name_t, buffer_size_t> > buffer_sizes_t;
typedef list< pair<file_descriptor_type_t, file_descriptors_t> >
	file_descriptors_type_t;
typedef list< pair<root_scope_t, path_calculations_t> >
	path_calculations_namespace_t;
typedef list< pair<root_scope_t, publishers_t> > publishers_namespace_t;
typedef list< pair<root_scope_t, subscribers_t> > subscribers_namespace_t;
typedef list< pair<content_identifier_t, packet_jitter_t> >
	packet_jitter_cid_t;

#endif /* LIB_TYPEDEF_HH_ */
