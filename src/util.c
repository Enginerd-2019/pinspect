/*
 * util.c - Shared utility functions
 *
 * Helper functions for path building, validation, and conversions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>
#include "util.h"
#include "pinspect.h"

#define BASE 10

int build_proc_path(pid_t pid, const char *file, char *out_path, size_t out_path_len)
{
    /* TODO: Implement path construction
     *
     * Steps:
     * 1. Use snprintf to build "/proc/<pid>/<file>" or "/proc/<pid>" if file is NULL
     * 2. Check return value to ensure buffer wasn't truncated
     * 3. Return 0 on success, -1 if buffer too small
     *
     * Example: build_proc_path(1234, "status", buf, sizeof(buf))
     *          should produce "/proc/1234/status"
     */

    int ret;

    if(file != NULL){
        ret = snprintf(out_path, out_path_len, "/proc/%d/%s", pid, file);
    }else{
        ret = snprintf(out_path, out_path_len, "/proc/%d", pid);
    }

    if(ret < 0 || ret >= (int)out_path_len){
        fprintf(stderr, "Buffer Error\n");
        return -1;
    }

    return 0;
}

bool pid_exists(pid_t pid)
{
    /* TODO: Implement PID existence check
     *
     * Steps:
     * 1. Build path to /proc/<pid>
     * 2. Use access() with F_OK to check existence
     * 3. Return true if exists, false otherwise
     *
     * Note: Race condition possible - process may exit after check
     */
    char buf[256];
    
    if (build_proc_path(pid, NULL, buf, sizeof(buf)) != 0) {
        return false;
    }
    
    return access(buf, F_OK) == 0;

}

pid_t parse_pid(const char *str)
{
    /* TODO: Implement PID string parsing
     *
     * Steps:
     * 1. Check for NULL or empty string
     * 2. Reject strings starting with non-digit (handles negative numbers)
     * 3. Use strtol() with base 10
     * 4. Check for conversion errors (endptr, errno, overflow)
     * 5. Validate result is in valid PID range (> 0, <= PID_MAX)
     *
     * Return parsed PID on success, -1 on error
     */

     if (str == NULL || *str == '\0') {
        fprintf(stderr, "Expected a PID argument\n");
        return -1;
    }

    const char *p = str;

    while (*p != '\0') {
        // Check if the current character is NOT a digit
        // Cast to unsigned char is a common practice for ctype functions
        if (!isdigit((unsigned char)*p)) { 
            // Found a non-digit character, so the entire string is not purely numeric
            fprintf(stderr, "The PID argument must be an integer\n");
            return -1; 
        }
        p++; // Move to the next character
    }

    errno = 0;
    pid_t pid = strtol(str, NULL, BASE);

    if (errno == ERANGE) {
        fprintf(stderr, "PID value out of range\n");
        return -1;
    }
    
    if (pid <= 0) {
        fprintf(stderr, "PID must be positive\n");
        return -1;
    }

    return pid;
}

const char *state_to_string(proc_state_t state)
{
    /* TODO: Implement state to string conversion
     *
     * Use switch statement to map:
     * - PROC_STATE_RUNNING    -> "Running"
     * - PROC_STATE_SLEEPING   -> "Sleeping"
     * - PROC_STATE_DISK_SLEEP -> "Disk Sleep"
     * - PROC_STATE_ZOMBIE     -> "Zombie"
     * - PROC_STATE_STOPPED    -> "Stopped"
     * - PROC_STATE_IDLE       -> "Idle"
     * - PROC_STATE_UNKNOWN    -> "Unknown"
     *
     * Return static string, never NULL
     */

    switch (state) {
    
        case PROC_STATE_RUNNING:
        return "Running";
    case PROC_STATE_SLEEPING:
        return "Sleeping";
    case PROC_STATE_DISK_SLEEP:
        return "Disk Sleep";
    case PROC_STATE_ZOMBIE:
        return "Zombie";
    case PROC_STATE_STOPPED:
        return "Stopped";
    case PROC_STATE_IDLE:
        return "Idle";
    case PROC_STATE_UNKNOWN:
    default:
        return "Unknown";
    }
}

proc_state_t char_to_state(char c)
{
    /* TODO: Implement character to state conversion
     *
     * Map single character codes from /proc/<pid>/status:
     * - 'R' -> PROC_STATE_RUNNING
     * - 'S' -> PROC_STATE_SLEEPING
     * - 'D' -> PROC_STATE_DISK_SLEEP
     * - 'Z' -> PROC_STATE_ZOMBIE
     * - 'T' -> PROC_STATE_STOPPED
     * - 'I' -> PROC_STATE_IDLE
     * - other -> PROC_STATE_UNKNOWN
     */

    switch (c) {
    
        case 'R':
        return PROC_STATE_RUNNING;
    case 'S':
        return PROC_STATE_SLEEPING;
    case 'D':
        return PROC_STATE_DISK_SLEEP;
    case 'Z':
        return PROC_STATE_ZOMBIE;
    case 'T':
        return PROC_STATE_STOPPED;
    case 'I':
        return PROC_STATE_IDLE;
    default:
        return PROC_STATE_UNKNOWN;
    }
}

/* TODO: Implement format_memory_size()
 *
 * Convert kilobytes to human-readable format (KB, MB, GB).
 * Use static buffer or require caller to provide buffer.
 *
 * Example: 1048576 KB -> "1.0 GB"
 */
