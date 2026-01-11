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
 * Reads /proc/<pid>/task/ directory and collects information about
 * each thread including TID, name, and state.
 *
 * Parameters:
 *   pid     - Process ID to inspect
 *   threads - Output pointer to allocated array of thread_info_t
 *   count   - Output pointer to number of entries in array
 *
 * Returns:
 *   0 on success (threads and count are populated)
 *  -1 on error (errno is set: ENOENT, EACCES, ENOMEM)
 *
 * Memory management:
 *   Caller must free the returned array using thread_info_free().
 *   If this function returns -1, no memory is allocated.
 *
 * Notes:
 *   - A single-threaded process returns 1 entry (the main thread)
 *   - The main thread's TID equals the PID
 *   - Thread list may change between call and use (TOCTOU)
 *
 * Example:
 *   thread_info_t *threads;
 *   int count;
 *   if (enumerate_threads(1234, &threads, &count) == 0) {
 *       for (int i = 0; i < count; i++) {
 *           printf("TID %d: %s (%s)\n",
 *                  threads[i].tid,
 *                  threads[i].name,
 *                  state_to_string(threads[i].state));
 *       }
 *       thread_info_free(threads);
 *   }
 */
int enumerate_threads(pid_t pid, thread_info_t **threads, int *count);

/*
 * Free memory allocated by enumerate_threads().
 *
 * Safe to call with NULL pointer.
 *
 * Parameters:
 *   threads - Array returned by enumerate_threads(), or NULL
 */
void thread_info_free(thread_info_t *threads);

#endif /* PROC_TASK_H */