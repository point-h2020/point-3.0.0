#include "coap.h"
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

long int d2_value = 0;
long int d4_value = 0;
long int d5_value = 0;

static void
a_A0_GET_handler(coap_context_t *ctx, struct coap_resource_t *resource, 
              const coap_endpoint_t *local_interface, coap_address_t *peer, 
              coap_pdu_t *request, str *token, coap_pdu_t *response) 
{
	std::cout << "Received get request" << std::endl;
	unsigned char buf[3];
	//const char* response_data  = "100";
	
	int response_value = rand() % 1000;
	char response_data[4];
	sprintf(response_data, "%d", response_value);
	response->hdr->code        = COAP_RESPONSE_CODE(205);
	if (request->hdr->token_length > 0)
		coap_add_token(response,request->hdr->token_length, request->hdr->token);
	coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
	coap_add_data  (response, strlen(response_data), (unsigned char *)response_data);
}

static void
a_A1_GET_handler(coap_context_t *ctx, struct coap_resource_t *resource, 
              const coap_endpoint_t *local_interface, coap_address_t *peer, 
              coap_pdu_t *request, str *token, coap_pdu_t *response) 
{
	std::cout << "Received get request" << std::endl;
	unsigned char buf[3];
	//const char* response_data  = "100";
	
	int response_value = rand() % 1000;
	char response_data[4];
	sprintf(response_data, "%d", response_value);
	response->hdr->code        = COAP_RESPONSE_CODE(205);
	if (request->hdr->token_length > 0)
		coap_add_token(response,request->hdr->token_length, request->hdr->token);
	coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
	coap_add_data  (response, strlen(response_data), (unsigned char *)response_data);
}

static void
a_A2_GET_handler(coap_context_t *ctx, struct coap_resource_t *resource, 
              const coap_endpoint_t *local_interface, coap_address_t *peer, 
              coap_pdu_t *request, str *token, coap_pdu_t *response) 
{
	std::cout << "Received get request" << std::endl;
	unsigned char buf[40];

	// if d4_value or d5_value == 0, decrease the A2 value
	int response_value;
	int leds_on = 0;
	if (d4_value == 0) {
			leds_on++;
	}
	if (d5_value == 0) {
			leds_on++;
	}
	
	if (leds_on == 2) { // range: 0-300
		response_value = rand() % 300;
	} else if (leds_on == 1) { // range: 300-700
		response_value = rand() % 400 + 300;
	} else { // leds_on == 0, range: 700-1000
		response_value = rand() % 300 + 700;
	}

	char response_data[4];
	sprintf(response_data, "%d", response_value);
    coap_opt_iterator_t opt_iter;
    coap_opt_t *option;
    if (request != NULL)
    {
        option = coap_check_option(request, COAP_OPTION_OBSERVE, &opt_iter);
        if (option != NULL)
        {
            std::cout << "The request contains the observe option" << std::endl;
            unsigned char* observe_value = coap_opt_value(option);
            if (observe_value[0] == 1)
                std::cout <<"De-register" << std::endl;
        }
    }
    response->hdr->type = COAP_MESSAGE_CON;
    response->hdr->code = COAP_RESPONSE_CODE(205);
    if (coap_find_observer(resource, peer, token)) {
        coap_add_option(response,
                    COAP_OPTION_OBSERVE,
                    coap_encode_var_bytes(buf, ctx->observe), buf);
    }
    else if (request->hdr->token_length > 0)
    {
        coap_add_token(response,request->hdr->token_length, request->hdr->token);
    }    
    coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
	coap_add_data  (response, strlen(response_data), (unsigned char *)response_data);
}

static void
a_D2_GET_handler(coap_context_t *ctx, struct coap_resource_t *resource, 
              const coap_endpoint_t *local_interface, coap_address_t *peer, 
              coap_pdu_t *request, str *token, coap_pdu_t *response) 
{
	std::cout << "Received get request to D2" << std::endl;
	unsigned char buf[3];
	//const char* response_data  = "100";
	char response_data[12];
	sprintf(response_data, "%ld", d2_value);

	response->hdr->code        = COAP_RESPONSE_CODE(205);
	if (request->hdr->token_length > 0)
		coap_add_token(response,request->hdr->token_length, request->hdr->token);
	coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
	coap_add_data  (response, strlen(response_data), (unsigned char *)response_data);
}

