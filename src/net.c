/*
 * net.c - Parse /proc/net/tcp and /proc/net/udp for process sockets
 *
 * Correlates socket inodes from process FDs with network connection
 * entries to identify which connections belong to a specific process.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include "net.h"
#include "proc_fd.h"
#include "util.h"

/* Initial capacity for socket array */
#define INITIAL_SOCKET_CAPACITY 16

/* Paths to network statistics files */
#define PROC_NET_TCP "/proc/net/tcp"
#define PROC_NET_UDP "/proc/net/udp"

/*
 * Convert TCP state enum to human-readable string.
 *
 * Returns a static string for each known state value.
 * Returns "UNKNOWN" for unrecognized values.
 * Never returns NULL.
 */
const char *tcp_state_to_string(tcp_state_t state)
{
    switch (state) {
        case TCP_ESTABLISHED: return "ESTABLISHED";
        case TCP_SYN_SENT:    return "SYN_SENT";
        case TCP_SYN_RECV:    return "SYN_RECV";
        case TCP_FIN_WAIT1:   return "FIN_WAIT1";
        case TCP_FIN_WAIT2:   return "FIN_WAIT2";
        case TCP_TIME_WAIT:   return "TIME_WAIT";
        case TCP_CLOSE:       return "CLOSE";
        case TCP_CLOSE_WAIT:  return "CLOSE_WAIT";
        case TCP_LAST_ACK:    return "LAST_ACK";
        case TCP_LISTEN:      return "LISTEN";
        case TCP_CLOSING:     return "CLOSING";
        default:              return "UNKNOWN";
    }
}