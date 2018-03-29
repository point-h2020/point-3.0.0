/*
 * hasan02.buet@gmail.com
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>
#include "coap.hpp"

void print(const uint8_t *buf, size_t buflen){
     while(buflen--)
		printf("%02X%s", *buf++, (buflen > 0) ? " " : "");
        printf("\n");
}

CoAP_packet::CoAP_packet() {
	/**<CoAP Packet*/
	__coappkt = (uint8_t*)calloc(4,sizeof(uint8_t));
	__pktlen= 4;
	__optionCount = 0;
	__maxDelta = 0;
	__data = NULL;
	__dataLen = 0;
	__isProxy = 0;
	__isObserve = 0;
	__observeValue = 0;
	__options = NULL;
	setVersion(1);
}

/*!
 * initialize the coap packet from the received packet
 */
CoAP_packet::CoAP_packet(uint8_t *buf, int buflen){

	if(buflen < 4 && buflen!=0) {
		std::cout<< "Packet size should not be less than 4"<< std::endl;
	}
	/**< payload */
	__coappkt = (uint8_t*)calloc(buflen,sizeof(uint8_t));
	memcpy(__coappkt, buf, buflen);
	__pktlen = buflen;
	__maxDelta = 0;
	__isProxy = 0;
	__isObserve = 0;
	__observeValue = 0;
	
	/**< parse the packet from received and set all the parameters*/
	__optionCount=getOptionCountFromRecv();
	__options = parseOptionsFromRcv();
	
}

CoAP_packet::~CoAP_packet() {
	free(__options);
	free(__coappkt);
}

CoAP_packet::CoAP_options* CoAP_packet::parseOptionsFromRcv(){
	
	uint16_t delta = 0, optionNumber = 0, optionValueLength = 0;
	int totalLength = 0, numOptions = 0, optionCount = 0;
	
	if(__optionCount==0)
       		optionCount=getOptionCountFromRecv();
	else
        	optionCount= __optionCount;

    	if(optionCount==0) return NULL;  	

	CoAP_options *options = (CoAP_options*) malloc(__optionCount*sizeof(CoAP_options));
	
	if(options==NULL) return NULL;
        /**< first option follows CoAP mandatory header and Token (if any) */
	int optionPos = COAP_HDR_SIZE + getTKL();
	for(int i=0; i<__optionCount; i++) {
		int extendedDelta=0, extendedLen=0;
		delta= (__coappkt[optionPos] & 0xF0) >> 4;
		
		
	    	if(delta==13) {
			extendedDelta=1;
			delta = __coappkt[optionPos+1] + 13;
		
			
		} else if(delta==14) {
                   /**< Handle two-byte value: First, the MSB + 269 is stored as delta value.
		    * After that, the option pointer is advanced to the LSB which is handled
		    * just like case delta == 13. */
		
			extendedDelta=2;
			delta = ((__coappkt[optionPos+1] << 8) | __coappkt[optionPos+2]) + 269;
		
		} 
		
		optionNumber += delta;
		optionValueLength = (__coappkt[optionPos] & 0x0F);
		
		if(optionValueLength==13) {
			extendedLen=1;
			optionValueLength = __coappkt[extendedDelta+optionPos+1] + 13;
		
		} else if(optionValueLength==14){
			extendedLen=2;
			optionValueLength = ((__coappkt[extendedDelta+optionPos+1] << 8) | __coappkt[optionPos+2]) + 269;
		
		} 		
		totalLength=1+extendedDelta+extendedLen+ optionValueLength;
                /**< got an option and record it*/
		if(optionNumber==COAP_OPTION_PROXY_URI){
		  __isProxy = 1;
		}
		if(optionNumber==COAP_OPTION_OBSERVE){
		  __isObserve = 1;

		  // parse the observe value, it's 3 bytes at most
		  if (optionValueLength == 1) {
			__observeValue = (uint32_t)__coappkt[optionPos+totalLength-optionValueLength];
		  } else if (optionValueLength == 2) {
			__observeValue = (uint32_t)(__coappkt[optionPos+totalLength-optionValueLength]*256
				+ __coappkt[optionPos+totalLength-optionValueLength+1]);
		  } else if (optionValueLength == 3) {
			__observeValue = (uint32_t)(__coappkt[optionPos+totalLength-optionValueLength]*65536
				+ __coappkt[optionPos+totalLength-optionValueLength+1]*256
				+ __coappkt[optionPos+totalLength-optionValueLength+2]);  
		  }
		  //printf("coap.cpp: observe optionValueLength: %d, value: %d\n",optionValueLength, __observeValue);
		}		
		options[i].delta = delta;
		options[i].optionNumber = optionNumber;
		options[i].optionValueLength = optionValueLength;
		options[i].optionLength = totalLength;
		options[i].optionValue = &__coappkt[optionPos+totalLength-optionValueLength];

		if (optionNumber > __maxDelta) { // update __maxDelta to match highest option number
			__maxDelta = optionNumber; 
		}
		//printf("parseOptions: optionNumber: %d, maxdelta: %d\n", optionNumber, __maxDelta);
		
		optionPos += totalLength; 
        numOptions++;
	}
	__optionCount=numOptions;
  
	return options;
}

