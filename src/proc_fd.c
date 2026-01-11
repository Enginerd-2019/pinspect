/*
 * proc_fd.c - Enumerate file descriptors from /proc/<PID>/fd
 *
 * Uses readdir() to list FD directory and readlink() to resolve targets.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include "proc_fd.h"
#include "util.h"
#include "pinspect.h"

/* Initial capacity for FD array (will grow if needed) */
#define INITIAL_FD_CAPACITY 64

/*
 * Check if a directory entry name is numeric (represents an FD).
 * Returns true if name consists only of digits.
 */
static bool is_numeric(const char *name)
{
    if (name == NULL || *name == '\0') {
        return false;
    }

    if(strcmp(name, ".") == 0|| strcmp(name, "..") == 0){
        return false;
    }
    
    const char *c = name;
    while(*c != '\0'){
        if(!isdigit((unsigned char)*c)){
            return false;
        }

        c++;
    }

    return true;
}

/*
 * Resolve a file descriptor symlink to its target.
 * Returns 0 on success, -1 on error (e.g., ENOENT if FD closed mid-read).
 */
static int resolve_fd_target(const char *path, char *target, size_t size)
{
    ssize_t len = readlink(path, target, size - 1);
    
    if(len < 0){
        return -1;
    }
    
    target[len] = '\0';

    return 0;

}

/*
 * Parse an fd_entry_t from a directory entry and its resolved target.
 * Populates fd number, target path, and socket detection fields.
 */
static void parse_fd_entry(const char *name, const char *target,
                           fd_entry_t *entry)
{
    entry->fd = atoi(name);

     strncpy(entry->target, target, sizeof(entry->target) - 1);
     entry->target[sizeof(entry->target) - 1] = '\0';

     if(parse_socket_inode(target, &entry->socket_inode)){
        entry->is_socket = true;
     }else{
        entry->is_socket = false;
        entry->socket_inode = 0;
     }
}

/*
 * Enumerate all file descriptors for a process.
 * Returns 0 on success with heap-allocated entries array, -1 on error.
 * Caller must free with fd_entries_free().
 */
int enumerate_fds(pid_t pid, fd_entry_t **entries, int *count)
{
    *entries = NULL;
    *count = 0;

    char path_buf[256];
    if (build_proc_path(pid, "fd", path_buf, sizeof(path_buf)) != 0) {
        return -1;
    }

    DIR *dir = opendir(path_buf);
    if (dir == NULL) {
        return -1;  // errno is set (ENOENT, EACCES, etc.)
    }

    int capacity = INITIAL_FD_CAPACITY;  // Start with room for 64 entries
    int num_fds = 0;                     // How many we've actually found

    // Allocate array on the heap
    fd_entry_t *array = malloc(capacity * sizeof(fd_entry_t));
    if (array == NULL) {
        closedir(dir);  // Don't leak the directory
        return -1;      // ENOMEM
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {

        if (!is_numeric(entry->d_name)) {
            continue;
        }

        // Build full path to /proc/<pid>/fd/<fd_num>
        char fd_path[PATH_MAX];
        snprintf(fd_path, sizeof(fd_path), "%s/%s", path_buf, entry->d_name);

        // Resolve the symlink
        char target[PATH_MAX];
        if (resolve_fd_target(fd_path, target, sizeof(target)) != 0) {
            continue;  // FD may have closed between readdir and readlink
        }

        // Parse and store the entry in our array
        parse_fd_entry(entry->d_name, target, &array[num_fds]);
        num_fds++;

        // Grow array if we've reached capacity
        if (num_fds == capacity) {
            capacity *= 2;
            fd_entry_t *new_array = realloc(array, capacity * sizeof(fd_entry_t));
            if (new_array == NULL) {
                // Allocation failed - cleanup and return error
                free(array);
                closedir(dir);
                return -1;
            }
            array = new_array;
        }
    }

    closedir(dir);

    // Handle case where process has no open FDs (rare but valid)
    if (num_fds == 0) {
        free(array);
        *entries = NULL;
        *count = 0;
        return 0;
    }

    // Shrink array to exact size 
    fd_entry_t *final_array = realloc(array, num_fds * sizeof(fd_entry_t));
    if (final_array != NULL) {
        array = final_array;
    }
    // If realloc fails, original array is still valid, so we can continue

    // Set output parameters and return success
    *entries = array;
    *count = num_fds;
    return 0;
}

void fd_entries_free(fd_entry_t *entries)
{
    free(entries);
}

bool parse_socket_inode(const char *target, unsigned long *inode)
{
    if (target == NULL) {
        return false;
    }
    return sscanf(target, "socket:[%lu]", inode) == 1;
}
