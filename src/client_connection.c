#include "client_connection.h"

int parse_client_http_request(client_connection *connection, char* data_buffer) {

	connection->request = malloc(sizeof(http_request));
    connection->request->cookies = g_hash_table_new_full(g_str_hash, g_str_equal, ghash_table_gchar_destroy, ghash_table_gchar_destroy);
    connection->request->queries = g_hash_table_new_full(g_str_hash, g_str_equal, ghash_table_gchar_destroy, ghash_table_gchar_destroy);
    connection->request->header_fields = g_hash_table_new_full(g_str_hash, g_str_equal, ghash_table_gchar_destroy, ghash_table_gchar_destroy);
    connection->request->payload = g_string_new("");

    int header_ret = http_request_parse_header(connection->request->header_fields, data_buffer);
    int queries_ret = 0;
    int cookies_ret = 0;
    int method_check = 0; // we check to see if we receive an http method we support
    int connection_header_check = 0;

    if(g_hash_table_contains(connection->request->header_fields, "http_method") == TRUE && header_ret == 0) {

    	gchar *http_method = g_hash_table_lookup(connection->request->header_fields, "http_method");

    	if(g_strcmp0(http_method, "GET") != 0 && g_strcmp0(http_method, "POST") != 0 && g_strcmp0(http_method, "HEAD") != 0) {
    		method_check = -1;
    	}

	}

	if(header_ret == 0) {

		connection->keep_alive = FALSE;

    	gchar *http_version = g_hash_table_lookup(connection->request->header_fields, "http_version");
    	gchar *connection_header = g_hash_table_lookup(connection->request->header_fields, "connection");
    	gboolean connection_header_exists = g_hash_table_contains(connection->request->header_fields, "connection");

    	if(connection_header_exists == TRUE) {

			gboolean connection_header_is_keep_alive = g_strcmp0(connection_header, "keep-alive") == 0 ? TRUE : FALSE;
			gboolean connection_header_is_close = g_strcmp0(connection_header, "close") == 0 ? TRUE : FALSE;

			if(connection_header_is_keep_alive) {
				connection->keep_alive = TRUE;
    		} else if(connection_header_is_close) {
    			connection->keep_alive = FALSE;
    		} else {
    			connection_header_check = -1;
    		}

    	} else {

    		if(g_strcmp0(http_version, "HTTP/1.1") == 0) {
    			connection->keep_alive = TRUE;
    		} else {
    			// we treat everything else as HTTP/1.0
    			connection->keep_alive = FALSE;
    		}

    	}

    }

    if(g_hash_table_contains(connection->request->header_fields, "http_uri") == TRUE && header_ret == 0) {

    	gchar *uri = g_hash_table_lookup(connection->request->header_fields, "http_uri");
    	queries_ret = http_request_parse_queries(connection->request->queries, uri);

    }

    if(g_hash_table_contains(connection->request->header_fields, "cookie") == TRUE && header_ret == 0) {

		gchar *cookie_field = g_hash_table_lookup(connection->request->header_fields, "cookie");
		cookies_ret = http_request_parse_cookies(connection->request->cookies, cookie_field);
    }

    if(header_ret == 0) {
    	http_request_parse_payload(connection->request->payload, data_buffer);
    }

    // blanket error return - did any error happen?
    return header_ret || queries_ret || cookies_ret || method_check || connection_header_check;
}

void reset_client_connection_http_request(client_connection *connection) {

    g_hash_table_destroy(connection->request->queries);
    g_hash_table_destroy(connection->request->header_fields);
    g_hash_table_destroy(connection->request->cookies);
    g_string_free(connection->request->payload, TRUE);

    free(connection->request);

    connection->request = NULL;
}

void reset_client_connection(client_connection *connection) {
    connection->request = NULL;
    connection->last_activity = time(NULL);
    connection->fd = CONN_FREE;
    connection->close = FALSE;
    connection->keep_alive = FALSE;
}