/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/* USB console commands: proxy=IPv4:port, proxy=off, proxy.status.
 * Changes persist in a separate NVS namespace and apply to new connections.
 * Reboot after configuration to reconnect every Muse service consistently. */
bool passport_proxy_command(const char *line);
#ifdef __cplusplus
}
#endif
