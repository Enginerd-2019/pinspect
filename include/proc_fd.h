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
 * Reads /proc/<pid>/fd/ and resolves each symlink to determine what the
 * descriptor points to. Returns heap-allocated array via entries parameter.
 * Caller must free with fd_entries_free().
 *
 * Returns 0 on success, -1 on error (ENOENT if process not found, EACCES
 * if permission denied, ENOMEM if allocation fails).
 */
int enumerate_fds(pid_t pid, fd_entry_t **entries, int *count);

/*
 * Free array of fd_entry_t returned by enumerate_fds().
 *
 * Safe to call with NULL pointer.
 */
void fd_entries_free(fd_entry_t *entries);

/*
 * Parse socket inode from symlink target string.
 *
 * Extracts inode from "socket:[12345]" format. Returns true if target is a
 * socket and inode was extracted, false otherwise.
 */
bool parse_socket_inode(const char *target, unsigned long *inode);

#endif /* PROC_FD_H */
