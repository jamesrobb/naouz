#include "message.h"

void g_string_fill_with_http_headers(gchar* key, gchar* val, GString *string_fill) {
	if(g_strcmp0(key, "http_uri") == 0 || g_strcmp0(key, "uri_path") == 0 || g_strcmp0(key, "http_method") == 0 || g_strcmp0(key, "http_version") == 0) {
		return;
	}
	g_string_append_printf(string_fill, "%s: %s<br />", key, val);
}

void g_string_fill_with_http_queries(gchar* key, gchar* val, GString *string_fill) {
	g_string_append_printf(string_fill, "%s: %s<br />", key, val);
}

void http_build_body(GString *body, gchar *body_options, gchar *body_text) {

	if(g_strcmp0(body_options, "") == 0) {
		g_string_append(body, "<body>\n");
	} else {
		g_string_append_printf(body, "<body %s>\n", body_options);
	}

	g_string_append_printf(body, "%s\n</body>\n", body_text);

	return;
}

void http_build_document(GString *document, gchar *title, gchar *body) {

	gchar *format = "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"UTF-8\">\n<title>%s</title>\n</head>\n\n%s\n\n</html>";
	g_string_append_printf(document, format, title, body);

}

void http_build_header(GString *header, gchar *response_code, GPtrArray *cookie_array, int payload_length, gboolean keep_alive) {

	g_string_append_printf(header, "HTTP/1.1 %s%s", response_code, NEWLINE_DELIM);
    g_string_append_printf(header, "Server: naouz/%s%s", NAOUZ_VERSION, NEWLINE_DELIM);
    g_string_append_printf(header, "Accept-Ranges: bytes%s", NEWLINE_DELIM);
    g_string_append_printf(header, "Content-Type: text/html%s", NEWLINE_DELIM);

    if(keep_alive == FALSE) {
    	g_string_append_printf(header, "Connection: close%s", NEWLINE_DELIM);
    	g_info("Sending 'Connection: close' to client");
    }

    if(cookie_array != NULL) {
    	GString *cookie_string = g_string_new("Set-Cookie: ");
    	
    	for(int i = 0; i < cookie_array->len; i+=2) {
    		if(cookie_array->pdata[i+1] != NULL) {
    			g_string_append_printf(cookie_string, "%s=%s;", (gchar *) cookie_array->pdata[i], (gchar *) cookie_array->pdata[i+1]);
    		}
    		else {
    			g_string_append_printf(cookie_string, "%s=\"\";", (gchar *) cookie_array->pdata[i]);
    		}
    	}

    	g_string_append_printf(header, "%s%s", cookie_string->str, NEWLINE_DELIM);
    	g_string_free(cookie_string, TRUE);
    }

    g_string_append_printf(header, "Content-Length: %d%s%s", payload_length, NEWLINE_DELIM, NEWLINE_DELIM);

    return;
}

void http_request_get_hostname(GString *host_name, GHashTable *header_fields) {

	if(g_hash_table_contains(header_fields, "host") == TRUE) {

		g_string_append(host_name, g_hash_table_lookup(header_fields, "host"));

	} else {

		g_string_append(host_name, g_get_host_name());

	}

}

void http_request_print(http_request *request) {
	g_hash_table_foreach(request->header_fields, (GHFunc)ghash_table_strstr_iterator, "field: %s, value: %s\n");
	return;
}

int http_request_parse_cookies(GHashTable *cookies, gchar *http_cookies) {
	
	g_info("parsing cookies");
	// got to split by semi colons
	gchar **cookie_split = g_strsplit(http_cookies, REQUEST_COOKIE_DELIM, 0);
	int cookie_counter = 0;

	while(cookie_split[cookie_counter]) {

		if(g_strcmp0(cookie_split[cookie_counter], "") == 0) {
			break;
		}

		// split by equals sign 
		gchar **split_key_values = g_strsplit(cookie_split[cookie_counter], REQUEST_COOKIE_KEY_VALUE_DELIM, 2);

		// currently ignores garbage cookie data, might need to come back to this so it returns an error (and therefore we recognize a bad request)
		if(g_strv_length(split_key_values) == 0) {
			g_strfreev(split_key_values);
			continue;
		}

		gchar *key;
		gchar *value;
		// chug (trim leading whitespace) if the first letter of the first value a space
		if(g_ascii_isspace(split_key_values[0][0])) {
			g_strchug(split_key_values[0]);
		}

		// if we have a key value pair
		if(split_key_values[1]) {
			// chug if value has leading whitespace
			if(g_ascii_isspace(split_key_values[1][0])) {
				g_strchug(split_key_values[1]);
			}
			key = g_malloc(gchar_array_len(split_key_values[0]));
			g_stpcpy(key, split_key_values[0]);

			value = g_malloc(gchar_array_len(split_key_values[1]));
			g_stpcpy(value, split_key_values[1]);
			
		}
		// if we only have a value
		else {
			key = g_malloc(gchar_array_len(split_key_values[0]));
			g_stpcpy(key, split_key_values[0]);
			value = NULL;
		}
		gchar *key_stripped = g_malloc(gchar_array_len(split_key_values[0]));
		gchar *value_stripped = g_malloc(gchar_array_len(split_key_values[1]));
		gchar_char_strip(key_stripped, key, '"');
		gchar_char_strip(value_stripped, value, '"');

		if(!g_hash_table_contains(cookies, key_stripped)){
			g_hash_table_insert(cookies, key_stripped, value_stripped);
		}

		g_free(key);
		if(value != NULL) {
			g_free(value);
		}
		g_strfreev(split_key_values);
		cookie_counter++;
	}

	g_strfreev(cookie_split);
	return 0;
}

