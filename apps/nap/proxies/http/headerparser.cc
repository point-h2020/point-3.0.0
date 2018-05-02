/*
 * headerparser.cc
 *
 *  Created on: 1 Feb 2017
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

#include "headerparser.hh"

using namespace proxies::http::headerparser;

LoggerPtr HeaderParser::logger(Logger::getLogger("proxies.http.headerparser"));

HeaderParser::HeaderParser() {}

HeaderParser::~HeaderParser() {}

string HeaderParser::readFqdn(uint8_t *packet, uint16_t &packetSize)
{
	return readFqdn((char *)packet, packetSize);
}

bool HeaderParser::mpdRequest(uint8_t *packet, uint16_t &packetSize)
{
	string httpPacket;
	size_t pos = 0;

	if (packetSize > 100)
	{//limit the packet size to not parse a large message for nothing
		httpPacket.assign((char *)packet, 100);
	}
	else
	{
		httpPacket.assign((char *)packet, packetSize);
	}

	// make the string lower case
	std::transform(httpPacket.begin(), httpPacket.end(), httpPacket.begin(),
			::tolower);

	pos = httpPacket.find(".mpd", ++pos);

	if (pos != std::string::npos)
	{
		LOG4CXX_DEBUG(logger, "HTTP request has an MPD file as the resource");
		return true;
	}

	return false;
}

bool HeaderParser::mpdResponse(uint8_t *packet, uint16_t &packetSize)
{
	string httpPacket;
	size_t pos = 0;

	if (packetSize > 1000)
	{//limit the packet size to not parse a large message for nothing
		httpPacket.assign((char *)packet, 1000);
	}
	else
	{
		httpPacket.assign((char *)packet, packetSize);
	}

	// make the string lower case
	std::transform(httpPacket.begin(), httpPacket.end(), httpPacket.begin(),
			::tolower);

	pos = httpPacket.find("<mpd", ++pos);

	if (pos != std::string::npos)
	{
		LOG4CXX_DEBUG(logger, "HTTP response comprises an XML-based <MPD> "
				"schema");
		return true;
	}

	return false;
}

string HeaderParser::readFqdn(char *packet, uint16_t &packetSize)
{
	string httpPacket;
	size_t pos = 0;
	ostringstream fqdn;

	// limit the num of bytes to be parsed in case POST/PUT arrived w/o header
	if (packetSize > 1000)
	{//limit the packet size to not parse a large message for nothing
		httpPacket.assign(packet, 1000);
	}
	else
	{
		httpPacket.assign(packet, packetSize);
	}

	// make the string lower case
	std::transform(httpPacket.begin(), httpPacket.end(), httpPacket.begin(),
			::tolower);
	pos = httpPacket.find("host: ", ++pos);

	if (pos != std::string::npos)
	{
		size_t fqdnLength = 0;
		size_t positionStart = pos + 6;
		string character;
		character = httpPacket[positionStart];

		// First get length of FQDN
		while (strcmp(character.c_str(), "\n") != 0 &&
				(pos + 6) <= packetSize)
		{
			fqdnLength++;
			pos++;
			character = httpPacket[pos + 6];
		}


		size_t it = 0;
		string colon(":");
		/* Now read the FQDN until end (and ignore last blank character) or
		 * until ':' character occurs. Some clients add the non-standard HTTP
		 * port to the host field as the FQDN, e.g. an HTTP request to
		 * sync.point on Port 3127 becomes 'host: sync.flips:3127'
		 */
		while (it < (fqdnLength - 1))
		{
			character = httpPacket[positionStart + it];

			if (character.compare(colon) == 0)
			{
				LOG4CXX_DEBUG(logger, "FQDN was given including the server port"
						" ... truncated this information from FQDN")
				it = fqdnLength;
			}
			else
			{
				fqdn << httpPacket[positionStart + it];
				it++;
			}
		}

		LOG4CXX_DEBUG(logger, "FQDN '" << fqdn.str() << "' found in HTTP"
				"request");
	}
	else
	{
		LOG4CXX_TRACE(logger, "FQDN could not be found in HTTP request");
	}

	LOG4CXX_ASSERT(logger, fqdn.str().length() == 0, "FQDN not present in HTTP "
			"request");
	return fqdn.str();
}

http_methods_t HeaderParser::readHttpRequestMethod(uint8_t *packet,
		uint16_t &packetSize)
{
	return readHttpRequestMethod((char *)packet, packetSize);
}

