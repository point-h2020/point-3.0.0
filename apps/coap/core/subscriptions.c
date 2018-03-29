#include "subscriptions.h"
#include "coap.hpp"
#include "clients.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static struct coap_subscription* subscriptions_list = NULL;

void print_pkt(const uint8_t *buf, size_t buflen){
     while(buflen--)
		printf("%02X%s", *buf++, (buflen > 0) ? " " : "");
        printf("\n");
}

void forward_subscription(unsigned char* coapPacket, int coapPacketLen, 
                 unsigned char* token, int tokenLen, int isObserve)
{
  struct coap_subscription* subscription;
  
  //log_debug(l, "forward_subscription: token: %s, TL: %d, observe: %d\n", token, tokenLen, isObserve);

  if ((coapPacket == NULL) || (token == NULL))
	  return;
  
  for(subscription = subscriptions_list; subscription != NULL; subscription = subscription->next) {
    // check if the token in the subscription matches with a given token
	if (subscription->tokenLen == tokenLen) {
	  if (memcmp(subscription->token, token, tokenLen) == 0) { 

        // parse the packet, and update the token in the packet just in case
        CoAP_packet *p = new CoAP_packet((uint8_t *)coapPacket, coapPacketLen);

        //printf("matched subscription: old packet length is: %d\n", p->getpktLen());
	    //print_pkt(p->getCoapPacket(), p->getpktLen());
        //p->setTokenValue(subscription->token, subscription->token_len);
        //printf("matched subscription: new packet length is: %d\n", p->getpktLen());

        p->setMessageID(subscription->message_id);

        if(subscription->iteration == 0) 
          p->setType(CoAP_packet::COAP_TYPE_ACK);

        //log_debug(l, "forward_subscription: sending to client, packet is: \n"); 
        //print_pkt(p->getCoapPacket(), p->getpktLen());
        send_client_message(subscription->client->sd, p->getCoapPacket(), p->getpktLen(), subscription->client);
        subscription->iteration=1;
      }
	}
  }
}

void match_subscription(const struct client_node* client, char* uri, int uriLen, unsigned char* token, int tokenLen, int* suppressFlag, int* tokenFlag)
{
  struct coap_subscription* node;

  for( node = subscriptions_list; node != NULL; node = node->next ) {
	if (node->tokenLen == tokenLen) {
		if (memcmp(node->token, token, tokenLen)==0) {
			//log_debug(l, "subscription.c/match_subscription: match token\n");
			*tokenFlag=1;
		}
	}
    if( memcmp((char*)node->resource_uri, uri, uriLen)==0){
      //log_debug(l, "subscription.c/match_subscription: match resource subscription\n");
      *suppressFlag=1;
     }		
  }
}

void add_subscription(struct client_node* client, char* resource_uri, int uriLen, unsigned char* token, int tokenLen, uint16_t msgID)
{
  struct coap_subscription* node = ( struct coap_subscription*)calloc(1, sizeof(struct coap_subscription));
  
  node->client = client;
 
  node->resource_uri=(char*)calloc(uriLen+1,sizeof(unsigned char));
  node->resource_uri[uriLen]='\0';	
  memcpy(node->resource_uri, resource_uri, uriLen);

  node->resource_uri_len=uriLen; 

  node->token=(unsigned char*)calloc(tokenLen+1,sizeof(unsigned char));
  node->token[tokenLen]='\0';	
  memcpy(node->token, token, tokenLen);
  node->tokenLen=tokenLen;
  node->message_id=msgID;
  node->iteration = 0;

 
  if(subscriptions_list == NULL){ 
    subscriptions_list = node;
    subscriptions_list-> next = NULL;
  }else{
    node->next = subscriptions_list;
    subscriptions_list = node;
  }
  
}


void remove_client_subscriptions(const struct client_node* client)
{
  struct coap_subscription* current_node = subscriptions_list, *previous_node = NULL;

  while( current_node != NULL ) {
    if( current_node->client == client ) {
        if (current_node == subscriptions_list ){
           subscriptions_list = current_node->next;
           free(current_node);
        }else{
           previous_node->next = current_node->next;
           free(current_node);
        }
       break;
    }else{
       previous_node = current_node;
       current_node = current_node->next;
    }
      
  }
}

void print_subscriptions(){
  struct coap_subscription* node;

  for( node = subscriptions_list; node != NULL; node = node->next ) {
    printf("Subscriptions: token: %s  resource uri: %s \n", node->token, node->resource_uri);
  }
}
