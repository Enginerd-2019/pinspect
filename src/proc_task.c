/*
 * proc_task.c - Enumerate threads from /proc/<PID>/task
 *
 * Uses readdir() to list task directory and reads per-thread
 * comm and status files for thread details.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include "proc_task.h"
#include "util.h"

/* Initial capacity for thread array (will grow if needed) */
#define INITIAL_THREAD_CAPACITY 32

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
 * Read thread name from /proc/<pid>/task/<tid>/comm.
 * Returns 0 on success, -1 on error (uses "???" as fallback name).
 */
static int read_thread_name(pid_t pid, pid_t tid, char *name, size_t size)
{
    char path[PATH_MAX];
    if (build_task_path(pid, tid, "comm", path, sizeof(path)) != 0) {
        strncpy(name, "???", size - 1);
        name[size - 1] = '\0';
        return -1;
    }

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        /* TOCTOU race: thread exited between readdir and fopen */
        strncpy(name, "???", size - 1);
        name[size - 1] = '\0';
        return -1;
    }

    if (fgets(name, size, fp) == NULL) {
        strncpy(name, "???", size - 1);
        name[size - 1] = '\0';
        fclose(fp);
        return -1;
    }

    fclose(fp);

    /* Strip trailing newline from comm file */
    size_t len = strlen(name);
    if (len > 0 && name[len - 1] == '\n') {
        name[len - 1] = '\0';
    }

    return 0;
}

/*
 * Read thread state from /proc/<pid>/task/<tid>/status.
 * Returns PROC_STATE_UNKNOWN on error.
 */
static proc_state_t read_thread_state(pid_t pid, pid_t tid)
{
    char path[PATH_MAX];
    if (build_task_path(pid, tid, "status", path, sizeof(path)) != 0) {
        return PROC_STATE_UNKNOWN;
    }

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        /* TOCTOU race: thread exited between readdir and fopen */
        return PROC_STATE_UNKNOWN;
    }

    char line[256];
    proc_state_t state = PROC_STATE_UNKNOWN;

    while (fgets(line, sizeof(line), fp) != NULL) {
        char state_char;
        if (sscanf(line, "State: %c", &state_char) == 1) {
            state = char_to_state(state_char);
            break;
        }
    }

    fclose(fp);
    return state;
}

/*
 * Implementation of enumerate_threads() - see proc_task.h for API docs.
 */
int enumerate_threads(pid_t pid, thread_info_t **threads, int *count)
{
    *threads = NULL;
    *count = 0;

    char path[PATH_MAX];
    if (build_proc_path(pid, "task", path, sizeof(path)) != 0) {
        return -1;
    }

    DIR *dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }

    int capacity = INITIAL_THREAD_CAPACITY;
    int num_threads = 0;

    thread_info_t *array = malloc(capacity * sizeof(thread_info_t));
    if (array == NULL) {
        closedir(dir);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!is_numeric(entry->d_name)) {
            continue;
        }

        pid_t tid = atoi(entry->d_name);

        read_thread_name(pid, tid, array[num_threads].name,
                        sizeof(array[num_threads].name));
        array[num_threads].tid = tid;
        array[num_threads].state = read_thread_state(pid, tid);

        num_threads++;

        /* Double capacity when full (amortized O(1) insertion) */
        if (num_threads == capacity) {
            capacity *= 2;
            thread_info_t *new_array = realloc(array,
                                              capacity * sizeof(thread_info_t));
            if (new_array == NULL) {
                free(array);
                closedir(dir);
                return -1;
            }
            array = new_array;
        }
    }

    closedir(dir);

    if (num_threads == 0) {
        free(array);
        *threads = NULL;
        *count = 0;
        return 0;
    }

    /* Shrink to exact size to minimize memory footprint */
    thread_info_t *final_array = realloc(array,
                                         num_threads * sizeof(thread_info_t));
    if (final_array != NULL) {
        array = final_array;
    }

    *threads = array;
    *count = num_threads;
    return 0;
}

void thread_info_free(thread_info_t *threads)
{
    free(threads);
}