/*
 * bampers.cc
 *
 *  Created on: Nov 25, 2015
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

#include <blackadder.hpp>
#include <boost/thread.hpp>
#include <ctime>

#include "icneventhandler.hh"
#include "bampers.hh"

Bampers::Bampers(Namespace &namespaceHelper)
	: Link(namespaceHelper),
	  Node(namespaceHelper),
	  Port(namespaceHelper),
	  Statistics(namespaceHelper),
	  _namespaceHelper(namespaceHelper)
{
	startIcnEventHandlerThread();
	_blackadder = _namespaceHelper.getBlackadderInstance();
	// publish and subscribe to /management/monitoringTrigger namespace
	ostringstream scopeId;
	string prefix = string();
	scopeId << setw(16) << setfill('0') << NAMESPACE_MANAGEMENT;
#ifdef BAMPERS_DEBUG
	cout << "((BAMPERS)) "
			<< "Publishing management root scope " << scopeId.str() << endl;

#endif
	_blackadder->publish_scope(hex_to_chararray(scopeId.str()),
			hex_to_chararray(prefix), DOMAIN_LOCAL, NULL, 0);
	prefix = scopeId.str();
	scopeId.flush();
	scopeId.str("");
	scopeId << setw(16) << setfill('0') << MANAGEMENT_II_MONITORING;
#ifdef BAMPERS_DEBUG
	cout << "((BAMPERS)) "
			<< "Subscribe to monitoring information item " << scopeId.str()
			<< " under father scope " << prefix << endl;
#endif
	_blackadder->subscribe_info(hex_to_chararray(scopeId.str()),
			hex_to_chararray(prefix), DOMAIN_LOCAL, NULL, 0);
}

Bampers::~Bampers() {
	delete _blackadder;
}

void Bampers::shutdown(Blackadder *blackadder)
{
	blackadder->disconnect();
}

void Bampers::startIcnEventHandlerThread()
{
	IcnEventHandler icnEventHandler(_namespaceHelper);
	boost::thread handlerThread(icnEventHandler);
}