/*
 * check if Proxy URI found in the coap packet
 */
int CoAP_packet::isProxy(){
  return __isProxy;
}

/*
 * returns the observe-bit
 */
int CoAP_packet::isObserve(){
  return __isObserve;
}

uint32_t CoAP_packet::observeValue(){
  return __observeValue;
}


/*
 * return the hostURI from coap packet
 */
void CoAP_packet::getHostfromOptions(char** host, int* hostLen){

	CoAP_packet::CoAP_options* options = getOptions();
	if(options==NULL) return;
    
    	int len=0;
		
	for(int i=0; i< __optionCount; i++) {
	
		if(options[i].optionNumber == COAP_OPTION_URI_HOST) {
		  len = options[i].optionValueLength;
		  *host=(char*)malloc((len+1)*(sizeof(char)));
		  memcpy(*host, (options+i)->optionValue, len);
		  *(*host + len ) = '\0';
		 	
		}
	}
	*hostLen=len;
}

/*
 * return the Path of the resource from coap packet
 */
void CoAP_packet::getPathfromOptions(char** path, int* pathLen){

	CoAP_packet::CoAP_options* options = getOptions();
	if(options==NULL) return;
    
    	int len=0;
		
	for(int i=0; i< __optionCount; i++) {
	
		if(options[i].optionNumber ==  COAP_OPTION_URI_PATH) {
		  len = options[i].optionValueLength;
		  *path=(char*)malloc((len+1)*(sizeof(char)));
		  memcpy(*path, (options+i)->optionValue, len);
		  *(*path + len ) = '\0';
		 	
		}
	}
	*pathLen=len;
}


void CoAP_packet::getProxyURI(char** proxyURI, int* proxyURILen){

	CoAP_packet::CoAP_options* options = getOptions();
	if(options==NULL) return;
    
    	int urilen=0;
		
	for(int i=0; i< __optionCount; i++) {
	
		if(options[i].optionNumber == COAP_OPTION_PROXY_URI) {
		  urilen = options[i].optionValueLength;
		  *proxyURI=(char*)realloc(*proxyURI, (urilen+1)*(sizeof(char)));
		  memcpy(*proxyURI, (options+i)->optionValue, urilen);
		  *(*proxyURI + urilen) = '\0';
		 	
		}
	}
	*proxyURILen=urilen;
}

int CoAP_packet::getOptionCountFromRecv(){

	uint16_t delta =0, optionNumber = 0, optionValueLength = 0;
	int totalLength = 0;

	
	int optionPos = COAP_HDR_SIZE + getTKL();
    	int parseBytes = __pktlen-optionPos;
	int numOptions = 0;
	
	while(1) {
		
		int extendedDelta=0, extendedLen=0;
		
		if(parseBytes>0) {
			uint8_t optionHeader = __coappkt[optionPos];
			if(optionHeader == 0xFF) { // payload marker found, exit
				//std::cout<< "Payload Starts"<< std::endl;
				__optionCount = numOptions;
				return numOptions;
			}			
		} else{
			 __optionCount=numOptions;
			 return numOptions;
		}	  		
		
		delta= (__coappkt[optionPos] & 0xF0) >> 4;
		
	   	 if(delta==13) {
			extendedDelta=1;
			delta = __coappkt[optionPos+1] + 13;
		
		} else if(delta==14) {
			extendedDelta=2;
			delta = ((__coappkt[optionPos+1] << 8) | __coappkt[optionPos+2]) + 269;

		
		} 
		
		optionNumber += delta;
		optionValueLength = (__coappkt[optionPos] & 0x0F);
		
		
		if(optionValueLength==13) {
			extendedLen=1;
			optionValueLength = __coappkt[extendedDelta+optionPos+1] + 13;
			

		} else if(optionValueLength==14) {
		
			extendedLen=2;
			optionValueLength = ((__coappkt[extendedDelta+optionPos+1] << 8) | __coappkt[optionPos+2]) + 269;

			
		} 
		
		totalLength=1+extendedLen+extendedDelta+ optionValueLength;

		//printf("getOptionCount: found option, number: %d, valueL: %d, totalLength: %d\n", 
			//optionNumber, optionValueLength, totalLength);
		
		parseBytes -= totalLength;
		optionPos += totalLength; 
		numOptions++;
	}
    __optionCount = numOptions;
	
	return numOptions;
}
 
