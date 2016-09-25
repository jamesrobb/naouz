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

#include "log.h"
#include "request.h"

#define MAX_CLIENT_CONNS    50
#define MAX_CONN_BACKLOG    10
#define DATA_BUFFER_LENGTH  77056 // 64KB
#define CONN_FREE           -1
#define NAOUZ_VERSION       ((gchar*) "0.2")

int main(int argc, char *argv[]) {

    // we set a custom logging callback
    g_log_set_handler (NULL, 
                       G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, 
                       httpd_log_all_handler_cb, 
                       NULL);

    int master_socket;
    int new_socket;
    int clients[MAX_CLIENT_CONNS];
    int client_addr_len;
    int select_activity;
    int incoming_sd_max; // max socket (file) descriptor for incoming connections
    int working_sd; // socket descriptor currently being operated on
    struct sockaddr_in server_addr, client_addr;
    char data_buffer[DATA_BUFFER_LENGTH];
    fd_set incoming_fds;
    struct timeval select_timeout = {1, 0}; // 1 second

    // we initialize clients sockect fds
    for(int i = 0; i < MAX_CLIENT_CONNS; i++) {
        clients[i] = CONN_FREE;
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
    server_addr.sin_port = htons(32000);
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

            working_sd = clients[i];

            if(working_sd > CONN_FREE) {
                FD_SET(working_sd, &incoming_fds);
            }

            if(working_sd > incoming_sd_max) {
                incoming_sd_max = working_sd;
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

                    if(clients[i] == CONN_FREE) {
                        clients[i] = new_socket;
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

            working_sd = clients[i];

            if(working_sd == CONN_FREE) {
                continue;
            }

            if(FD_ISSET(working_sd, &incoming_fds)) {

                int read_val = read(working_sd, data_buffer, DATA_BUFFER_LENGTH);
                getpeername(working_sd, (struct sockaddr*)&client_addr , (socklen_t*)&client_addr_len);

                // is connecting being closed?
                if(read_val == 0) {

                    g_info("closing connection on socket fd %d, ip %s, port %d", working_sd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    clients[i] = CONN_FREE;

                } else {

                    g_info("received some data from socket fd %d, ip %s, port %d", working_sd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    http_request request;
                    int request_ret_val = parse_http_request(data_buffer, &request);

                    if(request_ret_val == 0) {
                        http_request_print(&request);
                    }

                    g_hash_table_destroy(request.value_table);

                    GString *welcome = g_string_new("HTTP/1.1 200 OK\n");
                    GString *welcome_payload = g_string_new("welcome to naouz!");
                    g_string_append_printf(welcome, "Connection: close\n");
                    g_string_append_printf(welcome, "Server: naouz/%s\n", NAOUZ_VERSION);
                    g_string_append_printf(welcome, "Accept-Ranges: bytes\n");
                    g_string_append_printf(welcome, "Content-Type: text/html\n");
                    g_string_append_printf(welcome, "Content-Length: %d\n\n", (int) welcome_payload->len);
                    g_string_append_printf(welcome, "%s", welcome_payload->str);

                    if(send(working_sd, welcome->str, welcome->len, 0) != welcome->len){
                        g_critical("failed to send() welcome message on socket fd %d, ip %s, port %d", working_sd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    }

                    g_string_free(welcome, TRUE);
                    g_string_free(welcome_payload, TRUE);

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
	return 0;
}
