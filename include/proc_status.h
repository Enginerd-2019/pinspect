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
 * Populates proc_info_t with name, state, UID/GID, memory stats, and thread
 * count. Zombie and kernel thread processes have no memory fields (set to 0).
 *
 * Returns 0 on success, -1 on error (ENOENT if process exited, EACCES if
 * permission denied).
 */
int read_proc_status(pid_t pid, proc_info_t *info);

#endif /* PROC_STATUS_H */
