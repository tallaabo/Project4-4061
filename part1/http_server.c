#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5

int keep_going = 1;

void handle_sigint(int signo) {
    keep_going = 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }

    const char *serve_dir = argv[1];
    const char *port = argv[2];

    // Setup SIGINT handler
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // Setup server socket
    struct addrinfo hints, *res;
    int server_fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_fd < 0) {
        perror("socket");
        freeaddrinfo(res);
        return 1;
    }

    if (bind(server_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind");
        close(server_fd);
        freeaddrinfo(res);
        return 1;
    }

    freeaddrinfo(res);

    if (listen(server_fd, LISTEN_QUEUE_LEN) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    // Main server loop
    while (keep_going) {
        int conn_fd = accept(server_fd, NULL, NULL);
        if (conn_fd < 0) {
            if (errno == EINTR) {
                break;
            }
            perror("accept");
            continue;
        }

        char resource_name[BUFSIZE];
        if (read_http_request(conn_fd, resource_name) == 0) {
            char full_path[BUFSIZE * 2];
            snprintf(full_path, sizeof(full_path), "%s%s", serve_dir, resource_name);
            write_http_response(conn_fd, full_path);
        }

        close(conn_fd);
    }

    close(server_fd);
    return 0;
}
