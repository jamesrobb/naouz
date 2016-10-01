#include "client_connection.h"

int parse_client_http_request(client_connection *connection, char* data_buffer) {

	connection->request = malloc(sizeof(http_request));
    connection->request->cookies = g_hash_table_new_full(g_str_hash, g_str_equal, ghash_table_gchar_destroy, ghash_table_gchar_destroy);
    connection->request->queries = g_hash_table_new_full(g_str_hash, g_str_equal, ghash_table_gchar_destroy, ghash_table_gchar_destroy);
    connection->request->header_fields = g_hash_table_new_full(g_str_hash, g_str_equal, ghash_table_gchar_destroy, ghash_table_gchar_destroy);
    connection->request->payload = g_string_new("");

    GString *uri = g_string_new("");
    GString *cookie_field = g_string_new("");
    int header_ret = http_request_parse_header(connection->request->header_fields, data_buffer);
    int queries_ret = 0;
    int cookies_ret = 0;
    int method_check = 0; // we check to see if we receive an http method we support

    if(g_hash_table_contains(connection->request->header_fields, "http_method") == TRUE && header_ret == 0) {

    	GString *http_method = g_string_new(g_hash_table_lookup(connection->request->header_fields, "http_method"));

    	if(g_strcmp0(http_method->str, "GET") != 0 && g_strcmp0(http_method->str, "POST") != 0 && g_strcmp0(http_method->str, "HEAD") != 0) {
    		method_check = -1;
    	}

    	g_string_free(http_method, TRUE);
	}

    if(g_hash_table_contains(connection->request->header_fields, "http_uri") == TRUE && header_ret == 0) {

    	g_string_append(uri, g_hash_table_lookup(connection->request->header_fields, "http_uri"));
    	queries_ret = http_request_parse_queries(connection->request->queries, uri->str);

    }

    if(g_hash_table_contains(connection->request->header_fields, "Cookie") == TRUE && header_ret == 0) {

		g_string_append(cookie_field, g_hash_table_lookup(connection->request->header_fields, "Cookie"));
		cookies_ret = http_request_parse_cookies(connection->request->cookies, cookie_field->str);
    }

    if(header_ret == 0) {
    	http_request_parse_payload(connection->request->payload, data_buffer);
    }

    g_string_free(uri, TRUE);
    g_string_free(cookie_field, TRUE);

    // blanket error return - did any error happen?
    return header_ret || queries_ret || cookies_ret || method_check;
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
}