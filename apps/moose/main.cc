/*
l * main.cc
 *
 *  Created on: 8 Feb 2016
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

#include <blackadder.hpp>
#include <boost/program_options.hpp>
#include <libconfig.h++>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/helpers/pool.h>
#include <log4cxx/fileappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/simplelayout.h>
#include <signal.h>

#include "types/enumerations.hh"
#include "types/scopes.hh"
#include "types/typedef.hh"
#include "filesystemchecker/filesystemchecker.hh"
#include "messagestack/messagestack.hh"
#include "monitoring/reporter.hh"
#include "monitoring/statistics.hh"
#include "mysqlconnector/mysqlconnector.hh"

/**
 * DISCLAIMER
 *
 * Ugly ugly code!!!
 *
 * TODO
 * - read configuration file in dedicated class
 * - hide subscription to information items in BAMPERS
 */

using namespace libconfig;
namespace po = boost::program_options;
using namespace std;

// A helper log4cxx function to simplify the main part.
template<class T> ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
    return os;
}

log4cxx::ConsoleAppender * consoleAppender =
		new log4cxx::ConsoleAppender(log4cxx::LayoutPtr(
				new log4cxx::SimpleLayout()));
log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("logger");
Blackadder *ba;
IcnId rootScope;
Scopes scopes(logger);
boost::thread *mysqlConnectorThreadPointer;
boost::thread *filesystemCheckerThreadPointer;
/*!
 * \brief Callback if Ctrl+C was hit
 */
