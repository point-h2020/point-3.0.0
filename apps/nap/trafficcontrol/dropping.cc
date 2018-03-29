/*
 * dropping.cc
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

#include "dropping.hh"

using namespace trafficcontrol::dropping;

LoggerPtr Dropping::logger(Logger::getLogger("trafficcontrol.dropping"));

Dropping::Dropping(Configuration &configuration)
	: _configuration(configuration)
{}

Dropping::~Dropping() {}

bool Dropping::dropPacket()
{
	// Dropping was not requested by user
	if (_configuration.tcDropRate() == -1)
	{
		LOG4CXX_TRACE(logger, "This packet will not be dropped");
		return false;
	}
	if ((rand() % _configuration.tcDropRate()) == 0)
	{
		LOG4CXX_DEBUG(logger, "This packet will be dropped");
		return true;
	}
	return false;
}
