/*
 * management.cc
 *
 *  Created on: 26 Aug 2016
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

#include "management.hh"

using namespace namespaces::management;

Management::Management(Blackadder *icnCore, std::mutex &icnCoreMutex,
		Configuration &configuration)
	: DnsLocal(icnCore, icnCoreMutex, configuration)
{}

Management::~Management() {}

void Management::forwarding(IcnId &cid, bool state)
{
	switch (atoi(cid.scopeId(2).c_str()))
	{
	case MANAGEMENT_II_DNS_LOCAL:
		DnsLocal::forwarding(state);
		break;
	}
}
