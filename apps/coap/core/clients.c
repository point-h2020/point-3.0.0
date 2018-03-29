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


#include "clients.h"
#include "subscriptions.h"

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "utils.h"
#include "logger.h"
#include "coap.hpp"

static struct client_node* clients_list = NULL;

static struct logger* l = NULL;
void clear_clients_list();
static const struct timeval tv_keep_alive = {KEEP_ALIVE_TIMEOUT/1000, KEEP_ALIVE_TIMEOUT%1000};

void init_clients()
{
  l = init_logger(stdout, stderr, stderr);
}

void shutdown_clients()
{
  clear_clients_list();
  shutdown_logger(l);
}

void clear_clients_list()
{
  struct client_node* node = clients_list, *tmp = NULL;

  while( node != NULL ) {
    tmp = node->next;
   /**< remove_client_subscriptions(node); */
    free(node->token);
    free(node);
    node = tmp;
  }
}

void remove_client( struct client_node* client, unsigned char* token, int tokenLen){

  struct client_node *current_node, *previous_node;

  current_node = clients_list;
  previous_node = NULL;
  while( current_node != NULL )
  {
	  if (current_node->tokenLen == tokenLen) {
		if( memcmp(current_node->token, token, tokenLen)==0 ) {
          if(current_node == clients_list){               
             //print_client(current_node); 
             clients_list=current_node->next;
             free(current_node); 
           } else{
              previous_node->next = current_node->next;
              free(current_node);
           }
			break; 
		} 
		  
	} else{
          previous_node = current_node;
          current_node = current_node->next;
      }

 }
}


void forward_response(unsigned char* coapPacket, 
                 int coapPacketLen, 
                 unsigned char* token, int tokenLen)
{
  struct client_node *node;
  node = clients_list;
  
  while(node != NULL)
  {
	if (node->tokenLen == tokenLen) {
		if(memcmp(node->token, token, tokenLen)== 0) 
			break;
	}
    node = node->next;
  }
  
  //log_debug(l,"clients.c/forward_response, len: %d\n", coapPacketLen);
  
  if (node != NULL) {
	send_client_message(node->sd, coapPacket, coapPacketLen, node);
	remove_client(node, token, tokenLen);
  }
}



/**< Send response to the client */
int send_client_message(int sd, const unsigned char* msg, int msglen, const struct client_node* node){

  return sendto(sd, msg, msglen, 0, (struct sockaddr*)&node->addr, node->addr_len);
}

void add_client(const struct sockaddr_storage* addr, socklen_t len, int sd, unsigned char* token, int tokenLen)
{
  struct client_node* node;

  node =(client_node *) malloc(sizeof(struct client_node));
  memset(&node->addr, 0, sizeof(struct sockaddr_storage));
  memcpy(&node->addr, addr, len);
  node->addr_len = len;
  node->sd=sd;
  node->token=(unsigned char*)calloc(tokenLen+1,sizeof(unsigned char));	
  memcpy(node->token, token, tokenLen);
  node->tokenLen = tokenLen;
  node->token[tokenLen]='\0'; 
  
  if(clients_list == NULL){
     clients_list = node;
     clients_list -> next = NULL;
  }else{
     node->next=clients_list;
     clients_list = node;
  }
  gettimeofday(&node->last_seen, 0);

}


/* parse a packet from clients */
int read_client(int sd, struct client_node** ret_node, char** msg,  int* len)
{
  
  char buf[BUF_SIZE + 1];
  size_t recvlen;
  struct sockaddr_storage from;
  struct client_node* node;
  socklen_t fromlen = sizeof(struct sockaddr);
  int tokenLen=0;
  unsigned char* token;
  int flag = 0;
  
  recvlen = recvfrom(sd, buf, BUF_SIZE, 0, (struct sockaddr*)&from, &fromlen);
  if (recvlen == 0) {
      log_error(l, "Connection is closed.\n");
      return -1;
   }
  else if (recvlen < 0) {
      log_error(l, "Recvfrom failed.\n");
      return -1;
   }
  log_debug(l, "\nReceived %u bytes from client\n", recvlen);
  buf[recvlen] = '\0';
  
  //print_detail((uint8_t*)buf, recvlen);

  *len=recvlen;
  *msg=(char *)malloc((recvlen+1)* sizeof(char));
  memset(*msg, 0, recvlen+1);
  memcpy(*msg, buf, recvlen+1);

  CoAP_packet *p = new CoAP_packet((uint8_t *)buf,recvlen);


  tokenLen=p->getTKL();
  token=(unsigned char*)calloc(tokenLen+1, sizeof(unsigned char));

  memcpy(token, p->getToken(), tokenLen);
  token[tokenLen]='\0';

  for( node = clients_list; node != NULL; node = node->next )
  {
    if( compare_sockaddr( &node->addr, &from ) == 0 ) break;
  }
  /**< New client, add to the list */
  if( node == NULL ) {
    //log_debug(l, "New client, adding to the list of clients.\n");
    add_client(&from, fromlen, sd, token, tokenLen);
    node = clients_list;
  }else flag = 1;
  
  free(token);
  delete(p);
  
  if(flag == 1) return -1;

  *ret_node = node;
  
  return 0;
}


int prune_expired_clients()
{
  struct client_node *node, *prev;
  struct timeval tv, tvelapsed, tvremain;
  int pruned = 0;

  gettimeofday(&tv, 0);
  
  node = clients_list;
  prev = NULL;
  while( node != NULL )
  {
      if( !timeval_subtract( &tvelapsed, &tv, &node->last_seen ) ) {
      /**< if timeout - elapsed is negative, delete the client */
      if( timeval_subtract( &tvremain, &tv_keep_alive, &tvelapsed) ) {
        log_debug(l, "Client time out, deleting.\n");
        if( prev ) prev->next = node->next;
        else clients_list = node->next;
        free(node);
        pruned++;
        if( prev ) node = prev->next; else node = clients_list;
        if( node == NULL ) break; else continue;
      }
    }
    prev = node;
    node = node->next;
  }
  return pruned;
}

void print_client(struct client_node* node)
{
  log_print(l, "Last seen at %d.%d\n", node->last_seen.tv_sec, node->last_seen.tv_usec);
  log_print(l, "this client has token %s \n", node->token);
  log_print(l, "lets store the client socket des %d \n", node->sd);
}
