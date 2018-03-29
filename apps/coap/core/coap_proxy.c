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

#include "coap_proxy.h"
#include "subscriptions.h"
#include "coaphandler.hh"
#include <arpa/inet.h>
#include <net/if.h>

static struct logger* l=NULL;
static int counter = 0;
static struct suppressed_token* token_list = NULL;


void print_token_list(){
  struct suppressed_token* node;

  for( node = token_list; node != NULL; node = node->next ) {
    printf("token: %s \n", node->token);
  }
}

void add_token(unsigned char* token, int tokenLen,  uint16_t msgID)
{
  struct suppressed_token* temp = (struct suppressed_token *)calloc(1, sizeof(struct suppressed_token));
  
  /*printf("suppressed token \n");*/
 
  temp->token=(unsigned char*)calloc(tokenLen+1, sizeof(unsigned char));
  temp->token[tokenLen]='\0';	
  memcpy(temp->token, token, tokenLen);

  temp->token_len=tokenLen;
  temp->message_id= msgID;
  temp->next = token_list;
  token_list = temp;  
}

int match_token(unsigned char* token, int tokenLen,  uint16_t msgID)
{
  struct suppressed_token* temp;

  for( temp = token_list; temp != NULL; temp = temp->next ) {
    printf("\n match suppressed token_list: %u : %u  \n", temp->message_id, msgID);
    if((temp->message_id) == msgID){
      printf("match suppressed token  \n");
      return 1;
     }	
  }
 return 0;
}


void init_coap_proxy(){
   l = init_logger(stdout, stderr, stderr);
}

void shutdown_coap_proxy()
{
  shutdown_logger(l);
}



void print_coap_packet_details(char *msg ,int len){

  CoAP_packet *p = new CoAP_packet((uint8_t *)msg,len);
  p->print_coap_packet((const uint8_t*)msg, len);
}


void wait( int seconds )
{  
    #ifdef _WIN32
        Sleep( 1000 * seconds );
    #else
        sleep( seconds );
    #endif
}

/* merge */

int send_request_to_server(char const* ipv6addr, uint8_t* msg, int len){
  int  n, hostLen=-1, dst_len;
  struct sockaddr_in6 dst_addr;
  int portno = 5683;
  char* host;

  CoAP_packet *pkt = new CoAP_packet((uint8_t *)msg,len);

  host=(char*)calloc(1, sizeof(char));
  pkt->getHostfromOptions(&host, &hostLen);
  host[len-1]='\0';

  /*Destination address of the coap server */
  memset(&dst_addr,0,sizeof(dst_addr));

  inet_pton(AF_INET6, ipv6addr, &dst_addr.sin6_addr);
  dst_addr.sin6_port = htons(portno);
  dst_len = sizeof(dst_addr);
  dst_addr.sin6_family = AF_INET6;

  n = sendto(server_sd, msg, len, 0, (struct sockaddr*)&dst_addr , dst_len);  
 
  if (n < 0) 
     log_error(l,"ERROR in sendto");

  return n;
}



