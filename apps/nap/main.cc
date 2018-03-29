/**
 * main.cc
 *
 * Created on 19 April 2015
 * 		Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
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
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <log4cxx/logger.h>
#include "log4cxx/basicconfigurator.h"
#include <log4cxx/propertyconfigurator.h>
#include <mapiagent.hpp>
#include <moly/moly.hh>
#include <mutex>
#include <net/ethernet.h>
#include <pcap.h>
#include <signal.h>
#include <stdlib.h>
#include <string>
#include <thread>

#include <api/napsa.hh>
#include <configuration.hh>
#include <monitoring/collector.hh>
#include <monitoring/statistics.hh>
#include <types/enumerations.hh>
#include <demux/demux.hh>
#include <icncore/icn.hh>
#include <sockets/ipsocket.hh>
#include <namespaces/namespaces.hh>
#include <proxies/http/httpproxy.hh>
#include <types/ipaddress.hh>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

using namespace api::napsa;
using namespace configuration;
using namespace demux;
using namespace icn;
using namespace log4cxx;
using namespace monitoring::collector;
using namespace monitoring::statistics;
using namespace namespaces;
using namespace namespaces::ip;
using namespace namespaces::http;
using namespace proxies::http;
using namespace std;

namespace po = boost::program_options;
// A helper log4cxx function to simplify the main part.
template<class T>

ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
    return os;
}

LoggerPtr logger(Logger::getLogger("NAP"));

/*!
 * \brief Capturing user interactions
 *
 * \param sig The signal captured
 */
void sigproc(int sig);

/*!
 * \brief Capture SIGSEGV, write core dump and exit
 */
void sigsegv(int sig);

void shutdown();

Blackadder *icnCore;
Namespaces *namespacesPointer;
MapiAgent mapiAgent;
HttpProxy *httpProxyPointer;
Configuration *configurationPointer;
bool run = true;
/*std::thread *collectorThreadPointer;
std::thread *demuxThreadPointer;
std::thread *vdemuxThreadPointer;
std::thread *napSaThreadPointer;
std::thread *httpProxyThreadPointer;*/
std::vector<std::thread> mainThreads;

