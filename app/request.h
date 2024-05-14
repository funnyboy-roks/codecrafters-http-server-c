#include <stddef.h>

typedef struct {
    char *key;
    char *value;
} Header;

typedef struct {
    char *method;
    char *path;
    Header *headers;
    size_t headers_len;
    char *body;
    size_t body_len;
} Request;

char *serres(Response res, size_t *len);