static void
a_D2_PUT_handler(coap_context_t *ctx, struct coap_resource_t *resource, 
              const coap_endpoint_t *local_interface, coap_address_t *peer, 
              coap_pdu_t *request, str *token, coap_pdu_t *response) 
{
	long int value = 0;
	unsigned char* data;
	char *tmp;
	size_t data_len;

	std::cout << "Received put request to D2" << std::endl;

	if (coap_get_data(request, &data_len, &data)) {
		tmp = (char *)calloc(data_len, sizeof(char));
		memcpy(tmp, (char *)data, data_len);
		value = strtol(tmp, NULL, 10);
		d2_value = value;
		std::cout << "\tValue of the put request is: " << value << std::endl;
	}

	unsigned char buf[3];
	response->hdr->code        = COAP_RESPONSE_CODE(204);
	if (request->hdr->token_length > 0)
		coap_add_token(response,request->hdr->token_length, request->hdr->token);
	//coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
	//coap_add_data  (response, 1, request->data);
}

static void
a_D4_GET_handler(coap_context_t *ctx, struct coap_resource_t *resource, 
              const coap_endpoint_t *local_interface, coap_address_t *peer, 
              coap_pdu_t *request, str *token, coap_pdu_t *response) 
{
	std::cout << "Received get request to D4" << std::endl;
	unsigned char buf[3];
	char response_data[12];
	sprintf(response_data, "%ld", d4_value);

	response->hdr->code        = COAP_RESPONSE_CODE(205);
	if (request->hdr->token_length > 0)
		coap_add_token(response,request->hdr->token_length, request->hdr->token);
	coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
	coap_add_data  (response, strlen(response_data), (unsigned char *)response_data);
}

static void
a_D4_PUT_handler(coap_context_t *ctx, struct coap_resource_t *resource, 
              const coap_endpoint_t *local_interface, coap_address_t *peer, 
              coap_pdu_t *request, str *token, coap_pdu_t *response) 
{
	long int value = 0;
	unsigned char* data;
	char *tmp;
	size_t data_len;

	std::cout << "Received put request to D4" << std::endl;

	if (coap_get_data(request, &data_len, &data)) {
		tmp = (char *)calloc(data_len, sizeof(char));
		memcpy(tmp, (char *)data, data_len);
		value = strtol(tmp, NULL, 10);
		d4_value = value;
		std::cout << "\tValue of the put request is: " << value << std::endl;
	}

	unsigned char buf[3];
	response->hdr->code        = COAP_RESPONSE_CODE(204);
	if (request->hdr->token_length > 0)
		coap_add_token(response,request->hdr->token_length, request->hdr->token);
	//coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
	//coap_add_data  (response, 1, request->data);
}

static void
a_D5_GET_handler(coap_context_t *ctx, struct coap_resource_t *resource, 
              const coap_endpoint_t *local_interface, coap_address_t *peer, 
              coap_pdu_t *request, str *token, coap_pdu_t *response) 
{
	std::cout << "Received get request to D5" << std::endl;
	unsigned char buf[3];
	char response_data[12];
	sprintf(response_data, "%ld", d5_value);

	response->hdr->code        = COAP_RESPONSE_CODE(205);
	if (request->hdr->token_length > 0)
		coap_add_token(response,request->hdr->token_length, request->hdr->token);
	coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
	coap_add_data  (response, strlen(response_data), (unsigned char *)response_data);
}

