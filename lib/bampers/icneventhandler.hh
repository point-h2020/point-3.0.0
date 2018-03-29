/*
 * icneventhandler.cc
 *
 *  Created on: Dec 6, 2015
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of BlAckadder Monitoring wraPpER clasS (BAMPERS).
 *
 * BAMPERS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * BAMPERS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * BAMPERS. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_BAMPERS_ICNEVENTHANDLER_HH_
#define LIB_BAMPERS_ICNEVENTHANDLER_HH_

#include "namespace.hh"
#include "types/icnid.hh"

/*!
 * \brief ICN message handler for MONAs
 */
class IcnEventHandler {
public:
	/*!
	 * \brief Constructor
	 */
	IcnEventHandler(Namespace &namespaceHelper);
	/*!
	 * \brief Destructor
	 */
	~IcnEventHandler();
	/*!
	 * \brief Functor
	 */
	void operator()();
	//void update(uint8_t nodeType, string hostname, STATE state);
	//void update(LINK_ID linkId, STATE state);
private:
	Namespace &_namespaceHelper; /*!< Reference to the class Namespace */
};
#endif /* LIB_BAMPERS_ICNEVENTHANDLER_HH_ */