void sigfun(int sig) {
	(void) signal(sig, SIG_DFL);
	LOG4CXX_INFO(logger, "Termination requested ... cleaning up");
	mysqlConnectorThreadPointer->interrupt();
	filesystemCheckerThreadPointer->interrupt();
	ba->unpublish_scope(rootScope.binId(), rootScope.binPrefixId(),
			DOMAIN_LOCAL, NULL, 0);
	scopes.erase(rootScope);
	ba->disconnect();
	delete ba;
	LOG4CXX_INFO(logger, "Bye for now");
	exit(0);
}
// Main function
int main(int ac, char* av[])
{
	string serverIpAddress;
	string serverPort;
	string username;
	string password;
	string database;
	string visApiDirectory;
	moose::Statistics statistics(logger);
	moose::Reporter reporter(logger, statistics);
	// data points
	data_points_t dataPoints;
	log4cxx::BasicConfigurator::configure(
			log4cxx::AppenderPtr(consoleAppender));
	ostringstream ossDescription;
	ossDescription << "\nMOnitOring SErver (MOOSE)\n\nAuthor:\tSebastian"
			" Robitzsch, InterDigital Europe, Ltd.\n\t"
			"<sebastian.robitzsch@interdigital.com>\n";
	ossDescription <<
"            `-.         '.       `-.)\\.'  /\n"
"               `._       ''-. `:.      _.'\n"
"                  `-._...__.::::.__.--'\n"
"                       _.-..'''''.\n"
"               _.---.__`._.       `-.\n"
"        ___..-'             `o>      `-.\n"
"   .-```                           <)   )\n"
" .'                         `._.-.`-._.'\n"
"/                           /     `-'\n"
"|            '             /\n"
"|     .       '          .'\n"
" \\     \\       \\       .'|\n"
"  '    / -..__.|     /   |\n"
"  |   /|   |   \\    '\\   /\n"
"  \\   | \\  |    |  | |  |\n"
"   \\ /   . |    |  /  \\ |\n"
"   | |    \\ \\   | |   | |\n"
"   | \\     | `. | |   | |\n"
"   |  \\    /   `' |  /_  `.\n"
"   /'  \\   `---/_  `.  `.\\.'\n"
"    `.\\.'        `.\\.'\n\nAllowed options";
	po::options_description desc(ossDescription.str().c_str());
	desc.add_options()
	("configuration,c", po::value< string >(), "libconfig-based configuration "
			"file. Template in /usr/doc/share/moose. Default read from "
			"/etc/moose/moose.cfg")
	("debug,d", po::value< string >(), "Enable verbosity (FATAL|ERROR|INFO|"
			"DEBUG|TRACE)")
	("help,h", "Print this help message");
	po::positional_options_description p;
	po::variables_map vm;
	po::store(
			po::command_line_parser(ac, av).options(desc).positional(p).run(),
			vm);
	po::notify(vm);

	// Print help and exit
	if (vm.count("help")) {
		cout << desc;
		return EXIT_SUCCESS;
	}

	MessageStack messageStack(logger, statistics);

	// Reading verbose level
	if (vm.count("debug"))
	{
		if (vm["debug"].as< string >() == "FATAL")
		{
			log4cxx::Logger::getRootLogger()->setLevel(
					log4cxx::Level::getFatal());
			LOG4CXX_ERROR(logger, "Verbose level set to FATAL");
		}
		else if (vm["debug"].as< string >() == "ERROR")
		{
			log4cxx::Logger::getRootLogger()->setLevel(
					log4cxx::Level::getError());
			LOG4CXX_ERROR(logger, "Verbose level set to ERROR");
		}
		else if (vm["debug"].as< string >() == "INFO")
		{
			log4cxx::Logger::getRootLogger()->setLevel(
					log4cxx::Level::getInfo());
			LOG4CXX_INFO(logger, "Verbose level set to INFO");
		}
		else if (vm["debug"].as< string >() == "DEBUG")
		{
			log4cxx::Logger::getRootLogger()->setLevel(
					log4cxx::Level::getDebug());
			LOG4CXX_DEBUG(logger, "Verbose level set to DEBUG");
		}
		else if (vm["debug"].as< string >() == "TRACE")
		{
			log4cxx::Logger::getRootLogger()->setLevel(
					log4cxx::Level::getTrace());
			LOG4CXX_TRACE(logger, "Verbose level set to TRACE");
		}
		else
		{
			LOG4CXX_ERROR(logger, "Unknown debug mode");
			return EXIT_FAILURE;
		}
	}
	else
	{
		log4cxx::Logger::getRootLogger()->setLevel(
				log4cxx::Level::getFatal());
	}

	if (getuid() != 0)
	{
		cout << "moose must be started with root (sudo) privileges\n";
		return EXIT_FAILURE;
	}

	// Determine CFG location NAP config
	string configurationFile;

	if (vm.count("configuration"))
	{
		configurationFile.assign(vm["configuration"].as<string>().c_str());
	}
	else
	{
		configurationFile.assign("/etc/moose/moose.cfg");
	}

	// Read CFG file
	Config cfg;
	try
	{
		cfg.readFile(configurationFile.c_str());
	}
	catch(const FileIOException &fioex)
	{
		LOG4CXX_ERROR(logger,"Cannot read " << configurationFile);
		return EXIT_FAILURE;
	}
	catch(const ParseException &pex)
	{
		LOG4CXX_ERROR(logger, "Parse error in "	<< configurationFile);
		return EXIT_FAILURE;
	}

	// Reading root
	const Setting& root = cfg.getRoot();

	try
	{
		LOG4CXX_INFO(logger, "Reading configuration file "
				<< configurationFile);
		const Setting &mooseConfig = root["mooseConfig"];

		if (!mooseConfig.lookupValue("ipAddress", serverIpAddress))
		{
			LOG4CXX_FATAL(logger, "'ipAddress' could not be read");
			return EXIT_FAILURE;
		}

		if (!mooseConfig.lookupValue("port", serverPort))
		{
			LOG4CXX_FATAL(logger, "'port' could not be read");
			return EXIT_FAILURE;
		}

		if (!mooseConfig.lookupValue("username", username))
		{
			LOG4CXX_FATAL(logger, "'username' could not be read");
			return EXIT_FAILURE;
		}

		if (!mooseConfig.lookupValue("password", password))
		{
			LOG4CXX_FATAL(logger, "'password' could not be read");
			return EXIT_FAILURE;
		}

		if (!mooseConfig.lookupValue("database", database))
		{
			LOG4CXX_FATAL(logger, "'database' could not be read");
			return EXIT_FAILURE;
		}

		if (!mooseConfig.lookupValue("visApiDir", visApiDirectory))
		{
			LOG4CXX_FATAL(logger, "Vis directory to enable/disable datapoints "
					<< "at run-time is missing");
			return EXIT_FAILURE;
		}
	}
	catch(const SettingNotFoundException &nfex)
	{
		LOG4CXX_FATAL(logger, "Setting not found in "
				<< vm["configuration"].as<string>());
		return false;
	}

	ba = Blackadder::Instance(true);
	// Subscribing to monitoring namespace
	rootScope.rootNamespace(NAMESPACE_MONITORING);
	scopes.add(rootScope);
	// publish root scope
	ba->publish_scope(rootScope.binId(), rootScope.binPrefixId(), DOMAIN_LOCAL,
			NULL, 0);
	scopes.setPublicationState(rootScope, true);
	//find data points, publish the respective scopes and subscribe to them
	const Setting& cfgDataPoints = root["mooseConfig"]["dataPoints"];

	try
	{
		if (cfgDataPoints.lookupValue("topology", dataPoints.topology))
		{
			if (dataPoints.topology)
			{
				LOG4CXX_DEBUG(logger, "Reading the entire topology has been "
						"enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Topology variable declared in "
						<< configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("bufferSizes", dataPoints.bufferSizes))
		{
			if (dataPoints.bufferSizes)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'bufferSizes' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'bufferSizes' declared in "
						<< configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("channelAquisitionTime",
				dataPoints.channelAquisitionTime))
		{
			if (dataPoints.channelAquisitionTime)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'channelAquisitionTime' "
						"enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'channelAquisitionTime' "
						"declared in " << configurationFile << " but set to "
						"false");
			}
		}

		if (cfgDataPoints.lookupValue("cmcGroupSize", dataPoints.cmcGroupSize))
		{
			if (dataPoints.cmcGroupSize)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'cmcGroupSize' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'cmcGroupSize' declared in "
						<< configurationFile << " but set to false");
			}
		}

		// CPU Utilisation
		if (cfgDataPoints.lookupValue("cpuUtilisation",
				dataPoints.cpuUtilisation))
		{
			if (dataPoints.cpuUtilisation)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'cpuUtilisation' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'cpuUtilisation' declared in "
						<< configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("e2elatency", dataPoints.e2elatency))
		{
			if (dataPoints.e2elatency)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'e2elatency' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'e2elatency' declared in "
						<< configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("fileDescriptorsType",
				dataPoints.fileDescriptorsType))
		{
			if (dataPoints.fileDescriptorsType)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'fileDescriptorsType' "
						"enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'fileDescriptorsType' "
						"declared in " << configurationFile << " but set to "
						"false");
			}
		}

		if (cfgDataPoints.lookupValue("httpRequestsFqdn",
				dataPoints.httpRequestsFqdn))
		{
			if (dataPoints.httpRequestsFqdn)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'httpRequestsFqdn' "
						"enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'httpRequestsFqdn' "
						"declared in " << configurationFile << " but set to "
								"false");
			}
		}

		if (cfgDataPoints.lookupValue("linkState", dataPoints.linkState))
		{
			if (dataPoints.linkState)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'linkState' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'linkState' declared "
						"in " << configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("matchesNamespace",
				dataPoints.matchesNamespace))
		{
			if (dataPoints.matchesNamespace)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'matchesNamespace' "
						"enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'matchesNamespace' "
						"declared in " << configurationFile << " but set to "
								"false");
			}
		}

		if (cfgDataPoints.lookupValue("networkLatencyFqdn",
				dataPoints.networkLatencyFqdn))
		{
			if (dataPoints.networkLatencyFqdn)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'networkLatencyFqdn'"
						"enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'networkLatencyFqdn' "
						"declared in " << configurationFile << " but set to "
								"false");
			}
		}

		if (cfgDataPoints.lookupValue("nodeState", dataPoints.nodeState))
		{
			if (dataPoints.nodeState)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'nodeState' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'nodeState' declared "
						"in " << configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("packetDropRate",
				dataPoints.packetDropRate))
		{
			if (dataPoints.packetDropRate)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'packetDropRate' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'packetDropRate' declared in "
						<< configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("packetErrorRate",
				dataPoints.packetErrorRate))
		{
			if (dataPoints.packetErrorRate)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'packetErrorRate' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'packetErrorRate' declared "
						"in " << configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("pathCalculationsNamespace",
				dataPoints.pathCalculationsNamespace))
		{
			if (dataPoints.pathCalculationsNamespace)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'pathCalculationsNamespace' "
						"enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'pathCalculationsNamespace' "
						"declared in " << configurationFile << " but set to "
								"false");
			}
		}

		if (cfgDataPoints.lookupValue("portState", dataPoints.portState))
		{
			if (dataPoints.portState)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'portState' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'portState' declared "
						"in " << configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("publishersNamespace",
				dataPoints.publishersNamespace))
		{
			if (dataPoints.publishersNamespace)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'publishersNamespace' "
						"enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'publishersNamespace' "
						"declared in " << configurationFile << " but set to "
								"false");
			}
		}

		if (cfgDataPoints.lookupValue("txBytesHttp", dataPoints.txBytesHttp))
		{
			if (dataPoints.txBytesHttp)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'txBytesHttp' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'txBytesHttp' declared in "
						<< configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("txBytesIp", dataPoints.txBytesIp))
		{
			if (dataPoints.txBytesIp)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'txBytesIp' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'txBytesIp' declared in "
						<< configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("txBytesIpMulticast",
				dataPoints.txBytesIpMulticast))
		{
			if (dataPoints.txBytesIpMulticast)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'txBytesIpMulticast' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'txBytesIpMulticast' declared"
						" in " << configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("txBytesPort", dataPoints.txBytesPort))
		{
			if (dataPoints.txBytesPort)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'txBytesPort' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'txBytesPort' declared in "
						<< configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("rxBytesHttp", dataPoints.rxBytesHttp))
		{
			if (dataPoints.rxBytesHttp)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'rxBytesHttp' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'rxBytesHttp' declared in "
						<< configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("rxBytesIp", dataPoints.rxBytesIp))
		{
			if (dataPoints.rxBytesIp)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'rxBytesIp' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'rxBytesIp' declared in "
						<< configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("rxBytesIpMulticast",
				dataPoints.rxBytesIpMulticast))
		{
			if (dataPoints.rxBytesIpMulticast)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'rxBytesIpMulticast' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'rxBytesIpMulticast' declared"
						" in " << configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("rxBytesPort", dataPoints.rxBytesPort))
		{
			if (dataPoints.rxBytesPort)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'rxBytesPort' enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'rxBytesPort' declared in "
						<< configurationFile << " but set to false");
			}
		}

		if (cfgDataPoints.lookupValue("subscribersNamespace",
				dataPoints.subscribersNamespace))
		{
			if (dataPoints.subscribersNamespace)
			{
				dataPoints.statistics = true;
				LOG4CXX_DEBUG(logger, "Data point 'subscribersNamespace' "
						"enabled");
			}
			else
			{
				LOG4CXX_TRACE(logger, "Data point 'subscribersNamespace' "
						"declared in " << configurationFile << " but set to "
						"false");
			}
		}
	}
	catch(const SettingNotFoundException &nfex)
	{
		LOG4CXX_FATAL(logger, "Data point not found in "
				<< vm["configuration"].as<string>());
	}

	// Database connector
	MySqlConnector mySqlConnector(logger, serverIpAddress, serverPort, username,
			password, database, messageStack);
	boost::thread mySqlConnectorThread(mySqlConnector);
	mysqlConnectorThreadPointer = &mySqlConnectorThread;
	//Visualisation synchroniser
	FilesystemChecker filesystemChecker(logger, visApiDirectory, scopes, ba);
	boost::thread filesystemCheckerThread(filesystemChecker);
	filesystemCheckerThreadPointer = &filesystemCheckerThread;
	//reporter
	boost::thread reporterThread(reporter);
	// Publish management root scope and advertise information item trigger
	IcnId managementIcnId;
	managementIcnId.rootNamespace(NAMESPACE_MANAGEMENT);
	managementIcnId.append(MANAGEMENT_II_MONITORING);
	LOG4CXX_DEBUG(logger, "Publishing management root scope "
			<< managementIcnId.rootScopeId());
	ba->publish_scope(managementIcnId.binRootScopeId(),
			managementIcnId.binEmpty(), DOMAIN_LOCAL, NULL, 0);
	LOG4CXX_DEBUG(logger, "Advertise monitoring information item "
			<< managementIcnId.id() << " under father scope path "
			<< managementIcnId.prefixId());
	ba->publish_info(managementIcnId.binId(), managementIcnId.binPrefixId(),
			DOMAIN_LOCAL, NULL, 0);
	// Publish root monitoring scope and subscribe to it
	IcnId monitoringIcnId;
	monitoringIcnId.rootNamespace(NAMESPACE_MONITORING);
	LOG4CXX_DEBUG(logger, "Publishing monitoring root scope "
				<< monitoringIcnId.rootScopeId());
	ba->publish_scope(monitoringIcnId.binRootScopeId(),
			monitoringIcnId.binEmpty(), DOMAIN_LOCAL, NULL, 0);
	ba->subscribe_scope(monitoringIcnId.binId(), monitoringIcnId.binEmpty(),
			DOMAIN_LOCAL, NULL, 0);
	LOG4CXX_DEBUG(logger, "Subscribed to monitoring root "
			<< monitoringIcnId.print());
	(void) signal(SIGINT, sigfun);
	IcnId icnId;
	IcnId cid;
	string eventId;

	while (true)
	{
		Event ev;
		ba->getEvent(ev);
		eventId = chararray_to_hex(ev.id);
		icnId = eventId;

		switch (ev.type)
		{
			case SCOPE_PUBLISHED:
			{
				LOG4CXX_DEBUG(logger, "SCOPE_PUBLISHED received for scope path "
						<< icnId.print());
				scopes.add(icnId);
				scopes.setPublicationState(icnId, true);
				CIdAnalyser icnIdAnalyser(icnId);

				// nodes branch
				if (dataPoints.statistics && icnIdAnalyser.nodesScope())
				{
					/* /monitoring/topologyNodes/nodeRole/nodeId/infItem
					 */
					if (icnId.scopeLevels() > 3)
					{
						// NAP related data points
						if (icnIdAnalyser.nodeRole() == NODE_ROLE_NAP ||
								icnIdAnalyser.nodeRole() == NODE_ROLE_GW)
						{
							if (dataPoints.channelAquisitionTime)
							{
								cid = icnId;
								cid.append(II_CHANNEL_AQUISITION_TIME);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II "
										"'Channel Aquisition Time': "
										<< cid.print());
							}

							if (dataPoints.cmcGroupSize)
							{
								cid = icnId;
								cid.append(II_CMC_GROUP_SIZE);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II 'CMC "
										"group size': " << cid.print());
							}

							if (dataPoints.httpRequestsFqdn)
							{
								cid = icnId;
								cid.append(II_HTTP_REQUESTS_FQDN);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II "
										"'# of HTTP requests': "<< cid.print());
							}

							if (dataPoints.networkLatencyFqdn)
							{
								cid = icnId;
								cid.append(II_NETWORK_LATENCY_FQDN);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II "
										"'Latency per FQDN': " << cid.print());
							}

							if (dataPoints.rxBytesHttp)
							{
								cid = icnId;
								cid.append(II_RX_BYTES_HTTP);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II "
										"'RX Bytes HTTP': " << cid.print());
							}

							if (dataPoints.rxBytesIp)
							{
								cid = icnId;
								cid.append(II_RX_BYTES_IP);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II "
										"'RX Bytes IP': " << cid.print());
							}

							if (dataPoints.rxBytesIpMulticast)
							{
								cid = icnId;
								cid.append(II_RX_BYTES_IP_MULTICAST);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II "
										"'RX Bytes IP Multicast': "
										<< cid.print());
							}

							if (dataPoints.txBytesHttp)
							{
								cid = icnId;
								cid.append(II_TX_BYTES_HTTP);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II "
										"'TX Bytes HTTP': " << cid.print());
							}

							if (dataPoints.txBytesIp)
							{
								cid = icnId;
								cid.append(II_TX_BYTES_IP);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II "
										"'TX Bytes IP': " << cid.print());
							}

							if (dataPoints.txBytesIpMulticast)
							{
								cid = icnId;
								cid.append(II_TX_BYTES_IP_MULTICAST);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II "
										"'TX Bytes IP Multicast': "
										<< cid.print());
							}
						}
						// IP endpoints related
						else if (icnIdAnalyser.nodeRole() == NODE_ROLE_SERVER ||
								icnIdAnalyser.nodeRole() == NODE_ROLE_UE)
						{
							if (dataPoints.e2elatency)
							{
								// NID -> simply subscribe
								if (icnId.scopeLevels() == 4)
								{
									ba->subscribe_scope(icnId.binId(),
											icnId.binPrefixId(), DOMAIN_LOCAL,
											NULL, 0);
									LOG4CXX_DEBUG(logger, "Subscribed to scope "
											<< icnId.print()
											<< " (nodes branch - "
													"e2elatency)");
								}
								// endpoint ID. append with II & subscribe
								else if (icnId.scopeLevels() == 5)
								{
									cid = icnId;
									cid.append(II_END_TO_END_LATENCY);
									ba->subscribe_info(cid.binId(),
											cid.binPrefixId(), DOMAIN_LOCAL,
											NULL, 0);
									LOG4CXX_DEBUG(logger, "Subscribed to II 'E2"
											"E latency': " << cid.print());
								}
							}
							if (dataPoints.cpuUtilisation)
							{
								// NID -> simply subscribe
								if (icnId.scopeLevels() == 4)
								{
									ba->subscribe_scope(icnId.binId(),
											icnId.binPrefixId(), DOMAIN_LOCAL,
											NULL, 0);
									LOG4CXX_DEBUG(logger, "Subscribed to scope "
											<< icnId.print()
											<< " (Nodes branch, statistics)");
								}
								// endpoint ID. append with II & subscribe
								else if (icnId.scopeLevels() == 5)
								{
									cid = icnId;
									cid.append(II_CPU_UTILISATION);
									ba->subscribe_info(cid.binId(),
											cid.binPrefixId(), DOMAIN_LOCAL,
											NULL, 0);
									LOG4CXX_DEBUG(logger, "Subscribed to II 'CP"
											"U Utilisation': " << cid.print());
								}
							}
						}
						// RV
						else if (icnIdAnalyser.nodeRole() == NODE_ROLE_RV)
						{
							if (dataPoints.matchesNamespace)
							{
								cid = icnId;
								cid.append(II_MATCHES_NAMESPACE);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II 'MATCHE"
										"S NAMESPACE': " << cid.print());
							}

							if (dataPoints.publishersNamespace)
							{
								cid = icnId;
								cid.append(II_PUBLISHERS_NAMESPACE);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II 'PUBLIS"
										"HERS NAMESPACE': " << cid.print());
							}

							if (dataPoints.publishersNamespace)
							{
								cid = icnId;
								cid.append(II_SUBSCRIBERS_NAMESPACE);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II 'SUBSCR"
										"IBERS NAMESPACE': " << cid.print());
							}
						}
						// TM
						else if (icnIdAnalyser.nodeRole() == NODE_ROLE_TM)
						{
							if (dataPoints.pathCalculationsNamespace)
							{
								cid = icnId;
								cid.append(II_PATH_CALCULATIONS_NAMESPACE);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II 'PATH "
										"CALCULATIONS NAMESPACE': "
										<< cid.print());
							}
						}
						// FN
						else if (icnIdAnalyser.nodeRole() == NODE_ROLE_FN)
						{
							if (dataPoints.rxBytesPort)
							{
								cid = icnId;
								cid.append(II_RX_BYTES_PORT);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II "
										"'RX Bytes Port': "	<< cid.print());
							}

							if (dataPoints.txBytesPort)
							{
								cid = icnId;
								cid.append(II_TX_BYTES_PORT);
								ba->subscribe_info(cid.binId(),
										cid.binPrefixId(), DOMAIN_LOCAL, NULL,
										0);
								LOG4CXX_DEBUG(logger, "Subscribed to II "
										"'TX Bytes Port': "	<< cid.print());
							}
						}

						// independently from node role
						if (dataPoints.bufferSizes)
						{
							cid = icnId;
							cid.append(II_BUFFER_SIZES);
							ba->subscribe_info(cid.binId(),
									cid.binPrefixId(), DOMAIN_LOCAL, NULL,
									0);
							LOG4CXX_DEBUG(logger, "Subscribed to II 'Buffer "
									"sizes': " << cid.print());
						}

						if (dataPoints.cpuUtilisation)
						{
							cid = icnId;
							cid.append(II_CPU_UTILISATION);
							ba->subscribe_info(cid.binId(),
									cid.binPrefixId(), DOMAIN_LOCAL, NULL,
									0);
							LOG4CXX_DEBUG(logger, "Subscribed to II 'CPU "
									"utilisation': " << cid.print());
						}

						if (dataPoints.fileDescriptorsType)
						{
							cid = icnId;
							cid.append(II_FILE_DESCRIPTORS_TYPE);
							ba->subscribe_info(cid.binId(),
									cid.binPrefixId(), DOMAIN_LOCAL, NULL,
									0);
							LOG4CXX_DEBUG(logger, "Subscribed to II 'II_FILE_"
									"DESCRIPTORS_TYPE utilisation': "
									<< cid.print());
						}

						if (dataPoints.nodeState)
						{
							cid = icnId;
							cid.append(II_STATE);
							ba->subscribe_info(cid.binId(),
									cid.binPrefixId(), DOMAIN_LOCAL, NULL,
									0);
							LOG4CXX_DEBUG(logger, "Subscribed to II 'State"
									"': " << cid.print());
						}
					}
					// less or equal to 3 scope levels > simply subscribe
					else
					{
						ba->subscribe_scope(icnId.binId(), icnId.binPrefixId(),
								DOMAIN_LOCAL, NULL, 0);
						LOG4CXX_DEBUG(logger, "Subscribed to "
								<< icnId.print() << " (Nodes branch, "
										"statistics)");
					}
				}
				else if (dataPoints.statistics && icnIdAnalyser.linksScope())
				{
					/* /monitoring/links/linkId/dstNid/srcNid/srcPid/linkType
					 * /		1 /	  2 /	 3 /	4 /	  5  /	 6  / 7
					 */
					if (icnId.scopeLevels() <= 6)
					{
						ba->subscribe_scope(icnId.binId(), icnId.binPrefixId(),
								DOMAIN_LOCAL, NULL, 0);
						LOG4CXX_DEBUG(logger, "Subscribed to " << icnId.print()
								<< " (Links branch, statistics)");
					}
					else
					{
						if (dataPoints.linkState)
						{
							cid = icnId;
							cid.append(II_STATE);
							ba->subscribe_info(cid.binId(), cid.binPrefixId(),
									DOMAIN_LOCAL, NULL, 0);
							LOG4CXX_DEBUG(logger, "Subscribed to II 'State': "
									<< cid.print()
									<< " (Links branch, statistics)");
						}
					}
				}
				else if (dataPoints.statistics && icnIdAnalyser.portsScope())
				{
					/* /monitoring/topologyPorts/nodeId/portId/infItem
					 * * /		1 /			  2 /	 3 /	4 /	  5
					 */
					if (icnId.scopeLevels() <= 3)
					{
						ba->subscribe_scope(icnId.binId(), icnId.binPrefixId(),
								DOMAIN_LOCAL, NULL, 0);
						LOG4CXX_DEBUG(logger, "Subscribed to " << icnId.print()
								<< " (Ports branch, statistics)");
					}
					else
					{
						if (dataPoints.packetDropRate)
						{
							cid = icnId;
							cid.append(II_PACKET_DROP_RATE);
							ba->subscribe_info(cid.binId(), cid.binPrefixId(),
									DOMAIN_LOCAL, NULL, 0);
							LOG4CXX_DEBUG(logger, "Subscribed to II 'Packet "
									"Drop Rate': " << cid.print()<< " (Ports "
									"branch, statistics)");
						}

						if (dataPoints.packetErrorRate)
						{
							cid = icnId;
							cid.append(II_PACKET_ERROR_RATE);
							ba->subscribe_info(cid.binId(), cid.binPrefixId(),
									DOMAIN_LOCAL, NULL, 0);
							LOG4CXX_DEBUG(logger, "Subscribed to II 'Packet "
									"Error Rate': " << cid.print()<< " (Ports "
									"branch, statistics)");
						}

						if (dataPoints.rxBytesPort)
						{
							cid = icnId;
							cid.append(II_RX_BYTES_PORT);
							ba->subscribe_info(cid.binId(), cid.binPrefixId(),
									DOMAIN_LOCAL, NULL, 0);
							LOG4CXX_DEBUG(logger, "Subscribed to II 'RX Bytes "
									"Port': " << cid.print()<< " (Ports "
									"branch, statistics)");
						}

						if (dataPoints.txBytesPort)
						{
							cid = icnId;
							cid.append(II_TX_BYTES_PORT);
							ba->subscribe_info(cid.binId(), cid.binPrefixId(),
									DOMAIN_LOCAL, NULL, 0);
							LOG4CXX_DEBUG(logger, "Subscribed to II 'TX Bytes "
									"': " << cid.print()<< " (Ports branch, "
									"statistics)");
						}

						if (dataPoints.portState)
						{
							cid = icnId;
							cid.append(II_STATE);
							ba->subscribe_info(cid.binId(), cid.binPrefixId(),
									DOMAIN_LOCAL, NULL, 0);
							LOG4CXX_DEBUG(logger, "Subscribed to II 'State"
									"': " << cid.print()<< " (Ports branch, "
									"statistics)");
						}
					}
				}

				// Topology (nodes) branch
				if (dataPoints.topology && icnIdAnalyser.nodesScope())
				{
					if (icnId.scopeLevels() < 4)
					{
						LOG4CXX_DEBUG(logger, "Subscribed to scope "
								<< icnId.print()<< " (Nodes branch, topology)");
						ba->subscribe_scope(icnId.binId(), icnId.binPrefixId(),
								DOMAIN_LOCAL, NULL, 0);
					}
					else
					{// subscribe straight to information item 'name'
						cid = icnId;
						icnId.append(II_NAME);
						LOG4CXX_DEBUG(logger, "Subscribed to CID "
								<< icnId.print()<< " (Nodes branch, topology)");
						ba->subscribe_info(icnId.binId(), icnId.binPrefixId(),
								DOMAIN_LOCAL, NULL, 0);
					}
				}
				// Links
				else if (dataPoints.topology && icnIdAnalyser.linksScope())
				{
					ba->subscribe_scope(icnId.binId(), icnId.binPrefixId(),
							DOMAIN_LOCAL, NULL, 0);
					LOG4CXX_DEBUG(logger, "Subscribed to " << icnId.print()
							<< " (Links branch, topology)");
				}
				// Ports
				else if (dataPoints.topology && icnIdAnalyser.portsScope())
				{
					if (icnId.scopeLevels() <= 3)
					{
						LOG4CXX_DEBUG(logger, "Subscribed to scope "
								<< icnId.print()<< " (Ports branch, topology)");
						ba->subscribe_scope(icnId.binId(), icnId.binPrefixId(),
								DOMAIN_LOCAL, NULL, 0);
					}
					else
					{
						cid = icnId;
						cid.append(II_NAME);
						LOG4CXX_DEBUG(logger, "Subscribed to CID "
								<< cid.print() << " (Ports branch, topology)");
						ba->subscribe_info(cid.binId(), cid.binPrefixId(),
								DOMAIN_LOCAL, NULL, 0);
					}
				}
				break;
			}
			case SCOPE_UNPUBLISHED:
				LOG4CXX_DEBUG(logger, "SCOPE_UNPUBLISHED: "
						<< chararray_to_hex(ev.id));
				break;
			case START_PUBLISH:
			{
				LOG4CXX_DEBUG(logger, "START_PUBLISH received under scope path "
						<< icnId.print());
				BoostrappingMessageTypes event = BOOTSTRAP_SERVER_UP;
				ba->publish_data(icnId.binIcnId(), DOMAIN_LOCAL, NULL, 0,
						&event, sizeof(uint8_t));
				LOG4CXX_DEBUG(logger, "Start trigger published using CID "
						<< icnId.print());
				break;
			}
			case STOP_PUBLISH:
				LOG4CXX_DEBUG(logger, "STOP_PUBLISH: " << icnId.print());
				break;
			case PUBLISHED_DATA:
				LOG4CXX_TRACE(logger, "PUBLISHED_DATA: " << icnId.print());
				scopes.add(icnId);
				messageStack.write(icnId, (uint8_t *)ev.data, ev.data_len);
				break;
		}
	}

	mysqlConnectorThreadPointer->interrupt();
	ba->unpublish_scope(rootScope.binId(), rootScope.binPrefixId(),
			DOMAIN_LOCAL, NULL, 0);
	scopes.erase(rootScope);
	ba->disconnect();
	delete ba;
	return 0;
}
