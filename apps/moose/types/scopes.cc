/*
 * scopes.cc
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

#include "scopes.hh"

Scopes::Scopes(log4cxx::LoggerPtr logger)
	: _logger(logger)
{}

Scopes::~Scopes() {}

void Scopes::add(IcnId &icnId)
{
	_mutex.lock();

	if (_scopesMap.find(icnId.uint()) != _scopesMap.end())
	{
		_mutex.unlock();
		return;
	}

	ScopeDescriptor scopeDescriptor;
	scopeDescriptor = icnId;
	_scopesMap.insert(pair<uint32_t, ScopeDescriptor>(icnId.uint(),
			scopeDescriptor));
	LOG4CXX_DEBUG(_logger, "New scope " << icnId.print() << " added");
	_mutex.unlock();
}

void Scopes::add(string icnIdString)
{
	IcnId icnId;
	icnId = icnIdString;
	add(icnId);
}

void Scopes::erase(IcnId &icnId)
{
	_mutex.lock();
	_itScopesMap = _scopesMap.find(icnId.uint());

	if (_itScopesMap == _scopesMap.end())
	{
		LOG4CXX_DEBUG(_logger, "Scope " << icnId.print() << " is not known. "
				"Nothing to be erased");
		_mutex.unlock();
		return;
	}

	_scopesMap.erase(icnId.uint());
	LOG4CXX_DEBUG(_logger, "Scope " << icnId.print() << " erased");
	_mutex.unlock();
}

bool Scopes::published(IcnId &icnId)
{
	bool state = false;
	_mutex.lock();
	_itScopesMap = _scopesMap.find(icnId.uint());

	if (_itScopesMap != _scopesMap.end())
	{
		state = _itScopesMap->second.published();
	}

	_mutex.unlock();
	return state;
}

bool Scopes::subscribed(IcnId &icnId)
{
	bool state = false;
	_mutex.lock();
	_itScopesMap = _scopesMap.find(icnId.uint());

	if (_itScopesMap != _scopesMap.end())
	{
		state = _itScopesMap->second.subscribed();
	}

	_mutex.unlock();
	return state;
}

void Scopes::setPublicationState(IcnId &icnId, bool state)
{
	_mutex.lock();
	_itScopesMap = _scopesMap.find(icnId.uint());

	if (_itScopesMap == _scopesMap.end())
	{
		LOG4CXX_ERROR(_logger, "Publication state cannot be changed. Scope "
				"path " << icnId.print() << " unknown");
		_mutex.unlock();
		return;
	}

	(*_itScopesMap).second.setPublicationState(state);
	_mutex.unlock();
}

void Scopes::setSubscriptionState(IcnId &icnId, bool state)
{
	_mutex.lock();
	_itScopesMap = _scopesMap.find(icnId.uint());

	if (_itScopesMap == _scopesMap.end())
	{
		LOG4CXX_ERROR(_logger, "Subscription state cannot be changed. Scope "
				"path " << icnId.print() << " unknown");
		_mutex.unlock();
		return;
	}

	(*_itScopesMap).second.setSubscriptionState(state);
	_mutex.unlock();
}
