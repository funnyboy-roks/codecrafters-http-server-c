#include <stddef.h>

typedef struct {
    char *key;
    char *value;
} RequestHeader;

typedef struct {
    char *method;
    char *path;
    RequestHeader *headers;
    size_t headers_len;
    char *body;
    size_t body_len;
} Request;

Request parse_request(char *bytes, size_t len);
