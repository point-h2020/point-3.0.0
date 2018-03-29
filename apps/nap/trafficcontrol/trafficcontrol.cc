/*
 * trafficcontrol.cc
 *
 *  Created on: 24 Aug 2016
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

#include "trafficcontrol.hh"

using namespace trafficcontrol;

LoggerPtr TrafficControl::logger(Logger::getLogger("trafficcontrol"));

TrafficControl::TrafficControl(Configuration &configuration)
	: Dropping(configuration)
{}

TrafficControl::~TrafficControl() {}

bool TrafficControl::handle()
{
	if (Dropping::dropPacket())
	{
		return true;
	}

	return false;
}
