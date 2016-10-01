#include <errno.h>
#include <glib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

#include "client_connection.h"
#include "constants.h"
#include "log.h"
#include "message.h"


int master_listen_port = 0;

void build_bad_request_response();

void parse_colour_page_request(GString *response, client_connection *connection, char* uri, char* data_buffer);

void parse_generic_page_request(GString *response, client_connection *connection, char* uri, char* data_buffer);

static GOptionEntry option_entries[] = {
  { "port", 'p', 0, G_OPTION_ARG_INT, &master_listen_port, "port to listen for connection on", "N" },
  { NULL }
};

int main(int argc, char *argv[]) {

    // we set a custom logging callback
    g_log_set_handler (NULL, 
                       G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, 
                       httpd_log_all_handler_cb, 
                       NULL);


    // gather commnad line argument info
    GError *option_error = NULL;
    GOptionContext *option_context;

    option_context = g_option_context_new ("- naouz http server");
    g_option_context_add_main_entries (option_context, option_entries, NULL);

    if (!g_option_context_parse (option_context, &argc, &argv, &option_error)) {
      g_critical ("option parsing failed: %s. Exiting...", option_error->message);
      exit (1);
    }

    if(master_listen_port == 0) {
        g_print("a port number to listen on must be specified\n");
        exit(1);
    }

    if(option_error != NULL) {
        g_error_free(option_error);
    }
    g_option_context_free(option_context);


    // begin setting up the server
    int master_socket;
    int new_socket;
    client_connection *clients[MAX_CLIENT_CONNS];
    client_connection *working_client_connection; // client connecting we are currently dealing with
    int client_addr_len;
    int select_activity;
    int incoming_sd_max; // max socket (file) descriptor for incoming connections
    struct sockaddr_in server_addr, client_addr;
    char data_buffer[DATA_BUFFER_LENGTH];
    fd_set incoming_fds;
    struct timeval select_timeout = {1, 0}; // 1 second

    // we initialize clients sockect fds
    for(int i = 0; i < MAX_CLIENT_CONNS; i++) {
        clients[i] = malloc(sizeof(client_connection));
        reset_client_connection(clients[i]);
    }

    // create and bind tcp socket
    master_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(master_socket == -1) {
        g_critical("unable to create tcp socket. Exiting...");
        exit(1);   
    }

    // we set our master socket to allow multiple connections and resuse 
    
    // we use htons to convert data into network byte order
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(master_listen_port);
    bind(master_socket, (struct sockaddr *) &server_addr, (socklen_t) sizeof(server_addr));

    // we allow a backlog of MAX_CONN_BACKLOG
    if(listen(master_socket, MAX_CONN_BACKLOG) == 0) {
        g_info("listening...");
    } else {
        // we were unable to listen for as many backlogged connections as was desired
        GString *listen_error = g_string_new("");
        g_string_printf(listen_error, "%s", strerror(errno));

        g_critical("%s", listen_error->str);
        g_critical("unable to listen for MAX_CONN_BACKLOG connections. Exiting...");
        g_string_free(listen_error, TRUE);
        exit(1);
    }

    while (TRUE) {

        // zero out client address info and data buffer
        memset(data_buffer, 0, DATA_BUFFER_LENGTH);
        memset(&client_addr, 0, sizeof(client_addr));

        // set appropriate client address length (otherwise client_addr isn't populated correctly by accept())
        client_addr_len = sizeof(client_addr);

        FD_ZERO(&incoming_fds);
        FD_SET(master_socket, &incoming_fds);
        incoming_sd_max = master_socket;
        select_timeout.tv_sec = 1;
        for(int i = 0; i < MAX_CLIENT_CONNS; i++) {

            working_client_connection = clients[i];

            if(working_client_connection->fd > CONN_FREE) {
                FD_SET(working_client_connection->fd, &incoming_fds);
            }

            if(working_client_connection->fd > incoming_sd_max) {
                incoming_sd_max = working_client_connection->fd;
            }

        }

        select_activity = select(incoming_sd_max + 1, &incoming_fds, NULL, NULL, &select_timeout);
        //g_print("%d\n", select_activity);
        //g_info("select() timout or activity");

        // this is done to handle the case where select was not interrupted (ctrl+c) but returned an error.
        if(errno != EINTR && select_activity < 0){
            g_warning("select() error");
        }

        // new incoming connection
        if(FD_ISSET(master_socket, &incoming_fds)) {
            
            new_socket = accept(master_socket, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len);

            if(new_socket > -1) {

                g_info("new connection on socket fd %d, ip %s, port %d", new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                bool found_socket = FALSE;
                for(int i = 0; i < MAX_CLIENT_CONNS; i++) {

                    if(clients[i]->fd == CONN_FREE) {
                        reset_client_connection(clients[i]);
                        clients[i]->fd = new_socket;
                        found_socket = TRUE;
                        break;
                    }

                }

                if(found_socket == FALSE) {
                    g_critical("no free client connection slots available, unable to track this connection");
                    close(new_socket);
                }

            } else {

                g_critical("error establishing connection to client");

            }
        }

        // we check our client connections to activity
        for(int i = 0; i < MAX_CLIENT_CONNS; i++) {

            working_client_connection = clients[i];

            if(working_client_connection->fd == CONN_FREE) {
                continue;
            }

            if(FD_ISSET(working_client_connection->fd, &incoming_fds)) {

                int read_val = read(working_client_connection->fd, data_buffer, DATA_BUFFER_LENGTH);
                getpeername(working_client_connection->fd, (struct sockaddr*)&client_addr , (socklen_t*)&client_addr_len);

                // is connecting being closed?
                if(read_val == 0) {

                    g_info("closing connection on socket fd %d, ip %s, port %d", working_client_connection->fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    clients[i]->fd = CONN_FREE;

                } else {

                    g_info("received some data from socket fd %d, ip %s, port %d", working_client_connection->fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    // parse the http request (also allocates all the needed objects). zero indicates success
                    int parse_ret = parse_client_http_request(working_client_connection, data_buffer);

                    if(parse_ret == 0) {
                        http_request_print(working_client_connection->request);
                    }

                    GString *response = g_string_new("");
                    GString *host_name = g_string_new("");

                    http_request_get_hostname(host_name, working_client_connection->request->header_fields);

                    if(parse_ret == 0) {
                        
                        GString *uri_path = g_string_new(g_hash_table_lookup(working_client_connection->request->header_fields, "uri_path"));

                        // outputting query key/value pairs
                        //g_hash_table_foreach(working_client_connection->request->queries, (GHFunc)ghash_table_strstr_iterator, "QUERIES - key: %s, value: %s\n");

                        if(g_strcmp0(uri_path->str, "/colour") == 0) {
                            parse_colour_page_request(response, working_client_connection, uri_path->str, data_buffer);
                            g_info("trying to send 'colour'");
                        } else {
                            parse_generic_page_request(response, working_client_connection, uri_path->str, data_buffer);
                            g_info("trying to send 'page'");
                        }

                        g_string_free(uri_path, TRUE);

                    } else {
                        // prasing the request yieled an error

                        build_bad_request_response(response);
                    }

                    httpd_log_access(inet_ntoa(client_addr.sin_addr),
                                         ntohs(client_addr.sin_port),
                                         g_hash_table_lookup(working_client_connection->request->header_fields, "http_method"),
                                         host_name->str,
                                         g_hash_table_lookup(working_client_connection->request->header_fields, "http_uri"),
                                         parse_ret == 0 ? HTTP_STATUS_200 : HTTP_STATUS_400);


                    reset_client_connection_http_request(working_client_connection);

                    if(send(working_client_connection->fd, response->str, response->len, 0) != response->len){
                        g_critical("failed to send() welcome message on socket fd %d, ip %s, port %d", working_client_connection->fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    }

                    g_string_free(response, TRUE);
                    g_string_free(host_name, TRUE);
                }

            }
        }


        // /* We first have to accept a TCP connection, connfd is a fresh
        //    handle dedicated to this connection. */
        // socklen_t len = (socklen_t) sizeof(client_addr);
        // int connfd = accept(master_socket, (struct sockaddr *) &client_addr, &len);

        // /* Receive from connfd, not sockfd. */
        // ssize_t n = recv(connfd, buffer, sizeof(buffer) - 1, 0);

        // buffer[n] = '\0';
        // fprintf(stdout, "Received:\n%s\n", buffer);

        // /* Send the message back. */
        // send(connfd, buffer, (size_t) n, 0);

        // /* Close the connection. */
        // shutdown(connfd, SHUT_RDWR);
        // close(connfd);
    }

    shutdown(master_socket, SHUT_RDWR);
    close(master_socket);
	return 0;
}

void build_bad_request_response(GString *response) {

    GString *header = g_string_new("");
    GString *payload = g_string_new("");
    GString *html_body = g_string_new("");
    GString *body_text = g_string_new("400 Bad Request");
    int payload_length = 0;

    http_build_body(html_body, "", body_text->str);
    http_build_document(payload, "NAOUZ - 400 Bad Request", html_body->str);
    payload_length = payload->len;

    http_build_header(header, HTTP_STATUS_400, payload_length, NULL);

    g_string_append(response, header->str);
    g_string_append(response, payload->str);

    g_string_free(header, TRUE);
    g_string_free(payload, TRUE);
    g_string_free(html_body, TRUE);
    g_string_free(body_text, TRUE);

    return;
}


void parse_colour_page_request(GString *response, client_connection *connection, char* uri, char* data_buffer) {

    gchar *colour = "";
    GString *method = g_string_new(g_hash_table_lookup(connection->request->header_fields, "http_method"));
    GString *header = g_string_new("");
    GString *payload = g_string_new("");
    GString *html_body = g_string_new("");
    GString *body_text = g_string_new("");
    GString *body_options = g_string_new("");

    GPtrArray *cookie_array = g_ptr_array_new(); 

    int payload_length = 0;
    if(g_hash_table_contains(connection->request->queries, "colour")) {
        gchar *key = "colour";
        colour = g_hash_table_lookup(connection->request->queries, "colour");
        
        g_ptr_array_add(cookie_array, key);
        g_ptr_array_add(cookie_array, colour);
    }
    else if(g_hash_table_contains(connection->request->cookies, "colour")) {
        colour = g_hash_table_lookup(connection->request->cookies, "colour");
    }


    g_string_append_printf(body_options, "style=\"background-color:%s\"", colour);

    if(g_strcmp0(method->str, "POST") == 0) {
        g_string_append_printf(body_text, "<br><br>%s", data_buffer);
    }
    http_build_body(html_body, body_options->str, body_text->str);
    http_build_document(payload, "NAOUZ! colour page :)", html_body->str);

    payload_length = payload->len;
    http_build_header(header, HTTP_STATUS_200, payload_length, cookie_array);

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


void parse_generic_page_request(GString *response, client_connection *connection, char* uri, char* data_buffer) {

    GString *method = g_string_new(g_hash_table_lookup(connection->request->header_fields, "http_method"));
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

    if(g_strcmp0(method->str, "GET") == 0 || g_strcmp0(method->str, "POST") == 0) {

        g_string_append_printf(body_text,
                               "http://%s%s %s:%d", 
                               g_get_host_name(),
                               uri,
                               inet_ntoa(connection_addr.sin_addr), 
                               ntohs(connection_addr.sin_port));
    }

    if(g_strcmp0(method->str, "POST") == 0) {

        g_string_append_printf(body_text, "<br /><br />\n%s", connection->request->payload->str);

    }

    if(g_strcmp0(method->str, "HEAD") != 0) {

        http_build_body(html_body, "", body_text->str);
        http_build_document(payload, "NAOUZ! query page :)", html_body->str);
        payload_length = payload->len;

    }

    http_build_header(header, HTTP_STATUS_200, payload_length, NULL);

    g_string_append(response, header->str);
    g_string_append(response, payload->str);

    g_string_free(method, TRUE);
    g_string_free(header, TRUE);
    g_string_free(payload, TRUE);
    g_string_free(html_body, TRUE);
    g_string_free(body_text, TRUE);
    return;
}