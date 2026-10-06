/* SPDX-License-Identifier: MIT */
#include "proxy_protocol.h"
#include <stdio.h>
#include <string.h>

bool passport_proxy_parse(const char *value, passport_proxy_endpoint *out)
{
    if (!value || !out) return false;
    const char *p = value;
    for (unsigned i = 0; i < 4; ++i) {
        unsigned n = 0, digits = 0;
        const char *start = p;
        while (*p >= '0' && *p <= '9') {
            n = n * 10 + (unsigned)(*p++ - '0');
            if (++digits > 3 || n > 255) return false;
        }
        if (!digits || (digits > 1 && *start == '0')) return false;
        if (i == 0 && (n == 0 || n == 127 || n >= 224)) return false;
        if (*p++ != (i == 3 ? ':' : '.')) return false;
    }
    size_t len = (size_t)(p - value - 1);
    unsigned port = 0, digits = 0;
    while (*p >= '0' && *p <= '9') {
        port = port * 10 + (unsigned)(*p++ - '0');
        if (++digits > 5 || port > 65535) return false;
    }
    if (*p || !digits || !port || len >= sizeof(out->host)) return false;
    memset(out, 0, sizeof(*out));
    memcpy(out->host, value, len);
    out->port = (uint16_t)port;
    return true;
}

int passport_proxy_request(char *out, size_t cap, const char *host, size_t len, int port)
{
    if (!out || !host || !len || len > 253 || port < 1 || port > 65535) return -1;
    for (size_t i = 0; i < len; ++i) {
        char c = host[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '-')) return -1;
    }
    int n = snprintf(out, cap, "CONNECT %.*s:%d HTTP/1.1\r\nHost: %.*s:%d\r\n\r\n",
                     (int)len, host, port, (int)len, host, port);
    return n > 0 && (size_t)n < cap ? n : -1;
}

bool passport_proxy_response(const char *header, size_t len)
{
    if (!header || len < 16 || memcmp(header + len - 4, "\r\n\r\n", 4)) return false;
    return (!memcmp(header, "HTTP/1.1 200 ", 13) || !memcmp(header, "HTTP/1.0 200 ", 13));
}
