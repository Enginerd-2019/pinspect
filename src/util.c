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

int build_proc_path(pid_t pid, const char *file, char *buf, size_t buflen)
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
    (void)pid;
    (void)file;
    (void)buf;
    (void)buflen;
    return -1;
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
    (void)pid;
    return false;
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
    (void)str;
    return -1;
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
    (void)state;
    return "Unknown";
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
    (void)c;
    return PROC_STATE_UNKNOWN;
}

/* TODO: Implement format_memory_size()
 *
 * Convert kilobytes to human-readable format (KB, MB, GB).
 * Use static buffer or require caller to provide buffer.
 *
 * Example: 1048576 KB -> "1.0 GB"
 */
