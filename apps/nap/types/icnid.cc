/*
 * icnid.cc
 *
 *  Created on: 15 Feb 2016
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

#include <blackadder.hpp>

#include "icnid.hh"

IcnId::IcnId()
{
	_rootNamespace = 0;
	_icnIdHashed = 0;
	_icnIdString = string();
	_initialise();
}

IcnId::IcnId(RoutingPrefix routingPrefix)
{
	ostringstream oss;
	oss << setw(16) << setfill('0') << NAMESPACE_IP;
	oss << setw(16) << setfill('0') << routingPrefix.uint();
	_icnIdString = oss.str();
	_initialise();
}

IcnId::IcnId(uint32_t hashedFqdn)
{
	ostringstream oss;
	oss << setw(16) << setfill('0') << NAMESPACE_HTTP;
	oss << setw(16) << setfill('0') << hashedFqdn;
	_icnIdString = oss.str();
	_initialise();
}

IcnId::IcnId(RoutingPrefix routingPrefix, IpAddress ipAddress)
{
	ostringstream oss;
	oss << setw(16) << setfill('0') << NAMESPACE_IP;
	oss << setw(16) << setfill('0') << routingPrefix.uint();
	oss << setw(16) << setfill('0') << ipAddress.uint();
	_icnIdString = oss.str();
	_initialise();
}

IcnId::IcnId(string fqdn)
{
	ostringstream hashedFqdnOss;
	ostringstream oss;

	// wildcard... make CID /http/0
	if (fqdn.compare("*") == 0)
	{
		hashedFqdnOss << DEFAULT_HTTP_IID;
	}
	else
	{
		hashedFqdnOss << _hashString(fqdn);
	}

	uint8_t scopeLevels = _scopeLevels(hashedFqdnOss.str());
	// Now create ICN ID
	oss << setw(16) << setfill('0') << NAMESPACE_HTTP;
	oss << setw(scopeLevels * 16) << setfill('0') << hashedFqdnOss.str();
	_icnIdString = oss.str();
	_initialise();
	_fqdn = fqdn;
}

IcnId::IcnId(string fqdn, string resource)
{
	ostringstream hashedUrlOss;
	ostringstream oss;
	ostringstream urlOss;
	urlOss << fqdn << resource;
	hashedUrlOss << _hashString(urlOss.str());
	uint8_t scopeLevels = _scopeLevels(hashedUrlOss.str());
	// Now create ICN ID
	oss << setw(16) << setfill('0') << NAMESPACE_HTTP;
	oss << setw(scopeLevels * 16) << setfill('0') << hashedUrlOss.str();
	_icnIdString = oss.str();
	_initialise();
	_fqdn = fqdn;
	_resource = resource;
}

IcnId::IcnId(root_namespaces_t rootNamespace,
		information_items_t informationItem)
{
	ostringstream oss;
	oss << setw(16) << setfill('0') << rootNamespace;
	oss << setw(16) << setfill('0') << informationItem;
	_icnIdString = oss.str();
	_initialise();
}

void IcnId::access()
{
	_access();
}

const string IcnId::binEmpty()
{
	return hex_to_chararray("");
}

const string IcnId::binIcnId()
{
	return hex_to_chararray(_icnIdString);
}

const string IcnId::binId()
{
	return hex_to_chararray(id());
}

const string IcnId::binPrefixId()
{
	return hex_to_chararray(prefixId());
}

const string IcnId::binRootScopeId()
{
	return hex_to_chararray(rootScopeId());
}

const string IcnId::binScopeId(unsigned int scopeLevel)
{
	return hex_to_chararray(_icnIdString.substr(((scopeLevel - 1) * 16),
			16));
}

const string IcnId::binScopePath(unsigned int scopeLevel)
{
	return hex_to_chararray(_icnIdString.substr(0, scopeLevel * 16));
}

bool IcnId::empty()
{
	if (_icnIdHashed == 0)
	{
		return true;
	}

	return false;
}

const string IcnId::id()
{
	return _icnIdString.substr(_icnIdString.length() - 16, 16);
}

bool IcnId::fidRequested()
{
	return _fidRequested;
}

void IcnId::fidRequested(bool state)
{
	_fidRequested = state;
}

bool IcnId::forwarding()
{
	return _forwarding;
}

void IcnId::forwarding(bool status)
{
	_forwarding = status;
}

const boost::posix_time::ptime IcnId::lastAccessed()
{
	boost::posix_time::ptime lastAccessed;
	lastAccessed = _lastAccessed;
	return _lastAccessed;
}

uint32_t IcnId::length()
{
	return binIcnId().length();
}

uint16_t IcnId::rootNamespace()
{
	return _rootNamespace;
}

void IcnId::operator=(string &str)
{
	_icnIdString = str;
	_hashIcnId();
	_setRootNamespace();
	forwarding(false);
}

bool IcnId::pausing()
{
	return _pausing;
}

void IcnId::pausing(bool state)
{
	_pausing = state;
}

const string IcnId::prefixId()
{
	return _icnIdString.substr(0, _icnIdString.length() - 16);
}

const string IcnId::print()
{
	ostringstream oss;
	size_t i = 0;

	while (i < _icnIdString.length())
	{
		if ((i % 16) == 0)
		{
			oss << "/";
		}
		oss << _icnIdString[i];
		i++;
	}

	return oss.str();
}

const string IcnId::printFqdn()
{
	return _fqdn;
}

const string IcnId::printPrefixId()
{
	ostringstream oss;
	size_t i = 0;
	string prefix = prefixId();

	while (i < prefix.length())
	{
		if ((i % 16) == 0)
		{
			oss << "/";
		}
		oss << prefix[i];
		i++;
	}

	return oss.str();
}

const string IcnId::printScopePath(size_t scopeLevel)
{
	ostringstream oss;
	size_t i = 0;
	string path = scopePath(scopeLevel);

	while (i < path.length())
	{
		if ((i % 16) == 0)
		{
			oss << "/";
		}
		oss << path[i];
		i++;
	}

	return oss.str();
}

const string IcnId::rootScopeId()
{
	return _icnIdString.substr(0, 16);
}

const string IcnId::scopeId(unsigned int scopeLevel)
{
	return _icnIdString.substr(((scopeLevel - 1) * 16), 16);
}

size_t IcnId::scopeLevels()
{
	return _icnIdString.length() / 16;
}

const string IcnId::scopePath(unsigned int scopeLevel)
{
	return _icnIdString.substr(0, scopeLevel * 16);
}

string IcnId::str()
{
	return _icnIdString;
}

uint32_t IcnId::uint()
{
	return _icnIdHashed;
}

uint32_t IcnId::uintId()
{
	return atoi(id().c_str());
}

void IcnId::_access()
{
	_lastAccessed = boost::posix_time::microsec_clock::local_time();
}

void IcnId::_hashIcnId()
{
	_icnIdHashed = 0;
	const char *string = _icnIdString.c_str();

	while( *string != 0 )
	{
		_icnIdHashed = _icnIdHashed * 31 + *string++;
	}
}

uint32_t IcnId::_hashString(string str)
{
	uint32_t _stringHashed = 0;
	const char *string = str.c_str();

	while( *string != 0 )
	{
		_stringHashed = _stringHashed * 31 + *string++;
	}

	return _stringHashed;
}

void IcnId::_initialise()
{
	//_ageMutex = new std::mutex;
	fidRequested(false);
	forwarding(false);
	pausing(false);
	_access();
	_setRootNamespace();
	_hashIcnId();
}

uint8_t IcnId::_scopeLevels(string str)
{
	uint8_t scopeLevels = 0;

	// not a multiple of 16
	if ((str.length() % 16) != 0)
	{
		scopeLevels = str.length() / 16;
		scopeLevels++;
	}
	else
	{
		scopeLevels = str.length() / 16;
	}

	return scopeLevels;
}

void IcnId::_setRootNamespace()
{
	_rootNamespace = atoi(_icnIdString.substr(0, 16).c_str());
}
