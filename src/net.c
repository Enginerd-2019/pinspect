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
// __attribute__((unused))
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
        return;
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

 /*
 * Check if an inode exists in a set of target inodes.
 *
 * Used to determine if a network connection belongs to our target
 * process by matching socket inodes from /proc/net/tcp against
 * inodes extracted from the process's file descriptors.
 *
 * Parameters:
 *   inode    - The inode to search for
 *   set      - Array of target inodes to match against
 *   set_size - Number of elements in the set array
 *
 * Returns:
 *   true if inode is found in set
 *   false if not found, or if set is NULL/empty
 *
 * Note: Uses linear search O(n). Sufficient for typical socket
 * counts (< 100). For high-performance needs, consider a hash set.
 */
static bool inode_in_set(unsigned long inode, unsigned long *set, int set_size)
{
    // No set to check
    if(set == NULL || set_size == 0){
        return false;
    }

    for(int i = 0; i < set_size; i++){
        if(set[i] == inode){
            return true;
        }
    }

    // If you made it here, inode was not in the set
    return false;

}

/*
 * Parse /proc/net/tcp or /proc/net/udp for matching socket inodes.
 *
 * Reads the specified network statistics file line by line, extracting
 * connection information for any entries whose inode matches our target
 * set (sockets owned by the process we're inspecting).
 *
 * Parameters:
 *   path          - Path to network file ("/proc/net/tcp" or "/proc/net/udp")
 *   is_tcp        - true for TCP connections, false for UDP
 *   target_inodes - Array of socket inodes belonging to our target process
 *   inode_count   - Number of inodes in target_inodes array
 *   results       - Output pointer to allocated array of socket_info_t
 *   result_count  - Output pointer to number of matching connections
 *
 * Returns:
 *   0 on success (results may be empty if no matches)
 *  -1 on error (file open failure, memory allocation failure)
 *
 * Memory management:
 *   Caller must free *results using socket_list_free().
 *   On error, no memory is allocated.
 *
 * File format parsed (columns):
 *   sl local_address rem_address st tx:rx tr:tm retrnsmt uid timeout inode
 *   We extract: local_address, rem_address, st (state), inode
 */
static int parse_net_file(const char *path, bool is_tcp,
                          unsigned long *target_inodes, int inode_count,
                          socket_info_t **results, int *result_count)
{
    /* Initialize outpurts */
    *results = NULL;
    *result_count = 0;

    /* No inodes match. Nothing to do*/
    if(target_inodes == NULL || inode_count == 0){
        return 0;
    }

    FILE *fp = fopen(path, "r");
    if(fp == NULL){
        return -1;
    }

    /* Allocate initial array (may need to be resized later) */
    int capacity = INITIAL_SOCKET_CAPACITY;
    socket_info_t *array = malloc(capacity * sizeof(socket_info_t));
    if(array == NULL){
        fclose(fp);
        return -1;
    }

    int count = 0;
    char line[512];

    /* Skip the header line */
    if(fgets(line, sizeof(line), fp) == NULL){
        free(array);
        fclose(fp);
        return 0; /* The file was empty, not an error */
    }

    /* Parse each connection line */
    while(fgets(line, sizeof(line), fp) != NULL){
        unsigned int slot;
        char local_addr[64], remote_addr[64];
        unsigned int state;
        unsigned int uid;
        unsigned long inode;

        /*
         * Parse the line. Format:
         * sl local_address rem_address st tx:rx tr:tm retrnsmt uid timeout inode ...
         *
         * We only need: local_address, rem_address, st, inode
         */
        int matched = sscanf(line,
                            " %u: %63s %63s %X %*s %*s %*s %u %*d %lu",
                            &slot, local_addr, remote_addr, &state, &uid, &inode);
        
        if(matched < 6){
            continue; /* Line was malformed and should be skipped */
        }

        /* Check if this inode belongs to our process */
        if(!inode_in_set(inode, target_inodes, inode_count)){
            continue; /* inode doesn't belong to process*/
        }

        /* Parse addresses */
        uint32_t local_ip, remote_ip;
        uint16_t local_port, remote_port;

        if(parse_hex_addr(local_addr, &local_ip, &local_port) !=0 || 
           parse_hex_addr(remote_addr, &remote_ip, &remote_port) != 0){
           continue; 
        }

        /* Store the result */
        array[count].is_tcp = is_tcp;
        array[count].local_addr = local_ip;
        array[count].local_port = local_port;
        array[count].remote_addr = remote_addr;
        array[count].remote_port = remote_port;
        array[count].inode = inode;
        count++;

        /* Grow the array if required */
        if(count == capacity){
            capacity *= 2;
            socket_info_t *new_array = realloc(array, capacity * sizeof(socket_info_t));

            if(new_array == NULL){
                free(array);
                fclose(fp);
                return -1;
            }

            array = new_array;
        }
    }

    fclose(fp);

    /* Handle empty results */
    if(count == 0){
        free(array);
        *results = NULL;
        *result_count = 0;
        return 0;
    }

    /* Shrink array to exact size */
    socket_info_t *final = realloc(array, count * sizeof(socket_info_t));
    if(final != NULL){
        array = final;
    }

    *results = array;
    *result_count = 0;
    return 0;
}

