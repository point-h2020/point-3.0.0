/*
 * httprequestsfqdn.cc
 *
 *  Created on: 2 Jun 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the MOnitoring LibrarY (MOLY).
 *
 * MOLY is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOLY is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOLY. If not, see <http://www.gnu.org/licenses/>.
 */

#include "httprequestsfqdn.hh"

HttpRequestsFqdn::HttpRequestsFqdn(string fqdn, uint32_t httpRequests)
	: _fqdn(fqdn),
	  _httpRequests(httpRequests)
{
	_size = sizeof(uint32_t) + _fqdn.length() + sizeof(uint32_t);
	_pointer = (uint8_t*)malloc(_size);
	_composePacket();
}

HttpRequestsFqdn::HttpRequestsFqdn(uint8_t *pointer, uint32_t size)
	: _pointer(pointer),
	  _size(size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

HttpRequestsFqdn::~HttpRequestsFqdn()
{
	free(_pointer);
}

string HttpRequestsFqdn::fqdn()
{
	return _fqdn;
}

uint32_t HttpRequestsFqdn::httpRequests()
{
	return _httpRequests;
}

uint8_t * HttpRequestsFqdn::pointer()
{
	return _pointer;
}

string HttpRequestsFqdn::print()
{
	ostringstream oss;
	oss << " | FQDN: " << _fqdn;
	oss << " | HTTP Requests: " << _httpRequests;
	return oss.str();
}

uint32_t HttpRequestsFqdn::size()
{
	return _size;
}

void HttpRequestsFqdn::_composePacket()
{
	uint8_t *bufferPointer = _pointer;
	uint32_t fqdnLength = _fqdn.length();
	// [1] FQDN length
	memcpy(bufferPointer, &fqdnLength, sizeof(uint32_t));
	bufferPointer += sizeof(uint32_t);
	// [2] FQDN
	memcpy(bufferPointer, _fqdn.c_str(), _fqdn.length());
	bufferPointer += _fqdn.length();
	// [3] HTTP requests
	memcpy(bufferPointer, &_httpRequests, sizeof(uint32_t));
}

void HttpRequestsFqdn::_decomposePacket()
{
	uint8_t *bufferPointer = _pointer;
	uint32_t fqdnLength;
	// [1] FQDN length
	memcpy(&fqdnLength, bufferPointer, sizeof(uint32_t));
	bufferPointer += sizeof(uint32_t);
	// [2] FQDN
	char fqdn[fqdnLength + 1];
	memcpy(fqdn, bufferPointer, fqdnLength);
	fqdn[fqdnLength] = '\0';
	_fqdn.assign(fqdn);
	bufferPointer += fqdnLength;
	// [3] HTTP requests
	memcpy(&_httpRequests, bufferPointer, sizeof(uint32_t));
}
