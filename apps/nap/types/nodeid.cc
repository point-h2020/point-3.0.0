/*
 * nodeid.cc
 *
 *  Created on: 4 Feb 2016
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

#include <iomanip>
#include <iostream>
#include "nodeid.hh"
#include <sstream>

NodeId::NodeId() {
	_nodeId = 0;
}

NodeId::NodeId(string nodeId)
{
	_nodeId = (nid_t)atol(nodeId.c_str());
}

NodeId::NodeId(nid_t nodeId)
{
	_nodeId = nodeId;
}

void NodeId::operator=(string &nodeId)
{
	_nodeId = (nid_t)atol(nodeId.c_str());
}

void NodeId::operator=(nid_t &nodeId)
{
	_nodeId = nodeId;
}

string NodeId::str()
{
	ostringstream oss;
	oss << setw(8) << setfill('0') << _nodeId;
	return oss.str();
}

nid_t NodeId::uint()
{
	return _nodeId;
}