int main(int ac, char* av[])
{
	bool baUserSpace;
	string device;
	uint32_t nodeId;
	po::options_description desc("\nNetwork Attachment Point (NAP)\n"
			"Author:\t Sebastian Robitzsch "
			"<sebastian.robitzsch@interdigital.com>\n\nAllowed options");
	desc.add_options()
	("configuration,c", po::value< string >(), "libconfig-based "
			"configuration file for the NAP. If not specified the NAP assumes "
			"it is located under /etc/nap/nap.cfg")
	//("daemon,d", "Run the NAP as a daemon")
	("help,h", "Print this help message")
	("kernel,k", "Tell NAP that Blackadder runs in kernel space")
	("version,v", "Print the version number of the NAP")
	;
	po::positional_options_description p;
	po::variables_map vm;
	po::store(po::command_line_parser(ac, av).options(desc).positional(p).run(),
			vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc;
		return EXIT_SUCCESS;
	}

	if (vm.count("version"))
	{
		cout << "This NAP runs version 3.3.1. More information about the "
				"released can be found in RELEASE which is located in the root "
				"directory of the NAP source folder\n";
		return EXIT_SUCCESS;
	}

	if (getuid() != 0)
	{
		if (logger->getEffectiveLevel()->toInt() > LOG4CXX_LEVELS_ERROR)
			cout << "The NAP must run with root (sudo) privileges\n";
		else
			LOG4CXX_ERROR(logger, "The NAP must run with root (sudo) "
					"privileges");
		return EXIT_FAILURE;
	}

	PropertyConfigurator::configure("/etc/nap/nap.l4j");

	// Reading whether Blackadder runs in user or kernel space
	if (vm.count("kernel"))
	{
		baUserSpace = false;
	}
	else
	{
		baUserSpace = true;
	}

	// Reading NAP config
	LOG4CXX_DEBUG(logger, "Reading NAP configuration file");
	Statistics statistics;
	Configuration configuration;
	configurationPointer = &configuration;

	if (vm.count("configuration"))
	{
		if (!configuration.parse(vm["configuration"].as< string >()))
		{
			LOG4CXX_FATAL(logger, "NAP config file could not be read");
			exit(CONFIGURATION_FILE_PARSER);
		}
	}
	else
	{
		if (!configuration.parse("/etc/nap/nap.cfg"))
		{
			LOG4CXX_FATAL(logger, "NAP config file could not be read");
			exit(CONFIGURATION_FILE);
		}
	}

	// Connecting to MAPI and fetch NID
	mapiAgent.set_MapiInstance(baUserSpace);

	if (!mapiAgent.get_nodeid(nodeId))
	{
		LOG4CXX_FATAL(logger, "Obtaining NID via MAPI failed");
		return EXIT_FAILURE;
	}

	configuration.nodeId(nodeId);
	// Connecting to ICN core
	icnCore = Blackadder::Instance(baUserSpace);
	// Creating IP socket
	IpSocket ipSocket(configuration, statistics);
	// Create TCP client instance
	std::mutex icnCoreMutex;
	Transport transport(icnCore, configuration, ipSocket, icnCoreMutex,
			statistics, &run);
	// Instantiate all namespace classes
	Namespaces namespaces(icnCore, icnCoreMutex, configuration, transport,
			statistics, &run);
	namespacesPointer = &namespaces;
	// Initialise the ICN namespaces (subscribing to respective scopes, etc)
	namespaces.initialise();
	// Start ICN handler
	Icn icn(icnCore, configuration, namespaces, transport, statistics, &run);
	LOG4CXX_DEBUG(logger, "Starting ICN handler thread");
	mainThreads.push_back(std::thread(icn));
	// Start demux
	Demux demux(namespaces, configuration, statistics, ipSocket, &run);
	LOG4CXX_DEBUG(logger, "Starting Demux thread");
	mainThreads.push_back(std::thread(demux));

	// NAP SA Listener
	if (configuration.httpHandler() && configuration.surrogacy())
	{
		NapSa napSa(namespaces, statistics);
		LOG4CXX_DEBUG(logger, "Starting NAP-SA listener thread");
		mainThreads.push_back(std::thread(napSa));
	}

	// HTTP proxy
	if (configuration.httpHandler())
	{
		httpProxyPointer = new HttpProxy(configuration, namespaces, statistics,
				&run);
		LOG4CXX_DEBUG(logger, "Starting HTTP proxy thread");
		mainThreads.push_back(std::thread(*httpProxyPointer));
	}

	// Monitoring
	if (configuration.molyInterval() > 0)
	{
		Collector collector(configuration, statistics, &run);
		LOG4CXX_DEBUG(logger, "Starting Collector thread (stats + MOLY app)");
		mainThreads.push_back(std::thread(collector));
	}
	else
	{
		LOG4CXX_DEBUG(logger, "Monitoring will be disabled, as MOLY interval is "
				"either not set in nap.cfg or it is 0: "
				<< configuration.molyInterval());
	}

	// register all possible signals
	signal(SIGHUP, sigproc);
	signal(SIGINT, sigproc);
	signal(SIGQUIT, sigproc);
	signal(SIGILL, sigproc);
	signal(SIGTRAP, sigproc);
	signal(SIGABRT, sigproc);
	signal(SIGBUS, sigproc);
	signal(SIGFPE, sigproc);
	signal(SIGKILL, sigproc);
	signal(SIGSEGV, sigproc);
	signal(SIGPIPE, sigproc);
	signal(SIGTERM, sigproc);
	signal(SIGSTKFLT, sigproc);
	signal(SIGSTOP, sigproc);
	signal(SIGURG, sigproc);
	signal(SIGXCPU, sigproc);
	signal(SIGXFSZ, sigproc);
	signal(SIGPWR, sigproc);
	signal(SIGSYS, sigproc);
	pause();
	return EXIT_SUCCESS;
}

