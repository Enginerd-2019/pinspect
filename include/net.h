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
 * Correlates socket inodes from /proc/<pid>/fd/ with entries in
 * /proc/net/tcp and /proc/net/udp to identify connections owned
 * by the specified process.
 *
 * Parameters:
 *   pid     - Process ID to inspect
 *   sockets - Output pointer to allocated array of socket_info_t
 *   count   - Output pointer to number of sockets found
 *
 * Returns:
 *   0 on success (sockets and count are populated)
 *  -1 on error (errno is set: ENOENT, EACCES, ENOMEM)
 *
 * Memory management:
 *   Caller must free the returned array using socket_list_free().
 *   If this function returns -1, no memory is allocated.
 *
 * Note:
 *   A process with no network connections returns 0 with *count = 0.
 *
 * Example:
 *   socket_info_t *sockets;
 *   int count;
 *   if (find_process_sockets(1234, &sockets, &count) == 0) {
 *       for (int i = 0; i < count; i++) {
 *           printf("%s connection on port %d\n",
 *                  sockets[i].is_tcp ? "TCP" : "UDP",
 *                  sockets[i].local_port);
 *       }
 *       socket_list_free(sockets);
 *   }
 */
int find_process_sockets(pid_t pid, socket_info_t **sockets, int *count);

/*
 * Free memory allocated by find_process_sockets().
 *
 * Safe to call with NULL pointer.
 *
 * Parameters:
 *   sockets - Array returned by find_process_sockets(), or NULL
 */
void socket_list_free(socket_info_t *sockets);

/*
 * Convert TCP state enum to human-readable string.
 *
 * Parameters:
 *   state - TCP state value
 *
 * Returns:
 *   Static string like "ESTABLISHED", "LISTEN", etc.
 *   Returns "UNKNOWN" for unrecognized values.
 *   Never returns NULL.
 */
const char *tcp_state_to_string(tcp_state_t state);

/*
 * Format an IP address and port as a string.
 *
 * Converts network byte order IP and host byte order port into
 * a human-readable string like "192.168.1.1:8080".
 *
 * Parameters:
 *   addr   - IPv4 address in network byte order
 *   port   - Port number in host byte order
 *   buf    - Output buffer
 *   buflen - Size of output buffer (recommend at least 22 bytes)
 *
 * The buffer will contain "IP:PORT" on success, or "?" on error.
 */
void format_ip_port(uint32_t addr, uint16_t port, char *buf, size_t buflen);

#endif /* NET_H */