/*
 * configuration.cc
 *
 *  Created on: 17 Apr 2016
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

#include "configuration.hh"

using namespace configuration;
using namespace log4cxx;

LoggerPtr Configuration::logger(Logger::getLogger("configuration"));

Configuration::Configuration()
{
	_backlog = 128;
	_bufferCleanerInterval = 10;// seconds
	_cNap = true;
	_hostBasedNap = false;
	_httpHandler = true;
	_httpProxyPort = 3127; // port
	_icnGatewayHttp = false;
	_icnGatewayIp = false;
	_igmp_genQueryTimer = 0;
	_igmpHandler = false;
	_igmp_redundantIGMP = 0;
	_igmp_genLeaveTimer = 0;
	_igmp_dropAfterQueriesNum = 0;
	_igmp_message_processing_lag = 0;
	_ltpInitialCredit = 10; // segments, not bytes
	_ltpRttListSize = 10; // Default
	_ltpRttMultiplier = 2;
	_localSurrogacy = false;
	_localSurrogateMethod = LOCAL_SURROGACY_METHOD_UNDEFINED;
	_localSurrogatePort = 80; // default HTTP server port
	_mitu = 1304; // octets
	_molyInterval = 0;// 0 = disabled
	_mtu = 1500; // octets
	_socketType = RAWIP;
	_surrogacy = false;
	_tcDropRate = -1; // packets
	_tcpClientSocketBufferSize = 4096;
	_tcpInterceptionPort = 80;
	_tcpServerSocketBufferSize = 4096;
	_timeout = 540;// [s] = 9min, default TCP
}

int Configuration::backlog()
{
	return _backlog;
}

void Configuration::addFqdn(IcnId &cid, IpAddress &ipAddress, uint16_t port)
{
	list<pair<IcnId, pair<IpAddress, uint16_t>>>::iterator fqdnsIt;
	_fqdnsMutex.lock();
	fqdnsIt = _fqdns.begin();
	while (fqdnsIt != _fqdns.end())
	{
		if (fqdnsIt->first.uint() == cid.uint())
		{
			if (fqdnsIt->second.first.uint() == ipAddress.uint())
			{
				LOG4CXX_DEBUG(logger, "CID " << cid.print() << " and IP address"
						" " << ipAddress.str() << " already exists in list of "
								"known FQDNs and their IP addresses");
				_fqdnsMutex.unlock();
				return;
			}
			// same FQDN but different IP
			fqdnsIt->second.first = ipAddress;
			LOG4CXX_TRACE(logger, "IP address for CID " << cid.print()
					<< " has been changed to " << ipAddress.str());
			_fqdnsMutex.unlock();
			return;
		}
		fqdnsIt++;
	}
	// FQDN has not been found. Add it
	pair<IpAddress, uint16_t> serverConfig;
	serverConfig.first = ipAddress;
	serverConfig.second = port;
	_fqdns.push_back(pair<IcnId, pair<IpAddress, uint16_t>>(cid, serverConfig));
	LOG4CXX_TRACE(logger, "CID (FQDN) " << cid.print() << " and its endpoint "
			"serving it on IP " << ipAddress.str() << ":" << port << " has been"
			" added to list of known FQDNs");
	_fqdnsMutex.unlock();
}

uint32_t Configuration::bufferCleanerInterval()
{
	return _bufferCleanerInterval;
}

bool Configuration::cNap()
{
	return _cNap;
}

void Configuration::deleteFqdn(IcnId &cid)
{
	list<pair<IcnId, pair<IpAddress, uint16_t>>>::iterator fqdnsIt;
	_fqdnsMutex.lock();
	fqdnsIt = _fqdns.begin();
	while (fqdnsIt != _fqdns.end())
	{
		if (fqdnsIt->first.uint() == cid.uint())
		{
			LOG4CXX_TRACE(logger, "CID " << cid.print() << " deleted from list "
					"of list known FQDN");
			_fqdnsMutex.unlock();
			return;
		}
		fqdnsIt++;
	}
	_fqdnsMutex.unlock();
}

bool Configuration::icnGatewayHttp()
{
	return _icnGatewayHttp;
}

bool Configuration::icnGatewayIp()
{
	return _icnGatewayIp;
}

RoutingPrefix Configuration::icnGatewayRoutingPrefix()
{
	return _icnGatewayRoutingPrefix;
}
uint32_t Configuration::icnHeaderLength()
{
	return 20;//FIXME obtain the exact number (through MAPI maybe?)
}

bool Configuration::igmpHandler()
{
	return _igmpHandler;
}

string Configuration::igmp_genQueryIP()
{
	return _igmp_genQueryIP;
}

string Configuration::igmp_groupLeaveIP()
{
	return _igmp_groupLeaveIP;
}

uint32_t Configuration::igmp_genQueryTimer()
{
	return _igmp_genQueryTimer;
}

uint32_t Configuration::igmp_genLeaveTimer()
{
	return _igmp_genLeaveTimer;
}

uint16_t Configuration::igmp_redundantIGMP()
{
	return _igmp_redundantIGMP;
}

uint32_t Configuration::igmp_dropAfterQueriesNum()
{
	return _igmp_dropAfterQueriesNum;
}

string Configuration::igmp_sNapMCastIPs()
{
	return _igmp_sNapMCastIPs;
}

string Configuration::igmp_ignoreIGMPFrom()
{
	return _igmp_ignoreIGMPFrom;
}

string Configuration::igmp_ignoreMCastDataFrom()
{
	return _igmp_ignoreMCastDataFrom;
}

string Configuration::igmp_napOperationMode()
{
	return _igmp_napOperationMode;
}

uint32_t Configuration::igmp_message_processing_lag()
{
	return _igmp_message_processing_lag;
}

IpAddress Configuration::endpointIpAddress()
{
	return _endpointIpAddress;
}

list<pair<IcnId, pair<IpAddress, uint16_t>>> Configuration::fqdns()
{
	list<pair<IcnId, pair<IpAddress, uint16_t>>> fqdns;
	_fqdnsMutex.lock();
	fqdns = _fqdns;
	_fqdnsMutex.unlock();
	return fqdns;
}

bool Configuration::hostBasedNap()
{
	return _hostBasedNap;
}

RoutingPrefix Configuration::hostRoutingPrefix()
{
	return _hostRoutingPrefix;
}

map<uint32_t, RoutingPrefix> *Configuration::hostRoutingPrefixes()
{
	return &_routingPrefixes;
}

bool Configuration::httpHandler()
{
	return _httpHandler;
}

void Configuration::ipAddress(char *ipAddress)
{
	_ipAddress = IpAddress(ipAddress);
}

IpAddress Configuration::ipAddress()
{
	return _ipAddress;
}

uint16_t Configuration::httpProxyPort()
{
	return _httpProxyPort;
}

uint32_t Configuration::ltpInitialCredit()
{
	return _ltpInitialCredit;
}

uint32_t Configuration::ltpRttListSize()
{
	return _ltpRttListSize;
}

bool Configuration::localSurrogacy()
{
	return _localSurrogacy;
}

local_surrogacy_method_t Configuration::localSurrogateMethod()
{
	return _localSurrogateMethod;
}

IpAddress Configuration::localSurrogateIpAddress()
{
	return _localSurrogateIpAddress;
}

string Configuration::localSurrogateFqdn()
{
	return _localSurrogateFqdn;
}

uint32_t Configuration::localSurrogatePort()
{
	return _localSurrogatePort;
}

uint16_t Configuration::mitu()
{
	return (uint16_t)_mitu;
}

uint32_t Configuration::molyInterval()
{
	return _molyInterval;
}

uint16_t Configuration::mtu()
{
	return (uint16_t)_mtu;
}

string Configuration::networkDevice()
{
	return _networkDevice;
}

NodeId &Configuration::nodeId()
{
	return _nodeId;
}

void Configuration::nodeId(uint32_t nodeId)
{
	if (nodeId == 0)
	{
		LOG4CXX_WARN(logger, "NID obtained through MAPI is 0. Probably not what"
				" is should be");
	}

	_nodeId = nodeId;
	LOG4CXX_TRACE(logger, "NID set to " << _nodeId.uint());
}

bool Configuration::parse(string file)
{
	Config cfg;
	try
	{
		cfg.readFile(file.c_str());
	}
	catch(const FileIOException &fioex)
	{
		LOG4CXX_FATAL(logger, "Cannot read " << file);
		return false;
	}
	catch(const ParseException &pex)
	{
		LOG4CXX_FATAL(logger, "Parsing error in file " << file);
		return false;
	}
	// Reading root
	const Setting& root = cfg.getRoot();
	try
	{
		string ip, mask;
		string socketType;
		string localSurrogateMethod;
		const Setting &napConfig = root["napConfig"];

		// Interface
		if (!napConfig.lookupValue("interface", _networkDevice))
		{
			LOG4CXX_FATAL(logger, "'interface' is missing in " << file);
			return false;
		}

		LOG4CXX_TRACE(logger, "Network interface towards IP endpoints set to "
				<< _networkDevice);

		// MTU
		int sock;
		struct ifreq ifr;
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		ifr.ifr_addr.sa_family = AF_INET;
		strncpy(ifr.ifr_name, _networkDevice.c_str(), IFNAMSIZ-1);

		if(ioctl(sock, SIOCGIFMTU, &ifr) == -1)
		{
			LOG4CXX_WARN(logger, "MTU could not be read for interface "
					<< _networkDevice);
		}
		else
		{
			_mtu = ifr.ifr_mtu;
			LOG4CXX_TRACE(logger, "MTU of " << _mtu << " read for network "
					"device");
		}

		close(sock);

		// MITU configured by user
		if (napConfig.lookupValue("mitu", _mitu))
		{
			if (_mitu > 1304)
			{
				LOG4CXX_INFO(logger, "At the moment the NAP can only guess the "
						"exact maximum MITU. It is recommended to not increase "
						"it above 1304 octets");
			}

			// make it a multiple of 8
			if ((_mitu % 8) != 0)
			{
				LOG4CXX_TRACE(logger, "Making given MITU of " << _mitu << " a "
						"multiple of 8");
				_mitu = _mitu - (_mitu % 8);
			}

			LOG4CXX_DEBUG(logger, "MITU set to " << _mitu);
		}

		// Routing prefix
		if (!napConfig.lookupValue("networkAddress", ip))
		{
			LOG4CXX_FATAL(logger, "'networkAddress' is missing in " << file);
			return false;
		}

		LOG4CXX_TRACE(logger, "Network address set to " << ip);

		if(!napConfig.lookupValue("netmask", mask))
		{
			LOG4CXX_FATAL(logger, "'netmask' is missing in " << file);
			return false;
		}

		LOG4CXX_DEBUG(logger, "Netmask for network address set to " << mask);
		_hostRoutingPrefix.networkAddress(ip);
		_hostRoutingPrefix.netmask(mask);
		LOG4CXX_DEBUG(logger, "Routing prefix this NAP servers set to "
				<< _hostRoutingPrefix.str());

		// I'm the ICN GW
		if (mask.compare("0.0.0.0") == 0)
		{
			_icnGatewayIp = true;
			string networkAddress;
			string netmask;

			if (!napConfig.lookupValue("icnGwNetworkAddress", networkAddress))
			{
				LOG4CXX_INFO(logger, "Please provide the network address of the"
						" ICN GW so that the NAP does not get too overloaded "
						"with dropping packets unneccessarily");
			}

			_icnGatewayRoutingPrefix.networkAddress(networkAddress);
			LOG4CXX_TRACE(logger, "ICN GW network address set to "
					<< _icnGatewayRoutingPrefix.networkAddress().str());

			if (!napConfig.lookupValue("icnGwNetmask", netmask))
			{
				LOG4CXX_INFO(logger, "Please provide the netmask of the ICN GW "
						"so that the NAP does not get too overloaded "
						"with dropping packets unneccessarily");
				_icnGatewayRoutingPrefix.networkAddress(string());
			}

			_icnGatewayRoutingPrefix.netmask(netmask);
			LOG4CXX_TRACE(logger, "ICN GW netmask set to "
					<< _icnGatewayRoutingPrefix.netmask().str());
		}

		// Host-based deployment
		if(napConfig.lookupValue("ipEndpoint", ip))
		{
			_hostBasedNap = true;
			_endpointIpAddress = ip;
		}

		// LTP Credit
		if (napConfig.lookupValue("ltpInitialCredit", _ltpInitialCredit))
		{
			LOG4CXX_DEBUG(logger, "Initial credit for LTP sessions set to "
					<< _ltpInitialCredit << " segments");
		}

		if (napConfig.lookupValue("httpHandler", _httpHandler))
		{
			if (!_httpHandler)
			{
				LOG4CXX_DEBUG(logger, "HTTP handler deactivated");
			}
			else
			{
				LOG4CXX_TRACE(logger, "HTTP handler stays activated");
			}
		}

		/*!
		 * HTTP Proxy Configurations
		 */
		// HTTP proxy port
		if (_httpHandler && napConfig.lookupValue("httpProxyPort",
				_httpProxyPort))
		{
			LOG4CXX_TRACE(logger, "HTTP proxy port set to " << _httpProxyPort);
		}
		else if (_httpHandler &&
				!napConfig.lookupValue("httpProxyPort", _httpProxyPort))
		{
			LOG4CXX_TRACE(logger, "HTTP handler enabled but proxy port not "
					"specified. Using default transparent proxy port "
					<< _httpProxyPort);
		}

		if (_httpHandler && napConfig.lookupValue("tcpInterceptionPort",
				_tcpInterceptionPort))
		{
			LOG4CXX_DEBUG(logger, "TCP interception port for HTTP proxy set to "
					<< _tcpInterceptionPort);
		}
		else if(_httpHandler && !napConfig.lookupValue("tcpInterceptionPort",
				_tcpInterceptionPort))
		{
			LOG4CXX_TRACE(logger, "Default TCP interception port "
					<< _tcpInterceptionPort << " used for HTTP proxy");
		}

		// TCP socket buffer sizes
		if (_httpHandler && napConfig.lookupValue("tcpClientSocketBufferSize",
				_tcpClientSocketBufferSize))
		{
			if (_tcpClientSocketBufferSize > 65535)
			{
				_tcpClientSocketBufferSize = 65535;
				LOG4CXX_INFO(logger, "TCP client socket buffer sizes larger "
						"then 65535 not supported");
			}

			LOG4CXX_DEBUG(logger, "TCP client socket buffer set to "
					<< _tcpClientSocketBufferSize);
		}

		if (_httpHandler && napConfig.lookupValue("tcpServerSocketBufferSize",
				_tcpServerSocketBufferSize))
		{
			if (_tcpServerSocketBufferSize > 65535)
			{
				_tcpServerSocketBufferSize = 65535;
				LOG4CXX_INFO(logger, "TCP client socket buffer sizes larger "
						"then 65535 not supported");
			}

			//FIXME if WU/WUD is properly implemented in cNAP this goes away
			if ((uint32_t)(ltpInitialCredit() * mitu())
					< _tcpServerSocketBufferSize)
			{
				LOG4CXX_WARN(logger, "TCP server socket buffer size too large! "
						"It will be lowered to "
						<< _tcpServerSocketBufferSize -	(ltpInitialCredit()
						* mitu()) << " bytes");
				_tcpServerSocketBufferSize = _tcpServerSocketBufferSize -
						(ltpInitialCredit()	* mitu());
			}

			LOG4CXX_TRACE(logger, "TCP server socket buffer set to "
					<< _tcpServerSocketBufferSize);
		}
		else if (_httpHandler)
		{//FIXME if WU/WUD is properly implemented in cNAP this goes away
			if ((uint32_t)(ltpInitialCredit() * mitu())
					< _tcpServerSocketBufferSize)
			{
				_tcpServerSocketBufferSize = ltpInitialCredit() * mitu();
				LOG4CXX_DEBUG(logger, "TCP server socket buffer size "
						"automatically adjusted to "
						<< _tcpServerSocketBufferSize << " bytes to prevent LTP"
								" flow control operations in the cNAP")
			}
		}

		//Backlog
		int backlog;
		char *ptr;

		// try reading it
		if ((ptr = getenv("LISTENQ")) != NULL)
		{
			backlog = atoi(ptr);
		}
		else
		{
			backlog = 128;// default value in modern linux systems
		}

		if (napConfig.lookupValue("backlog", _backlog))
		{
			if (backlog < _backlog)
			{
				_backlog = backlog;
				LOG4CXX_INFO(logger, "TCP backlog cannot be larger than "
						<< _backlog);
			}
			else
			{
				LOG4CXX_DEBUG(logger, "TCP backlog set to " << _backlog);
			}
		}
		else
		{
			_backlog = backlog;
			LOG4CXX_TRACE(logger, "TCP backlog not configured. Set to "
					<< _backlog);
		}

		// Buffer cleaner interval
		if (napConfig.lookupValue("bufferCleanerInterval",
				_bufferCleanerInterval))
		{
			LOG4CXX_TRACE(logger, "Buffer cleaner interval set to "
					<< _bufferCleanerInterval << "s");
		}
		else
		{
			LOG4CXX_DEBUG(logger, "Buffer cleaner interval was not provided. "
					"Using default value of " << _bufferCleanerInterval << "s");
		}

		// LTP timeout multiplier
		if (napConfig.lookupValue("ltpRttMultiplier", _ltpRttMultiplier))
		{
			LOG4CXX_DEBUG(logger, "LTP multiplier for RTT-based timeout counter"
					" set to " << _ltpRttMultiplier);
		}

		// LTP RTT list size
		if (napConfig.lookupValue("ltpRttListSize", _ltpRttListSize))
		{
			if (_ltpRttListSize < 1)
			{
				LOG4CXX_WARN(logger, "'ltpRttListSize' cannot be smaller than "
						"1");
			}
			else
			{
				LOG4CXX_DEBUG(logger, "LTP RTT list size set to "
						<< _ltpRttListSize);
			}
		}

		// Surrogacy
		if (_httpHandler && napConfig.lookupValue("surrogacy", _surrogacy))
		{
			if (_surrogacy)
			{
				_cNap = false;
				LOG4CXX_DEBUG(logger, "eNAP functionality enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "eNAP functionality disabled");
			}
		}

		if (napConfig.lookupValue("localSurrogateMethod", localSurrogateMethod))
		{
			if (localSurrogateMethod.compare("kernel") == 0)
			{
				LOG4CXX_TRACE(logger, "Local surrogate method 'kernel'"
						"enabled");
				_localSurrogateMethod = LOCAL_SURROGACY_METHOD_KERNEL;
				string localSurrogateIpAddress;

				if (napConfig.lookupValue("localSurrogateIpAddress",
						localSurrogateIpAddress))
				{
					_localSurrogateIpAddress = localSurrogateIpAddress;
					LOG4CXX_TRACE(logger, "Local surrogacy IP address set to "
							<< _localSurrogateIpAddress.str());
				}
				else
				{
					LOG4CXX_ERROR(logger, "You forgot to provide "
							"localSurrogateIpAddress");
					return false;
				}
			}
			else if(localSurrogateMethod.compare("nap") == 0)
			{
				_localSurrogateMethod = LOCAL_SURROGACY_METHOD_NAP;
				LOG4CXX_TRACE(logger, "Local surrogate method 'nap' enabled");

				if (_httpHandler && napConfig.lookupValue("localSurrogateFqdn",
						_localSurrogateFqdn))
				{
					_localSurrogacy = true;
					LOG4CXX_TRACE(logger, "Local surrogate server activated for "
							"FQDN " << _localSurrogateFqdn);

					if (napConfig.lookupValue("localSurrogatePort",
							_localSurrogatePort))
					{
						LOG4CXX_TRACE(logger, "Local surrogate server port set "
								"to " << _localSurrogatePort);
					}
				}
			}
			else
			{
				LOG4CXX_ERROR(logger, "Local surrogacy method could not be read"
						". Must be 'kernel' or 'nap' ... not '"
						<< localSurrogateMethod << "'");
			}
		}

#ifdef TRAFFIC_CONTROL
		// Traffic control - Drop rate
		if (napConfig.lookupValue("tcDropRate", _tcDropRate))
		{
			if (_tcDropRate == 0)
			{
				LOG4CXX_FATAL(logger, "TC drop rate cannot be 0, dude!");
				return false;
			}

			LOG4CXX_TRACE(logger, "Traffic control drop rate set to "
					<< (float)1 / _tcDropRate);
		}
#endif
		// MOLY interface
		if (napConfig.lookupValue("molyInterval", _molyInterval))
		{
			LOG4CXX_TRACE(logger, "MOLY reporting interval set to "
					<< _molyInterval);
		}

		if (napConfig.lookupValue("socketType", socketType))
		{
			if (socketType.compare("libnet") == 0)
			{
				_socketType = LIBNET;
				LOG4CXX_TRACE(logger, "Socket type is LIBNET");
			}
			else if (socketType.compare("linux") == 0)
			{
				_socketType = RAWIP;
				LOG4CXX_TRACE(logger, "Socket type is RAWIP");
			}
			else
			{
				LOG4CXX_FATAL(logger,"Unknown socket type");
			}
		}
		else
		{
			_socketType = RAWIP;
			LOG4CXX_TRACE(logger, "Socket type is RAWIP");
		}
		
		/**
		 * IGMP handler params
		 */ 
		if (napConfig.lookupValue("igmpHandler", _igmpHandler))
		{
			if (_igmpHandler)
			{
				LOG4CXX_TRACE(logger, "IGMP handler enabled ");
			}
		}

		if (napConfig.lookupValue("igmp_groupLeaveIP", _igmp_groupLeaveIP))
		{
			LOG4CXX_TRACE(logger, "IGMP handler igmp_groupLeaveIP: "
					<< _igmp_groupLeaveIP);
		}

		if (napConfig.lookupValue("igmp_genQueryIP", _igmp_genQueryIP))
		{
			LOG4CXX_TRACE(logger, "IGMP handler igmp_genQueryIP: "
					<< _igmp_genQueryIP);
		}

		if (napConfig.lookupValue("igmp_genQueryTimer", _igmp_genQueryTimer))
		{
			LOG4CXX_TRACE(logger, "IGMP handler igmp_genQueryTimer: "
					<<_igmp_genQueryTimer);
		}

		if (napConfig.lookupValue("igmp_genLeaveTimer", _igmp_genLeaveTimer))
		{
			LOG4CXX_TRACE(logger, "IGMP handler igmp_genLeaveTimer: "
					<< _igmp_genLeaveTimer);
		}

		if (napConfig.lookupValue("igmp_redundantIGMP", _igmp_redundantIGMP))
		{
			LOG4CXX_TRACE(logger, "IGMP handler igmp_redundantIGMP: "
					<< _igmp_redundantIGMP);
		}

		if (napConfig.lookupValue("igmp_dropAfterQueriesNum",
				_igmp_dropAfterQueriesNum))
		{
			LOG4CXX_TRACE(logger, "IGMP handler igmp_dropAfterQueriesNum: "
					<< _igmp_dropAfterQueriesNum);
		}

		if (napConfig.lookupValue("igmp_sNapMCastIPs", _igmp_sNapMCastIPs))
		{
			LOG4CXX_TRACE(logger, "IGMP handler igmp_sNapMCastIPs: "
					<< _igmp_sNapMCastIPs);
		}

		if (napConfig.lookupValue("igmp_ignoreIGMPFrom", _igmp_ignoreIGMPFrom))
		{
			LOG4CXX_TRACE(logger, "IGMP handler igmp_ignoreIGMPFrom: "
					<< _igmp_ignoreIGMPFrom);
		}

		if (napConfig.lookupValue("igmp_ignoreMCastDataFrom",
				_igmp_ignoreMCastDataFrom))
		{
			LOG4CXX_TRACE(logger, "IGMP handler igmp_ignoreMCastDataFrom: "
					<<_igmp_ignoreMCastDataFrom);
		}

		if (napConfig.lookupValue("igmp_napOperationMode",
				_igmp_napOperationMode))
		{
			LOG4CXX_TRACE(logger, "IGMP handler igmp_napOperationMode: "
					<< _igmp_napOperationMode);
		}

		if (napConfig.lookupValue("igmp_message_processing_lag",
				_igmp_message_processing_lag))
		{
			LOG4CXX_TRACE(logger, "IGMP handler igmp_message_processing_lag "
					<< _igmp_message_processing_lag);
		}

	}
	catch(const SettingNotFoundException &nfex)
	{
			LOG4CXX_FATAL(logger, "A mandatory root setting is missing in "
					<< file);
			return false;
	}

	// Reading routing prefixes
	const Setting& routingPrefixes = root["napConfig"]["routingPrefixes"];
	size_t count = routingPrefixes.getLength();

	for (size_t i = 0; i < count; i++)
	{
		string ip, mask;
		const Setting &routingPrefix = routingPrefixes[i];

		if (!(routingPrefix.lookupValue("networkAddress", ip)))
		{
			LOG4CXX_FATAL(logger, "Network address of a routing prefix could"
					"not be read in " << file);
			return false;
		}

		if (!(routingPrefix.lookupValue("netmask", mask)))
		{
			LOG4CXX_FATAL(logger, "Netmask of a routing prefix could not be"
					"read in " << file);
			return false;
		}

		// adding prefix to map
		RoutingPrefix prefix(ip, mask);
		_routingPrefixes.insert(pair<uint32_t, RoutingPrefix>(prefix.uint(),
				prefix));
		LOG4CXX_TRACE(logger, "Routing prefix " << prefix.str() << " added");
	}

	// Reading FQDN Registrations
	if (_httpHandler)
	{
		const Setting &fqdns = root["napConfig"]["fqdns"];
		string id, prefixId;
		count = fqdns.getLength();

		for (size_t i = 0; i < count; i++)
		{
			string ip, fqdn;
			int port;
			const Setting &f = fqdns[i];

			// FQDN
			if (!(f.lookupValue("fqdn", fqdn)))
			{
				LOG4CXX_FATAL(logger, "FQDN could not be read");
				return false;
			}
			else
			{
				//check if HTTP wildcard has been configured
				if (fqdn.compare("*") == 0)
				{
					_icnGatewayHttp = true;
				}
			}

			// IP address
			if (!(f.lookupValue("ipAddress", ip)))
			{
				LOG4CXX_FATAL(logger, "IP address for FQDN registration"
						<< " could not be read");
				return false;
			}

			// Port
			if (!(f.lookupValue("port", port)))
			{
				port = 80;
			}

			_cNap = false;
			IpAddress ipAddress(ip);
			pair<IpAddress, uint16_t> serverConfig;
			serverConfig.first = ipAddress;
			serverConfig.second = port;
			_fqdns.push_front(pair<string, pair<IpAddress, uint16_t>>(fqdn,
					serverConfig));
			LOG4CXX_TRACE(logger, "FQDN " << fqdn << " and its IP address "
					<< ipAddress.str() << " + Port " << port << " added");
		}
	}

	return true;
}

uint16_t Configuration::ltpRttMultiplier()
{
	return _ltpRttMultiplier;
}

SocketType Configuration::socketType()
{
	return _socketType;
}

bool Configuration::surrogacy()
{
	return _surrogacy;
}

int Configuration::tcDropRate()
{
	return _tcDropRate;
}

uint16_t Configuration::tcpClientSocketBufferSize()
{
	return _tcpClientSocketBufferSize;
}

int Configuration::tcpInterceptionPort()
{
	return _tcpInterceptionPort;
}

uint16_t Configuration::tcpServerSocketBufferSize()
{
	return _tcpServerSocketBufferSize;
}

uint16_t Configuration::timeout()
{
	return _timeout;
}
