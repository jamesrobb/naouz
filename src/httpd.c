#include <glib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "log.h"

#define MAX_CLIENT_CONNS    50
#define MAX_CONN_BACKLOG    10

int main(int argc, char *argv[]) {

    // we set a custom logging callback
    g_log_set_handler (NULL, 
                       G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, 
                       httpd_log_all_handler_cb, 
                       NULL);

    int master_socket, connfd, new_socket, client_socket[MAX_CLIENT_CONNS];
    struct sockaddr_in server, client;
    char buffer[512];

    // create and bind tcp socket
    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    // we use htons to convert data into network byte order
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(32000);
    bind(master_socket, (struct sockaddr *) &server, (socklen_t) sizeof(server));

    // we allow a backlog of MAX_CONN_BACKLOG
    if(listen(master_socket, MAX_CONN_BACKLOG)) {
        g_info("listening...");
    } else {
        g_critical("unable to listen for MAX_CONN_BACKLOG connections...");
    }


    return 0;





    for (;;) {
        /* We first have to accept a TCP connection, connfd is a fresh
           handle dedicated to this connection. */
        socklen_t len = (socklen_t) sizeof(client);
        int connfd = accept(master_socket, (struct sockaddr *) &client, &len);

        /* Receive from connfd, not sockfd. */
        ssize_t n = recv(connfd, buffer, sizeof(buffer) - 1, 0);

        buffer[n] = '\0';
        fprintf(stdout, "Received:\n%s\n", buffer);

        /* Send the message back. */
        send(connfd, buffer, (size_t) n, 0);

        /* Close the connection. */
        shutdown(connfd, SHUT_RDWR);
        close(connfd);
    }
	return 0;
}
