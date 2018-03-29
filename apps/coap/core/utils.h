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



#ifndef UTILS_H
#define UTILS_H

#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>


/* 1000 msec = 1 second */
#define KEEP_ALIVE_INTERVAL 5000
#define RETRANSMIT_TIMEOUT 200

using namespace std;

/* 
 * @param result keep alive interval time (x-y)
 * @param x current time
 * @param y init time when coap proxy receives a packet from a coap-client
 * 
 */
int timeval_subtract (struct timeval *result, const struct timeval *x, struct timeval *y);

/* 
 * compare two addresses
 * @param a1 and
 * @param a2
 * 
 */
int compare_sockaddr(struct sockaddr_storage* a1, struct sockaddr_storage* a2);

void print_hex_buffer(const char *message, uint8_t *buffer, int len);

#endif
