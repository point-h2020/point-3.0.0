#include "coaphandler.hh"
#include <iostream>
#include "openssl/sha.h"
#include <fstream>
#include "coap_proxy.h"
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <vector>
#include <string>


CoAPHandler* CoAPHandler::CoAPHandlerInstance = NULL;


void CoAPHandler::invokeCoAPHandler (std::string endpoinsFile)
{
	std::ifstream infile(endpoinsFile);
	std::string   fqdn, hostip;
	CoAPHandler* handler = CoAPHandler::getInstance(true);

	while(infile >> fqdn >> hostip)
	{
                cout<< "host: "<<fqdn<<endl;
                cout<< "hostip: "<<hostip<<endl;		
		handler->registerHostURI(fqdn,hostip);
	}
}

void CoAPHandler::handleCoapResponseFromServer(const struct sockaddr_storage* addr, socklen_t *addrlen, uint8_t* response_packet, int len){

	CoAPHandler* handler = CoAPHandler::getInstance();

	CoAP_packet *pkt=new CoAP_packet(response_packet, len);

	// create a vector to store token
	vector<uint8_t> tokenV(pkt->getToken(), pkt->getToken() + pkt->getTKL());
	
	// read server's address
	struct sockaddr_in6 *server = (struct sockaddr_in6 *)addr;
	char host_address[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &server->sin6_addr, host_address, sizeof(host_address));

	if (debug) {
		cout << "handleCoapResponseFromServer: received packet from: " << host_address << "." << endl;

		//print_hex_buffer("\tCoAP packet is: ", pkt->getCoapPacket(), pkt->getpktLen());
	}

	// prepare packet for sending to ICN
	prepareCoapPacket(pkt);
	
	/*if (debug) {
		print_hex_buffer("Updated Server packet to be sent to ICN is: ", 
						 pkt->getCoapPacket(), pkt->getpktLen());
	}*/

	handler->publishCoapResponseToICN(host_address, pkt->getCoapPacket(), pkt->getpktLen(), tokenV, pkt->isObserve());
	delete pkt;
}


void _eventHandler(Event *ev)
{
    CoAPHandler* handler = CoAPHandler::getInstance();
    handler->eventHandler(ev);
    delete ev;
}

void CoAPHandler::handleCoapRequestFromClient(std::string proxyURI, std::string host, uint8_t *request_packet, int len)
{
            	
	std::string SIdHost                  = sha256_hash(host,ID_LEN);
	std::string SId                      = COAP_SCOPE_ID + SIdHost;
	std::string RId     = CoAPHandler::generateRandomString(ID_LEN);
	
	pendingICNPublications[SId+RId].proxyURI = proxyURI;
	pendingICNPublications[SId+RId].SId  = SId;
	pendingICNPublications[SId+RId].RId  = RId;
	
	// TODO: this overlaps with coap_proxy.c/handle_coap_request
	CoAP_packet *pkt = new CoAP_packet(request_packet, len);
	
	// store the original token from the request
	pendingICNPublications[SId+RId].token = vector<uint8_t> (pkt->getToken(), pkt->getToken() + pkt->getTKL());

	prepareCoapPacket(pkt);

	// create a request packet
	pendingICNPublications[SId+RId].coap_packet = (uint8_t *)calloc(sizeof(uint8_t), pkt->getpktLen());
	memcpy(pendingICNPublications[SId+RId].coap_packet, pkt->getCoapPacket(), sizeof(uint8_t)*pkt->getpktLen());
    pendingICNPublications[SId+RId].packetLen = pkt->getpktLen();

    if (debug)
	{
		//cout << "Received CoAP request: " << request_packet << endl;
		cout << "Received CoAP request: " << endl;
		cout << "   host: " <<  host << endl;
		cout << "   resourse uri: " << proxyURI << endl;
		cout << "   observe is: " << pkt->isObserve() << endl;
		cout << "   SIdHost: " << chararray_to_hex(SIdHost) << endl;
		cout << " Will advertise request as: " << chararray_to_hex(RId) << endl;
	}
	
	delete pkt;

	if(adverisedICNIds.find(SId+RId) != adverisedICNIds.end())
	{
		publishCoapRequestToICN(SId+RId);           
		return;
	}
        
	adverisedICNIds.insert(SId+RId);
	
	ba->publish_scope(COAP_SCOPE_ID, "", DOMAIN_LOCAL, NULL, 0);
	ba->publish_scope(SIdHost, COAP_SCOPE_ID, DOMAIN_LOCAL, NULL, 0);
	ba->publish_info (RId,SId,DOMAIN_LOCAL, NULL, 0);   
}