int CoAP_packet::getResourceURIfromOptions(char** resultURI, int* len){

	int tmplen=0, queryflag = 1, urilen=0;
	char separator = '/';
	char resURI[50]="";
	
	
	if(__optionCount==0) {
		*resURI = '\0';
		*len = 0;
		return 0;
	}

	CoAP_packet::CoAP_options* options = getOptions();
	
	if(options==NULL) {
		*resURI = '\0';
		*len = 0;
		return 0;
	}
	
	CoAP_options* temp = NULL;
   	
	
	for(int i=0; i<__optionCount; i++) {
		temp = &options[i];
		tmplen = temp->optionValueLength;
		
	    if(temp->optionNumber==COAP_OPTION_URI_PATH||temp->optionNumber==COAP_OPTION_URI_QUERY)
		{
			if(temp->optionNumber==COAP_OPTION_URI_QUERY){
				if(queryflag) {
					*(resURI+strlen(resURI)-1) = '?';
					queryflag = 0;
				}
				separator = '&';
			}
			urilen += tmplen;
			strncat(resURI, (char*) temp->optionValue,tmplen);
			resURI[urilen++] = separator;			
		}
	}
	
	resURI[urilen] ='\0';
	urilen--;
	*len=urilen;
	*resultURI = (char*)realloc(*resultURI, urilen*(sizeof(char)));
	memcpy(*resultURI, resURI, urilen);
	//printf("Testing, URI is: .%s.%s. length is: %d\n", *resultURI, resURI, urilen);
	return 1;	
}

uint8_t CoAP_packet::getCode() {
	
	//printf("Return the code from the function: %d \n",  __coappkt[1]);
	return __coappkt[1];
}


uint8_t CoAP_packet::getVersion() {
	return (__coappkt[0]&0xC0)>>6;
}


int CoAP_packet::getTKL() {
	return (__coappkt[0] & 0x0F);
}


/*
 * Returns the 16 bit message ID.
 * mesasge ID is stored in network byteorder 
 */ 
uint16_t CoAP_packet::getMessageID() {
	
	uint8_t msb = __coappkt[2];
	uint8_t lsb= __coappkt[3];
	uint16_t messageID = ((uint16_t)msb << 8) | lsb;
	return messageID;
}

/*
 * Print human readable coap options 
 */
CoAP_packet::CoAP_options* CoAP_packet:: getOptions(){
 return __options;
}

/*
 * print options which are found in the coap packet
 */
void CoAP_packet::printOption(){

	CoAP_packet::CoAP_options* options = getOptions();
	if(options==NULL) return;
    
    
		
	for(int i=0; i< __optionCount; i++) {
		std::cout   << " option: " << i+1 << std:: endl
					<< " option number:   " <<  options[i].optionNumber << std:: endl	
					<< " option delta:   " << options[i].delta  << std:: endl
					<< " option name :   "  << std:: endl;
	
		switch(options[i].optionNumber) {
			case COAP_OPTION_IF_MATCH: std::cout   << "IF_MATCH" << std:: endl;
				break;
			case COAP_OPTION_URI_HOST: std::cout   <<"URI_HOST" << std:: endl;
				break;
			case COAP_OPTION_ETAG: std::cout   << "ETAG" << std:: endl;
				break;
			case COAP_OPTION_IF_NONE_MATCH: std::cout   << "IF_NONE_MATCH"<< std:: endl;
				break;
			case COAP_OPTION_OBSERVE: std::cout   << "OBSERVE" << std:: endl;
				break;
			case COAP_OPTION_URI_PORT: std::cout   <<"URI_PORT" << std:: endl;
				break;
			case COAP_OPTION_LOCATION_PATH: std::cout   <<"LOCATION_PATH"<< std:: endl;
				break;
			case COAP_OPTION_URI_PATH: std::cout   <<"URI_PATH"<< std:: endl;
				break;
			case COAP_OPTION_CONTENT_FORMAT: std::cout   <<"CONTENT_FORMAT"<< std:: endl;
				break;
			case COAP_OPTION_MAX_AGE: std::cout   << "MAX_AGE"<< std:: endl;
				break;
			case COAP_OPTION_URI_QUERY: std::cout   << "URI_QUERY"<< std:: endl;
				break;
			case COAP_OPTION_ACCEPT: std::cout   << "ACCEPT"<< std:: endl;
				break;
			case COAP_OPTION_PROXY_URI: std::cout   << "PROXY_URI"<< std:: endl;
				break;
			case COAP_OPTION_PROXY_SCHEME: std::cout   << "PROXY_SCHEME"<< std:: endl;
				break;
			
			default:
				printf("Unknown option");
				break;
		}
		std::cout << "Option lenght: " << options[i].optionValueLength << std::endl;
		printf("Value: ");
		for(int j=0; j<options[i].optionValueLength; j++) {
			char c = options[i].optionValue[j];
			if (options[i].optionNumber == 7) {
				printf("\\%02X ",c);	
				continue;
			}
			
			if(c>='!'&&c<='~') {
				printf("%c",c);
			}
			else printf("\\%02X ",c);			
		}
		printf("\n");
	}
	
}


