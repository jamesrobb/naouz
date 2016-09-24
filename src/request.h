#ifndef REQUEST_H
#define REQUEST_H

#include <glib.h>
#include <stdbool.h>

#include "log.h"
#include "util.h"

typedef struct _http_request
{
	GHashTable *value_table;
} http_request;

int parse_http_request(char* data_buffer, http_request *request);

#endif