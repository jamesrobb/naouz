#include "page.h"

void build_bad_request_response(GString *response) {

    GString *header = g_string_new("");
    GString *payload = g_string_new("");
    GString *html_body = g_string_new("");
    GString *body_text = g_string_new("400 Bad Request");
    int payload_length = 0;

    http_build_body(html_body, "", body_text->str);
    http_build_document(payload, "NAOUZ - 400 Bad Request", html_body->str);
    payload_length = payload->len;

    http_build_header(header, HTTP_STATUS_400, "text/html", NULL, payload_length, TRUE);

    g_string_append(response, header->str);
    g_string_append(response, payload->str);

    g_string_free(header, TRUE);
    g_string_free(payload, TRUE);
    g_string_free(html_body, TRUE);
    g_string_free(body_text, TRUE);

    return;
}

void parse_favicon_request(GString *response, client_connection *connection) {

	GString *method = g_string_new(g_hash_table_lookup(connection->request->header_fields, "http_method"));
    GString *header = g_string_new("");
    char *payload = NULL;
    long int payload_length = 0;

    if(g_strcmp0(method->str, "GET") == 0) {

    	FILE *f = fopen(FAVICON_LOCATION, "rb");

    	if(f != NULL) {

    		fseek(f, 0, SEEK_END);
			long f_size = ftell(f);
			fseek(f, 0, SEEK_SET);

			payload = malloc(f_size + 1);
			long int bytes_read = fread(payload, 1, f_size, f);
			payload[f_size] = 0;

			if(bytes_read > 0) {
	    		payload_length = bytes_read;
    		}

    	} else {
    		g_warning("failed to open favicon.ico");
    	}
        
        fclose(f);
    }
    
    http_build_header(header, HTTP_STATUS_200, "image/x-icon", NULL, payload_length, connection->keep_alive);

    g_string_append(response, header->str);

    for(int i = 0; i < payload_length; i++) {
    	g_string_append_c(response, (gchar) payload[i]);
    }

    free(payload);
    g_string_free(method, TRUE);
    g_string_free(header, TRUE);

    return;
}

void parse_colour_page_request(GString *response, client_connection *connection) {

    gchar *colour = "";
    GString *method = g_string_new(g_hash_table_lookup(connection->request->header_fields, "http_method"));
    GString *header = g_string_new("");
    GString *payload = g_string_new("");
    GString *html_body = g_string_new("");
    GString *body_text = g_string_new("");
    GString *body_options = g_string_new("");

    GPtrArray *cookie_array = g_ptr_array_new(); 

    int payload_length = 0;
    
    if(g_hash_table_contains(connection->request->queries, "bg")) {
        gchar *key = "bg";
        colour = g_hash_table_lookup(connection->request->queries, "bg");
        
        g_ptr_array_add(cookie_array, key);
        g_ptr_array_add(cookie_array, colour);
    }
    else if(g_hash_table_contains(connection->request->cookies, "bg")) {
        colour = g_hash_table_lookup(connection->request->cookies, "bg");
    }

    g_string_append_printf(body_options, "style=\"background-color:%s\"", colour);

    if(g_strcmp0(method->str, "POST") == 0) {
        g_string_append_printf(body_text, "%s", connection->request->payload->str);
    }
    if(g_strcmp0(method->str, "HEAD") != 0) {
        http_build_body(html_body, body_options->str, body_text->str);
        http_build_document(payload, "NAOUZ! colour page :)", html_body->str);
        payload_length = payload->len;
    }
    
    http_build_header(header, HTTP_STATUS_200, "text/html", cookie_array, payload_length, connection->keep_alive);

    g_string_append(response, header->str);
    g_string_append(response, payload->str);

    g_ptr_array_free(cookie_array, TRUE);
    g_string_free(method, TRUE);
    g_string_free(header, TRUE);
    g_string_free(payload, TRUE);
    g_string_free(html_body, TRUE);
    g_string_free(body_options, TRUE);
    g_string_free(body_text, TRUE);
    return;
}


