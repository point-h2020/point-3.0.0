/*
 * coap_proxy.h
 *
 *  Author: Hasan Mahmood Aminul Islam <hasan02.buet@gmail.com>
 *
 * This file is part of Blackadder.
 *
 * Blackadder is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
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


#ifndef COAP_H_
#define  COAP_H_

#include <unistd.h>
#include <stdint.h>


#define COAP_HDR_SIZE           4
#define COAP_PAYLOAD_START      0xFF	  /* payload marker */
#define COAP_DEFAULT_VERSION    1         /* version of CoAP supported */
#define COAP_DEFAULT_SCHEME     "coap"    /* the default scheme for CoAP URIs */


#define COAP_MEDIATYPE_TEXT_PLAIN                     0 	/* text/plain (UTF-8) */
#define COAP_MEDIATYPE_APPLICATION_LINK_FORMAT       40 	/* application/link-format */
#define COAP_MEDIATYPE_APPLICATION_XML               41 	/* application/xml */
#define COAP_MEDIATYPE_APPLICATION_OCTET_STREAM      42 	/* application/octet-stream */
#define COAP_MEDIATYPE_APPLICATION_RDF_XML           43 	/* application/rdf+xml */
#define COAP_MEDIATYPE_APPLICATION_EXI               47 	/* application/exi  */
#define COAP_MEDIATYPE_APPLICATION_JSON              50 	/* application/json  */

 

