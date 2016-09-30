#ifndef REQUEST_H
#define REQUEST_H

#include <glib.h>
#include <stdbool.h>

#include "log.h"
#include "util.h"

#define NEWLINE_DELIM		((gchar *) "\r\n")
#define REQUEST_LINE_DELIM	((gchar *) " ")
#define REQUEST_FIELD_DELIM	((gchar *) ": ")
#define REQUEST_URI_DELIM ((gchar *) "?")
#define REQUEST_QUERY_DELIM ((gchar *) "&")
#define REQUEST_QUERY_KEY_VALUE_DELIM ((gchar *) "=")

typedef struct _http_request
{
	GHashTable *queries;
	GHashTable *header_fields;
} http_request;

int parse_http_request(char* data_buffer, GHashTable *header_fields);

void http_request_print(http_request *request);

int http_request_parse_queries(gchar *http_uri, GHashTable *queries);

#endif