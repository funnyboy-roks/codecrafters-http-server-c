#include "request.h"

Request parse_request(char *bytes, size_t len)
{
    Request out = {0};

    out.method = bytes;
    int i;
    for (i = 0; bytes[i] != ' '; ++i);
    bytes[i] = '\0';

    out.path = bytes + i + 1;
    for (i = 0; bytes[i] != ' '; ++i);
    bytes[i] = '\0';

    return out;
}

