#pragma once
#include <stdbool.h>
#include <sys/select.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_NVS_NOT_FOUND 1
typedef enum { ESP_TLS_INIT, ESP_TLS_CONNECTING, ESP_TLS_HANDSHAKE, ESP_TLS_FAIL, ESP_TLS_DONE } esp_tls_conn_state_t;
typedef struct { bool is_plain_tcp, non_block; int timeout_ms; void *crt_bundle_attach; const char *common_name; } esp_tls_cfg_t;
typedef struct esp_tls { int sockfd; bool is_tls; fd_set rset, wset; esp_tls_conn_state_t conn_state; void *error_handle; } esp_tls_t;
esp_err_t esp_tls_plain_tcp_connect(const char *, int, int, const esp_tls_cfg_t *, void *, int *);
