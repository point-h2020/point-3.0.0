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



#ifndef CLIENTS_H
#define CLIENTS_H

#include <sys/time.h>
#include <netinet/in.h>


/* 1000 msec = 1 second */
#define KEEP_ALIVE_INTERVAL 5000
#define KEEP_ALIVE_TIMEOUT 15000

#undef BUF_SIZE
#define BUF_SIZE 512

/*!
 * client_node maintains the subscription of active clients
 */

struct client_node {
  int sd;
  struct client_node* next;
  struct sockaddr_storage addr;
  socklen_t addr_len;
  unsigned char *token;
  int tokenLen;
  struct timeval last_seen;
};

/*!
 *
 * @param port listening port for coap-client subscription
 */
int client_socket(in_port_t port);

int server_socket(in_port_t port);

/* 
 * Send response to the client 
 * @param sd socket descriptor to forward the response to coap-client 
 * @param msg coap response packet from the coap-server
 * @param msglen length of coap response packet
 * @param node coap-client to forward the response  
 */
int send_client_message(int sd, const unsigned char* msg,int msglen, const struct client_node* node); 

/*! 
 * Send response to the client 
 * @param sd 
 * @param node
 * @param msg
 * @param len  
 */

void forward_response(unsigned char* coapPacket, 
                 int coapPacketLen, 
                 unsigned char* token, int tokenLen);


/*! 
 * Send response to the client 
 * @param sd 
 * @param node
 * @param msg
 * @param len  
 */

int read_client(int sd, struct client_node** node, char** msg, int* len); 

/*! 
 * Remove client subscription if timer expires
 */
int prune_expired_clients();

/*
 * Print the coap-client information
 * @param node coap-client
 */
void print_client(struct client_node* node);


void init_clients();
void shutdown_clients();

#endif
