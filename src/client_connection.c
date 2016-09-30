#include "client_connection.h"

int parse_client_http_request(client_connection *connection, char* data_buffer) {

	connection->request = malloc(sizeof(http_request));
    connection->request->cookies = g_hash_table_new_full(g_str_hash, g_str_equal, ghash_table_gchar_destroy, ghash_table_gchar_destroy);
    connection->request->queries = g_hash_table_new_full(g_str_hash, g_str_equal, ghash_table_gchar_destroy, ghash_table_gchar_destroy);
    connection->request->header_fields = g_hash_table_new_full(g_str_hash, g_str_equal, ghash_table_gchar_destroy, ghash_table_gchar_destroy);

    GString *uri = NULL;
    GString *cookie_field = NULL;
    int header_ret = parse_http_header(connection->request->header_fields, data_buffer);
    int queries_ret = 0;
    int cookies_ret = 0;

    if(g_hash_table_contains(connection->request->header_fields, "http_uri") == TRUE && header_ret == 0) {

    	uri = g_string_new(g_hash_table_lookup(connection->request->header_fields, "http_uri"));
    	queries_ret = http_request_parse_queries(connection->request->queries, uri->str);

    }

    if(g_hash_table_contains(connection->request->header_fields, "Cookie") == TRUE && header_ret == 0) {

		cookie_field = g_string_new(g_hash_table_lookup(connection->request->header_fields, "Cookie"));
		cookies_ret = http_request_parse_cookies(connection->request->cookies, cookie_field->str);
    }

    g_string_free(uri, TRUE);
    g_string_free(cookie_field, TRUE);

    // blanket error return - did any error happen?
    return header_ret || queries_ret || cookies_ret;
}

void reset_client_connection_http_request(client_connection *connection) {

    g_hash_table_destroy(connection->request->queries);
    g_hash_table_destroy(connection->request->header_fields);
    g_hash_table_destroy(connection->request->cookies);

    free(connection->request);

    connection->request = NULL;
}

void reset_client_connection(client_connection *connection) {
    connection->request = NULL;
    connection->last_activity = time(NULL);
    connection->fd = CONN_FREE;
}