http_methods_t HeaderParser::readHttpRequestMethod(char *packet,
		uint16_t &packetSize)
{
	string httpPacket;
	http_methods_t httpMethod = HTTP_METHOD_UNKNOWN;
	size_t pos = 0;
	size_t posConnect = 0;
	size_t posDelete = 0;
	size_t posExtension = 0;
	size_t posGet = 0;
	size_t posHead = 0;
	size_t posOptions = 0;
	size_t posPost = 0;
	size_t posPut = 0;
	size_t posTrace = 0;

	if (packetSize < 50)
	{
		httpPacket.assign(packet, packetSize);
	}
	else
	{
		httpPacket.assign(packet, 50);
	}

	std::transform(httpPacket.begin(), httpPacket.end(), httpPacket.begin(),
				::tolower);

	while (httpMethod == HTTP_METHOD_UNKNOWN)
	{
		pos++;
		posConnect = httpPacket.find("connect", ++posConnect);

		if (posConnect != std::string::npos)
		{
			httpMethod = HTTP_METHOD_REQUEST_CONNECT;
			break;
		}

		posDelete = httpPacket.find("delete", ++posDelete);

		if (posDelete != std::string::npos)
		{
			httpMethod = HTTP_METHOD_REQUEST_DELETE;
			break;
		}

		posExtension = httpPacket.find("extension", ++posExtension);

		if (posExtension != std::string::npos)
		{
			httpMethod = HTTP_METHOD_REQUEST_EXTENSION;
			break;
		}

		posGet = httpPacket.find("get", ++posGet);

		if (posGet != std::string::npos)
		{
			httpMethod = HTTP_METHOD_REQUEST_GET;
			break;
		}

		posHead = httpPacket.find("head", ++posHead);

		if (posHead != std::string::npos)
		{
			httpMethod = HTTP_METHOD_REQUEST_HEAD;
			break;
		}

		posOptions = httpPacket.find("options", ++posOptions);

		if (posOptions != std::string::npos)
		{
			httpMethod = HTTP_METHOD_REQUEST_OPTIONS;
			break;
		}

		posPost = httpPacket.find("post", ++posPost);

		if (posPost != std::string::npos)
		{
			httpMethod = HTTP_METHOD_REQUEST_POST;
			break;
		}

		posPut = httpPacket.find("put", ++posPut);

		if (posPut != std::string::npos)
		{
			httpMethod = HTTP_METHOD_REQUEST_PUT;
			break;
		}

		posPut = httpPacket.find("delete", ++posPut);

		if (posPut != std::string::npos)
		{
			httpMethod = HTTP_METHOD_REQUEST_DELETE;
			break;
		}

		posTrace = httpPacket.find("trace", ++posTrace);

		if (posTrace != std::string::npos)
		{
			httpMethod = HTTP_METHOD_REQUEST_TRACE;
			break;
		}

		// End of packet reached
		if (pos == packetSize)
		{
			break;
		}
	}

	if (httpMethod != HTTP_METHOD_UNKNOWN)
	{
		LOG4CXX_TRACE(logger, "Method '" << httpMethod << "' found in HTTP "
				"request");
	}

	return httpMethod;
}

http_methods_t HeaderParser::readHttpResponseMethod(uint8_t *packet,
		uint16_t &packetSize)
{
	return readHttpResponseMethod((char *)packet, packetSize);
}

http_methods_t HeaderParser::readHttpResponseMethod(char *packet,
		uint16_t &packetSize)
{
	string httpPacket;
	http_methods_t httpMethod = HTTP_METHOD_UNKNOWN;
	size_t pos = 0;
	size_t posOk = 0;

	if (packetSize < 50)
	{
		httpPacket.assign(packet, packetSize);
	}
	else
	{
		httpPacket.assign(packet, 50);
	}

	std::transform(httpPacket.begin(), httpPacket.end(), httpPacket.begin(),
				::tolower);

	while (httpMethod == HTTP_METHOD_UNKNOWN)
	{
		pos++;
		posOk = httpPacket.find("ok", ++posOk);

		if (posOk != std::string::npos)
		{
			httpMethod = HTTP_METHOD_RESPONSE_OK;
			break;
		}

		// End of packet reached
		if (pos == packetSize)
		{
			break;
		}
	}

	if (httpMethod != HTTP_METHOD_UNKNOWN)
	{
		LOG4CXX_TRACE(logger, "Method '" << httpMethod << "' found in HTTP "
				"response");
	}

	return httpMethod;
}

string HeaderParser::readResource(uint8_t *packet, uint16_t &packetSize)
{
	return readResource(packet, packetSize);
}

string HeaderParser::readResource(char *packet, uint16_t &packetSize)
{
	string httpPacket;
	size_t pos = 0;
	ostringstream resource;
	httpPacket.assign(packet, packetSize);
	resource << "/";
	pos = httpPacket.find(" /", ++pos);

	if (pos != std::string::npos)
	{
		string character;
		character = httpPacket[pos + 2];

		while (strcmp(character.c_str(), " ") != 0)
		{
			resource << httpPacket[pos + 2];
			pos++;
			character = httpPacket[pos + 2];
		}
	}
	else
	{
		LOG4CXX_ERROR(logger, "Could not find resource in HTTP packet");
	}

	LOG4CXX_TRACE(logger, "Resource '" << resource.str() << "' found in HTTP"
			" request");

	return resource.str();
}
