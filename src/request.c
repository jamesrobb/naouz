#include "request.h"

int parse_http_request(char* data_buffer, http_request *request) {

	bool return_with_error = FALSE;
	gchar **buffer_split_line = g_strsplit((gchar *) data_buffer, NEWLINE_DELIM, 0);

	request->value_table = g_hash_table_new_full(g_str_hash, g_str_equal, ghash_table_gchar_destroy, ghash_table_gchar_destroy);

	// yay pointer arithematic
	for(int i = 0; buffer_split_line[i]; i++) {

		gchar **current_line;
		return_with_error = FALSE;

		// special case for first line of HTTP request, the request line (method, uri, http version)
		if(i == 0) {

			current_line = g_strsplit(buffer_split_line[i], REQUEST_LINE_DELIM, 0);

			int token_count = 0;
			for(int j = 0; current_line[j]; j++) {

				bool val_set = FALSE;
				gchar *value = g_malloc(gchar_array_len(current_line[j]));
				g_stpcpy(value, current_line[j]);

				// we get the http method, uri, and version
				if(j == 0) {
					val_set = TRUE;
					gchar *key = g_malloc(12);
					g_stpcpy(key, "http_method\0");
					g_hash_table_insert(request->value_table, key, value);
				}
				if(j == 1) {
					val_set = TRUE;
					gchar *key = g_malloc(9);
					g_stpcpy(key, "http_uri\0");
					g_hash_table_insert(request->value_table, key, value);
				}
				if(j == 2) {
					val_set = TRUE;
					gchar *key = g_malloc(13);
					g_stpcpy(key, "http_version\0");
					g_hash_table_insert(request->value_table, key, value);
				}

				if(val_set == FALSE) {
					g_free(value);
				} else {
					token_count++;
				}
			}

			if(token_count != 3) {
				g_warning("malformed http request (method, uri, version)");
				return_with_error = TRUE;
			}

		} else {

			if(g_strcmp0(buffer_split_line[i], "") == 0) {
				// end of header fields (hopefully)
				break;
			}

			current_line = g_strsplit(buffer_split_line[i], REQUEST_FIELD_DELIM, 0);

			if(current_line[0] && current_line[1]) {

				gchar *key = g_malloc(gchar_array_len(current_line[0]));
				g_stpcpy(key, current_line[0]);

				gchar *value = g_malloc(gchar_array_len(current_line[1]));
				g_stpcpy(value, current_line[1]);

				g_hash_table_insert(request->value_table, key, value);

			} else {
				g_warning("malformed http request (fields)");
				return_with_error = TRUE;
			}

		}

		g_strfreev(current_line);
		
		if(return_with_error == TRUE) {
			break;
		}

	}

	g_strfreev(buffer_split_line);

	return (return_with_error == TRUE ? 1 : 0);
}

void http_request_print(http_request *request) {
	g_hash_table_foreach(request->value_table, (GHFunc)ghash_table_strstr_iterator, "field: %s, value: %s\n");
	return;
}

int http_request_parse_queries(gchar *http_uri, GHashTable *key_value_table) {

	// first try to get anything in a query field 
	// query field is anything after ? in the http uri
	gchar **initial_split = g_strsplit(http_uri, REQUEST_URI_DELIM, 0);

	// if we have multiple question marks return error
	if(initial_split[2]) {
		return -1;
	}

	// if we managed to split at ? we get queries at the back in field 2
	if(initial_split[1]) {
		// queries are like this key=value&key=value and so on
		gchar **split_queries = g_strsplit(initial_split[1], REQUEST_QUERY_DELIM, 0);
		int current_index = 0;
		while(split_queries[current_index]){
			gchar **split_key_values = g_strsplit(split_queries[current_index], REQUEST_QUERY_KEY_VALUE_DELIM, 2);
			g_hash_table_insert(key_value_table, split_key_values[0], split_key_values[1]);
			current_index++;
		}
	}
	return 0;
}