void sigproc(int sig)
{
	switch (sig)
	{
	case SIGHUP:
		LOG4CXX_INFO(logger, "'Hangup' signal received. Shutting down");
		break;
	case SIGINT:
		LOG4CXX_INFO(logger, "'Interrupt' signal received. Shutting down");
		break;
	case SIGQUIT:
		LOG4CXX_INFO(logger, "'Quit' signal received. Shutting down");
		break;
	case SIGILL:
		LOG4CXX_ERROR(logger, "'Illegal instruction' signal received. Shutting "
				"down");
		break;
	case SIGTRAP:
		LOG4CXX_FATAL(logger, "damn ...... general protection (SIGTRAP) was "
				"issued by the CPU ... talk immediately to Sebo!");
		break;
	case SIGABRT:
		LOG4CXX_FATAL(logger, "Something sent a SIGABRT. Most likely libthread. "
						"Increase the maximal number of active threads in "
						"/proc/sys/kernel/threads-max");
		break;
	case SIGBUS:
		LOG4CXX_ERROR(logger, "'BUS error' signal received. Shutting "
				"down");
		break;
	case SIGFPE:
		LOG4CXX_ERROR(logger, "'Floating-point exception' signal received."
				"Shutting down");
		break;
	case SIGKILL:
		LOG4CXX_WARN(logger, "Don't SIGKILL dude! Think about the threads and "
			"how to the NAP is supposed to shut down the TCP proxy "
			"listening socket properly. Please use '*kill -15' and not "
			"'*kill -9' in the future. Much appreciated");
		break;
	case SIGSEGV:
		LOG4CXX_FATAL(logger, "SIGSEGV caught");
		break;
	case SIGUSR2:
		LOG4CXX_FATAL(logger, "SIGUSR2: User-defined signal 2 (POSIX)");
		break;
	case SIGPIPE:
		LOG4CXX_FATAL(logger, "SIGPIPE received. Most likely the write() "
				"function towards an IP endpoint threw");
		break;
	case SIGTERM:
		LOG4CXX_INFO(logger, "Termination requested via SIGTERM");
		break;
	case SIGSTKFLT:
		LOG4CXX_ERROR(logger, "'Stack fault' signal received. Shutting down");
		break;
	case SIGSTOP:
		LOG4CXX_ERROR(logger, "'Stop, unblockable' signal received. Shutting "
				"down");
		break;
	case SIGURG:
		LOG4CXX_ERROR(logger, "'Urgent condition on socket' signal received. "
				"Shutting down");
		break;
	case SIGXCPU:
		LOG4CXX_ERROR(logger, "'CPU limit exceeded' signal received. Shutting "
				"down");
		break;
	case SIGXFSZ:
		LOG4CXX_ERROR(logger, "'File size limit exceeded' signal received."
				"Shutting down");
		break;
	case SIGPWR:
		LOG4CXX_ERROR(logger, "'Power failure restart' signal received. "
				"Shutting down");
		break;
	case SIGSYS:
		LOG4CXX_ERROR(logger, "'Bad system call' signal received. Shutting "
				"down");
		break;
	default:
		LOG4CXX_INFO(logger, "Unknown signal received: " << sig << ". Just "
				"stopping");
	}

	shutdown();
	exit(0);
}

void shutdown()
{
	run = false;
	uint32_t waitingTime = 0;
	// sleep to let all threads finish
	waitingTime = configurationPointer->molyInterval();

	if (waitingTime < configurationPointer->bufferCleanerInterval())
	{
		waitingTime = configurationPointer->bufferCleanerInterval();
	}

	//waitingTime += 1;
	waitingTime = 1;
	LOG4CXX_INFO(logger, "Waiting for " << waitingTime << "s for all threads to"
			" stop");
	std::this_thread::sleep_for(std::chrono::seconds(waitingTime));
	namespacesPointer->uninitialise();

	if (configurationPointer->httpHandler())
	{
		httpProxyPointer->tearDown();
	}

	for (auto it = mainThreads.begin(); it != mainThreads.end(); it++)
	{
		it->detach();
	}

	LOG4CXX_INFO(logger, "Disconnecting from Blackadder");
	icnCore->disconnect();
	delete icnCore;
	LOG4CXX_INFO(logger, "Exiting ... bye, bye nappers");
}