/*
 * Find all network sockets belonging to a process.
 *
 * This is the main entry point for network connection tracking.
 * It correlates socket inodes from the process's file descriptors
 * with entries in /proc/net/tcp and /proc/net/udp.
 *
 * Algorithm:
 *   1. Enumerate all FDs for the process using enumerate_fds()
 *   2. Extract socket inodes from FD entries (where is_socket == true)
 *   3. Parse /proc/net/tcp for TCP connections matching our inodes
 *   4. Parse /proc/net/udp for UDP sockets matching our inodes
 *   5. Merge TCP and UDP results into a single array
 *
 * Parameters:
 *   pid     - Process ID to inspect
 *   sockets - Output pointer to allocated array of socket_info_t
 *   count   - Output pointer to number of sockets found
 *
 * Returns:
 *   0 on success (sockets and count are populated)
 *  -1 on error (errno set: ENOENT, EACCES, ENOMEM)
 *
 * Memory management:
 *   Caller must free *sockets using socket_list_free().
 *   A process with no network connections returns 0 with *count = 0.
 *   On error, no memory is allocated (*sockets = NULL, *count = 0).
 */
int find_process_sockets(pid_t pid, socket_info_t **sockets, int *count)
{
    /* Initialize outputs */
    *sockets = NULL;
    *count = 0;

    /* Get all file descriptors for the process */
    fd_entry_t *fds= NULL;
    int fd_count = 0;

    if(enumerate_fds(pid, &fds, &count) != 0){
        return -1;
    }

    /* Extract socket inodes from FD entrires */
    unsigned long *socket_inodes = NULL;
    int inode_count = 0;

    if(fd_count > 0){
        socket_inodes = malloc(fd_count * sizeof(unsigned long));
        if(socket_inodes == NULL){
            fd_entries_free(fds);
            return -1;
        }

        for(int i = 0; i < fd_count; i++){
            if(fds[i].is_socket){
                socket_inodes[inode_count++] = fds[i].socket_inode;
            }
        }
    }

    /* Done with FD entries */
    fd_entries_free(fds);

    /* No socket - return empty success */
    if(inode_count == 0){
        free(socket_inodes);
        return 0;
    }

    /* Parse TCP connections */
    socket_info_t *tcp_sockets = NULL;
    int tcp_count = 0;

    if(parse_net_file(PROC_NET_TCP, true, socket_inodes, inode_count, &tcp_sockets, & tcp_count) != 0){
        free(socket_inodes);
        return -1;
    }

    /* Parse UDO sockets */
    socket_info_t *udp_sockets = NULL;
    int udp_count = 0;

    if(parse_net_file(PROC_NET_UDP, false, socket_inodes, inode_count, &tcp_sockets, & tcp_count) != 0){
        free(socket_inodes);
        socket_list_free(tcp_sockets);
        return -1;
    }

    /* Done with array */
    free(socket_inodes);

    /* Merge TCP and UDP results */
    int total_count = tcp_count + udp_count;

    if(total_count == 0){
        socket_list_free(tcp_sockets);
        socket_list_free(udp_sockets);
        return 0;
    }

    socket_info_t *merged = malloc(total_count * sizeof(socket_info_t));
    if(merged == NULL){
        socket_list_free(tcp_sockets);
        socket_list_free(udp_sockets);
        return -1;
    }


    /* Copy TCP results */
    if(tcp_count > 0){
        memcpy(merged, tcp_sockets, tcp_count * sizeof(socket_info_t));
        socket_list_free(tcp_sockets);
    }

    /* Copy UDP results */
    if(udp_count > 0){
        memcpy(merged + tcp_count, udp_sockets, udp_count * sizeof(socket_info_t));
        socket_list_free(udp_sockets);
    }

    /* Memory is freed in main */
    *sockets = merged;
    *count = total_count;
    return 0;
}

/*
 * Free memory allocated by find_process_sockets().
 *
 * Safe to call with NULL pointer (free(NULL) is a no-op in C).
 * Provided for API consistency with other modules (fd_entries_free,
 * thread_info_free) and to allow future changes to internal
 * allocation strategy without affecting callers.
 *
 * Parameters:
 *   sockets - Array returned by find_process_sockets(), or NULL
 */
void socket_list_free(socket_info_t *sockets)
{
    free(sockets);  /* free(NULL) is safe */
}