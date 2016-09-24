#include "request.h"

int parse_http_request(char* data_buffer, http_request *request) {
	gchar *delimiters_line = "\n";
	gchar **buffer_split_line = g_strsplit((gchar *) data_buffer, delimiters_line, 0);

	request->value_table = g_hash_table_new(g_str_hash, g_str_equal);

	// yay pointer arithematic
	for(int i = 0; buffer_split_line[i]; i++) {

		gchar **current_line;

		// special case for first line of HTTP requests (method, uri, http version)
		if(i == 0) {

			current_line = g_strsplit(buffer_split_line[i], " ", 0);

			for(int j = 0; current_line[j]; j++) {

				bool val_set = FALSE;
				gchar *value = g_malloc(gchar_array_len(current_line[j]));
				g_stpcpy(value, current_line[j]);

				// we get the http method, uri, and version
				if(j == 0) {
					val_set = TRUE;
					g_hash_table_insert(request->value_table, "http_method", value);
				}
				if(j == 1) {
					val_set = TRUE;
					g_hash_table_insert(request->value_table, "http_uri", value);
				}
				if(j == 2) {
					val_set = TRUE;
					g_hash_table_insert(request->value_table, "http_version", value);
				}

				if(val_set == FALSE) {
					g_free(value);
				}
			}

		}

		g_strfreev(current_line);
		break;
	}

	g_strfreev(buffer_split_line);
	return 0;
}