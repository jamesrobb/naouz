#ifndef REQUEST_H
#define REQUEST_H

#include <glib.h>
#include <stdbool.h>

#include "constants.h"
#include "log.h"
#include "util.h"

#define NEWLINE_DELIM		((gchar *) "\r\n")
#define HTTP_PAYLOAD_DELIM	((gchar *) "\r\n\r\n")
#define REQUEST_LINE_DELIM	((gchar *) " ")
#define REQUEST_FIELD_DELIM	((gchar *) ": ")
#define REQUEST_URI_DELIM 	((gchar *) "?")
#define REQUEST_QUERY_DELIM ((gchar *) "&")
#define REQUEST_QUERY_KEY_VALUE_DELIM ((gchar *) "=")
#define REQUEST_COOKIE_DELIM ((gchar *) ";")
#define REQUEST_COOKIE_KEY_VALUE_DELIM ((gchar *) "=")

#define HTTP_STATUS_200 	((gchar *) "200 OK")
#define HTTP_STATUS_400 	((gchar *) "400 Bad Request")

typedef struct _http_request
{
	GHashTable *cookies;
	GHashTable *queries;
	GHashTable *header_fields;
	GString *payload;
} http_request;

void build_http_body(GString *body, gchar *body_options, gchar *body_text);

void build_http_document(GString *document, gchar *title, gchar *body);

void build_http_header(GString *header, gchar *response_code, int payload_length, GPtrArray *cookie_array); 

void http_request_print(http_request *request);

int http_request_parse_cookies(GHashTable *cookies, gchar *http_cookies);

int http_request_parse_header(GHashTable *header_fields, char* data_buffer);

int http_request_parse_payload(GString *http_payload, char *data_buffer);

int http_request_parse_queries(GHashTable *queries, gchar *http_uri);


#endif