void CoAPHandler::publishCoapResponseToICN(std::string server, uint8_t *body, int len, 
										   vector<uint8_t> token, int isObserve)
{
	uint8_t *packet = (uint8_t *)calloc(sizeof(uint8_t), len);
	memcpy(packet, body, sizeof(uint8_t)*len);

	if (debug)
    {

    }
    ba->publish_scope(COAP_SCOPE_ID, "", DOMAIN_LOCAL, NULL, 0);

	std::string itemId = tokenToItemId[token];
	
    if (debug)
    {      
		string tokenS(token.begin(), token.end());
        cout << "Publishing server's CoAP response" << endl;
        //cout << "   token: " << tokenS << "." << endl;
        //cout << "   response: " << body << endl;
		cout << "   observe: " << isObserve << endl;
        cout << "   server address: " << server << endl;
        cout << " Will forward to " <<  pendingResponses[itemId].nodeIdsPerServer[server].size() << " clients using " << chararray_to_hex(itemId) <<  endl;  /* group */
    } 


    ba->publish_data(itemId, DOMAIN_LOCAL, NULL, 0, pendingResponses[itemId].nodeIdsPerServer[server], packet, len);
   
	if (isObserve != 1) { // remove state if the packet isn't observe packet
		pendingResponses[itemId].nodeIdsPerServer.erase(server);
		
		if (pendingResponses[itemId].nodeIdsPerServer.empty()) { // remove state
			//cout << "publishCoapResponseToICN: erasing token" << endl;
			tokenToItemId.erase(token);
		}
	}
}

void CoAPHandler::handleCoapResponseFromICN(std::string itemId, uint8_t *data, int data_len)
{
	std::string proxyURI = itemIdToProxyURI[itemId];
	
	if (debug)
	{
		cout << "Received data from ICN: " <<  endl;
		cout << "   itemId:" << chararray_to_hex(itemId) << endl;
		//cout << "   data:" << data << ":hex: " << chararray_to_hex((char *)data) << endl;
		cout << "   data length: " << data_len << endl;
		//cout << "   proxyURI:" << proxyURI << endl;
	}
	
	CoAP_packet *response_pkt = new CoAP_packet(data, data_len);
        
	int isObserve = response_pkt->isObserve();

	for (vector<uint8_t> v : *itemIdToCTokens[itemId]) {
		// loop through all tokens in the list, and reconstruct CoAP packets
		// then call send_coap_response for each packet
		/*if (debug) {
			string temp(v.begin(), v.end());
			cout << "CNAP: handleCoapResponseFromICN: current token in the token list is: " << temp << "." << endl;
		}*/
		
		response_pkt->setMessageID(rand() % 65535);
		response_pkt->setTokenValue(v.data(), v.size());
		
		send_coap_response(v.data(), v.size(), response_pkt->getCoapPacket(), response_pkt->getpktLen(), isObserve);
	}
}


void CoAPHandler::publishCoapRequestToICN(std::string itemId)
{
	std::string proxyURI 	 = pendingICNPublications[itemId].proxyURI;
    uint8_t *request 		 = pendingICNPublications[itemId].coap_packet;
    int requestLen			 = pendingICNPublications[itemId].packetLen;
	std::string RId			 = sha256_hash(request, requestLen, ID_LEN);
	std::string iSubId       = COAP_SCOPE_ID + RId;
	itemIdToProxyURI[iSubId] = pendingICNPublications[itemId].proxyURI;

	// update token list
	if (itemIdToCTokens[iSubId] == NULL) { // create a new entry
		try {
			globalTokenList->push_back(*(new list<vector<uint8_t>>));
		} catch (bad_alloc& ba) {
			cerr << "Bad alloc exception: " << ba.what() << endl;
			exit(1);
		}
		
		// newest entry is in the back of the list
		itemIdToCTokens[iSubId] = &(globalTokenList->back()); 
	}
	
	itemIdToCTokens[iSubId]->push_back(pendingICNPublications[itemId].token);
	// there is no easy way to search the list, this will remove duplicate tokens
	itemIdToCTokens[iSubId]->unique(); 
	
	if (debug)
	{
		cout << "Will publish CoAP request with Itemid: " << chararray_to_hex(itemId) << endl;
		//cout << "\tproxy URI: " << proxyURI << " request length: " << requestLen << endl;
		cout << "\tWill implicit subscribe to the response (iSubid) " << chararray_to_hex(iSubId) << endl;
		//print_hex_buffer("\tRequest packet to ICN: ", request, requestLen);
	}

	ba->publish_data_isub(itemId, DOMAIN_LOCAL, NULL, 0, iSubId, request, requestLen);
	pendingICNPublications.erase(itemId);    
}