/*!
 * coap header
 * http://tools.ietf.org/html/rfc7252
 *	     0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |Ver| T |  TKL  |      Code     |          Message ID           |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |   Token (if any, TKL bytes) ...                               |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |   Options (0 or more) ...                                     | 
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |1 1 1 1 1 1 1 1|    Payload (if any) ...                       |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
class CoAP_packet {
	
	public:
	
	       /*!
		* CoAP Options
		*/
		struct CoAP_options {
			uint16_t delta;
			uint16_t optionNumber;
			uint16_t optionValueLength;
			int optionLength;
			uint8_t *optionValue;
		};
		
	       /*!
		* CoAP request URI
		*/		
		struct CoAP_URI{
			char* scheme;
     	                char* host;			/**< host part of the URI */
			char* port;		        /**< The port in host byte order */
			char* path;			/**< Beginning of the first path segment. Use coap_split_path() to create Uri-Path options */
			char* query;			/**< The query part if present */
		};

	       /*!
		* CoAP method codes (GET, POST, PUT, DELETE)
		*/		
		enum CoAP_method_code{
		   COAP_METHOD_GET,
		   COAP_METHOD_POST,
		   COAP_METHOD_PUT,
		   COAP_METHOD_DELETE		 
	    };
		
	       /*!
		* CoAP Message Type (Confirmable, Non-confirmable, Ack, Reset)
		*/		
		enum CoAP_msg_type{
		   COAP_TYPE_CON = 0x00,
		   COAP_TYPE_NON = 0x10,
		   COAP_TYPE_ACK = 0x20,
         	   COAP_TYPE_RST = 0x30
        	};

	       /*!
		* CoAP method codes
		*/		
		enum CoAP_response_code {			
			COAP_CODE_CREATED=0x41,
			COAP_CODE_DELETED,
			COAP_CODE_VALID,
			COAP_CODE_CHANGED,
			COAP_CODE_CONTENT,
			COAP_CODE_BAD_REQUEST=0x80,
			COAP_CODE_UNAUTHORIZED,
			COAP_CODE_BAD_OPTION,
			COAP_CODE_FORBIDDEN,
			COAP_CODE_NOT_FOUND,
			COAP_CODE_METHOD_NOT_ALLOWED,
			COAP_CODE_NOT_ACCEPTABLE,
			COAP_CODE_PRECONDITION_FAILED,
			COAP_CODE_REQUEST_ENTITY_TOO_LARGE,
			COAP_CODE_UNSUPPORTED_CONTENT_FORMAT,
			COAP_CODE_INTERNAL_SERVER_ERROR=0xA0,
			COAP_CODE_NOT_IMPLEMENTED,
			COAP_CODE_BAD_GATEWAY,
			COAP_CODE_SERVICE_UNAVAILABLE,
			COAP_CODE_GATEWAY_TIMEOUT,
			COAP_CODE_PROXYING_NOT_SUPPORTED,
			COAP_CODE_UNDEFINED_CODE=0xFF
		};


	       /*!
		* CoAP Options Registry
		* http://tools.ietf.org/html/rfc7252#section-6.4
		*/		
		enum Option {
			COAP_OPTION_IF_MATCH=1,
			COAP_OPTION_URI_HOST=3,
			COAP_OPTION_ETAG,
			COAP_OPTION_IF_NONE_MATCH,
			COAP_OPTION_OBSERVE,
			COAP_OPTION_URI_PORT,
			COAP_OPTION_LOCATION_PATH,
			COAP_OPTION_URI_PATH=11,
			COAP_OPTION_CONTENT_FORMAT,
			COAP_OPTION_MAX_AGE=14,
			COAP_OPTION_URI_QUERY,
			COAP_OPTION_ACCEPT=17,
			COAP_OPTION_LOCATION_QUERY=20,
			COAP_OPTION_PROXY_URI=35,
			COAP_OPTION_PROXY_SCHEME=39,
			COAP_OPTION_SIZE1=60,
			COAP_OPTION_EXTRA_TOKEN=65000
		};
		
	       /*!
		* CoAP method codes
		*/		
		CoAP_packet(uint8_t *pkt, int pktlen);

	       /*!
		* @Return The version of coap packet
		*/		
		uint8_t getVersion();

	       /*!
		* @version set in coap packet
		* @see coap header
		*/		
		int setVersion(uint8_t version);

	       /*!
		* set
		* @param type in coap packet
		* @see coap header
		*/		
		void setType(CoAP_packet::CoAP_msg_type type);

	       /*!
		* @Return coap message type
   		* @see coap header
		*/		
		CoAP_packet::CoAP_msg_type getType();
	       /*!
		* set 
		* @param tkl token lenght in coap packet
		*/		

		/** tokens **/
		int setTKL(uint8_t tkl);

	       /*!
		* @Return The token length of coap packet
		*/		
		int getTKL();

	       /*!
		* @return reference pointer to the first byte of 
		* the token in the coap packet
		*/		
		uint8_t* getTokenValue();

	       /*!
		* set
		* @param token of 
		* @param tokenLength in coap packet
		*/		
		int setTokenValue(uint8_t *token, uint8_t tokenLength);

	       /*!
		* set 16 bits
		* @param messageID in network byte order in coap packet
		*/
		int setMessageID(uint16_t messageID);

	       /*!
		* @Return the 16 bit message ID.
 		* mesasge ID is stored in network byteorder 
		*/		
		uint16_t getMessageID();

	       /*!
		* @Return parse the coap packet and records the coap options 
		*/		
		CoAP_options* parseOptionsFromRcv();

	       /*!
		* @Return count the number of options of the coap packet parsing the coap packet
		*/		
		int getOptionCountFromRecv();
		

		/* 
		 * @Return The proxy URI of the requested coap packet
		 */
		void getProxyURI(char** proxyURI, int* proxyURILen);
		
	       /*!
		* @Return The resource URI composed from coap options
		*/		
		int getResourceURIfromOptions(char** uri, int* urilen);

	       /*!
		* @Return The Host URI of coap packet
		*/		
		void getHostfromOptions(char** host, int* hostLen);

	       /*!
		* assign the resource URI and length of the URI
		* to path and pathLen respectively
		* @param path The resource URI of coap packet
		* @param pathLen The length of the resource URI
		*/		
		void getPathfromOptions(char** path, int* pathLen);

	       /**< get the private variables */
	       /*!
		* @Return the coap packet
		*/		
		uint8_t* getCoapPacket();

	       /*!
		* @Return the length of coap packet
		*/		
		int getpktLen();

	       /*!
		* @Return 1 if Proxy-URI is found in the received
		*           coap packet else 0 
		*/		
		int isProxy();

		/*!
		* @Return 1 if Proxy-URI is found in the received
		*           coap packet else 0 
		*/		
		int isObserve();


		/*
		 * @return value of the observe option, (default is 0, check isObserve() first)
		 */
		uint32_t observeValue();
		
	       /*!
		* @Return the number of options of the coap packet
		*/		
		int getOptionCount();
		
	       /*!
		* @Return the coap options from the received coap packet
		* @see struct CoAP_options
		*/		
		CoAP_options* getOptions();

	       /*!
		* @Return the method codes of the coap packet
		*/		
		uint8_t getCode();

	       /*!
		* Set 
		* @param responseCode in the coap packet
		*/		
		void setResponseCode(CoAP_response_code responseCode);

	       /*!
		* Set 
		* @param methodCode in the coap packet
		*/		
		void setMethodCode(CoAP_method_code methodCode);

	       /*!
		* @Return the token of the coap packet
		*/		
		unsigned char* getToken();

	       /*!
		* @param t add token in the coap packet
		* @param tokenLen add the tokenLen in the coap packet
		*/		
		int addToken(const unsigned char *t, uint8_t tokenLen);

	       /*!
		* Add
		* @param content add content in the coap packet payload
		* @param length length of the payload
		*/		
		int addPayload(uint8_t *content, int length);

	       /*!
		* parse URI 
		* @param uri
		* @return uri components
		* @see struct CoAP_URI 
		*/		
		CoAP_URI* parse(char* uri);

	       /*!
		* @param uri holds the parsed uri components from __coapuri private var
		*/		
		void setParsedURI(CoAP_URI* uri);

	       /*!
		* @return __coapuri private var
		*/		
		CoAP_URI* getParsedURI();

	       /*!
		* @param opt coap option
		* @param delta the relative value of delta encoding
		* @param optionlength
		*/		
		size_t coap_option_encode(uint8_t *opt, uint16_t delta, uint8_t optionLength);
	       	unsigned int coap_encode_var_bytes(unsigned char *resConFormat, unsigned int val);

	       /*!
		* @param optionType coap option type
		* @see enum Options
		* @param optionValueLength The length of the optionValue
		* @param optionValue 
		*/		
		int insertOption(uint16_t optionType, uint16_t optionValueLength, uint8_t *optionValue);

	       /*!
		* constructor
		*/		
		CoAP_packet();

	       /*!
		* destructor
		*/		
		~CoAP_packet();

	       /*!
		* @param buf coap packet
		* @param the length of the coap packet
		*/		
		void print_coap_packet(const uint8_t *buf, size_t buflen);
		
		void printOption();
		void printHuman();
                void adjust_pkt_len(int value);
		
	private:
		uint8_t *__coappkt; /**<coap packet pointer*/
		int __pktlen;
		int __isProxy;
		int __isObserve;
		uint32_t __observeValue;
		unsigned char *__token; /**<token of the coap packet*/
		int __optionCount;
		uint16_t __maxDelta;	/**<highest option number*/       
		uint8_t* __data; /**<coap packet payload*/
		int __dataLen;
		CoAP_URI* __coapuri;
		CoAP_options* __options; /**<coap options present in the coap packet*/
		
};

#endif
