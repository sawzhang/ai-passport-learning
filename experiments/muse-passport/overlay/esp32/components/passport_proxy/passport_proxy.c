/* SPDX-License-Identifier: MIT */
#include "passport_proxy.h"
#include "proxy_protocol.h"
#include "esp_tls.h"
#include "esp_tls_private.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

static const char *TAG = "passport_proxy";
static const char *NS = "passport_proxy";

/* One committed string prevents a torn host/port update. No credentials. */
static int load_endpoint(char *value, size_t cap)
{
    nvs_handle_t h;
    esp_err_t e = nvs_open(NS, NVS_READONLY, &h);
    if (e == ESP_ERR_NVS_NOT_FOUND) { value[0] = 0; return 0; }
    if (e != ESP_OK) return -1;
    e = nvs_get_str(h, "endpoint", value, &cap);
    nvs_close(h);
    if (e == ESP_ERR_NVS_NOT_FOUND) { value[0] = 0; return 0; }
    return e == ESP_OK ? (value[0] ? 1 : 0) : -1;
}

bool passport_proxy_command(const char *line)
{
    if (!strcmp(line, "proxy.status")) {
        char value[24];
        int state = load_endpoint(value, sizeof(value));
        printf("@proxy %s\n", state < 0 ? "error: NVS unavailable" : state ? value : "off");
        fflush(stdout);
        return true;
    }
    if (strncmp(line, "proxy=", 6)) return false;
    const char *value = line + 6;
    passport_proxy_endpoint endpoint;
    bool off = !strcmp(value, "off");
    if (!off && !passport_proxy_parse(value, &endpoint)) {
        printf("@proxy.error use Mac LAN IPv4:port or off (not 127.0.0.1/0.0.0.0)\n");
    } else {
        nvs_handle_t h;
        esp_err_t e = nvs_open(NS, NVS_READWRITE, &h);
        if (e == ESP_OK) {
            e = nvs_set_str(h, "endpoint", off ? "" : value);
            if (e == ESP_OK) e = nvs_commit(h);
            nvs_close(h);
        }
        puts(e == ESP_OK ? "@proxy.saved reboot to reconnect" : "@proxy.error storage failed");
    }
    fflush(stdout);
    return true;
}

static bool wait_socket(int fd, bool writing, int64_t deadline)
{
    for (;;) {
        int64_t remaining = deadline - esp_timer_get_time();
        if (remaining <= 0) return false;
        struct timeval tv = { .tv_sec = remaining / 1000000, .tv_usec = remaining % 1000000 };
        fd_set set;
        FD_ZERO(&set); FD_SET(fd, &set);
        int r = select(fd + 1, writing ? NULL : &set, writing ? &set : NULL, NULL, &tv);
        if (r < 0 && errno == EINTR) continue;
        return r > 0;
    }
}

static bool tunnel(int fd, const char *host, int len, int port, int64_t deadline)
{
    char request[576];
    int n = len > 0 ? passport_proxy_request(request, sizeof(request), host, (size_t)len, port) : -1;
    if (n < 0) return false;
    int sent = 0;
    while (sent < n) {
        if (!wait_socket(fd, true, deadline)) return false;
        ssize_t r = send(fd, request + sent, (size_t)(n - sent), 0);
        if (r < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        if (r <= 0) return false;
        sent += (int)r;
    }
    /* Read exactly the header: no TLS bytes are consumed or discarded. */
    /* Bound the header to 2 KiB without reserving it on this C3's stack. */
    char response[17] = {0};
    char tail[4] = {0};
    size_t used = 0;
    while (used < 2048) {
        if (!wait_socket(fd, false, deadline)) return false;
        char c;
        ssize_t r = recv(fd, &c, 1, 0);
        if (r < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        if (r != 1) return false;
        if (used < 13) response[used] = c;
        memmove(tail, tail + 1, 3); tail[3] = c;
        ++used;
        if (used >= 4 && !memcmp(tail, "\r\n\r\n", 4)) {
            memcpy(response + 13, tail, 4);
            return used >= 16 && passport_proxy_response(response, sizeof(response));
        }
    }
    return false;
}

static int prepare(const char *host, int len, int port, const esp_tls_cfg_t *cfg, esp_tls_t *tls)
{
    if (!cfg || cfg->is_plain_tcp || !tls || tls->conn_state != ESP_TLS_INIT) return 0;
    char value[24];
    int state = load_endpoint(value, sizeof(value));
    if (state == 0) return 0;
    passport_proxy_endpoint proxy;
    if (state < 0 || !passport_proxy_parse(value, &proxy)) {
        ESP_LOGE(TAG, "invalid/unavailable saved proxy; direct fallback disabled");
        tls->conn_state = ESP_TLS_FAIL;
        return -1;
    }
    int timeout = cfg->timeout_ms > 0 ? cfg->timeout_ms : 15000;
    int64_t deadline = esp_timer_get_time() + (int64_t)timeout * 1000;
    esp_tls_cfg_t tcp = *cfg;
    tcp.non_block = false;
    tcp.is_plain_tcp = true;
    tcp.timeout_ms = timeout;
    int fd = -1;
    esp_err_t e = esp_tls_plain_tcp_connect(proxy.host, (int)strlen(proxy.host), proxy.port,
                                           &tcp, tls->error_handle, &fd);
    if (e != ESP_OK) goto fail;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0 ||
        !tunnel(fd, host, len, port, deadline) ||
        fcntl(fd, F_SETFL, cfg->non_block ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0) goto fail;
    tls->sockfd = fd;
    tls->is_tls = true;
    FD_ZERO(&tls->rset); FD_SET(fd, &tls->rset); tls->wset = tls->rset;
    tls->conn_state = ESP_TLS_CONNECTING;
    ESP_LOGI(TAG, "CONNECT tunnel ready; starting verified TLS");
    return 1;
fail:
    if (fd >= 0) close(fd);
    tls->conn_state = ESP_TLS_FAIL;
    ESP_LOGE(TAG, "proxy TCP/CONNECT failed; check Mac listener, firewall and upstream proxy");
    return -1;
}

int __real_esp_tls_conn_new_sync(const char *, int, int, const esp_tls_cfg_t *, esp_tls_t *);
int __real_esp_tls_conn_new_async(const char *, int, int, const esp_tls_cfg_t *, esp_tls_t *);

int __wrap_esp_tls_conn_new_sync(const char *host, int len, int port, const esp_tls_cfg_t *cfg, esp_tls_t *tls)
{
    if (prepare(host, len, port, cfg, tls) < 0) return -1;
    return __real_esp_tls_conn_new_sync(host, len, port, cfg, tls);
}

int __wrap_esp_tls_conn_new_async(const char *host, int len, int port, const esp_tls_cfg_t *cfg, esp_tls_t *tls)
{
    /* CONNECT negotiation is bounded and runs in the network worker. Once
     * attached, the original async TLS state machine resumes normally. */
    if (prepare(host, len, port, cfg, tls) < 0) return -1;
    return __real_esp_tls_conn_new_async(host, len, port, cfg, tls);
}
