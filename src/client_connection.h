#ifndef _CLIENT_CONNECTION_H
#define _CLIENT_CONNECTION_H

#include <glib.h>
#include <stdlib.h>

#include "message.h"
#include "time.h"

typedef struct _client_connection {
    http_request *request;
    time_t last_activity;
    int fd;
    gboolean close; // if this flag is set, we know we can issue close() on the connection
    gboolean keep_alive;
} client_connection;

int parse_client_http_request(client_connection *connection, char* data_buffer);

void reset_client_connection_http_request(client_connection *connection);

void reset_client_connection(client_connection *connection);

#endif