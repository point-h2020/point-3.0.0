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



#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

struct logger
{
  FILE* f_print;
  FILE* f_debug;
  FILE* f_error;
};

struct logger* init_logger(FILE *print_file, FILE *debug_file, FILE *error_file);
void log_print(const struct logger* l, const char* format, ...);
void log_debug(const struct logger* l, const char* format, ...);
void log_error(const struct logger* l, const char* format, ...);
void shutdown_logger(struct logger* l);

#endif

