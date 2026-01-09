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
 *
 * Note: Zombie and kernel thread processes lack Vm* fields - this is not
 * treated as an error, and those fields remain zero-initialized.
 */
static int parse_status_line(const char *line, proc_info_t *info)
{
    if (strncmp(line, "Name:", 5) == 0) {
        sscanf(line, "Name:\t%15s", info->name);
        return 0;
    }
    if (strncmp(line, "State:", 6) == 0) {
        char state_char;
        if (sscanf(line, "State:\t%c", &state_char) == 1) {
            info->state = char_to_state(state_char);
        }
        return 0;
    }
    if (strncmp(line, "Uid:", 4) == 0) {
        if (sscanf(line, "Uid:\t%u\t%u", &info->uid_real, &info->uid_effective) == 2) {
            return 0;
        }
    }
    if (strncmp(line, "Gid:", 4) == 0) {
        if (sscanf(line, "Gid:\t%u\t%u", &info->gid_real, &info->gid_effective) == 2) {
            return 0;
        }
    }
    if(strncmp(line, "VmSize:", 7) == 0){
        if(sscanf(line, "VmSize:\t%lu", &info->vm_size_kb) == 1){
            return 0;
        }
    }
    if(strncmp(line, "VmRSS:", 6) == 0){
        if(sscanf(line, "VmRSS:\t%lu", &info->vm_rss_kb) == 1){
            return 0;
        }
    }
    if(strncmp(line, "VmPeak:", 7) == 0){
        if(sscanf(line, "VmPeak:\t%lu", &info->vm_peak_kb) == 1){
            return 0;
        }
    }
    if(strncmp(line, "Threads:", 8) == 0){
        if(sscanf(line, "Threads:\t%d", &info->thread_count) == 1){
            return 0;
        }
    }

    return 1;
}

/*
 * Read and parse /proc/<pid>/status file.
 *
 * Returns 0 on success, -1 on error with errno set:
 * - ENOENT: Process doesn't exist or exited during read
 * - EACCES: Permission denied (common for other users' processes)
 */
int read_proc_status(pid_t pid, proc_info_t *info)
{
    memset(info, 0, sizeof(*info));
    info->pid = pid;

    char path_buf[256];
    build_proc_path(pid, "status", path_buf, sizeof(path_buf));

    FILE *file = fopen(path_buf, "r");
    if(file == NULL){
        return -1;
    }

    char line_buffer[256];
    while(fgets(line_buffer, sizeof(line_buffer), file) != NULL){
        parse_status_line(line_buffer, info);
    }

    fclose(file);
    return 0;
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