/*
 * set version in coap packet
 */
int CoAP_packet::setVersion(uint8_t version) {
	if(version>3) return 0;
	__coappkt[0] &= 0x3F;
	__coappkt[0] |= (version << 6);
	return 1;
}

/*
 * get CoAP message type NON, CON, ACK, RESET
 */
CoAP_packet::CoAP_msg_type CoAP_packet::getType() {
	return (CoAP_packet::CoAP_msg_type)(__coappkt[0]&0x30);
}


/*
 * set type (CON, NON, RST, ACK) in coap packet
 */
void CoAP_packet::setType(CoAP_packet::CoAP_msg_type msgType) {
	__coappkt[0] &= 0xCF;
	__coappkt[0] |= msgType;
}

/*
 * set Token lenght in coap packet
 */
int CoAP_packet::setTKL(uint8_t toklen) {
	if(toklen>8) return 1;
	__coappkt[0] &= 0xF0;
	__coappkt[0] |= toklen;
	return 0;
}

/*
 * set message id (network byte order) in coap packet
 */
int CoAP_packet::setMessageID(uint16_t messageID) {
	__coappkt[3] = messageID & 0xFF ;
	__coappkt[2] = (messageID >> 8) & 0xFF;
	
	return 0;
}


/*
 * set response code in coap packet
 */
void CoAP_packet::setResponseCode(CoAP_packet::CoAP_response_code responseCode) {
	__coappkt[1] = responseCode;
}

void CoAP_packet::setMethodCode(CoAP_packet::CoAP_method_code methodCode){
	
	__coappkt[1] = (uint8_t)methodCode;
	//printf("setmethodcode:  %02X \n",__coappkt[1]);
}

/*
 * get Token from coap packet
 */
unsigned char* CoAP_packet::getToken() {
	if(getTKL()==0) return NULL;
	return &__coappkt[4];
}


/*
 * @brief sets token value in the CoAP packet
 * also updates the token lenght and resizes the packet if necessary
 * @return 1 on success, 0 otherwise
 */ 
int CoAP_packet::setTokenValue(uint8_t *token, uint8_t tokenLength) {

	if (tokenLength > 8) // max length is 8
		return 0;
	
	if ((tokenLength > 0) && (token == NULL)) // token is empty, but length is non-zero
		return 0;
	
	// check existing token length
	int oldTokenLength = __coappkt[0] & 0x0F;
	int diff = tokenLength - oldTokenLength;
	
	//printf("setTokenValue() called: old TL: %d, new TL: %d, diff: %d\n", oldTokenLength, tokenLength, diff);
	
	if (diff != 0) { // resize the packet, and move packet data accordingly
		if (diff > 0) { // bigger token, first reallocate space, then move data
			__coappkt = (uint8_t *)realloc(__coappkt, __pktlen + diff);
			memmove((void*)&__coappkt[COAP_HDR_SIZE + tokenLength], 
				(void*)&__coappkt[COAP_HDR_SIZE + oldTokenLength],
				__pktlen - COAP_HDR_SIZE - oldTokenLength);
			
		} else { // diff < 0, first move data, then reallocate space
			memmove((void*)&__coappkt[COAP_HDR_SIZE + tokenLength], 
				(void*)&__coappkt[COAP_HDR_SIZE + oldTokenLength],
				__pktlen - COAP_HDR_SIZE - oldTokenLength);
			__coappkt = (uint8_t *)realloc(__coappkt, __pktlen + diff);
		}

		__pktlen += diff;

		// update the token length field in the packet
		__coappkt[0] &= 0xF0;
		__coappkt[0] |= tokenLength;
	} 
	
	// update the token value
	if (tokenLength > 0)
		memcpy((void *)&__coappkt[COAP_HDR_SIZE], token, tokenLength);
	
	/*printf("setTokenValue: new coap packet is:\n");
	uint8_t *tmp = __coappkt;
	for (int i=0; i < __pktlen; i++) {
		printf("%02x ", (unsigned int) *(tmp+i));
	}
	printf("\n");*/
	
	return 1;
}

void CoAP_packet::adjust_pkt_len(int value){

 __pktlen-=value;

}



/*
 * add payload into coap packet
 */
int CoAP_packet::addPayload(uint8_t *content, int length){
	
	if ((content == NULL) || (length == 0)) {
		return 0;
	}
	
	int i, payloadOffset = -1;
	
	for (i=0; i<__pktlen; i++) {
		if (__coappkt[i] == 0xFF) {
			payloadOffset = i;
			break;
		}
	}
	
	if (payloadOffset == -1) { // no payload marker exists, add it first
		__coappkt=(uint8_t *)realloc(__coappkt, __pktlen + length + 1);
		__coappkt[__pktlen] = COAP_PAYLOAD_START;
		__pktlen += length + 1;
	} else {
		__coappkt=(uint8_t *)realloc(__coappkt, __pktlen + length);
		__pktlen += length;
	}
	
	memcpy(__coappkt + __pktlen - length, content, length);
	return 1;

}