void CoAPHandler::handleCoapRequestFromICN(uint8_t *data, int data_len, std::string nodeId, std::string isubID)
{
	if (debug)
	{
		cout << "Received CoAP request over ICN from " <<  nodeId << 
			" len: " << data_len << endl;
		//cout << "\tdata:  " << data << endl;
		cout << "\tisubID is: " << isubID << "." << endl;
		
		//print_hex_buffer("\tpacket received from ICN: ", data, data_len);
	}

	CoAP_packet *pkt = new CoAP_packet(data, data_len);

	char *host_string;
	int hostLen;

	host_string=(char*)calloc(1, sizeof(char));
	pkt->getHostfromOptions(&host_string, &hostLen);

	std::string host((char*)host_string, hostLen);
	int numCoAPServers = uriToIP[host].size();

	if (debug) {
		cout << "hosts in sNAP side from the packet: "<< host_string << endl;
		cout << "\tnumber of coap servers: "<< numCoAPServers << endl;
	}
	
	//cout << "SNAP: received from ICN, iSubID is: " << chararray_to_hex(isubID) << endl;
	
	// check Item ID - token mapping
	if (itemIdToSToken[isubID].empty()) {
		// generate a new token for communication with the server
		
		string s = generateRandomString(8);
		vector<uint8_t> tokenV(s.c_str(), s.c_str() + s.length());
		itemIdToSToken[isubID] = tokenV;
		pkt->setTokenValue((uint8_t *)s.c_str(), s.length());
		
		if (debug) {
			string token_string((char*)pkt->getToken(), 8);
			string token_string2(tokenV.begin(), tokenV.end());
			cout << "SNAP: token doesn't exist, generating new token" << endl;
			//cout << "\tNew token is: " << token_string << ":" << token_string2 << ". len: " << pkt->getTKL() << endl;
		}
	} else {
		//string temp(itemIdToSToken[isubID].begin(), itemIdToSToken[isubID].end());
		//cout << "temp: " << temp << ". size: " << itemIdToSToken[isubID].size() << endl;

		// update token in the packet based on the stored value
		pkt->setTokenValue(itemIdToSToken[isubID].data(), itemIdToSToken[isubID].size());
	}
	
	// update token - to Item ID mapping 
	tokenToItemId[itemIdToSToken[isubID]] = isubID;
	
	// set message ID in the packet
	pkt->setMessageID(rand() % 65535);
	
	// check for the observe unsubscription (option value = 1)
	if (pkt->isObserve()) {
		if (pkt->observeValue() == 1) {
			// remove itemIdToSToken to token mapping
			if (debug) {
				cout << "handleCoapRequestFromICN: received observe value 1, removing state" << endl;
			}
			itemIdToSToken.erase(isubID);
		}
	}

	// forward the request to all CoAP servers associated with the URI
	// using send_request_to_server()
	for (int x=0; x < numCoAPServers; x++)
	{
		std::string ipv6addr = uriToIP[host][x];
		
		pendingResponses[isubID].nodeIdsPerServer[ipv6addr].push_back(nodeId);
		if (pendingResponses[isubID].nodeIdsPerServer[ipv6addr].size() == 1)
		{     
			cout<< "handleCoapRequestFromICN sending request to: "<<  ipv6addr 
				<< " request len: " << pkt->getpktLen() << endl; 
			
			//print_hex_buffer("\tpacket to be sent to server: ", 
							 //pkt->getCoapPacket(), pkt->getpktLen());

			send_request_to_server(ipv6addr.c_str(), pkt->getCoapPacket(), pkt->getpktLen());
        } else {
			if (debug)
			{
	     		cout << "handleCoapRequestFromICN: A similar request is pending" << endl;
	     		cout << "   I will maintain state for multicast delivery" <<endl;
			}
	    } 
	} /* End for */

	free(host_string);
	delete(pkt);
} 

void CoAPHandler::eventHandler(Event *ev)
{
    switch (ev->type) 
    {
        case PUBLISHED_DATA:
        {
            handleCoapResponseFromICN(ev->id, (uint8_t *)ev->data, ev->data_len);
            break;
        }
        case START_PUBLISH:
        {
            publishCoapRequestToICN(ev->id);
            break;
        }
        case PUBLISHED_DATA_iSUB:
        {
			handleCoapRequestFromICN((uint8_t *)ev->data, ev->data_len, ev->nodeId, ev->isubID);
            break;
        }
        
    }
}

