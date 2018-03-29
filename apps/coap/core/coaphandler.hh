#ifndef COAPHANDLER_HPP
#define COAPHANDLER_HPP

#include <nb_blackadder.hpp>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <vector>
#include <arpa/inet.h>
#include <boost/functional/hash.hpp>
#include "coap.hpp"

#define ID_LEN 8

using namespace std;

struct pending_response
{
    unordered_map<std::string,std::list<std::string>> nodeIdsPerServer;  /* Group Communication */
    bool observe;
};

struct ICN_publication
{
	std::string SId;
	std::string RId;
	std::string data;
	std::string proxyURI;
	uint8_t *coap_packet;
	vector<uint8_t> token;
    int packetLen;
};

/* Helper function to calcute a hash over a pair of strings, necessary for maps */
struct pair_string_hash {
	size_t operator() (const pair<string, string> &s) const {
		return boost::hash_value(s.first + s.second);
	}
};

/* Helper function to calcute a hash over a vector and a string, necessary for maps */
struct pair_vector_string_hash {
	size_t operator() (const pair<vector<uint8_t>, string> &s) const {
		size_t seed = 0;
		boost::hash_combine(seed, s.first);
		boost::hash_combine(seed, s.second);
		return seed;
	}
};

/* Helper function to calcute a hash over a vector, necessary for maps */
struct vector_hash {
	size_t operator() (const vector<uint8_t> &s) const {
		return boost::hash_value(s);
	}
};

class CoAPHandler
{
    public:
        /* @brief It is invoked by the forward proxy, on the CoAP client 
         * side when a new CoAP request is received. It is implemented by
         * a cNAP
         * @param host the Uri-Host as defined in RFC 7252
         * @param host the Uri-Path as defined in RFC 7252
         * @param token the token used to match requests with responses (null if there is no token)
         * @param coapPacket the whole CoAP packet
         * @param coapPacketLen the size of the CoAP packet
         * @param requestType 0 for GET, 1 for PUT
         * @param isConfirmable true if an ACK is required (default), false otherwise
         * @param query the Uri-Query as defined in RFC 7252
         * @param body the body of the request (e.g., when POST is used)
         * @param bodyLen the length of the request body
         */ 
        void handleCoapRequestFromClient(std::string proxyURI, std::string host, uint8_t *coap_request_packet, int packetLen);

        /* @brief It is invoked by the forward proxy, on the CoAP client 
         * side when a new CoAP request is received. Is is implemented by
         * a sNAP
         * @param token the token used to match requests with responses (null if there is no token)
         * @param body the body of the request (e.g., when POST is used)
         * @param bodyLen the length of the request body
         */ 
        //void publishCoapResponseToICN(std::string server, uint8_t *body, int len);
        void publishCoapResponseToICN(std::string server, uint8_t *body, int len, vector<uint8_t> token, int isObserve);

        void handleCoapResponseFromServer(const struct sockaddr_storage* addr, socklen_t* addrlen, uint8_t* response_packet, int len);
                                
        void eventHandler(Event *ev);
        static CoAPHandler* getInstance(bool debug=true);
	void invokeCoAPHandler (std::string endpoinsFile);
        /**
         * It will be replaced in the future
         * USE WITH CARE
         **/ 
        void registerHostURI(std::string hostURI, std::string IPv6);
		
		/**
		 * Parses the packet and adds token to the itemIdToCTokens list
		 * Should be merged with the coap_proxy client matching functionality
		 */
		void addClientTokenToList(CoAP_packet *pkt);

        //char* getUriToIP(std::string host);
    private:
        NB_Blackadder*                              ba;
        bool                                        debug;
        bool                                        implScopePublished;
        std::string                                 COAP_SCOPE_ID;
        unordered_map<std::string,ICN_publication>  pendingICNPublications;
        unordered_map<std::string,pending_response> pendingResponses;
		
		/* Maps server-side token to ICN Item ID, used by server-side NAP */
		unordered_map<vector<uint8_t>, std::string, vector_hash> tokenToItemId;
		
		/* Maps Item ID to the server-side token, used by server-side NAP */
		unordered_map<std::string, vector<uint8_t>> itemIdToSToken;
		
		/* Maps Item ID to the list of client-side tokens, used by client-side NAP */
		unordered_map<std::string, list<vector<uint8_t>>*> itemIdToCTokens;
		
        unordered_map<std::string,std::string>      itemIdToProxyURI; 
        unordered_set<std::string>                  adverisedICNIds;
        unordered_map<std::string,std::vector<std::string>> uriToIP; 
        static CoAPHandler*                         CoAPHandlerInstance;

		/* List to hold all token lists, tokens may contain NULL characters, 
		 * therefore std::string can't be used */
		list<list<vector<uint8_t>>> *globalTokenList;
        
        CoAPHandler();
        CoAPHandler(bool debug=true);
		~CoAPHandler();
        std::string sha256_hash(std::string input, int length = 8);
		std::string sha256_hash(uint8_t *input, int input_length, int output_length);
        std::string generateRandomString(int length = 8);
        void        publishCoapRequestToICN(std::string itemId);
		void        handleCoapRequestFromICN(uint8_t *data, int data_len, std::string nodeId, std::string isubID);
		void        handleCoapResponseFromICN(std::string itemId, uint8_t *data, int data_len);
		
		/**
		 * @brief Prepares the CoAP packet for transmission over ICN network.
		 * Strips token, sets message ID to 0, and changes packet type from 
		 * CONfirmable to NONconfirmable
		 * @param packet packet to modify
		 */
		void	prepareCoapPacket(CoAP_packet *packet);
};

#endif
