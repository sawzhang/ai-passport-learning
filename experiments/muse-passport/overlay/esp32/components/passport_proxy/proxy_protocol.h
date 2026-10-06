/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { char host[16]; uint16_t port; } passport_proxy_endpoint;
bool passport_proxy_parse(const char *value, passport_proxy_endpoint *out);
int passport_proxy_request(char *out, size_t cap, const char *host, size_t len, int port);
bool passport_proxy_response(const char *header, size_t len);