int handle_coap_request(struct client_node* node, int client_sd,  char *msg, int len)
{ 
  unsigned char *token;  
  int tokenLen=0;
  int hostLen=0, pathLen=0, urilen=0, observeFlag=0, suppressFlag=0, tokenFlag=0;
  /**< __coappkt from the IP endpoint (client)*/
  CoAP_packet *pkt = new CoAP_packet((uint8_t *)msg,len);
  CoAPHandler* handler = CoAPHandler::getInstance(true);

  // CONfirmable messages should be confirmed immediately by the proxy
  if (pkt->getType() == CoAP_packet::CoAP_msg_type::COAP_TYPE_CON) {
	  log_debug(l, "handle_coap_request: CON message received from client, replying with an empty ACK immediately\n");
	  
	  // construct an empty ACK packet with a matching MessageID
	  CoAP_packet *ack_pkt = new CoAP_packet();
	  ack_pkt->setType(CoAP_packet::CoAP_msg_type::COAP_TYPE_ACK);
	  ack_pkt->setMessageID(pkt->getMessageID());

	  sendto(node->sd, ack_pkt->getCoapPacket(), ack_pkt->getpktLen(), 0, (struct sockaddr*)&node->addr, node->addr_len);
	  delete ack_pkt;
  }
   
  /**< Check if Packet contains  Proxy option*/
  if((pkt->isProxy()) == 1){
      char * proxy_uri;
      int purilen=0;
      proxy_uri=(char*)malloc(sizeof(char));
      pkt->getProxyURI(&proxy_uri,&purilen);	  
      CoAP_packet::CoAP_URI* tmp= pkt->parse(proxy_uri);
      CoAP_packet *coap_request_from_puri = new CoAP_packet();
      coap_request_from_puri->setType((CoAP_packet::CoAP_msg_type)pkt->getType());
      coap_request_from_puri->setMethodCode((CoAP_packet::CoAP_method_code)pkt->getCode());	
      coap_request_from_puri->setMessageID(pkt->getMessageID());

      tokenLen=pkt->getTKL();
      token=(unsigned char*)calloc(tokenLen+1, sizeof(unsigned char));
      memcpy(token, pkt->getToken(), tokenLen);
      token[tokenLen]='\0';
      
      if(tokenLen !=0){ 
        coap_request_from_puri->setTokenValue(token, tokenLen);	
      }   
      if(tmp->host!=NULL){
         coap_request_from_puri->insertOption(CoAP_packet::COAP_OPTION_URI_HOST, (uint16_t) strlen(tmp->host), (uint8_t*) tmp->host);	
      }

      /*
       * proxy is set and now check if observe is set.
       * if set insert observe option in the packet
       * create or do appropriate actions on the subscription list
       *
       * insert option sequentially according to option registry 
       */
      observeFlag = pkt->isObserve();	
      if(observeFlag == 1) {
		coap_request_from_puri->insertOption(CoAP_packet::COAP_OPTION_OBSERVE, 0, NULL);
		match_subscription(node, proxy_uri, purilen, token, tokenLen, &suppressFlag, &tokenFlag);
        
		// add subscription even if the token is known, different clients may have same tokens
		add_subscription(node, proxy_uri, purilen, token, tokenLen, coap_request_from_puri->getMessageID()); 
        //print_subscriptions();
      }

      if (tmp->port!=NULL) {
		unsigned char portbuf[2];
         int port= strtol(tmp->port, NULL, 10);
         coap_request_from_puri->insertOption(CoAP_packet::COAP_OPTION_URI_PORT, coap_request_from_puri->coap_encode_var_bytes(portbuf, port), portbuf);
      }
      
      if (tmp->path!=NULL) {
        char *parser_ptr;
		char *curstr;      
        int sLen=0;
        curstr = tmp->path;
		
        do{
          sLen = 0; 
          parser_ptr = strchr(curstr, '/');
          sLen= parser_ptr-curstr;
          if(sLen>0)
          { 
            coap_request_from_puri->insertOption(CoAP_packet::COAP_OPTION_URI_PATH, (uint16_t) sLen, (uint8_t*)curstr);
			parser_ptr++;	
            curstr=parser_ptr;            
		}else{
            if(curstr!='\0') coap_request_from_puri->insertOption(CoAP_packet::COAP_OPTION_URI_PATH, (uint16_t) strlen(curstr), (uint8_t*)curstr);
          }        
	 
        }while(parser_ptr!='\0');   

         /*
          printf("uri path split completed. \n"); 
          coap_request_from_puri->insertOption(CoAP_packet::COAP_OPTION_URI_PATH, (uint16_t) strlen(tmp->path), (uint8_t*)tmp->path);
         */
      } 
      
      if (tmp->query!=NULL) {
         coap_request_from_puri->insertOption(CoAP_packet::COAP_OPTION_URI_QUERY, (uint16_t) strlen(tmp->query), (uint8_t*)tmp->query);	
      }
            
      // Copy the payload from the original packet to the modified packet
      // FIXME: this should be merged to coap.cpp
	  uint8_t *orgPacketData = pkt->getCoapPacket();
	  int i, found = -1;
	  for (i=0; i<pkt->getpktLen(); i++) {
		if (orgPacketData[i] == 0xFF) {
			//printf("Payload found in the packet\n");
			found = i;
			break;
		}
	  }
	  
	  if (found >= 0) { // payload marker takes 1 byte, therefore +1, -1
		coap_request_from_puri->addPayload(orgPacketData + found + 1, pkt->getpktLen() - found - 1);
	  }
      
      uint8_t *request = coap_request_from_puri->getCoapPacket();
      int rlen = coap_request_from_puri->getpktLen();
      
      //log_debug(l, "coap_proxy.c/handle_coap_request: suppressFlag: %d   observeflag:  %d   TokenFlag:  %d  \n", 
	  //	suppressFlag, observeFlag, tokenFlag);     
	  //print_hex_buffer("\tpacket is: ", request, rlen);
	  if (suppressFlag) {
		  add_token(token, tokenLen, coap_request_from_puri->getMessageID());
		  if (tokenFlag == 0) { // previously unknown token should be added to the list
			handler->addClientTokenToList(coap_request_from_puri); // should be merged...
		  }
	  }
      if (suppressFlag!=1 && observeFlag && !tokenFlag)      
		handler->handleCoapRequestFromClient(proxy_uri, tmp->host, request, rlen); /* coaphandler.cc*/    

	  if (!observeFlag)
		handler->handleCoapRequestFromClient(proxy_uri, tmp->host, request, rlen);
      
      delete(pkt);
      delete(coap_request_from_puri);
      free(proxy_uri);
	  free(token);
      free(tmp->scheme);
      free(tmp->host);
      free(tmp->port);
      free(tmp->path);
      free(tmp->query);	
      free(tmp);
      
  }else{
      /**< -P option is not set, forward the request to reverse proxy  */
      char* ruri;
      char* host;
      char* path; 
      ruri=(char*)malloc(sizeof(char));
      pkt-> getResourceURIfromOptions(&ruri, &urilen);
      
	
      host=(char*)calloc(1, sizeof(char));
      pkt->getHostfromOptions(&host, &hostLen);
	
      path=(char*)calloc(1,sizeof(char));
      pkt->getPathfromOptions(&path, &pathLen);
   
	/**<Token*/
      CoAP_packet *pkt=new CoAP_packet((uint8_t *)msg,len);
      tokenLen=pkt->getTKL();
      token=(unsigned char*)malloc((tokenLen+1)*sizeof(unsigned char));
      memcpy(token, pkt->getToken(), tokenLen);
      token[tokenLen]='\0';
      //printf("counter:  %d \n", counter); 
      print_token_list();	

      if(match_token(token, tokenLen, pkt->getMessageID()) == 0){ 	
         /*FIXME WITH coaphandler function. */
         
      }
      free(token);
      free(ruri);
      free(host);
      free(path);
   }
 
  return 0;
}

