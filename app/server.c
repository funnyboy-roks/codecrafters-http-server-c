#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

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

	// You can use print statements as follows for debugging, they'll be visible when running tests.
    printf("Logs from your program will appear here!\n");

    struct sockaddr_in client_addr;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Socket creation failed: %s...\n", strerror(errno));
        return 1;
    }

    // Since the tester restarts your program quite often, setting REUSE_PORT
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
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
        if ((client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) == -1) {
            printf("ERROR: %m\n");
            exit(1);
        }

        char buf[256] = {0};
        ssize_t len = read(client_fd, buf, 256);

        if (len == -1) {
            printf("ERROR: %m\n");
            exit(1);
        }

        printf("buf = %s", buf);
        printf("buf = "); print_bytes(buf, len); printf("\n");

        char *response = "HTTP/1.1 200 OK\r\n\r\n";
        send(client_fd, response, strlen(response), 0);

        close(client_fd);
    }

    close(server_fd);

    return 0;
}