/*
 * return the coap packet that is built 
 */

uint8_t* CoAP_packet::getCoapPacket() {
	return __coappkt;
}

/*
 * parse a given uri  
 */
CoAP_packet::CoAP_URI* CoAP_packet::parse(char* uri){

 
	const char *parser_ptr;
	const char *curstr;
        int len=0;

	/* Allocate space for the URI*/
	CoAP_packet::CoAP_URI *uri_comp = (CoAP_packet::CoAP_URI*)calloc(1, sizeof(CoAP_packet::CoAP_URI));
	if (uri_comp==NULL) {
		return NULL;
	}
	uri_comp->scheme = NULL;
	uri_comp->host = NULL;
	uri_comp->port = NULL;
	uri_comp->path = NULL;
	uri_comp->query = NULL;
    
	curstr = uri;

	//std::cout << curstr << std:: endl;

	/* Scheme */

	parser_ptr = strchr(curstr, ':');

	if ( parser_ptr == NULL) {  

		parser_ptr = strchr(curstr, '/');   
		//return NULL;
	}else{
		len=parser_ptr-curstr;
		uri_comp->scheme = (char*)malloc((len+1)*sizeof(char));
    
		if(uri_comp->scheme == NULL){
			return NULL;
		}
		strncpy (uri_comp->scheme, curstr, len);	
	   *(uri_comp->scheme + len) = '\0';
	    parser_ptr++; 
        curstr=parser_ptr;
 
    }
	
	//debug
	//std::cout << uri_comp->scheme << "  now parser ptr pos:  " << parser_ptr << " and curstr now: "<< curstr << std:: endl;

	/* URI-Host*/
	
	//debug
	//std::cout <<  "curstr now== "<< curstr << std:: endl;


	while(*curstr == '/') 
		curstr++;
	parser_ptr=curstr;

	//debug
	//std::cout << uri_comp->scheme << "  now parser ptr pos:  " << parser_ptr << " and curstr now: "<< curstr << std:: endl;

  
	if(*parser_ptr == '['){
		curstr++;
		while(*parser_ptr == ']') parser_ptr++;
	}else{
	        
		while(*parser_ptr != ':' && *parser_ptr != '/') parser_ptr++;
	}   
	// std::cout<< "IPv4 end" << std::endl;
	len=parser_ptr-curstr;
	uri_comp->host= (char*)malloc((len+1)*sizeof(char));
	strncpy (uri_comp->host, curstr, len);	
	*(uri_comp->host + len) = '\0'; 
  
	//debug
	//std::cout << uri_comp->host  << std:: endl;

	curstr=parser_ptr;
	
	//debug
	//std::cout << uri_comp->scheme << "  now parser ptr pos:  " << parser_ptr << " and curstr now: "<< curstr << std:: endl;

	/*URI-Port*/
	if(*parser_ptr == ':') {
	    curstr=++parser_ptr;
	    
	    while(*parser_ptr != '/') parser_ptr++;

	    len=parser_ptr-curstr;
	    uri_comp->port= (char*)malloc((len+1)*sizeof(char));
	    strncpy (uri_comp->port, curstr, len);	
	    *(uri_comp->port + len) = '\0'; 
	    parser_ptr++;	
        }else{
	     parser_ptr++;
             uri_comp->port= (char*)malloc(sizeof(char));
	     uri_comp->port= NULL;	
        }
 
	
 
	//debug
	//std::cout << " port: "<< uri_comp->port  << std:: endl;

  
	/* URI-Path */

	curstr=parser_ptr;

	//std::cout << "current string: "<< curstr  <<" ...parse pointer: "<< parser_ptr << std:: endl;
	
	while ( *parser_ptr != '\0' && *parser_ptr != '#' && *parser_ptr != '?') {
        parser_ptr++;
	}
	//std::cout << "current string: "<< curstr  <<" ...parse pointer: "<< parser_ptr << std:: endl;
	
	len=parser_ptr-curstr;
	uri_comp->path= (char*)malloc((len+1)*sizeof(char));
	strncpy (uri_comp->path, curstr, len);	
	*(uri_comp->path + len) = '\0'; 
  
	//debug
	//std::cout << uri_comp->path  << std:: endl;

	/* URI-Query */

 	curstr=parser_ptr;
	if ( *parser_ptr != '\0' && *parser_ptr == '?') {      
           parser_ptr++;
	   curstr=parser_ptr;
           while ( *parser_ptr != '\0' && *parser_ptr != '#') parser_ptr++;
	   
          len=parser_ptr-curstr;
	  uri_comp->query= (char*)malloc((len+1)*sizeof(char));
	  strncpy (uri_comp->query, curstr, len);	
	  *(uri_comp->query + len) = '\0'; 
   	}
 
        /*std::cout << " URI Scheme: " << uri_comp->scheme  << std:: endl
 	    << " URI host:   " <<  uri_comp->host  << std:: endl	
 	    << " URI port:   " <<  uri_comp->port  << std:: endl
 	    << " URI path:   " <<  uri_comp->path  << std:: endl
 	    << " URI query:  " <<  uri_comp->query  << std:: endl;   
	*/
	
  
	return uri_comp;

}

