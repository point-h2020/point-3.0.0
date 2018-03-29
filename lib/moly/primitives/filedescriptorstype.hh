/*
 * filedescriptorstype.hh
 *
 *  Created on: 18 Oct 2017
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

#ifndef MOLY_PRIMITIVES_FILEDESCRIPTORSTYPE_HH_
#define MOLY_PRIMITIVES_FILEDESCRIPTORSTYPE_HH_

#include "../molyhelper.hh"
#include "../typedef.hh"

/*!
 * \brief Implementation of the FILE_DESCRIPTORS_TYPE_M primitive
 */
class FileDescriptorsType: private MolyHelper
{
public:
	/*!
	 * \brief Constructor for processes
	 */
	FileDescriptorsType(node_role_t nodeRole,
			file_descriptors_type_t fileDescriptorsType);
	/*!
	 * \brief Constructor for MONAs
	 */
	FileDescriptorsType(uint8_t *pointer, uint32_t pointerSize);
	/*!
	 * \brief Destructor
	 */
	~FileDescriptorsType();
	/*!
	 * \brief Obtain the list of open descriptors and the descript types
	 *
	 * \return The list
	 */
	file_descriptors_type_t fileDescriptorsType();
	/*!
	 * \brief Obtain the role of the node which reports buffer sizes
	 */
	node_role_t nodeRole();
	/*!
	 * \brief Pointer to the packet
	 *
	 * \return Packet pointer
	 */
	uint8_t * pointer();
	/*!
	 * \brief Obtain the official primitive name
	 *
	 * \return Primitive name according to the specification
	 */
	string primitiveName();
	/*!
	 * \brief Print out the content of the primitive
	 *
	 * The print out order is determined by the MOLY specification
	 */
	string print();
	/*!
	 * \brief Size
	 *
	 * \return Return the size of the pointer returned by pointer()
	 */
	uint32_t size();
private:
	node_role_t _nodeRole;/*!< Node role */
	file_descriptors_type_t _fileDescriptorsType;
	file_descriptors_type_t::iterator _it;
	uint8_t *_pointer;/*!< Pointer to the packet for MONA */
	uint32_t _pointerSize;/*!< Size of _pointer for MONA */
	/*!
	 * \brief Compose packet
	 *
	 * This method composes the packet according to the specification.
	 */
	void _composePacket();
	/*!
	 * \brief Decompose packet
	 *
	 * This method decomposes the packet according to the specification.
	 */
	void _decomposePacket();
};

#endif /* MOLY_PRIMITIVES_FILEDESCRIPTORSTYPE_HH_ */
