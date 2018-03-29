/* RV information gathering agent for Moly integration v0.9
   Copyright (C) 2017 RWTH Aachen University, Institute for Networked Systems
   Author: Janne Riihijärvi (08.11.2017)
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/error/error.h"
#include "rapidjson/error/en.h"

#include <moly/moly.hh>
#include <blackadder_enums.hpp>
#include <stdio.h>

using namespace std;

int process_rv_reply(stringstream& ss, Moly &moly);
root_scope_t map_root_scope(uint64_t s);
int connect_to_rv();

int main(int argc, char **argv)
{
    // Initialize connection to the local MONA
    Moly moly;

    if (!moly.Process::startReporting()) {
        cout << "Moly initialization failed, exiting...\n";
        return EXIT_FAILURE;
    }

    cout << "Trigger received. Start reporting RV information to MONA.\n";

    // Allocate a buffer for the JSON reply
    int buflen = 1048576;
    char *buf = new char[buflen+1];
    int received;

    int server = connect_to_rv();
    int first = 1;

    // Request string for communicating with rvinfo
    const char *request = "READ localRV.dump\n";

    // Request RV information every 5 seconds and report results through MOLY
    while (1) {
        // Sleep between iterations
        sleep(5);

	if (server < 0) {
	    printf("Error in server connection, reconnecting...\n");
	    server = connect_to_rv();
	    first = 1;
	}

	printf("rv_to_moly: Querying RV JSON data...");
        memset(buf, 0, buflen);
        if (send(server, request, strlen(request), 0) < 0) {
	    shutdown(server, SHUT_RDWR);
	    close(server);
	    server = -1;
	    break;
	}
        received = recv(server, buf, buflen, 0);
        if (received < 0) {
	    shutdown(server, SHUT_RDWR);
	    close(server);
	    server = -1;
	    break;
	}
	printf(" response received (%d bytes read).\n", received);

#ifdef RV_MOLY_DEBUG
	cout << buf << endl;
#endif

	// If we received more than just the response header process the data
	// We might need to read multiple chunks if the JSON reply is very large
	if (received > 26) {
	    stringstream ss;
	    ss.write(buf, received);

	    // Read the first protocol lines from the stream, including data length
	    // needed keeps track of the amount of data still needed to be fetched for the body
            string str;
	    int needed = received;
	    int datalen;
	    if (first) {
	        getline(ss, str);
	        needed -= str.length() + 1;
		first = 0;
	    }
            getline(ss, str);
	    needed -= str.length() + 1;
            getline(ss, str);
	    needed -= str.length() + 1;
	    sscanf(str.c_str(), "DATA %d", &datalen);
	    needed -= datalen;

#ifdef RV_MOLY_DEBUG
	    printf("Target data length is %d. Still need %d.\n", datalen, needed); 
#endif

	    while (needed > 0) {
#ifdef RV_MOLY_DEBUG
		printf("Getting next chunk from RV...\n");
#endif
	        received = recv(server, buf, buflen, 0);
		ss.write(buf, received);
		needed -= received;
#ifdef RV_MOLY_DEBUG
		printf("Received %d bytes. %d still needed.\n", received, needed);
#endif
	    }

            process_rv_reply(ss, moly);
	}
    }
}

int connect_to_rv() {
    // The RV data can be queried through TCP port 55500 when BA deployed with --rvinfo
    // Assume for now that local RV runs on the same host as this program
    int port = 55500;
    string host = "localhost";
    struct hostent *hostEntry;

    hostEntry = gethostbyname(host.c_str());
    if (!hostEntry) {
        cout << "No such host name: " << host << endl;
        return(-1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);

    int server = socket(PF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("socket");
        return(-1);
    }

    if (connect(server, (const struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return(-1);
    }

    return server;
}


// Parse the RV information reply and send results through Moly
int process_rv_reply(stringstream& ss, Moly &moly) {
    printf("rv_to_moly: Processing RV JSON data...");

#ifdef RV_MOLY_DEBUG
    stringstream json_debug;
    stringstream::pos_type pos = ss.tellg();
    json_debug << ss.rdbuf();
    printf("\nJSON to parse:\n%s", json_debug.str().c_str());
    ss.clear();
    ss.seekg(pos, ss.beg);
#endif

    // Extract the JSON part of the reply and parse it
    using namespace rapidjson;

    IStreamWrapper json_ss(ss);
    Document document;
    ParseResult parse_ok = document.ParseStream(json_ss);
    if (!parse_ok) {
        printf("JSON parse error: %s (%u)\n", GetParseError_En(parse_ok.Code()), parse_ok.Offset());
        return -1;
    }

    const Value& scopes = document["scopes"];

    // Collect pub/sub/matches counts into hash tables
    unordered_map<int, int> pubs;
    unordered_map<int, int> subs;
    unordered_map<int, int> matches;

    for (SizeType i = 0; i < scopes.Size(); i++) {
	// Extract root scope ID from the entire scope ID and translate RV/TM scopes
	std::string id_str = scopes[i][0]["id"].GetString();
	root_scope_t root_scope = map_root_scope(strtoul(id_str.substr(0, 16).c_str(), NULL, 16));

	// Add publishers, subscribers, and matches 
	pubs[root_scope] += scopes[i][0]["pub"].Size();
	subs[root_scope] += scopes[i][0]["sub"].Size();
	if (scopes[i][0]["pub"].Size() > 0 && scopes[i][0]["sub"].Size() > 0)
	    matches[root_scope]++;
    }

    // Pack all the results for Moly and send
    publishers_namespace_t pubs_moly;
    subscribers_namespace_t subs_moly;
    matches_namespace_t matches_moly;

    for (unordered_map<int, int>::const_iterator it = pubs.cbegin(); it != pubs.cend(); it++) {
#ifdef RV_MOLY_DEBUG
        printf("Pubs: %d = %d\n", it->first, it->second);
#endif
	pubs_moly.push_back(pair<root_scope_t, publishers_t>(it->first, it->second));
    }

    for (unordered_map<int, int>::const_iterator it = subs.cbegin(); it != subs.cend(); it++) {
#ifdef RV_MOLY_DEBUG
        printf("Subs: %d = %d\n", it->first, it->second);
#endif
	subs_moly.push_back(pair<root_scope_t, subscribers_t>(it->first, it->second));
    }

    for (unordered_map<int, int>::const_iterator it = matches.cbegin(); it != matches.cend(); it++) {
#ifdef RV_MOLY_DEBUG
        printf("Matches: %d = %d\n", it->first, it->second);
#endif
	matches_moly.push_back(pair<root_scope_t, matches_t>(it->first, it->second));
    }

    printf(" done, passing on to Moly...");
    moly.Process::publishersNamespace(pubs_moly);
    moly.Process::subscribersNamespace(subs_moly);
    moly.Process::matchesNamespace(matches_moly);
    printf(" done. Sleeping again.\n");

    return 0;
}

root_scope_t map_root_scope(uint64_t s) {
    if (s < 65536)
    	return s;
    else {
	enum root_namespaces_t s_mapped = NAMESPACE_UNKNOWN;
	if (s == strtoul("FFFFFFFFFFFFFFFF", NULL, 16))
            s_mapped = NAMESPACE_APP_RV;
	if (s == strtoul("FFFFFFFFFFFFFFFE", NULL, 16))
            s_mapped = NAMESPACE_RV_TM;
	if (s == strtoul("FFFFFFFFFFFFFFFD", NULL, 16))
            s_mapped = NAMESPACE_TM_APP;
	if (s == strtoul("FFFFFFFFFFFFFFFB", NULL, 16))
            s_mapped = NAMESPACE_LINK_STATE;
	if (s == strtoul("FFFFFFFFFFFFFFFA", NULL, 16))
            s_mapped = NAMESPACE_PATH_MANAGEMENT;
	return s_mapped;
    }
}
