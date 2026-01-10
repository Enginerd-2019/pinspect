/*
 * proc_fd.h - File descriptor enumeration API
 *
 * Functions for enumerating and analyzing /proc/<PID>/fd/.
 */

#ifndef PROC_FD_H
#define PROC_FD_H

#include <sys/types.h>
#include <stdbool.h>
#include "pinspect.h"

/*
 * Enumerate all file descriptors for a process.
 *
 * Reads /proc/<pid>/fd/ directory and resolves each symlink to determine
 * what the file descriptor points to.
 *
 * Parameters:
 *   pid       - Process ID to inspect
 *   entries   - Output pointer to array of fd_entry_t (heap-allocated)
 *   count     - Output parameter for number of entries in array
 *
 * Returns 0 on success, -1 on error:
 *   ENOENT - Process doesn't exist or /proc/<pid>/fd/ not found
 *   EACCES - Permission denied (common for other users' processes)
 *   ENOMEM - Memory allocation failed
 *
 * On success, caller must free the returned array with fd_entries_free().
 * On error, *entries is set to NULL and *count is set to 0.
 *
 * Note: The FD directory may change while we're reading it (process can
 * open/close files). We read a snapshot and ignore readdir/readlink errors
 * for individual entries that disappear mid-read.
 */
int enumerate_fds(pid_t pid, fd_entry_t **entries, int *count);

/*
 * Free array of fd_entry_t returned by enumerate_fds().
 *
 * Safe to call with NULL pointer.
 */
void fd_entries_free(fd_entry_t *entries);

/*
 * Parse socket inode from symlink target.
 *
 * Socket symlinks have format "socket:[12345]" where 12345 is the inode.
 * This function extracts the inode number.
 *
 * Parameters:
 *   target - Symlink target string (e.g., "socket:[67890]")
 *   inode  - Output pointer for extracted inode number
 *
 * Returns true if target is a socket and inode was extracted.
 * Returns false if target is not a socket format.
 */
bool parse_socket_inode(const char *target, unsigned long *inode);

/* TODO: Week 4 - Add fd_entry_has_inode() helper to check if any FD
 * matches a given inode. Useful when correlating network sockets. */

#endif /* PROC_FD_H */