int handle_coap_response() {

  uint8_t buf[BUF_SIZE + 1];
  size_t recvlen;
  struct sockaddr_storage from;
  socklen_t fromlen = sizeof(struct sockaddr_in6); // FIXME: IPv6 dependency
  bzero(&from, fromlen);
  
  recvlen = recvfrom(server_sd, buf, BUF_SIZE, 0, (struct sockaddr*)&from, &fromlen);
  if (recvlen == 0) {
      log_error(l, "Connection is closed.\n");
      return -1;
   }
  else if (recvlen < 0) {
      log_error(l, "Recvfrom failed.\n");
      return -1;
   }
  log_debug(l, "\nReceived %u bytes from server\n", recvlen);
  buf[recvlen] = '\0';
  
  // parse packet to check if it's CONfirmable
  CoAP_packet *pkt = new CoAP_packet((uint8_t *)buf, recvlen);

  // CONfirmable messages should be confirmed immediately by the proxy
  if (pkt->getType() == CoAP_packet::CoAP_msg_type::COAP_TYPE_CON) {
	log_debug(l, "handle_coap_response: CON message received from server, replying with an empty ACK immediately\n");
	  
	// construct an empty ACK packet with a matching MessageID
	CoAP_packet *ack_pkt = new CoAP_packet();
	ack_pkt->setType(CoAP_packet::CoAP_msg_type::COAP_TYPE_ACK);
	ack_pkt->setMessageID(pkt->getMessageID());
  
	sendto(server_sd, ack_pkt->getCoapPacket(), ack_pkt->getpktLen(), 0, (struct sockaddr *)&from, fromlen);
	delete ack_pkt;
  }
	
  CoAPHandler* handler = CoAPHandler::getInstance(true);
  handler->handleCoapResponseFromServer(&from, &fromlen, buf,recvlen);
  
  return 1;
}

void send_coap_response( unsigned char* token,
                         int tokenLen,
                         unsigned char* coapPacket,
                         int coapPacketLen, 
			 int isObserve)
{

  /*printf("isObserve: %d \n", isObserve);*/
  if(isObserve == 1) 
	forward_subscription (coapPacket, coapPacketLen, token, tokenLen, isObserve);
  else
        forward_response(coapPacket, coapPacketLen, token, tokenLen);

}

