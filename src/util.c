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

/*
 * Build a path to a /proc file.
 * If file is NULL, builds path to /proc/<pid> directory.
 * Returns 0 on success, -1 if path would be truncated.
 */
int build_proc_path(pid_t pid, const char *file, char *out_path,
                    size_t out_path_len)
{
    int ret;

    if (file != NULL) {
        ret = snprintf(out_path, out_path_len, "/proc/%d/%s", pid, file);
    } else {
        ret = snprintf(out_path, out_path_len, "/proc/%d", pid);
    }

    if (ret < 0 || ret >= (int)out_path_len) {
        return -1;
    }

    return 0;
}

/*
 * Build a path to a thread-specific /proc file.
 * Returns 0 on success, -1 if path would be truncated.
 */
int build_task_path(pid_t pid, pid_t tid, const char *file,
                    char *buf, size_t buflen)
{
    int written = snprintf(buf, buflen, "/proc/%d/task/%d/%s",
                           pid, tid, file);
    if (written < 0 || (size_t)written >= buflen) {
        return -1;
    }
    return 0;
}

/*
 * Check if a process exists by testing access to /proc/<pid>.
 */
bool pid_exists(pid_t pid)
{
    char buf[256];

    if (build_proc_path(pid, NULL, buf, sizeof(buf)) != 0) {
        return false;
    }

    return access(buf, F_OK) == 0;
}

/*
 * Parse a string as a PID.
 * Returns the PID on success, -1 on error.
 * Rejects negative numbers, non-numeric strings, and zero.
 */
pid_t parse_pid(const char *str)
{
    if (str == NULL || *str == '\0') {
        return -1;
    }

    /* Verify string contains only digits */
    const char *p = str;
    while (*p != '\0') {
        if (!isdigit((unsigned char)*p)) {
            return -1;
        }
        p++;
    }

    errno = 0;
    pid_t pid = strtol(str, NULL, BASE);

    if (errno == ERANGE || pid <= 0) {
        return -1;
    }

    return pid;
}

/*
 * Convert process state enum to human-readable string.
 */
const char *state_to_string(proc_state_t state)
{
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

/*
 * Convert /proc state character to enum.
 * Maps: R=Running, S=Sleeping, D=Disk Sleep, Z=Zombie, T=Stopped, I=Idle
 */
proc_state_t char_to_state(char c)
{
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
