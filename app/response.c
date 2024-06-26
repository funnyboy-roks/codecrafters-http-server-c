#include "response.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void serres(char *buf, Response res, size_t *len)
{
    const size_t buf_cap = 1024;

    *len = 0;

    *len += snprintf(buf + *len,
        buf_cap - *len,
        "HTTP/1.1 %d %s\r\n",
        res.status == 0 ? 200 : res.status,
        res.status_text == NULL ? "OK" : res.status_text
    );

    for (size_t i = 0; i < res.headers_len; ++i) {
        // "key: value\r\n"
        *len += snprintf(buf + *len,
            buf_cap - *len,
            "%s: %s\r\n",
            res.headers[i].key,
            res.headers[i].value
        );
    }

    if (res.body_len) {
        *len += snprintf(buf + *len,
            buf_cap - *len,
            "Content-Length: %ld\r\n",
            res.body_len
        );
    }

    *len += snprintf(buf + *len,
        buf_cap - *len,
        "\r\n"
    );

    memcpy(buf + *len, res.body, res.body_len);
    *len += res.body_len;
}
