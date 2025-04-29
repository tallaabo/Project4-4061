#include "http.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFSIZE 512

const char *get_mime_type(const char *file_extension) {
    if (strcmp(".txt", file_extension) == 0) {
        return "text/plain";
    } else if (strcmp(".html", file_extension) == 0) {
        return "text/html";
    } else if (strcmp(".jpg", file_extension) == 0) {
        return "image/jpeg";
    } else if (strcmp(".png", file_extension) == 0) {
        return "image/png";
    } else if (strcmp(".pdf", file_extension) == 0) {
        return "application/pdf";
    } else if (strcmp(".mp3", file_extension) == 0) {
        return "audio/mpeg";
    }

    return NULL;
}

int read_http_request(int fd, char *resource_name) {
    char buffer[BUFSIZE];
    int total_bytes = 0;
    int bytes_read;

    while (1) {
        bytes_read = read(fd, buffer + total_bytes, sizeof(buffer) - total_bytes - 1);
        if (bytes_read < 0) {
            perror("read");
            return -1;
        }
        if (bytes_read == 0) {
            break;
        }
        total_bytes += bytes_read;
        buffer[total_bytes] = '\0';

        if (strstr(buffer, "\r\n\r\n") != NULL) {
            break;
        }
    }

    char method[16];
    char path[256];
    char version[16];

    if (sscanf(buffer, "%s %s %s", method, path, version) != 3) {
        return -1;
    }

    if (strcmp(method, "GET") != 0) {
        return -1;
    }

    strcpy(resource_name, path);
    return 0;
}

int write_http_response(int fd, const char *resource_path) {
    struct stat st;
    if (stat(resource_path, &st) != 0) {
        const char *not_found_response = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        write(fd, not_found_response, strlen(not_found_response));
        return 0;
    }

    int file_fd = open(resource_path, O_RDONLY);
    if (file_fd < 0) {
        perror("open");
        return -1;
    }

    const char *extension = strrchr(resource_path, '.');
    const char *mime_type;
    if (extension == NULL) {
        mime_type = "application/octet-stream";
    } else {
        mime_type = get_mime_type(extension);
    }

    char header[BUFSIZE];
    int header_len = snprintf(header, sizeof(header),
                              "HTTP/1.0 200 OK\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %ld\r\n"
                              "\r\n",
                              mime_type, st.st_size);

    if (write(fd, header, header_len) != header_len) {
        perror("write header");
        close(file_fd);
        return -1;
    }

    char buffer[BUFSIZE];
    int bytes;
    while ((bytes = read(file_fd, buffer, sizeof(buffer))) > 0) {
        if (write(fd, buffer, bytes) != bytes) {
            perror("write body");
            close(file_fd);
            return -1;
        }
    }

    close(file_fd);
    return 0;
}
