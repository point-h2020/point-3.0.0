/*
 * hasan02.buet@gmail.com
 */

#ifndef SUBSCRIPTIONS_H
#define SUBSCRIPTIONS_H


#include "clients.h"
#include <stdbool.h>
//#include <sys/time.h>



/* Holds information about a subscription
 * look-up subscription using TOKEN 
 * (Token is the only information found in the response packet)  
 */
 struct coap_subscription{
     char* resource_uri;
     int resource_uri_len;
     unsigned char* token;
     int tokenLen;
     int iteration;
     uint16_t message_id;
     struct coap_subscription* next;
     struct client_node* client;
   };

 void add_subscription(struct client_node* client, char* resource_uri,int uri_len, unsigned char* token, int tokenLen,  uint16_t msgID);
 
 /* Remove subscriptions of the client from the list */
 void remove_client_subscriptions(const struct client_node* client);

 void match_subscription(const struct client_node* client, char* resource_uri, int uriLen, unsigned char* token, int tokenLen, int* suppressFlag, int* tokenFlag);

 
 void print_subscriptions();

 void coap_subscription_init(coap_subscription *);

 void forward_subscription(unsigned char* coapPacket, 
                 int coapPacketLen, 
                 unsigned char* token, int tokenLen, int isObserve);

#endif
