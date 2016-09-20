/* your code goes here. */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	int sockfd;
    struct sockaddr_in server, client;
    char buffer[512];
    /* Create and bind a TCP socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    /* Network functions need arguments in network byte order instead of
       host byte order. The macros htonl, htons convert the values. */
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(32000);
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));
    /* Before the server can accept messages, it has to listen to the
       welcome port. A backlog of one connection is allowed. */
    listen(sockfd, 1);

    for (;;) {
        /* We first have to accept a TCP connection, connfd is a fresh
           handle dedicated to this connection. */
        socklen_t len = (socklen_t) sizeof(client);
        int connfd = accept(sockfd, (struct sockaddr *) &client, &len);

        /* Receive from connfd, not sockfd. */
        ssize_t n = recv(connfd, buffer, sizeof(message) - 1, 0);

        buffer[n] = '\0';
        fprintf(stdout, "Received:\n%s\n", message);

        /* Send the message back. */
        send(connfd, buffer, (size_t) n, 0);

        /* Close the connection. */
        shutdown(connfd, SHUT_RDWR);
        close(connfd);
    }
	return 0;
}
