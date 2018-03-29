#include "logger.h"

#include <stdarg.h>
#include <stdlib.h>

struct logger* init_logger(FILE* print_file, FILE* debug_file, FILE* error_file)
{
  struct logger* l = (logger *) malloc(sizeof(struct logger));
  l->f_print = print_file;
  l->f_debug = debug_file;
  l->f_error = error_file;
  return l;
}

void log_print(const struct logger* l, const char* format, ...)
{
  va_list arg;
  if( l->f_print == NULL ) return;
  va_start(arg, format);
  vfprintf(l->f_print, format, arg);
  va_end(arg);
}

#define DEBUG_LOG
void log_debug(const struct logger* l, const char* format, ...)
{
#ifdef DEBUG_LOG
  va_list arg;
  if( l->f_debug == NULL ) return;
  va_start(arg, format);
  vfprintf(l->f_debug, format, arg);
  va_end(arg);
#endif
}

void log_error(const struct logger* l, const char* format, ...)
{
  va_list arg;
  if( l->f_error == NULL ) return;
  va_start(arg, format);
  vfprintf(l->f_error, format, arg);
  va_end(arg);
}

void shutdown_logger(struct logger* l)
{
  free(l);
}
