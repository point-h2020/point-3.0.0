/*
 * icnid.cc
 *
 *  Created on: 03 June 2016
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

#include "icnid.hh"

IcnId::IcnId() {}

IcnId::~IcnId() {}

void IcnId::append(uint32_t identifier)
{
	ostringstream oss;
	oss << _icnIdString;
	oss << setw(16) << setfill('0') << identifier;
	_icnIdString = oss.str();
	_hashIcnId();
}

string IcnId::binEmpty()
{
	return hex_to_chararray("");
}

string IcnId::binIcnId()
{
	return hex_to_chararray(_icnIdString);
}

string IcnId::binId()
{
	return hex_to_chararray(id());
}

string IcnId::binPrefixId()
{
	return hex_to_chararray(prefixId());
}

string IcnId::binRootScopeId()
{
	return hex_to_chararray(rootScopeId());
}

string IcnId::id()
{
	return _icnIdString.substr(_icnIdString.length() - 16, 16);
}

void IcnId::operator=(const char *c)
{
	_icnIdString.assign(c);
}

void IcnId::operator=(string &str)
{
	_icnIdString = str;
	_hashIcnId();
}

string IcnId::prefixId()
{
	return _icnIdString.substr(0, _icnIdString.length() - 16);
}

string IcnId::print()
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

void IcnId::rootNamespace(root_namespaces_t rootNamespace)
{
	ostringstream oss;
	oss << setw(16) << setfill('0') << (int)rootNamespace;
	_icnIdString = oss.str();
	_hashIcnId();
}

string IcnId::rootScopeId()
{
	return _icnIdString.substr(0, 16);
}

uint8_t IcnId::scopeLevels()
{
	return _icnIdString.length() / 16;
}

string IcnId::str()
{
	return _icnIdString;
}

uint32_t IcnId::uint()
{
	return _icnIdHashed;
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
