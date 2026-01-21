/*
 * net.h - Network connection parsing interface
 *
 * Public API for finding network connections belonging to a process.
 */

#ifndef NET_H
#define NET_H

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "pinspect.h"

/*
 * Find all network sockets belonging to a process.
 *
 * Correlates socket inodes from /proc/<pid>/fd/ with /proc/net/tcp and
 * /proc/net/udp to identify connections. Returns heap-allocated array via
 * sockets parameter. Caller must free with socket_list_free().
 *
 * Returns 0 on success, -1 on error (ENOENT if process not found, EACCES
 * if permission denied, ENOMEM if allocation fails).
 */
int find_process_sockets(pid_t pid, socket_info_t **sockets, int *count);

/*
 * Free memory allocated by find_process_sockets(). Safe to call with NULL.
 */
void socket_list_free(socket_info_t *sockets);

/*
 * Convert TCP state enum to human-readable string.
 *
 * Returns static string like "ESTABLISHED" or "LISTEN". Returns "UNKNOWN"
 * for unrecognized values. Never returns NULL.
 */
const char *tcp_state_to_string(tcp_state_t state);

/*
 * Format IP address and port as "192.168.1.1:8080" string.
 *
 * Takes network byte order IP and host byte order port. Buffer should be
 * at least 22 bytes.
 */
void format_ip_port(uint32_t addr, uint16_t port, char *buf, size_t buflen);

#endif /* NET_H */