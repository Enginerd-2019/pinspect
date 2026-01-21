/*
 * proc_task.h - Thread enumeration interface
 *
 * Provides API for enumerating threads of a Linux process
 * by reading /proc/<PID>/task/ directory.
 */

#ifndef PROC_TASK_H
#define PROC_TASK_H

#include <sys/types.h>
#include "pinspect.h"

/*
 * Enumerate all threads for a process.
 *
 * Reads /proc/<pid>/task/ and collects TID, name, and state for each thread.
 * Returns heap-allocated array via threads parameter. Caller must free with
 * thread_info_free().
 *
 * Returns 0 on success, -1 on error (ENOENT if process not found, EACCES
 * if permission denied, ENOMEM if allocation fails).
 */
int enumerate_threads(pid_t pid, thread_info_t **threads, int *count);

/*
 * Free memory allocated by enumerate_threads(). Safe to call with NULL.
 */
void thread_info_free(thread_info_t *threads);

#endif /* PROC_TASK_H */