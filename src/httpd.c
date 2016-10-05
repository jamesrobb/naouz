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
#include "page.h"

int master_listen_port = 0;

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
    int client_addr_len;
    int select_activity;
    int incoming_sd_max; // max socket (file) descriptor for incoming connections
    fd_set incoming_fds;
    char data_buffer[DATA_BUFFER_LENGTH];
    struct sockaddr_in server_addr, client_addr;
    struct timeval select_timeout = {1, 0}; // 1 second
    client_connection *clients[MAX_CLIENT_CONNS];
    client_connection *working_client_connection; // client connecting we are currently dealing with

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

            // get ip address and port of working client
            getpeername(working_client_connection->fd, (struct sockaddr*)&client_addr , (socklen_t*)&client_addr_len);

            if(time(NULL) - working_client_connection->last_activity >= CONNECTION_TIMEOUT) {

                g_info("detected timeout on fd %d, ip %s, port %d", working_client_connection->fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                working_client_connection->close = TRUE;

            }

            if(working_client_connection->close == TRUE) {
                
                g_info("closing connection on socket fd %d, ip %s, port %d", working_client_connection->fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                
                shutdown(working_client_connection->fd, SHUT_RDWR);
                close(working_client_connection->fd);
                working_client_connection->fd = CONN_FREE;

                continue;
            }

            if(FD_ISSET(working_client_connection->fd, &incoming_fds)) {

                int read_val = read(working_client_connection->fd, data_buffer, DATA_BUFFER_LENGTH);
                getpeername(working_client_connection->fd, (struct sockaddr*)&client_addr , (socklen_t*)&client_addr_len);

                // is connecting being closed?
                if(read_val == 0) {

                    if(working_client_connection->keep_alive == TRUE) {
                        //g_info("read end of buffer on keep-alive connection on socket fd %d, ip %s, port %d", working_client_connection->fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    } else {
                        working_client_connection->close = TRUE;
                    }

                } else {

                    // updating the most recent time we have done something with this connection
                    working_client_connection->last_activity = time(NULL);

                    g_info("received some data from socket fd %d, ip %s, port %d", working_client_connection->fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    // parse the http request (also allocates all the needed objects). zero indicates success
                    int parse_ret = parse_client_http_request(working_client_connection, data_buffer);

                    GString *response = g_string_new("");
                    GString *host_name = g_string_new("");

                    http_request_get_hostname(host_name, working_client_connection->request->header_fields);

                    if(parse_ret == 0) {
                        
                        gchar *uri_path = g_hash_table_lookup(working_client_connection->request->header_fields, "uri_path");

                        if(g_strcmp0(uri_path, "/colour") == 0) {

                            parse_colour_page_request(response, working_client_connection);
                            g_info("sending 'colour' page");

                        } else if(g_strcmp0(uri_path, "/headers") == 0) {

                            parse_header_page_request(response, working_client_connection);
                            g_info("sending 'headers' page");

                        } else if(g_strcmp0(uri_path, "/test") == 0) {

                            parse_queries_page_request(response, working_client_connection, host_name->str, uri_path);
                            g_info("sending 'queries' page");

                        } else if(g_strcmp0(uri_path, "/favicon.ico") == 0) {

                            parse_favicon_request(response, working_client_connection);
                            g_info("sending 'favicon.ico'");

                        } else {

                            parse_generic_page_request(response, working_client_connection, host_name->str, uri_path);
                            g_info("sending 'generic' page");

                        }

                    } else {
                        // parsing the request yieled an error
                        build_bad_request_response(response);
                    }

                    httpd_log_access(inet_ntoa(client_addr.sin_addr),
                                         ntohs(client_addr.sin_port),
                                         g_hash_table_lookup(working_client_connection->request->header_fields, "http_method"),
                                         host_name->str,
                                         g_hash_table_lookup(working_client_connection->request->header_fields, "http_uri"),
                                         parse_ret == 0 ? HTTP_STATUS_200 : HTTP_STATUS_400);


                    reset_client_connection_http_request(working_client_connection);

                    if(send(working_client_connection->fd, response->str, response->len, 0) != (int) response->len){
                        g_critical("failed to send() welcome message on socket fd %d, ip %s, port %d", working_client_connection->fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    }


                    g_string_free(response, TRUE);
                    g_string_free(host_name, TRUE);

                    if(working_client_connection->keep_alive == FALSE) {
                        // we do a half-shutdown, and stop writes to the client
                        shutdown(working_client_connection->fd, SHUT_WR);
                    }
                }

            }
        }


    }

    shutdown(master_socket, SHUT_RDWR);
    close(master_socket);
	return 0;
}