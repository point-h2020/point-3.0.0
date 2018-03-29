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

static struct logger* l=NULL;
int server_sd = -1;

void init_systems(){

  l = init_logger(stdout, stderr, stderr);
  init_clients();
  init_coap_proxy();
}

void shutdown_systems(){

  shutdown_logger(l);
  shutdown_clients();
  shutdown_coap_proxy();	
 }

void signal_handler( int signum ){
  log_print(l, "Shutdown.\n");
  shutdown_systems();
  exit(EXIT_SUCCESS);
}

int nap_loop(int client_sd){

  struct pollfd fds[2];
  int timeout_msecs = RETRANSMIT_TIMEOUT;
  int ret = 0;
  struct client_node* client = NULL;
  char* recvMsg;
  int  len;	

  fds[0].fd = client_sd;
  fds[0].events = POLLIN;

  fds[1].fd = server_sd;
  fds[1].events = POLLIN;

  for(;;)
  {
    ret = poll( fds, 2, timeout_msecs ); 
    if( ret > 0 ) 
       if( fds[0].revents & POLLIN ) {	
          recvMsg=NULL;
          if(read_client(client_sd, &client, &recvMsg, &len) == 0){
               handle_coap_request(client, client_sd, recvMsg, len);
              free(recvMsg);     	          
          }
     }
    if( fds[1].revents & POLLIN ) 
       handle_coap_response();	    
   
    if( ret == -1 ) {
      log_error(l, "Poll errors.\n");	
      exit(EXIT_FAILURE);
    }
  }
}

int main(int argc, char *argv[]){
  //in_port_t client_port = 5678, server_port = 5001;
  in_port_t client_port = 5683, server_port = 5001;
  int client_sd;

  setbuf(stdout, NULL);
  
  init_systems();

  if(argc == 2){
    CoAPHandler* handler=CoAPHandler::getInstance(true);
    handler->invokeCoAPHandler(argv[1]);  
  }
  
 
  if( signal(SIGINT, signal_handler) == SIG_IGN )
    signal(SIGINT, SIG_IGN);

  log_print(l, "CoAP Proxy started, port for client subscriptions: %u.\n", client_port);

  client_sd = client_socket( client_port);
  if( client_sd == -1 ) exit(EXIT_FAILURE);

  

  if(argc == 2){
    server_sd = server_socket( server_port);
    if( server_sd == -1 ) exit(EXIT_FAILURE);
    
  }

  nap_loop(client_sd);

  close(client_sd);
  if(argc == 2)
    close(server_sd);
  
  shutdown_systems();

  return EXIT_SUCCESS;
}
