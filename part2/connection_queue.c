#include "connection_queue.h"

#include <stdio.h>
#include <string.h>

int connection_queue_init(connection_queue_t *queue) {
    // TODO Not yet implemented
    queue->length = queue->read_idx = queue->write_idx = 0;
    queue->shutdown = 0;
    if (pthread_mutex_init(&queue->mutex, NULL) ||
        pthread_cond_init(&queue->not_empty, NULL) ||
        pthread_cond_init(&queue->not_full, NULL)) {
        return -1;
    }
    return 0;
}

int connection_queue_enqueue(connection_queue_t *queue, int connection_fd) {
    // TODO Not yet implemented
    pthread_mutex_lock(&queue->mutex);
    while (queue->length == CAPACITY && !queue->shutdown) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    queue->client_fds[queue->write_idx] = connection_fd;
    queue->write_idx = (queue->write_idx + 1) % CAPACITY;
    queue->length++;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

int connection_queue_dequeue(connection_queue_t *queue) {
    // TODO Not yet implemented
    pthread_mutex_lock(&queue->mutex);
    while (queue->length == 0 && !queue->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    if (queue->length == 0 && queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    int fd = queue->client_fds[queue->read_idx];
    queue->read_idx = (queue->read_idx + 1) % CAPACITY;
    queue->length--;
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    return fd;
}

int connection_queue_shutdown(connection_queue_t *queue) {
    // TODO Not yet implemented
    pthread_mutex_lock(&queue->mutex);
    queue->shutdown = 1;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

int connection_queue_free(connection_queue_t *queue) {
    // TODO Not yet implemented
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    pthread_mutex_destroy(&queue->mutex);
    return 0;
}
