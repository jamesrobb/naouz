#ifndef REQUEST_H
#define REQUEST_H

#include <glib.h>
#include <stdbool.h>

#include "log.h"
#include "util.h"

#define NEWLINE_DELIM		((gchar *) "\r\n")
#define REQUEST_LINE_DELIM	((gchar *) " ")
#define REQUEST_FIELD_DELIM	((gchar *) ": ")
#define REQUEST_URI_DELIM 	((gchar *) "?")
#define REQUEST_QUERY_DELIM ((gchar *) "&")
#define REQUEST_QUERY_KEY_VALUE_DELIM ((gchar *) "=")
#define REQUEST_COOKIE_DELIM ((gchar *) ";")
#define REQUEST_COOKIE_KEY_VALUE_DELIM ((gchar *) "=")

#define HTTP_STATUS_200 	((gchar *) "200 OK")

typedef struct _http_request
{
	GHashTable *cookies;
	GHashTable *queries;
	GHashTable *header_fields;
} http_request;

int parse_http_request(char* data_buffer, GHashTable *header_fields);

void http_request_print(http_request *request);

int http_request_parse_queries(gchar *http_uri, GHashTable *queries);

int http_request_parse_cookies(gchar *http_cookies, GHashTable *cookies);

#endif