static void
a_D5_PUT_handler(coap_context_t *ctx, struct coap_resource_t *resource, 
              const coap_endpoint_t *local_interface, coap_address_t *peer, 
              coap_pdu_t *request, str *token, coap_pdu_t *response) 
{
	long int value = 0;
	unsigned char* data;
	char *tmp;
	size_t data_len;

	std::cout << "Received put request to D5" << std::endl;

	if (coap_get_data(request, &data_len, &data)) {
		tmp = (char *)calloc(data_len, sizeof(char));
		memcpy(tmp, (char *)data, data_len);
		value = strtol(tmp, NULL, 10);
		d5_value = value;
		std::cout << "\tValue of the put request is: " << value << std::endl;
	}

	unsigned char buf[3];
	response->hdr->code        = COAP_RESPONSE_CODE(204);
	if (request->hdr->token_length > 0)
		coap_add_token(response,request->hdr->token_length, request->hdr->token);
	//coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
	//coap_add_data  (response, 1, request->data);
}

int main(int argc, char* argv[])
{
	coap_context_t*  ctx;
	coap_address_t   serv_addr;
	coap_resource_t* a_A0; 
	coap_resource_t* a_A1;
	coap_resource_t* a_A2;
	coap_resource_t* a_D2; 
	coap_resource_t* a_D4; 
	coap_resource_t* a_D5;
	fd_set           readfds; 
    struct timeval   timeout;   
	/* Prepare the CoAP server socket */ 
	coap_address_init(&serv_addr);
	serv_addr.addr.sin6.sin6_family = AF_INET6;
	serv_addr.addr.sin6.sin6_addr   = in6addr_any;
	serv_addr.addr.sin6.sin6_port   = htons(5683); 
	ctx                             = coap_new_context(&serv_addr);
	if (!ctx) exit(EXIT_FAILURE);
	/* Initialize the hello resource */
	a_A0 = coap_resource_init((unsigned char *)"/a/A0", 5, 0);
    a_A1 = coap_resource_init((unsigned char *)"/a/A1", 5, 0);
	a_A2 = coap_resource_init((unsigned char *)"/a/A2", 5, 0);
    a_D2 = coap_resource_init((unsigned char *)"/a/D2", 5, 0);
	a_D4 = coap_resource_init((unsigned char *)"/a/D4", 5, 0);
	a_D5 = coap_resource_init((unsigned char *)"/a/D5", 5, 0);
	coap_register_handler(a_A0, COAP_REQUEST_GET, a_A0_GET_handler);
    coap_register_handler(a_A1, COAP_REQUEST_GET, a_A1_GET_handler);
	coap_register_handler(a_A2, COAP_REQUEST_GET, a_A2_GET_handler);
    coap_register_handler(a_D2, COAP_REQUEST_GET, a_D2_GET_handler);
    coap_register_handler(a_D2, COAP_REQUEST_PUT, a_D2_PUT_handler);
    coap_register_handler(a_D4, COAP_REQUEST_GET, a_D4_GET_handler);
    coap_register_handler(a_D4, COAP_REQUEST_PUT, a_D4_PUT_handler);
    coap_register_handler(a_D5, COAP_REQUEST_GET, a_D5_GET_handler);
    coap_register_handler(a_D5, COAP_REQUEST_PUT, a_D5_PUT_handler);
    a_A2->observable = 1;
    coap_add_resource(ctx, a_A1);
	coap_add_resource(ctx, a_A0);
	coap_add_resource(ctx, a_A2);
    coap_add_resource(ctx, a_D2);
	coap_add_resource(ctx, a_D4);
	coap_add_resource(ctx, a_D5);
	/*Listen for incoming connections*/
    std::cout << "Start listening" << std::endl;
	while (1) {
        timeout.tv_sec  = 3;
        timeout.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_SET( ctx->sockfd, &readfds );
		int result = select( FD_SETSIZE, &readfds, 0, 0, &timeout );
		if ( result < 0 ) /* socket error */
		{
			exit(EXIT_FAILURE);
		} 
		else if ( result > 0 && FD_ISSET( ctx->sockfd, &readfds )) /* socket read*/
		{	 
            coap_read( ctx );  
                 
		} 
        else //timeout
        {
            a_A2->dirty = 1;  
            coap_check_notify(ctx);
        }
	}    
}
