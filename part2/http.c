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
    // TODO Not yet implemented
    char buf[BUFSIZE];
    int total = 0, n;

    while (1) {
        n = read(fd, buf + total, sizeof(buf) - total - 1);
        if (n < 0) { perror("read"); return -1; }
        if (n == 0) break;
        total += n;
        buf[total] = '\0';
        if (strstr(buf, "\r\n\r\n")) break;
    }

    char method[16], path[256], ver[16];
    if (sscanf(buf, "%15s %255s %15s", method, path, ver) != 3) {
        return -1;
    }

    if (strcmp(method, "GET") != 0) return -1;
    strcpy(resource_name, path);

    return 0;
}

int write_http_response(int fd, const char *resource_path) {
    // TODO Not yet implemented
    struct stat st;

    if (stat(resource_path, &st) < 0) {
        const char *resp = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        write(fd, resp, strlen(resp));
        return 0;
    }

    int f = open(resource_path, O_RDONLY);
    if (f < 0) {
        perror("open"); return -1;
    }

    const char *ext = strrchr(resource_path, '.');
    const char *mime = ext ? get_mime_type(ext) : "application/octet-stream";
    char header[BUFSIZE];
    int hlen = snprintf(header, sizeof(header),
                        "HTTP/1.0 200 OK\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %ld\r\n"
                        "\r\n",
                        mime, st.st_size);

    if (write(fd, header, hlen) != hlen) {
        perror("write header");
        close(f);
        return -1;
    }

    char buf[BUFSIZE];
    int r;
    
    while ((r = read(f, buf, sizeof(buf))) > 0) {
        if (write(fd, buf, r) != r) {
            perror("write body");
            close(f);
            return -1;
        }
    }
    close(f);

    return 0;
}
