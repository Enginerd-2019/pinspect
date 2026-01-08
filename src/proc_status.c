/*
 * proc_status.c - Parse /proc/<PID>/status
 *
 * Reads and extracts process information from the status file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "proc_status.h"
#include "util.h"
#include "pinspect.h"

/* Maximum line length in /proc/<pid>/status */
#define STATUS_LINE_MAX 256

/*
 * Parse a single line from /proc/<pid>/status.
 *
 * Matches the field name and extracts the value into proc_info_t.
 * Returns 0 if line was parsed, 1 if line didn't match any known field.
 */
static int parse_status_line(const char *line, proc_info_t *info)
{
    /* TODO: Implement line parsing
     *
     * Strategy: Check if line starts with known field names, then extract values.
     *
     * Fields to parse:
     * - "Name:\t<string>"     -> info->name (use sscanf with %15s to limit)
     * - "State:\t<char> ..."  -> info->state (extract first char, use char_to_state)
     * - "Uid:\t<r> <e> ..."   -> info->uid_real, info->uid_effective (first two values)
     * - "Gid:\t<r> <e> ..."   -> info->gid_real, info->gid_effective (first two values)
     * - "VmSize:\t<val> kB"   -> info->vm_size_kb
     * - "VmRSS:\t<val> kB"    -> info->vm_rss_kb
     * - "VmPeak:\t<val> kB"   -> info->vm_peak_kb
     * - "Threads:\t<val>"     -> info->thread_count
     *
     * Use strncmp() to check field prefix, then sscanf() to extract values.
     *
     * Note: Zombie/kernel threads lack Vm* fields - don't treat as error.
     */
    (void)line;
    (void)info;
    return 1;
}

int read_proc_status(pid_t pid, proc_info_t *info)
{
    /* TODO: Implement status file reading
     *
     * Steps:
     * 1. Initialize info struct to zero/defaults
     * 2. Set info->pid
     * 3. Build path to /proc/<pid>/status using build_proc_path()
     * 4. Open file with fopen() - handle ENOENT and EACCES appropriately
     * 5. Read line by line with fgets()
     * 6. Call parse_status_line() for each line
     * 7. Close file
     * 8. Return 0 on success, -1 on error
     *
     * Error handling:
     * - ENOENT: Process doesn't exist or exited during read
     * - EACCES: Permission denied (common for other users' processes)
     *
     * Note: It's not an error if Vm* fields are missing (zombies, kernel threads)
     */

    /* Stub: Initialize to zeros and call parse_status_line to avoid unused warning */
    memset(info, 0, sizeof(*info));
    info->pid = pid;
    parse_status_line("", info);

    errno = ENOSYS;  /* Function not implemented */
    return -1;
}

/* TODO: Implement read_proc_stat() for CPU time, start time, etc.
 *
 * /proc/<pid>/stat contains space-separated fields (not key: value format).
 * Fields of interest:
 * - Field 14: utime (user mode ticks)
 * - Field 15: stime (kernel mode ticks)
 * - Field 22: starttime (ticks since boot when process started)
 *
 * See: man 5 proc, section on /proc/[pid]/stat
 */

/* TODO: Implement read_proc_cmdline() for full command line
 *
 * /proc/<pid>/cmdline contains null-separated arguments.
 * Reading strategy:
 * 1. Read entire file into buffer
 * 2. Replace null bytes with spaces (except final one)
 * 3. Handle kernel threads (empty cmdline)
 */
