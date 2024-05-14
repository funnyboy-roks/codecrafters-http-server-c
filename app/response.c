#include "response.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *serres(Response res, size_t *len)
{
    const size_t buf_cap = 1024;
    char *out = malloc(buf_cap);

    char *ptr = out;

    ptr += snprintf(ptr,
        out + buf_cap - ptr,
        "HTTP/1.1 %d %s\r\n",
        res.status == 0 ? 200 : res.status,
        res.status_text == NULL ? "OK" : res.status_text
    );

    return out;
}
