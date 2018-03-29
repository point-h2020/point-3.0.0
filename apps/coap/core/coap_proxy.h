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



#ifndef COAP_PROXY_H
#define COAP_PROXY_H

#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <iostream>
#include <signal.h>

#include "utils.h"
#include "clients.h"
#include "logger.h"
#include "coap.hpp"
#include "coaphandler.hh"


#define HOSTNAME_SIZE 255

extern int server_sd;

struct suppressed_token {
  unsigned char * token;
  int token_len;
  uint16_t message_id;
  struct suppressed_token* next;
  struct sockaddr_storage addr;
};

/*!
 * invoked by main function upon reception a request from coap client. 
 * This function extracts the proxy URI string from PROXY_URI option of
 * the coap request packet. 
 * E.g., ./coap-client -v 100 -P proxy_address -m get coap://aueb.example.com/resource
 * PROXY_URI option contains "coap://aueb.example.com/resource"
 * This function creates a coap request packet using the proxy URI string from PROXY_URI option
 * @param node stores the client information 
 * @param msg request packet
 * @param len length of the request packet
*/
int handle_coap_request(struct client_node* node,
				  int client_sd,                                  
				  char *msg, 
				  int len) ;

int handle_coap_response();

/*!
 * Function used by sNAP , to fetch resource from sensors
 *
 * @param msg: coap request packet
 * @len length of the coap request packet
 */
int send_request_to_server(char const* ipv6addr, uint8_t* msg, int len);




/*!
 * invoked by cNAP to forward the response to IP endpoint (client)
 * @param:token token of the coap response. it will be used to look 
 *              up appropriate client to forward the response
 * @param coapPacket response packet
 * @param coapPacketLen length of the response packet
 */
void send_coap_response( unsigned char* token,
                         int tokenLen,
                         unsigned char* coapPacket,
                         int coapPacketLen, 
                         int isObserve);

/*!
 * invoked by cNAP to forward the response to IP endpoint (client)
 * @param argc command line argument count
 * @param argv command line arguments
 * @param:hostname IP address/domain name of reverse proxy
 * @param client_port listening port for the coap-client
 */
int parse_parameters(int argc, char *argv[],
                     char* hostname, 		  
		     in_port_t* client_port);

/*!
 * forward/reverse proxy loop listening for incoming packet
 * @param client_sd socket descriptor 
 * @param hostname IP address/domain name of reverse proxy
 *
 */
int client_loop(int client_sd);

/*!
 * For debug
 * @param msg coap packet           
 * @param len length of the coap packet
 */
void print_coap_packet_details(char *msg ,int len);

/*
 * initialize memory for client_node
 */
void init_coap_proxy();

/*
 * Free memory while closing the system
 */
void shutdown_coap_proxy();
#endif
