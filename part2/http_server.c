#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection_queue.h"
#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5
#define N_THREADS 5

int keep_going = 1;
const char *serve_dir;
static connection_queue_t queue;

void handle_sigint(int signo) {
    keep_going = 0;
}

void *worker_thread(void *_) {
    while (1) {
        int conn = connection_queue_dequeue(&queue);
        if (conn < 0) break;  // shutdown
        char resource[BUFSIZE], fullpath[BUFSIZE*2];
        if (read_http_request(conn, resource) == 0) {
            snprintf(fullpath, sizeof(fullpath), "%s%s", serve_dir, resource);
            write_http_response(conn, fullpath);
        }
        close(conn);
    }
    return NULL;
}

int main(int argc, char **argv) {
    // First argument is directory to serve, second is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    // Uncomment the lines below to use these definitions:
    serve_dir = argv[1];
    const char *port = argv[2];

    // TODO Implement the rest of this function

    // 1) Install SIGINT handler
    struct sigaction sa = { .sa_handler = handle_sigint };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // 2) Block all signals in this thread BEFORE creating workers
    sigset_t full, prev;
    sigfillset(&full);
    sigprocmask(SIG_SETMASK, &full, &prev);

    // 3) Init queue and spawn worker threads
    if (connection_queue_init(&queue) != 0) {
        perror("queue init");
        return 1;
    }
    pthread_t threads[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    // 4) Restore signal mask so main thread catches SIGINT
    sigprocmask(SIG_SETMASK, &prev, NULL);

    // 5) Setup listening socket
    struct addrinfo hints = { .ai_family = AF_INET,
                              .ai_socktype = SOCK_STREAM,
                              .ai_flags = AI_PASSIVE }, *res;
    int server_fd;
    if (getaddrinfo(NULL, port, &hints, &res) != 0 ||
        (server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0 ||
        bind(server_fd, res->ai_addr, res->ai_addrlen) < 0 ||
        listen(server_fd, LISTEN_QUEUE_LEN) < 0) {
        perror("setup server");
        return 1;
    }
    freeaddrinfo(res);

    // 6) Main accept loop
    while (keep_going) {
        int conn = accept(server_fd, NULL, NULL);
        if (conn < 0) {
            if (errno == EINTR) break;
            perror("accept");
            continue;
        }
        if (connection_queue_enqueue(&queue, conn) < 0) {
            close(conn);
            break;
        }
    }

    // 7) Shutdown
    close(server_fd);
    connection_queue_shutdown(&queue);
    for (int i = 0; i < N_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    connection_queue_free(&queue);

    return 0;
}
