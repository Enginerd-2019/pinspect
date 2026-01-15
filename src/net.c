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

/*
 * Parse hexadecimal address from /proc/net/tcp format.
 *
 * Parses strings like "0100007F:1F90" (127.0.0.1:8080) into
 * separate IP and port values.
 *
 * Parameters:
 *   hex  - Input string in "IIIIIIII:PPPP" format
 *   ip   - Output for IP address (converted to network byte order)
 *   port - Output for port number (host byte order)
 *
 * Returns:
 *   0 on success
 *  -1 on parse error or NULL input
 *
 * Note: The IP conversion uses htonl() because /proc stores IPs
 * in host byte order (little-endian on x86), but we need network
 * byte order for inet_ntoa() compatibility.
 */
static int parse_hex_addr(const char *hex, uint32_t *ip, uint16_t *port){
    
    if(hex == NULL || ip == NULL || port == NULL){
        return -1;
    }

    unsigned int ip_hex, port_hex;

    if(sscanf(hex, "%x:%x", &ip_hex, &port_hex) != 2){
        return -1;
    }

    /*
     * IP address handling:
     * /proc/net/tcp stores IPs in host byte order (little-endian on x86).
     * We need network byte order for inet_ntoa().
     *
     * Example on x86:
     *   hex = "0100007F" (127.0.0.1 in little-endian)
     *   ip_hex = 0x0100007F
     *   After htonl: 0x7F000001 (127.0.0.1 in network order)
     */
    *ip = htonl(ip_hex);

    /* Port is already in a usable format */
    *port = (uint16_t)port_hex;

    return 0;
}

/*
 * Format an IP address and port as a human-readable string.
 *
 * Converts network byte order IP and host byte order port into
 * a string like "192.168.1.1:8080".
 *
 * Parameters:
 *   addr   - IPv4 address in network byte order
 *   port   - Port number in host byte order
 *   buf    - Output buffer for formatted string
 *   buflen - Size of output buffer (recommend at least 22 bytes)
 *
 * Note: Uses inet_ntoa() internally, which returns a static buffer.
 * This function is safe because we copy to the caller's buffer
 * immediately via snprintf().
 */

 void format_ip_port(uint32_t addr, uint16_t port, char *buf, size_t buflen){

    if(buf == NULL || buflen == 0){
        return -1;
    }

    struct in_addr in;
    in.s_addr = addr; /* addr is already in network byte order*/

    /*
     * inet_ntoa() returns a pointer to a static buffer.
     * We must copy it immediately before another call overwrites it.
     */
    const char *ip_str = inet_ntoa(in);

    snprintf(buf, buflen, "%s:%u", ip_str, port);
 }