/*
 * helper.hh
 *
 *  Created on: 28 Sep 2017
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

#ifndef MOLY_MOLYHELPER_HH_
#define MOLY_MOLYHELPER_HH_

#include <blackadder.hpp>
#include "typedef.hh"

/*!
 * \brief Simple helper class to convert data
 */
class MolyHelper {
public:
	/*!
	 * \brief Constructor
	 */
	MolyHelper();
	/*!
	 * \brief Destructor
	 */
	~MolyHelper();
	/*!
	 * \brief Convert Enumeration buffers_t into human readable format
	 */
	string bufferNameToString(buffer_name_t buffer);
	/*!
	 * \brief Convert enumeration descriptor_types_t to human readable format
	 *
	 * \param descriptorType The type of file descriptor which should be
	 * translated
	 *
	 * \return Human readable format
	 */
	string descriptorTypeToString(file_descriptor_type_t descriptorType);
	/*!
	 * \brief Convert a node role into a human readable output
	 *
	 * \param The role
	 *
	 * \return A string
	 */
	string nodeRoleToString(node_role_t nodeRole);
	/*!
	 * \brief Convert a root scope ID into a human readable output
	 *
	 * \param rootScope The root scope as an integer representation
	 *
	 * \return The root scope as a human readable name
	 */
	string scopeToString(root_scope_t rootScope);
};

#endif /* MOLY_MOLYHELPER_HH_ */
