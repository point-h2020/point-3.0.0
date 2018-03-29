/*
 * port.hh
 *
 *  Created on: 26 Apr 2017
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

#ifndef BAMPERS_DATAPOINTS_PORT_HH_
#define BAMPERS_DATAPOINTS_PORT_HH_

#include <string>

#include "../namespace.hh"

using namespace std;
/*!
 * \brief Implementation of all port primitives
 */
class Port {
public:
	/*!
	 * \brief Constructor
	 */
	Port(Namespace &namespaceHelper);
	/*!
	 * \brief Destructor
	 */
	~Port();
	/*!
	 * \brief ADD_PORT
	 */
	void add(uint32_t nodeId, uint16_t portId, string portName);
	/*!
	 * \brief PACKET_DROP_RATE primitive implementation
	 *
	 * \param nodeId The NID on which the port resides
	 * \param portId The port ID for which the drop rate is reported
	 * \param rate The oacket drop rate
	 */
	void packetDropRate(uint32_t nodeId, uint16_t portId, uint32_t rate);
	/*!
	 * \brief PACKET_ERROR_RATE primitive implementation
	 *
	 * \param nodeId The NID on which the port resides
	 * \param portId The port ID for which the drop rate is reported
	 * \param rate The oacket error rate
	 */
	void packetErrorRate(uint32_t nodeId, uint16_t portId, uint32_t rate);
	/*!
	 * \brief PORT_STATE primitive implementaion
	 *
	 * \param nodeId The NID on which the port resides
	 * \param portId The port ID in question
	 * \param state The new port state
	 */
	void state(uint32_t nodeId, uint16_t portId, state_t state);
private:
	Namespace &_namespaceHelper; /*!< Reference to the class Namespace */
};

#endif /* BAMPERS_DATAPOINTS_PORT_HH_ */
