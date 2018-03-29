/*
 * reporter.cc
 *
 *  Created on: 12 Oct 2017
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

#include "reporter.hh"

namespace moose {

Reporter::Reporter(log4cxx::LoggerPtr logger, moose::Statistics &statistics)
	: _logger(logger),
	  _statistics(statistics)
{}

Reporter::~Reporter() {}

void Reporter::operator ()()
{
	Moly moly;
	MapiAgent mapiAgent;
	buffer_sizes_t messageStackSize;
	uint32_t nodeId;

	if (!moly.Process::startReporting())
	{
		LOG4CXX_ERROR(_logger, "MOLY could not be initialised");
		return;
	}

	mapiAgent.set_MapiInstance(true);

	if (!mapiAgent.get_nodeid(nodeId))
	{
		LOG4CXX_ERROR(_logger,"Obtaining NID via MAPI failed");
		return;
	}

	LOG4CXX_DEBUG(_logger, "Obtained NID " << nodeId << " through MAPI");

	if (!moly.addNode("MOOSE", nodeId, NODE_ROLE_MOOSE))
	{
		LOG4CXX_WARN(_logger, "Could not report my existence to MONA");
	}

	if (!moly.nodeState(NODE_ROLE_MOOSE, nodeId, NODE_STATE_UP))
	{
		LOG4CXX_WARN(_logger, "Could not report UP state existence to MONA");
	}

	while (true)
	{
		messageStackSize.push_back(pair<buffer_name_t, buffer_size_t>(
				BUFFER_MOOSE_MESSAGE_STACK, _statistics.messageStackSize()));

		if (!moly.bufferSizes(NODE_ROLE_MOOSE, messageStackSize))
		{
			LOG4CXX_WARN(_logger, "Message stack size could notbe reported to "
					"MONA");
		}

		messageStackSize.clear();
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

} /* namespace moose */
