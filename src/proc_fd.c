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
 * Check if directory entry is numeric (filter out "." and "..").
 */
static bool is_numeric(const char *name)
{
    if (name == NULL || *name == '\0') {
        return false;
    }

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return false;
    }

    const char *c = name;
    while (*c != '\0') {
        if (!isdigit((unsigned char)*c)) {
            return false;
        }
        c++;
    }

    return true;
}

/*
 * Resolve FD symlink to target.
 * Note: readlink() does not null-terminate, so we must do it explicitly.
 * Returns 0 on success, -1 on error.
 */
static int resolve_fd_target(const char *path, char *target, size_t size)
{
    ssize_t len = readlink(path, target, size - 1);

    if (len < 0) {
        return -1;
    }

    target[len] = '\0';  /* Critical: readlink() doesn't null-terminate */
    return 0;
}

/*
 * Parse FD entry from directory name and resolved symlink target.
 */
static void parse_fd_entry(const char *name, const char *target,
                           fd_entry_t *entry)
{
    entry->fd = atoi(name);

    strncpy(entry->target, target, sizeof(entry->target) - 1);
    entry->target[sizeof(entry->target) - 1] = '\0';

    if (parse_socket_inode(target, &entry->socket_inode)) {
        entry->is_socket = true;
    } else {
        entry->is_socket = false;
        entry->socket_inode = 0;
    }
}

/*
 * Implementation of enumerate_fds() - see proc_fd.h for API docs.
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
        return -1;
    }

    int capacity = INITIAL_FD_CAPACITY;
    int num_fds = 0;

    fd_entry_t *array = malloc(capacity * sizeof(fd_entry_t));
    if (array == NULL) {
        closedir(dir);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!is_numeric(entry->d_name)) {
            continue;
        }

        char fd_path[PATH_MAX];
        snprintf(fd_path, sizeof(fd_path), "%s/%s", path_buf, entry->d_name);

        char target[PATH_MAX];
        if (resolve_fd_target(fd_path, target, sizeof(target)) != 0) {
            /* TOCTOU race: FD closed between readdir and readlink */
            continue;
        }

        parse_fd_entry(entry->d_name, target, &array[num_fds]);
        num_fds++;

        /* Double capacity when full (amortized O(1) insertion) */
        if (num_fds == capacity) {
            capacity *= 2;
            fd_entry_t *new_array = realloc(array,
                                            capacity * sizeof(fd_entry_t));
            if (new_array == NULL) {
                free(array);
                closedir(dir);
                return -1;
            }
            array = new_array;
        }
    }

    closedir(dir);

    if (num_fds == 0) {
        free(array);
        *entries = NULL;
        *count = 0;
        return 0;
    }

    /* Shrink to exact size to minimize memory footprint */
    fd_entry_t *final_array = realloc(array, num_fds * sizeof(fd_entry_t));
    if (final_array != NULL) {
        array = final_array;
    }

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