int http_request_parse_header(GHashTable *header_fields, char* data_buffer) {

	bool return_with_error = FALSE;
	gchar **buffer_split_line = g_strsplit((gchar *) data_buffer, NEWLINE_DELIM, 0);

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
					// malloc 12 because it's the exact length of the key string
					gchar *key = g_malloc(12);
					g_stpcpy(key, "http_method");
					g_hash_table_insert(header_fields, key, value);
				}
				if(j == 1) {
					val_set = TRUE;
					gchar *key = g_malloc(9);
					g_stpcpy(key, "http_uri");
					gchar *path_key = g_malloc(9);
					g_stpcpy(path_key, "uri_path");
					
					// we do this so we have a specific index in the hash table that only has the path
					gchar **split_query_path = g_strsplit(value, REQUEST_URI_DELIM, 2);
			
					gchar *path_value = g_malloc(gchar_array_len(split_query_path[0]));
					g_stpcpy(path_value, split_query_path[0]);

					g_strfreev(split_query_path);
					g_hash_table_insert(header_fields, key, value);
					g_hash_table_insert(header_fields, path_key, path_value);
				}
				if(j == 2) {
					val_set = TRUE;
					gchar *key = g_malloc(13);
					g_stpcpy(key, "http_version");
					g_hash_table_insert(header_fields, key, value);
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

				// g_aciii_strdown allocates a new string in addition to making the ascii characters lowercase
				gchar *key = g_ascii_strdown(current_line[0], -1);
				//g_stpcpy(key, current_line[0]);

				gchar *value = g_malloc(gchar_array_len(current_line[1]));
				g_stpcpy(value, current_line[1]);

				g_hash_table_insert(header_fields, key, value);

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

int http_request_parse_payload(GString *http_payload, char *data_buffer) {

	// ret value is currently always zero, will implement logic later to detect an error condition
	int ret_val = 0;
	gchar *payload_start = g_strrstr(data_buffer, HTTP_PAYLOAD_DELIM);
	gchar payload_buffer[DATA_BUFFER_LENGTH];

	if(payload_start != NULL) {

		g_stpcpy(payload_buffer, payload_start + 4 * sizeof(gchar));
		g_string_append(http_payload, payload_buffer);

		ret_val = 0;
	}

	return ret_val;
}

int http_request_parse_queries(GHashTable *queries, gchar *http_uri) {

	// first try to get anything in a query field 
	// query field is anything after ? in the http uri
	gchar **initial_split = g_strsplit(http_uri, REQUEST_URI_DELIM, 0);

	// if we have multiple question marks return error
	if(g_strv_length(initial_split) > 2) {
		g_strfreev(initial_split);
		return -1;
	}

	// if we managed to split at ? we get queries at the back in field 2
	if(initial_split[1]) {

		// queries are like this key=value&key=value and so on
		gchar **split_queries = g_strsplit(initial_split[1], REQUEST_QUERY_DELIM, 0);
		int current_index = 0;

		while(split_queries[current_index]){
			gchar **split_key_values = g_strsplit(split_queries[current_index], REQUEST_QUERY_KEY_VALUE_DELIM, 2);
			
			gchar *key = g_malloc(gchar_array_len(split_key_values[0]));
			g_stpcpy(key, split_key_values[0]);

			gchar *value = g_malloc(gchar_array_len(split_key_values[1]));
			g_stpcpy(value, split_key_values[1]);

			g_hash_table_insert(queries, key, value);

			g_strfreev(split_key_values);
			current_index++;
		}

		g_strfreev(split_queries);
	}

	g_strfreev(initial_split);
	return 0;
}