/*
void CoAP_packet::setParsedURI(CoAP_packet::CoAP_URI* uri){
	
	__coapuri=(CoAP_packet::CoAP_URI*)malloc(sizeof(CoAP_packet::CoAP_URI));
	
	__coapuri->scheme=uri->scheme;
	__coapuri->host=uri->host;
	__coapuri->port= uri->port;
	__coapuri->path = uri->path;
	__coapuri->query=uri->query;

}*/

/*
 * return the parsed CoAP_URI 
 */

CoAP_packet::CoAP_URI* CoAP_packet::getParsedURI(){
	return __coapuri;
}


size_t CoAP_packet::coap_option_encode(uint8_t *opt, uint16_t delta, uint8_t optionLength){
 
	size_t index=0;
 	
	if (delta < 13) {
		opt[0] = delta << 4;
	} else if (delta < 270) {
		opt[index++] = 0xd0;
		opt[index] = delta - 13;
	} else {
		opt[index++] = 0xe0;
		opt[index++] = ((delta - 269) >> 8) & 0xff;
		opt[index] = (delta - 269) & 0xff;   
	}
    
	if (optionLength < 13) {
		opt[0] |= optionLength & 0x0f;
	} else if (optionLength < 270) {
		opt[0] |= 0x0d;
		opt[++index] = optionLength - 13;
	} else {
		opt[0] |= 0x0e;
		opt[++index] = ((optionLength - 269) >> 8) & 0xff;
		opt[++index] = (optionLength - 269) & 0xff;    
	}
	
	return index+1;
}

/*
 * insert CoAP options into CoAP packet
 * returns 1 on success, 0 on error
 */ 

int CoAP_packet::insertOption(uint16_t optionType, uint16_t optionValueLength, uint8_t *optionValue) {

	size_t optionSize=0;
	uint8_t* opt;
	//int oldlen=__pktlen;
	
	/**Check if options are in the ascending order**/  
	if (optionType < __maxDelta) {
		return 0; 
	} 
	
	opt = (uint8_t *)calloc(5, sizeof(uint8_t *));
	if (opt == NULL)
		return 0;
	
	//opt= __coappkt +__pktlen;
	uint16_t delta = optionType-__maxDelta; 
	//printf("InserOption delta is now: %d, maxdelta: %d\n", delta, __maxDelta);
	optionSize = coap_option_encode(opt, delta, optionValueLength); // size of the option length field
	
	int newOptionOffset = 0;
	
	// Calculate the starting point of the new option, 
	// options starts from COAP_HDR_SIZE + token length 
	newOptionOffset = COAP_HDR_SIZE + getTKL();

	for (int i=0; i < __optionCount; i++) { // loop through all existing options
		//printf("i: %d, optionlength: %d\n", i, __options[i].optionLength);
		newOptionOffset += __options[i].optionLength;
	}
	
	//printf("InsertOption: oType: %d, oSize %ld, oVLength: %d, o_offset %d, oValue: %s\n",
	//	optionType, optionSize, optionValueLength, newOptionOffset, optionValue);
	/*printf("InserOption: encoded option field is: \n");
	uint8_t *tmp = opt;
	while(*tmp) {
		printf("%02x ", (unsigned int) *tmp++);
	}	
	printf("\n");*/

	/*printf("InsertOption: old coap packet is:\n");
	uint8_t *pkt_data = __coappkt;
	while(*pkt_data) {
		printf("%02x ", (unsigned int) *pkt_data++);
	}	
	printf("\n");*/

	
	__coappkt=(uint8_t *)realloc(__coappkt, __pktlen + optionSize + optionValueLength);
	
	// move payload to the end of the packet (including the payload marker)
	memmove((void *)&__coappkt[newOptionOffset + optionSize + optionValueLength], 
			(void *)&__coappkt[newOptionOffset], __pktlen - newOptionOffset);

	// copy optionSize and optionValue to the packet
	memcpy((void*)&__coappkt[newOptionOffset], opt, optionSize);

	if (optionValue != NULL) {
		memcpy((void*)&__coappkt[newOptionOffset + optionSize], optionValue, optionValueLength);
	} else { // length is given but the value is NULL!? zero allocated bytes just in case
		bzero((void*)&__coappkt[newOptionOffset + optionSize], optionValueLength);
	}
	
	// Update the CoAP_options struct
	__options = (CoAP_options*) realloc(__options, (__optionCount + 1)*sizeof(CoAP_options));
	bzero((void *)&__options[__optionCount], sizeof(CoAP_options));
	__options[__optionCount].delta = delta;
	__options[__optionCount].optionNumber = optionType;
	__options[__optionCount].optionValueLength = optionValueLength;
	__options[__optionCount].optionLength = optionSize + optionValueLength;
	__options[__optionCount].optionValue = (uint8_t *)calloc(optionValueLength, sizeof(uint8_t));
	memcpy(__options[__optionCount].optionValue, optionValue, optionValueLength);
	
	__pktlen += optionSize + optionValueLength;
	__optionCount++;
	
	/*printf("InsertOption: new coap packet is:\n");
	pkt_data = __coappkt;
	while(*pkt_data) {
		printf("%02x ", (unsigned int) *pkt_data++);
	}	
	printf("\n");*/
	
	__maxDelta=optionType;
	free(opt);
	return 1;
}

