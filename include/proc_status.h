/*
 * proc_status.h - Process status parsing API
 *
 * Functions for reading and parsing /proc/<PID>/status.
 */

#ifndef PROC_STATUS_H
#define PROC_STATUS_H

#include <sys/types.h>
#include "pinspect.h"

/*
 * Read process info from /proc/<pid>/status.
 *
 * Populates the proc_info_t struct with:
 * - Process name, state, PID
 * - Real and effective UID/GID
 * - Memory stats (VmSize, VmRSS, VmPeak)
 * - Thread count
 *
 * Returns 0 on success, -1 on error.
 * On ENOENT (process exited), returns -1 with errno set.
 * On EACCES (permission denied), returns partial info where possible.
 *
 * Note: Zombie and kernel thread processes have no Vm* fields;
 * memory values will be set to 0 for these processes.
 */
int read_proc_status(pid_t pid, proc_info_t *info);

/* TODO: Add read_proc_stat() for additional fields like CPU time */

/* TODO: Add read_proc_cmdline() for full command line */

#endif /* PROC_STATUS_H */
