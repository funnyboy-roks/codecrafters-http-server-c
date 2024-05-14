#include <stddef.h>

typedef struct {
    char *key;
    char *value;
} Header;

typedef struct {
    int status;
    char *status_text;
    Header *headers;
    size_t headers_len;
    char *body;
    size_t body_len;
} Response;

char *serres(Response res, size_t *len);