void parse_generic_page_request(GString *response, client_connection *connection, gchar *host_name, gchar* uri) {

    gchar *method = g_hash_table_lookup(connection->request->header_fields, "http_method");
    GString *header = g_string_new("");
    GString *payload = g_string_new("");
    GString *html_body = g_string_new("");
    GString *body_text = g_string_new("");
    struct sockaddr_in connection_addr;
    int connection_addr_len;
    int payload_length = 0;

    connection_addr_len = sizeof(connection_addr);
    memset(&connection_addr, 0, connection_addr_len);

    //getpeername(working_client_connection->fd, (struct sockaddr*)&connection_addr , (socklen_t*)&connection_addr_len);

    getpeername(connection->fd, (struct sockaddr*)&connection_addr , (socklen_t*)&connection_addr_len);

    if(g_strcmp0(method, "GET") == 0 || g_strcmp0(method, "POST") == 0) {

        g_string_append_printf(body_text,
                               "http://%s%s %s:%d", 
                               host_name,
                               uri,
                               inet_ntoa(connection_addr.sin_addr), 
                               ntohs(connection_addr.sin_port));
    }

    if(g_strcmp0(method, "POST") == 0) {

        g_string_append_printf(body_text, "<br /><br />\n%s", connection->request->payload->str);

    }

    if(g_strcmp0(method, "HEAD") != 0) {

        http_build_body(html_body, "", body_text->str);
        http_build_document(payload, "NAOUZ! generic page :)", html_body->str);
        payload_length = payload->len;

    }

    http_build_header(header, HTTP_STATUS_200, "text/html", NULL, payload_length, connection->keep_alive);

    g_string_append(response, header->str);
    g_string_append(response, payload->str);

    g_string_free(header, TRUE);
    g_string_free(payload, TRUE);
    g_string_free(html_body, TRUE);
    g_string_free(body_text, TRUE);

    return;
}

void parse_header_page_request(GString *response, client_connection *connection) {
    GString *method = g_string_new(g_hash_table_lookup(connection->request->header_fields, "http_method"));
    GString *header = g_string_new("");
    GString *payload = g_string_new("");
    GString *html_body = g_string_new("");
    GString *body_text = g_string_new("");

    int payload_length = 0;

    g_hash_table_foreach(connection->request->header_fields, (GHFunc) g_string_fill_with_http_headers, body_text);

    if(g_strcmp0(method->str, "POST") == 0) {
        g_string_append_printf(body_text, "<br /><br />%s", connection->request->payload->str);
    }

    if(g_strcmp0(method->str, "HEAD") != 0) {
        http_build_body(html_body, "", body_text->str);
        http_build_document(payload, "NAOUZ! headers page :)", html_body->str);
        payload_length = payload->len;
    }
    
    http_build_header(header, HTTP_STATUS_200, "text/html", NULL, payload_length, connection->keep_alive);
    g_string_append(response, header->str);
    g_string_append(response, payload->str);


    g_string_free(method, TRUE);
    g_string_free(header, TRUE);
    g_string_free(payload, TRUE);
    g_string_free(html_body, TRUE);
    g_string_free(body_text, TRUE);

}

void parse_queries_page_request(GString *response, client_connection *connection, gchar *host_name, gchar *uri) {

    gchar *method = g_hash_table_lookup(connection->request->header_fields, "http_method");
    GString *header = g_string_new("");
    GString *payload = g_string_new("");
    GString *html_body = g_string_new("");
    GString *body_text = g_string_new("");
    struct sockaddr_in connection_addr;
    int connection_addr_len;
    int payload_length = 0;

    connection_addr_len = sizeof(connection_addr);
    memset(&connection_addr, 0, connection_addr_len);

    getpeername(connection->fd, (struct sockaddr*)&connection_addr , (socklen_t*)&connection_addr_len);

    if(g_strcmp0(method, "GET") == 0 || g_strcmp0(method, "POST") == 0) {

        g_string_append_printf(body_text,
                               "http://%s%s %s:%d", 
                               host_name,
                               uri,
                               inet_ntoa(connection_addr.sin_addr), 
                               ntohs(connection_addr.sin_port));

        g_string_append(body_text, "<br /><br />\n");
        g_hash_table_foreach(connection->request->queries, (GHFunc) g_string_fill_with_http_headers, body_text);
    }

    if(g_strcmp0(method, "POST") == 0) {

        g_string_append_printf(body_text, "<br /><br />\n%s", connection->request->payload->str);

    }

    if(g_strcmp0(method, "HEAD") != 0) {

        http_build_body(html_body, "", body_text->str);
        http_build_document(payload, "NAOUZ! queries page :)", html_body->str);
        payload_length = payload->len;

    }

    http_build_header(header, HTTP_STATUS_200, "text/html", NULL, payload_length, connection->keep_alive);

    g_string_append(response, header->str);
    g_string_append(response, payload->str);

    g_string_free(header, TRUE);
    g_string_free(payload, TRUE);
    g_string_free(html_body, TRUE);
    g_string_free(body_text, TRUE);

    return;
}