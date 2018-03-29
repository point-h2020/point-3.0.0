/*
 * elementtype.hh
 *
 *  Created on: 26 Apr 2017
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the ICN application MOnitOring SErver (MOOSE) which
 * comes with Blackadder.
 *
 * MOOSE is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOOSE is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOOSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SERVER_ELEMENTTYPE_HH_
#define SERVER_ELEMENTTYPE_HH_

#include <bampers/types/icnid.hh>
#include <moly/enum.hh>
#include <string>

#include "enumerations.hh"

/*!
 * \brief Converting node roles (MOLY) into element types (MySql DB)
 */
class ElementType {
public:
	/*!
	 * \brief Constructor
	 */
	ElementType(IcnId cId);
	/*!
	 * \brief Destructor
	 */
	~ElementType();
	/*!
	 * \brief Obtain the element type, as defined in element_type_t
	 */
	element_type_t elementType();
	/*!
	 * \brief Obtain the node role, as defined in node_role_t
	 */
	node_role_t nodeRole();
private:
	IcnId _cId;
	element_type_t _elementType;
	node_role_t _nodeRole;
	/*!
	 * \brief Convert node role into element type
	 *
	 * \param _nodeRole The node role, as defined in node_role_t
	 */
	element_type_t _convert(node_role_t nodeRole);
};

#endif /* SERVER_ELEMENTTYPE_HH_ */