/*
 * Returns the size of Response type in byte 
*/	
unsigned int CoAP_packet::coap_encode_var_bytes(unsigned char *buf, unsigned int resTypeVal) {
	unsigned int byteCount, i;

	for (byteCount = 0, i = resTypeVal; i && byteCount < sizeof(resTypeVal); ++byteCount)
		i >>= 8;
	i = byteCount;
	while (i>0) {
		buf[i--] = resTypeVal & 0xff;
		resTypeVal >>= 8;
	}

	return byteCount;
}

/*
 *  Returns the length of the PDU.
 */ 
int CoAP_packet::getpktLen() {
	return __pktlen;
}
/*
 * get number of options of the coap packet
 */

int CoAP_packet::getOptionCount() {
	return __optionCount;
}


void CoAP_packet::print_coap_packet(const uint8_t *buf, size_t buflen)
{
    while(buflen--)
		printf("%02X%s", *buf++, (buflen > 0) ? " " : "");
            //printf("%c%s", (unsigned char) *buf++, (buflen > 0) ? " " : "");
        printf("\n");
	
    std::cout<< "Printing coap packet " << std:: endl;	
	std::cout<< "Version: " << (int) getVersion() << std:: endl;
	std::cout<< "Type: " << getType() << std:: endl;
	std::cout<< "Code: " << (uint8_t) getCode() << std:: endl;
	std::cout<< "TKL: " << getTKL() << std:: endl;
	std::cout<< "ID: " << getMessageID() << std:: endl;
	
	
		
	std::cout<< "Option: " << getOptionCount() << std:: endl;

}

