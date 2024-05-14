#include <stddef.h>

typedef struct {
    char *key;
    char *value;
} ResponseHeader;

typedef struct {
    int status;
    char *status_text;
    ResponseHeader *headers;
    size_t headers_len;
    char *body;
    size_t body_len;
} Response;

void serres(char *buf, Response res, size_t *len);
