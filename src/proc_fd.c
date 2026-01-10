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
 *
 * /proc/<pid>/fd/ contains numeric entries (0, 1, 2, ...) plus "." and "..".
 * We only want the numeric ones.
 *
 * Returns true if name is all digits.
 */
static bool is_numeric(const char *name)
{
    /* TODO: Iterate through the string 'name'
     *
     * For each character:
     * - Check if it's a digit using isdigit()
     * - If any character is not a digit, return false
     *
     * Edge cases to handle:
     * - Empty string should return false
     * - "." and ".." should return false
     *
     * Return true only if all characters are digits.
     */   

    if(name == NULL || *name == '\0'){
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
 *
 * Uses readlink() to determine what the FD points to.
 * Does NOT follow further symlinks (we want the immediate target).
 *
 * Parameters:
 *   path   - Full path to /proc/<pid>/fd/<fd_num>
 *   target - Output buffer for resolved target
 *   size   - Size of target buffer
 *
 * Returns 0 on success, -1 on error.
 * Common errors: ENOENT (FD closed between readdir and readlink).
 */
static int resolve_fd_target(const char *path, char *target, size_t size)
{
    /* TODO: Use readlink() to resolve the symlink
     *
     * Steps:
     * 1. Call readlink(path, target, size - 1)
     *    - Note: size - 1 leaves room for null terminator
     * 2. Check return value:
     *    - If < 0, return -1 (error, errno is set by readlink)
     *    - If >= 0, that's the length of the resolved path
     * 3. Null-terminate the result: target[len] = '\0'
     *    - IMPORTANT: readlink() does NOT null-terminate!
     * 4. Return 0 for success
     *
     * Example targets you'll see:
     * - Regular files: "/home/user/file.txt"
     * - Pipes: "pipe:[12345]"
     * - Sockets: "socket:[67890]"
     * - Anonymous inodes: "anon_inode:[eventfd]"
     * - Deleted files: "/tmp/file (deleted)"
     *
     * See: man 2 readlink
     */

    ssize_t len = readlink(path, target, size - 1);
    
    if(len < 0){
        return -1;
    }
    
    target[len] = '\0';

    return 0;

}

/*
 * Parse an fd_entry_t from a directory entry and its resolved target.
 *
 * Fills in all fields of fd_entry_t including socket detection.
 */
static void parse_fd_entry(const char *name, const char *target,
                           fd_entry_t *entry)
{
    /* TODO: Parse the FD number and populate the entry
     *
     * Steps:
     * 1. Convert name (string) to integer using atoi() or strtol()
     *    - Store in entry->fd
     *
     * 2. Copy target string to entry->target
     *    - Use strncpy(entry->target, target, sizeof(entry->target) - 1)
     *    - Ensure null termination: entry->target[PATH_MAX - 1] = '\0'
     *
     * 3. Check if target is a socket using parse_socket_inode()
     *    - If true, set entry->is_socket = true and store the inode
     *    - If false, set entry->is_socket = false and socket_inode = 0
     *
     * Example:
     * - name = "3", target = "socket:[12345]"
     *   -> fd=3, is_socket=true, socket_inode=12345
     * - name = "4", target = "/dev/null"
     *   -> fd=4, is_socket=false, socket_inode=0
     */

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
 *
 * Main algorithm:
 * 1. Open /proc/<pid>/fd/ directory
 * 2. Allocate initial array for entries
 * 3. Read directory entries, filter numeric ones
 * 4. For each FD, resolve symlink and parse entry
 * 5. Grow array if needed
 * 6. Return array and count
 */
int enumerate_fds(pid_t pid, fd_entry_t **entries, int *count)
{
    /* TODO: Implement the main enumeration logic
     *
     * High-level algorithm:
     *
     * 1. Initialize outputs to safe defaults:
     *    *entries = NULL;
     *    *count = 0;
     *
     * 2. Build path to /proc/<pid>/fd using build_proc_path()
     *    - file parameter should be "fd"
     *
     * 3. Open directory with opendir()
     *    - If NULL, return -1 (errno is set: ENOENT or EACCES)
     *
     * 4. Allocate initial array:
     *    capacity = INITIAL_FD_CAPACITY
     *    array = malloc(capacity * sizeof(fd_entry_t))
     *    - Check for NULL (ENOMEM), cleanup and return -1
     *
     * 5. Read directory entries in a loop with readdir():
     *    while ((entry = readdir(dir)) != NULL) {
     *        - Skip if !is_numeric(entry->d_name)
     *        - Build full path to /proc/<pid>/fd/<fd_num>
     *          Use snprintf() to combine dir_path and entry->d_name
     *        - Call resolve_fd_target() to get symlink target
     *          If it fails, continue to next entry (FD may have closed)
     *        - Call parse_fd_entry() to fill array[num_fds]
     *        - Increment num_fds++
     *        - If num_fds == capacity, grow array:
     *          capacity *= 2
     *          realloc() to new size
     *          Check for NULL, cleanup and return -1
     *    }
     *
     * 6. Close directory with closedir()
     *
     * 7. If num_fds == 0:
     *    - free(array)
     *    - *entries = NULL
     *    - *count = 0
     *    - Return 0 (success, just no FDs - rare but valid)
     *
     * 8. Optional: shrink array to exact size with realloc()
     *    - Not required, but saves memory
     *
     * 9. Set outputs and return success:
     *    *entries = array
     *    *count = num_fds
     *    return 0
     *
     * Error handling considerations:
     * - readdir() errors: Check errno after loop if needed
     * - Individual readlink() failures: Expected (TOCTOU), skip entry
     * - Permission denied on directory: Return -1 early
     * - Process exits during read: Some entries fail, return partial list
     *
     * Memory management:
     * - On any error after malloc, free the array before returning -1
     * - Track allocated memory carefully during realloc
     *
     * See: man 3 opendir, man 3 readdir, man 3 closedir
     */

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
    /* TODO: Implement cleanup function
     *
     * This is simple:
     * - Check if entries != NULL
     * - If not NULL, call free(entries)
     *
     * Why this is separate from enumerate_fds():
     * - Clear ownership: caller knows to use this to cleanup
     * - Consistent with other modules (see memory management in CLAUDE.md)
     * - Safe to call multiple times or with NULL
     */

    if(entries != NULL){
       free(entries);
    }
}

bool parse_socket_inode(const char *target, unsigned long *inode)
{
    /* TODO: Extract inode from socket symlink target
     *
     * Socket targets have format: "socket:[12345]"
     *
     * Implementation:
     * 1. Use sscanf() to parse the format:
     *    int matched = sscanf(target, "socket:[%lu]", inode);
     *
     * 2. Check if exactly 1 item was matched:
     *    if (matched == 1) {
     *        return true;   // Success, inode is populated
     *    }
     *
     * 3. Return false if format doesn't match
     *
     * Examples:
     * - "socket:[67890]" -> inode=67890, return true
     * - "pipe:[12345]"   -> return false (not a socket)
     * - "/dev/null"      -> return false
     * - "anon_inode:[eventfd]" -> return false
     *
     * Note: We only care about sockets for Week 4 network correlation.
     * Other special targets (pipes, eventfd, etc.) are informational only.
     */

     return sscanf(target, "socket:[%lu]", inode) == 1;
}

/* TODO: Week 4 - Implement fd_entry_has_inode() helper
 *
 * bool fd_entry_has_inode(const fd_entry_t *entries, int count,
 *                         unsigned long inode);
 *
 * Purpose: Given an array of FD entries and a socket inode number,
 * check if any entry matches that inode.
 *
 * Algorithm:
 * 1. Iterate through entries array
 * 2. For each entry, check if:
 *    - entry.is_socket == true AND
 *    - entry.socket_inode == inode
 * 3. Return true if found, false if not found in entire array
 *
 * Use case: When parsing /proc/net/tcp, we get socket inodes.
 * This helper tells us if any of those sockets belong to our process.
 *
 * See CLAUDE.md Week 4 algorithm for context.
 */
