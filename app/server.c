#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "./request.h"
#include "./response.h"

#define DBG(...) do {                             \
    printf("[DBG] %s:%d ", __FILE__, __LINE__);   \
    printf(__VA_ARGS__);                          \
    printf("\n");                                 \
} while (0);

#define PANIC(...) do {                           \
    printf("[PANIC] %s:%d ", __FILE__, __LINE__); \
    printf(__VA_ARGS__);                          \
    printf("\n");                                 \
    exit(1);                                      \
} while (0);

void print_headers(void *head, size_t len) {
    printf("{\n");
    RequestHeader *headers = (RequestHeader *) head;
    for (size_t i = 0; i < len; ++i) {
        RequestHeader header = headers[i];
        printf("    %s: %s\n", header.key, header.value);
    }
    printf("}\n");
}

void print_bytes(char *bytes, size_t len)
{
    printf("[");
    for (size_t i = 0; i < len; ++i) {
        printf("%02x", bytes[i]);
        if (i < len - 1) {
            printf(", ");
        }
    }
    printf("]");
}

int main(int argc, char **argv)
{
    char *dir = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--directory")) {
            dir = argv[i + 1];
        }
    }
	// Disable output buffering
	setbuf(stdout, NULL);

    struct sockaddr_in client_addr;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) PANIC("Socket creation failed: %m...\n");

    // Since the tester restarts your program quite often, setting REUSE_PORT
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse))) PANIC("SO_REUSEPORT failed: %m \n");

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET ,
        .sin_port = htons(4221),
        .sin_addr = { htonl(INADDR_ANY) },
    };

    if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) {
        printf("Bind failed: %s \n", strerror(errno));
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog)) PANIC("Listen failed: %m \n");

    printf("Waiting for a client to connect...\n");
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd;
    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) == -1) PANIC("%m");

        char buf[256] = {0};
        ssize_t len = read(client_fd, buf, 256);

        if (len == -1) PANIC("%m");

        printf("buf = %s", buf);
        printf("buf = "); print_bytes(buf, len); printf("\n");

        Request req = parse_request(buf, len);

        printf("req.method = '%s'\n", req.method);
        printf("req.path = '%s'\n", req.path);
        printf("req.headers = ");
        print_headers(req.headers, req.headers_len);

        char *response = malloc(1024);
        size_t res_len;
        if (!strcmp(req.method, "GET") && !strcmp(req.path, "/user-agent")) {
            Response res = {0};
            ResponseHeader headers[] = {
                {
                    .key = "Content-Type",
                    .value = "text/plain",
                }
            };
            res.headers = headers;
            res.headers_len = sizeof(headers) / sizeof(*headers);
            printf("res.headers = ");
            print_headers(headers, 1);

            for(size_t i = 0; i < req.headers_len; ++i) {
                if (!strcmp(req.headers[i].key, "user-agent")) {
                    res.body = req.headers[i].value;
                    res.body_len = strlen(res.body);
                }
            }

            printf("res.body = %s\n", res.body);

            serres(response, res, &res_len);
            printf("res = %.*s", (int) res_len, response);
        } else if (!strcmp(req.method, "GET") && !strncmp(req.path, "/files/", sizeof("/files/") - 1)) {
            char *file = req.path + sizeof("/files/") - 1;

            DBG("file = %s", file);

            Response res = {0};
            ResponseHeader headers[] = {
                {
                    .key = "Content-Type",
                    .value = "application/octet-stream",
                }
            };
            res.headers = headers;
            res.headers_len = sizeof(headers) / sizeof(*headers);

            printf("res.headers = ");
            print_headers(headers, 1);

            FILE *f = fopen(file, "rb");
            if (access(file, F_OK)) {
                res_len = sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
            } else {
                if (f == NULL) {
                    PANIC("Can not open file for reading %s: %m", file);
                }
                fseek(f, 0, SEEK_END);
                long fsize = ftell(f);
                fseek(f, 0, SEEK_SET);
                response = realloc(response, 1024 + fsize);
                res.body = malloc(fsize);
                res.body_len = fread(res.body, 1, fsize, f);

                printf("res.body = %s\n", res.body);

                serres(response, res, &res_len);
                printf("res = %.*s", (int) res_len, response);
                free(res.body);
            }
        } else if (!strcmp(req.method, "GET") && !strncmp(req.path, "/echo/", sizeof("/echo/") - 1)) {
            char *s = req.path + sizeof("/echo/") - 1;
            Response res = {0};
            ResponseHeader headers[] = {
                {
                    .key = "Content-Type",
                    .value = "text/plain",
                }
            };
            res.headers = headers;
            res.headers_len = sizeof(headers) / sizeof(*headers);
            printf("res.headers = ");
            print_headers(headers, 1);

            res.body = s;
            res.body_len = strlen(s);
            printf("res.body = %s\n", res.body);

            serres(response, res, &res_len);
            printf("res = %.*s", (int) res_len, response);
        } else if (!strcmp(req.method, "GET") && !strcmp(req.path, "/")) {
            res_len = sprintf(response, "HTTP/1.1 200 OK\r\n\r\n");
        } else {
            res_len = sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
        }

        send(client_fd, response, strlen(response), 0);

        close(client_fd);
    }

    close(server_fd);

    return 0;
}
