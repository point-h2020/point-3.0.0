/*
 * scopedescriptor.cc
 *
 *  Created on: 12 Feb 2016
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

#include "scopedescriptor.hh"

ScopeDescriptor::ScopeDescriptor()
{
	_published = false;
	_subscribed = false;
}

ScopeDescriptor::~ScopeDescriptor() {}

void ScopeDescriptor::operator=(IcnId &icnId)
{
	_icnId = icnId;
}

bool ScopeDescriptor::published()
{
	return _published;
}

bool ScopeDescriptor::subscribed()
{
	return _subscribed;
}

void ScopeDescriptor::setPublicationState(bool state)
{
	_published = state;
}

void ScopeDescriptor::setSubscriptionState(bool state)
{
	_subscribed = state;
}
