#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "./request.h"

#define PANIC(...) do {                           \
    printf("[PANIC] %s:%d ", __FILE__, __LINE__); \
    printf(__VA_ARGS__);                          \
    printf("\n");                                 \
    exit(1);                                      \
} while (0);

void print_bytes(char *bytes, size_t len)
{
    printf("[");
    for (size_t i = 0; i < len; ++i) {
        printf("%02x", bytes[i]);
        if (i < len - 1) {
            printf(", ");
        }
    }
    printf("]\n");
}

int main(void)
{
	// Disable output buffering
	setbuf(stdout, NULL);

    struct sockaddr_in client_addr;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Socket creation failed: %s...\n", strerror(errno));
        return 1;
    }

    // Since the tester restarts your program quite often, setting REUSE_PORT
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse))) {
        printf("SO_REUSEPORT failed: %s \n", strerror(errno));
        return 1;
    }

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
    if (listen(server_fd, connection_backlog)) {
        printf("Listen failed: %s \n", strerror(errno));
        return 1;
    }

    printf("Waiting for a client to connect...\n");
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd;
    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) == -1) PANIC("%m");

        char buf[256] = {0};
        ssize_t len = read(client_fd, buf, 256);

        Request req = parse_request(buf, len);

        printf("req.method = '%s'\n", req.method);
        printf("req.path = '%s'\n", req.path);
        // printf("req.headers = '%s'", req.path);

        if (len == -1) PANIC("%m");

        printf("buf = %s", buf);
        printf("buf = "); print_bytes(buf, len); printf("\n");

        char *response;
        if (!strcmp(req.method, "GET") && !strcmp(req.path, "/")) {
            response = "HTTP/1.1 200 OK\r\n\r\n";
        } else {
            response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }

        send(client_fd, response, strlen(response), 0);

        close(client_fd);
    }

    close(server_fd);

    return 0;
}
