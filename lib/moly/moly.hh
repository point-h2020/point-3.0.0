/*
 * moly.hh
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

#ifndef MOLY_HH_
#define MOLY_HH_

#include "agent.hh"
#include "process.hh"
#include "typedef.hh"

/*!
 * \brief Monitoring library class providing all primitives for both
 * applications and agents
 *
 */
class Moly: public Agent, public Process
{
public:
	/*!
	 * \brief Constructor
	 */
	Moly();
	/*!
	 * \brief Destructor
	 */
	~Moly();
};

#endif /* MOLY_HH_ */

