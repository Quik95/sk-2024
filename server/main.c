#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

#include "common.h"
#include "connection.h"
#include "game.h"

#define LISTEN_PORT 2137

int main() {
    struct sockaddr_in server_sockaddr_in;
    server_sockaddr_in.sin_family = AF_INET;
    server_sockaddr_in.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_sockaddr_in.sin_port = htons(LISTEN_PORT);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (bind(sockfd, (struct sockaddr*) &server_sockaddr_in, sizeof(server_sockaddr_in)) < 0) {
        perror("bind has failed");
        return 1;
    }
    listen(sockfd, 5);

    printf("Starting accepting requests...\n");

    for (;;) {
        struct sockaddr_in client_sockaddr_in;
        socklen_t len = sizeof(client_sockaddr_in);

        int conn_fd = accept(sockfd, (struct sockaddr*) &client_sockaddr_in, &len);
        handle_connection(conn_fd);
    }

    close(sockfd);
    return 0;
}



