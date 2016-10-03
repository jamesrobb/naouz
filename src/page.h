#ifndef _PAGE_H
#define _PAGE_H

#include <arpa/inet.h>
#include <glib.h>
#include <netinet/in.h>
#include <string.h>

#include "client_connection.h"
#include "message.h"

void build_bad_request_response();

void parse_colour_page_request(GString *response, client_connection *connection, gchar* uri);

void parse_generic_page_request(GString *response, client_connection *connection, gchar *host_name, gchar *uri);

void parse_header_page_request(GString *response, client_connection *connection, gchar *uri);

void parse_queries_page_request(GString *response, client_connection *connection, gchar *host_name, gchar *uri);

#endif