#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <zlib.h>

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

int compressToGzip(const char* input, int inputSize, char* output, int outputSize)
{
    z_stream zs = {0};
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.avail_in = (uInt)inputSize;
    zs.next_in = (Bytef *)input;
    zs.avail_out = (uInt)outputSize;
    zs.next_out = (Bytef *)output;

    deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
    deflate(&zs, Z_FINISH);
    deflateEnd(&zs);
    return zs.total_out;
}

void print_headers(void *head, size_t len) {
    printf("{\n");
    RequestHeader *headers = (RequestHeader *)head;
    for (size_t i = 0; i < len; ++i) {
        RequestHeader header = headers[i];
        printf("    %s: %s\n", header.key, header.value);
    }
    printf("}\n");
}

char *get_header(void *headers, size_t len, char *name) {
    RequestHeader *hs = (RequestHeader *)headers;
    for (size_t i = 0; i < len; ++i) {
        RequestHeader header = hs[i];
        if (!strcmp(header.key, name)) {
            return header.value;
        }
    }
    return NULL;
}

void print_bytes(char *by, size_t len)
{
    unsigned char *bytes = (unsigned char *) by;
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
        if (!strcmp(argv[i], "--directory")) {
            dir = argv[i + 1];
        }
    }

    DBG("dir = %s", dir);

    
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

        printf("buf = %s\n", buf);
        printf("buf = "); print_bytes(buf, len); printf("\n");

        Request req = parse_request(buf, len);

        printf("req.method = '%s'\n", req.method);
        printf("req.path = '%s'\n", req.path);
        printf("req.headers = ");
        print_headers(req.headers, req.headers_len);
        printf("req.body = "); print_bytes(req.body, req.body_len); printf("\n");

        char *response = malloc(1024);
        size_t res_len;
        DBG("%s - %s", req.method, req.path);
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
        } else if (!strcmp(req.method, "POST") && !strncmp(req.path, "/files/", sizeof("/files/") - 1)) {
            char *file = req.path + sizeof("/files/") - 1;

            char full_path[256];
            sprintf(full_path, "%s/%s", dir, file);

            DBG("full_path = %s", full_path);

            Response res = {0};

            res.status = 201;
            res.status_text = "Created";

            FILE *f = fopen(full_path, "wb");
            if (f == NULL) {
                PANIC("Cannot open file for writing %s: %m", full_path);
            } else {
                if(!fwrite(req.body, 1, req.body_len, f)) PANIC("Unable to write to file %s: %m", full_path);
            }
            if (f) fclose(f);
            serres(response, res, &res_len);
        } else if (!strcmp(req.method, "GET") && !strncmp(req.path, "/files/", sizeof("/files/") - 1)) {
            char *file = req.path + sizeof("/files/") - 1;

            char full_path[256];
            sprintf(full_path, "%s/%s", dir, file);

            DBG("full_path = %s", full_path);

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

            FILE *f = fopen(full_path, "rb");
            if (access(full_path, F_OK)) {
                res_len = sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
                DBG("Can not open file for reading %s: %m", full_path);
            } else {
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
            if (f) fclose(f);
        } else if (!strcmp(req.method, "GET") && !strncmp(req.path, "/echo/", sizeof("/echo/") - 1)) {
            char *s = req.path + sizeof("/echo/") - 1;
            Response res = {0};
            ResponseHeader headers[2] = {
                [0]={
                    .key = "Content-Type",
                    .value = "text/plain",
                },
                [1]={0}
            };

            res.body = malloc(1024);

            res.headers = headers;
            res.headers_len = 1;

            char *ce = get_header(req.headers, req.headers_len, "accept-encoding");

            bool set_body = false;
            if (ce) {
                bool has_gzip = false;
                for (char *t;(t = strtok(ce, ", ")); ce = NULL) {
                    if (!strcmp(t, "gzip")) {
                        has_gzip = true;
                        break;
                    }
                }

                if (has_gzip) {
                    int len = compressToGzip(s, strlen(s), res.body, 1024);
                    if (len < 0) {
                        PANIC("Compression failed");
                    }
                    res.headers[res.headers_len].key = "Content-Encoding";
                    res.headers[res.headers_len++].value = "gzip";
                    DBG("len = %d", len);
                    res.body_len = len;
                    set_body = true;
                }
            }

            if (!set_body) {
                memcpy(res.body, s, res.body_len = strlen(s));
                printf("res.body = %s\n", res.body);
            } else {
                printf("res.body = ");
                print_bytes(res.body, res.body_len);
                printf("\n");
            }

            printf("res.headers = ");
            print_headers(headers, res.headers_len);

            serres(response, res, &res_len);
            free(res.body);
        } else if (!strcmp(req.method, "GET") && !strcmp(req.path, "/")) {
            res_len = sprintf(response, "HTTP/1.1 200 OK\r\n\r\n");
        } else {
            res_len = sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
        }

        send(client_fd, response, res_len, 0);

        close(client_fd);
    }

    close(server_fd);

    return 0;
}
