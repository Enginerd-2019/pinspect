/*
 * util.h - Utility function declarations
 *
 * Shared helper functions for path building, validation, and conversions.
 */

#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>
#include "pinspect.h"

/*
 * Build path to /proc/<pid>/ or /proc/<pid>/<file>.
 *
 * Returns 0 on success, -1 if buffer too small. Does not check if path exists.
 */
int build_proc_path(pid_t pid, const char *file, char *out_path, size_t out_path_len);

/*
 * Build path to /proc/<pid>/task/<tid>/<file>.
 *
 * Returns 0 on success, -1 if path would overflow buffer.
 */
int build_task_path(pid_t pid, pid_t tid, const char *file,
                    char *buf, size_t buflen);

/*
 * Check if /proc/<pid> exists.
 *
 * Returns true if accessible. Process may exit between check and use.
 */
bool pid_exists(pid_t pid);

/*
 * Parse and validate a PID string.
 *
 * Returns parsed PID on success, -1 if invalid (non-numeric, negative, or
 * overflow).
 */
pid_t parse_pid(const char *str);

/*
 * Convert process state enum to human-readable string.
 *
 * Returns static string like "Running", "Sleeping", etc.
 * Never returns NULL.
 */
const char *state_to_string(proc_state_t state);

/*
 * Convert single-character state code to proc_state_t enum.
 *
 * Handles: R, S, D, Z, T, I
 * Returns PROC_STATE_UNKNOWN for unrecognized characters.
 */
proc_state_t char_to_state(char c);

#endif /* UTIL_H */
