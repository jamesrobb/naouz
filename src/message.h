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

void g_string_fill_with_http_headers(gchar* key, gchar* val, GString *string_fill);

void g_string_fill_with_http_queries(gchar* key, gchar* val, GString *string_fill);

void http_build_body(GString *body, gchar *body_options, gchar *body_text);

void http_build_document(GString *document, gchar *title, gchar *body);

void http_build_header(GString *header, gchar *response_code, gchar *content_type, GPtrArray *cookie_array, int payload_length, gboolean keep_alive);

void http_request_get_hostname(GString *host_name, GHashTable *header_fields);

void http_request_print(http_request *request);

int http_request_parse_cookies(GHashTable *cookies, gchar *http_cookies);

int http_request_parse_header(GHashTable *header_fields, char* data_buffer);

int http_request_parse_payload(GString *http_payload, char *data_buffer);

int http_request_parse_queries(GHashTable *queries, gchar *http_uri);


#endif