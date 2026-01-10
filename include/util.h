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
 * Build a path to a file within /proc/<pid>/.
 *
 * Constructs paths like "/proc/1234/status" or "/proc/1234/fd".
 *
 * Returns 0 on success, -1 if buffer too small.
 * Does NOT check if path exists.
 */
int build_proc_path(pid_t pid, const char *file, char *out_path, size_t out_path_len);

/*
 * Check if a PID corresponds to an existing process.
 *
 * Returns true if /proc/<pid> exists and is accessible.
 * Note: Process may exit between check and subsequent operations.
 */
bool pid_exists(pid_t pid);

/*
 * Validate that a string represents a valid PID.
 *
 * Returns the parsed PID on success, -1 on error.
 * Rejects negative numbers, non-numeric input, and overflow.
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
