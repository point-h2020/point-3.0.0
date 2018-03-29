/*
 * subscriber.cc
 *
 *  Created on: 3 Jun 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of BAMPERS.
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

#include <bampers/bampers.hh>
#include <blackadder.hpp>
#include <signal.h>

Blackadder *blackadder;
IcnId icnId;
void sigfun(int sig) {
	(void) signal(sig, SIG_DFL);
	blackadder->unpublish_info(icnId.binId(), icnId.binPrefixId(), DOMAIN_LOCAL,
			NULL, 0);
	blackadder->disconnect();
	delete blackadder;
	exit(0);
}

int main ()
{
	CIdAnalyser cIdAnalyser;
	string eventId;
	blackadder = Blackadder::Instance(true);
	(void) signal(SIGINT, sigfun);
	// Create ICN ID and publish root scope
	icnId.rootNamespace(NAMESPACE_MONITORING);
	blackadder->publish_scope(icnId.binRootScopeId(), icnId.binEmpty(),
			DOMAIN_LOCAL, NULL, 0);
	// Append ICN ID and publish scope
	icnId.append(TOPOLOGY_NODES);
	blackadder->publish_scope(icnId.binId(), icnId.prefixId(), DOMAIN_LOCAL,
			NULL, 0);
	// Append ICN ID with node type and publish scope
	icnId.append(NODE_TYPE_NAP);
	blackadder->publish_scope(icnId.binId(), icnId.prefixId(), DOMAIN_LOCAL,
			NULL, 0);
	// Append ICN ID with node ID and publish scope
	icnId.append(111); // NodeID! this must be customised according to the
	// topology
	blackadder->publish_scope(icnId.binId(), icnId.prefixId(), DOMAIN_LOCAL,
			NULL, 0);
	// Append ICN ID with information item and subscribe to it
	icnId.append(II_HTTP_REQUESTS_PER_FQDN);
	blackadder->subscribe_info(icnId.binId(), icnId.binPrefixId(),
			DOMAIN_LOCAL, NULL, 0);

	// Now wait for data to arrive
	while(true)
	{
		Event ev;
		blackadder->getEvent(ev);
		eventId = chararray_to_hex(ev.id);
		cIdAnalyser = eventId;
		switch(cIdAnalyser.primitive())
		{
		case PRIMITIVE_TYPE_HTTP_REQUESTS_PER_FQDN:
			// Now read the data. Check the structure in
			// blackadder/lib/moly/primitives/httprequestperfqdn.cc::_decomposePacket()
			break;
		}
	}

	return 0;
}