/// Prints the PDU in human-readable format.
void CoAP_packet::printHuman() {
	printf("__________________ \n");
	
	printf("PDU is %d bytes long \n", __pktlen);
	printf("CoAP Version: %d \n",getVersion());
	printf("Message Type: ");
	switch(getType()) {
		case COAP_TYPE_CON:
			printf("Confirmable");
		break;

		case COAP_TYPE_NON:
			printf("Non-Confirmable");
		break;

		case COAP_TYPE_ACK:
			printf("Acknowledgement");
		break;

		case COAP_TYPE_RST:
			printf("Reset");
		break;
	}
	printf("Token length: %d \n",getTKL());
	printf("Code: ");


	switch(getCode()) {
	
		case COAP_METHOD_GET:
			printf("0.01 GET");
		break;
		case COAP_METHOD_POST:
			printf("0.02 POST");
		break;
		case COAP_METHOD_PUT:
			printf("0.03 PUT");
		break;
		case COAP_METHOD_DELETE:
			printf("0.04 DELETE");
		break;
		case COAP_CODE_CREATED:
			printf("2.01 Created");
		break;
		case COAP_CODE_DELETED:
			printf("2.02 Deleted");
		break;
		case COAP_CODE_VALID:
			printf("2.03 Valid");
		break;
		case COAP_CODE_CHANGED:
		__optionCount=getOptionCountFromRecv();
	__options = parseOptionsFromRcv();	printf("2.04 Changed");
		break;
		case COAP_CODE_CONTENT:
			printf("2.05 Content");
		break;
		case COAP_CODE_BAD_REQUEST:
			printf("4.00 Bad Request");
		break;
		case COAP_CODE_UNAUTHORIZED:
			printf("4.01 Unauthorized");
		break;
		case COAP_CODE_BAD_OPTION:
			printf("4.02 Bad Option");
		break;
		case COAP_CODE_FORBIDDEN:
			printf("4.03 Forbidden");
		break;
		case COAP_CODE_NOT_FOUND:
			printf("4.04 Not Found");
		break;
		case COAP_CODE_METHOD_NOT_ALLOWED:
			printf("4.05 Method Not Allowed0");
		break;
		case COAP_CODE_NOT_ACCEPTABLE:
			printf("4.06 Not Acceptable");
		break;
		case COAP_CODE_PRECONDITION_FAILED:
			printf("4.12 Precondition Failed");
		break;
		case COAP_CODE_REQUEST_ENTITY_TOO_LARGE:
			printf("4.13 Request Entity Too Large");
		break;
		case COAP_CODE_UNSUPPORTED_CONTENT_FORMAT:
			printf("4.15 Unsupported Content-Format");
		break;
		case COAP_CODE_INTERNAL_SERVER_ERROR:
			printf("5.00 Internal Server Error");
		break;
		case COAP_CODE_NOT_IMPLEMENTED:
			printf("5.01 Not Implemented");
		break;
		case COAP_CODE_BAD_GATEWAY:
			printf("5.02 Bad Gateway");
		break;
		case COAP_CODE_SERVICE_UNAVAILABLE:
			printf("5.03 Service Unavailable");
		break;
		case COAP_CODE_GATEWAY_TIMEOUT:
			printf("5.04 Gateway Timeout");
		break;
		case COAP_CODE_PROXYING_NOT_SUPPORTED:
			printf("5.05 Proxying Not Supported");
		break;
		case COAP_CODE_UNDEFINED_CODE:
			printf("Undefined Code");
		break;
	}
        printf("\n");
	// print message ID
	printf("Message ID: %u \n",getMessageID());

	// print token value
	int tokenLength = getTKL();
	uint8_t *tokenPointer = __coappkt+COAP_HDR_SIZE;
	if(tokenLength==0) {
		printf("No token.");
	} else {
		printf("Token of %d bytes. \n ",tokenLength);
		printf("   Value: 0x");
		for(int j=0; j<tokenLength; j++) {
			printf("%.2x",tokenPointer[j]);
		}
		printf(" ");
	}
	printf("\n");
	// print options
	__optionCount=getOptionCountFromRecv();
	__options = parseOptionsFromRcv();
	CoAP_packet::CoAP_options* options = __options;
	if(options==NULL) {
		return;
	}


	printf("%d options:",__optionCount);
	for(int i=0; i<__optionCount; i++) {
		printf("\nOPTION (%d/%d) \n",i + 1,__optionCount);
		printf("Option number (delta): %hu (%hu) \n",options[i].optionNumber,options[i].delta);
		printf("   Name: ");
		switch(options[i].optionNumber) {
			case COAP_OPTION_IF_MATCH:
				printf("IF_MATCH");
			break;
			case COAP_OPTION_URI_HOST:
				printf("URI_HOST");
			break;
			case COAP_OPTION_ETAG:
				printf("ETAG");
			break;
			case COAP_OPTION_IF_NONE_MATCH:
				printf("IF_NONE_MATCH");
			break;
			case COAP_OPTION_OBSERVE:
				printf("OBSERVE");
			break;
			case COAP_OPTION_URI_PORT:
				printf("URI_PORT");
			break;
			case COAP_OPTION_LOCATION_PATH:
				printf("LOCATION_PATH");
			break;
			case COAP_OPTION_URI_PATH:
				printf("URI_PATH");
			break;
			case COAP_OPTION_CONTENT_FORMAT:
				printf("CONTENT_FORMAT");
			break;
			case COAP_OPTION_MAX_AGE:
				printf("MAX_AGE");
			break;
			case COAP_OPTION_URI_QUERY:
				printf("URI_QUERY");
			break;
			case COAP_OPTION_ACCEPT:
				printf("ACCEPT");
			break;
			case COAP_OPTION_LOCATION_QUERY:
				printf("LOCATION_QUERY");
			break;
			case COAP_OPTION_PROXY_URI:
				printf("PROXY_URI");
			break;
			case COAP_OPTION_PROXY_SCHEME:
				printf("PROXY_SCHEME");
			break;
			
			case COAP_OPTION_SIZE1:
				printf("SIZE1");
			break;
			
			default:
				printf("Unknown option");
			break;
		}
  		printf("\n");
		printf("   Value length: %u \n",options[i].optionValueLength);
		printf("   Value: \"");
		for(int j=0; j<options[i].optionValueLength; j++) {
			char c = options[i].optionValue[j];
			if((c>='!'&&c<='~')||c==' ') {
				printf("%c",c);
			} else {
				printf("\\%.2d",c);
			}
		}
		printf("\"");
	}
	printf("\n");
	/*
	if(_payloadLength==0) {
		printf("No payload.");
	} else {
		printf("Payload of %d bytes",_payloadLength);
		printf("   Value: \"");
		for(int j=0; j<_payloadLength; j++) {
			char c = _payloadPointer[j];
			if((c>='!'&&c<='~')||c==' ') {
				printf("%c",c);
			} else {
				printf("\\%.2x",c);
			}
		}
		printf("\"");
	}*/
	free(options);
	printf("__________________"); 
}