CoAPHandler* CoAPHandler::getInstance(bool debug)
{
    if(!CoAPHandlerInstance)
    {
        CoAPHandlerInstance = new CoAPHandler(debug);
    }
    return CoAPHandlerInstance;
}

/**
 * It will be replaced with code that interacts with the RD
 * DO NOT RELY ON THIS FUNCTION
 *
 **/
void CoAPHandler::registerHostURI(std::string hostURI, std::string IPv6)
{
    uriToIP[hostURI].push_back(IPv6);  /*group communication*/
    std::string icnId = sha256_hash(hostURI, ID_LEN);
    if(adverisedICNIds.find(icnId) != adverisedICNIds.end())
	{
		return;
	} 
    adverisedICNIds.insert(icnId);
    ba->publish_scope(COAP_SCOPE_ID, "", DOMAIN_LOCAL, NULL, 0);
    ba->publish_scope(icnId, COAP_SCOPE_ID, DOMAIN_LOCAL, NULL, 0);
    ba->subscribe_scope(icnId,COAP_SCOPE_ID,DOMAIN_LOCAL, NULL, 0);
    if (debug)
    {
        cout << "Registered: "<< hostURI << " as: " << chararray_to_hex(icnId) << endl;
    }
}


CoAPHandler::CoAPHandler()
{
     CoAPHandler(true);
}

CoAPHandler::CoAPHandler(bool debug)
{
    this->debug             = debug;
    implScopePublished      = false;
    ba                      = NB_Blackadder::Instance(true);
    unsigned char prefix[8] = {0,0,0,0,0,0,0,5}; //note this byte 0 and not '0'
	COAP_SCOPE_ID           = std::string((char*)prefix,8);
    ba->setCallback(_eventHandler);

    // initialize token list
    globalTokenList = new list<list<vector<uint8_t>>>;
    
    if (debug)
    {
        cout <<"CoAPHandler initialized" << endl;
    } 
}

CoAPHandler::~CoAPHandler() {
}

std::string CoAPHandler::sha256_hash(std::string input, int length)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.size());
    SHA256_Final(hash, &sha256);
    std::string shash(reinterpret_cast<char*> (hash), length);
    return shash;
}

std::string CoAPHandler::sha256_hash(uint8_t *input, int input_length, int output_length)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input, input_length);
    SHA256_Final(hash, &sha256);
    std::string shash(reinterpret_cast<char*> (hash), output_length);
    return shash;
}

std::string CoAPHandler::generateRandomString(int length)
{
    char* buffer = new char [length];
    int fd = open("/dev/urandom", O_RDONLY);
    int _b = read(fd, buffer, length);
    std::string result(buffer, length);
    delete[] buffer;
    return result;
}


void CoAPHandler::prepareCoapPacket(CoAP_packet *packet) {
	
	// if the packet type is CONfirmable, set it to NONconfirmable
	// we handle confirmations locally
	if (packet->getType() == CoAP_packet::CoAP_msg_type::COAP_TYPE_CON) {
		packet->setType(CoAP_packet::CoAP_msg_type::COAP_TYPE_NON);
	}
	
	// empty the token and message ID
	packet->setTokenValue(NULL, 0);
	packet->setMessageID(0);
}


// Should be merged with coap_proxy
void CoAPHandler::addClientTokenToList(CoAP_packet *pkt) {

	vector<uint8_t> token(pkt->getToken(), pkt->getToken() + pkt->getTKL());
	
	// create a copy of the packet and remove unnecessary fields to generate iSubId
	CoAP_packet *tmp_pkt = new CoAP_packet(pkt->getCoapPacket(), pkt->getpktLen());
	prepareCoapPacket(tmp_pkt);
	
	std::string RId = sha256_hash(tmp_pkt->getCoapPacket(), tmp_pkt->getpktLen(), ID_LEN);
	std::string iSubId = COAP_SCOPE_ID + RId;
	
	if (debug) {
		string temp(token.begin(), token.end());
		cout << "CoAPHandler::addClientToken: adding new token: " << temp << ". for iSubID: " << iSubId << endl;
	}
	
	if (itemIdToCTokens[iSubId] != NULL) { // add token to the token list
		itemIdToCTokens[iSubId]->push_back(token);	
	} else { // this shouldn't happen
		cout << "CoAPHandler::addClientToken: Error: can't find token list for iSubID: " << iSubId << endl;
	}

	delete tmp_pkt;
}
