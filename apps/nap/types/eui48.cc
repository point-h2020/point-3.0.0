/*
 * eui48.cc
 *
 *  Created on: 8 Sep 2016
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

#include "eui48.hh"

Eui48::Eui48() {}

Eui48::Eui48(char *mac)
{
	ostringstream oss;
	oss << _eui48String[0] << _eui48String[1] << ":";
	oss << _eui48String[2] << _eui48String[3] << ":";
	oss << _eui48String[4] << _eui48String[5] << ":";
	oss << _eui48String[6] << _eui48String[7] << ":";
	oss << _eui48String[8] << _eui48String[9] << ":";
	oss << _eui48String[10] << _eui48String[11];
	_eui48String.assign(oss.str());
}

Eui48::~Eui48() {}

void Eui48::operator=(char *eui48)
{
	_eui48String.assign(eui48);
}

void Eui48::operator=(const char *eui48)
{
	_eui48String.assign(eui48);
}

void Eui48::operator=(string &eui48)
{
	_eui48String = eui48;
}

string Eui48::str()
{
	return _eui48String;
}
