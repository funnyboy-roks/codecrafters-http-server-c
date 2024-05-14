#include <stdlib.h>

#include "request.h"

void lower(char *str)
{
    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] += 'a' - 'A';
        }
    }
}

Request parse_request(char *bytes, size_t len)
{
    Request out = {0};

    out.method = bytes;
    int i = 0;
    for (; bytes[i] != ' '; ++i);
    bytes[i] = '\0';

    out.path = bytes + i + 1;
    for (; bytes[i] != ' '; ++i);
    bytes[i] = '\0';

    i += sizeof("HTTP/1.1\r\n");

    size_t headers_cap = 5;
    out.headers = malloc(headers_cap * sizeof(*out.headers));

    while (bytes[i] != '\r') {
        RequestHeader header = {0};

        header.key = bytes + i;
        for (; bytes[i] != ':'; ++i);
        bytes[i++] = '\0';
        bytes[i++] = '\0';

        header.value = bytes + i;
        for (; bytes[i] != '\r'; ++i);
        bytes[i++] = '\0';
        bytes[i++] = '\0';

        lower(header.key);

        if (out.headers_len == headers_cap) {
            headers_cap *= 2;
            out.headers = realloc(out.headers, headers_cap * sizeof(*out.headers));
        }
        out.headers[out.headers_len++] = header;
    }

    return out;
}

