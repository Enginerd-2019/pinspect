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
 * Check if a directory entry name is numeric (represents a TID).
 *
 * /proc/<pid>/task/ contains numeric entries (TIDs) plus "." and "..".
 * We only want the numeric ones.
 */
static bool is_numeric(const char *name)
{
    if(name == NULL || *name == '\0'){
        return false;
    }

    if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0){
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
 * Read thread name from /proc/<pid>/task/<tid>/comm.
 *
 * The comm file contains the thread's name (up to 15 characters).
 * Returns 0 on success, -1 on error.
 */
static int read_thread_name(pid_t pid, pid_t tid, char *name, size_t size)
{
    char path[PATH_MAX];
    if(build_task_path(pid, tid, "comm", path, sizeof(path)) != 0){
        strncpy(name, "???", size - 1);
        name[size - 1] = '\0';
        return -1;
    }

    FILE *fp = fopen(path, "r");

    if(fp == NULL){ 
        // Thread may have exited, use a fallback
        strncpy(name, "???", size - 1);
        name[size - 1] = '\0';
        return -1;
    }

    // Read the name (single line in comm, up to 15 chars + newline)
    if(fgets(name, size, fp) == NULL){
        strncpy(name, "???", size -1);
        name[size - 1] = '\0';
        fclose(fp);
        return -1;
    }

    fclose(fp);

    // Remove trailing newline if present
    size_t len = strlen(name);
    if(len > 0 && name[len - 1] == '\n'){
        name[len - 1] = '\0';
    }

    return 0;
}

/*
 * Read thread state from /proc/<pid>/task/<tid>/status.
 *
 * Parses the "State:" line to get the thread's current state.
 * Returns PROC_STATE_UNKNOWN on error.
 */
static proc_state_t read_thread_state(pid_t pid, pid_t tid)
{
    char path[PATH_MAX];
    if (build_task_path(pid, tid, "status", path, sizeof(path)) != 0) {
        return PROC_STATE_UNKNOWN;
    }

    FILE *fp = fopen(path, "r");
    if(fp == NULL){
        // Thread may have exited, use default state
        return PROC_STATE_UNKNOWN;
    }

    char line[256];
    proc_state_t state = PROC_STATE_UNKNOWN;

    while(fgets(line, sizeof(line), fp) != NULL){
        // Look for "State:" line
        char state_char;
        if(sscanf(line, "State: %c", &state_char) == 1){
            state = char_to_state(state_char);
            break;
        }
    }

    fclose(fp);
    return state;
}

/*
 * Enumerate all threads for a process.
 *
 * Returns 0 on success with heap-allocated threads array, -1 on error.
 * Caller must free with thread_info_free().
 */
int enumerate_threads(pid_t pid, thread_info_t **threads, int *count)
{
    *threads = NULL;
    *count = 0;

    // Build path to /proc/<pid>/task first
    char path[PATH_MAX];
    if(build_proc_path(pid, "task", path, sizeof(path)) != 0){
        return -1;
    }

    // Open the /proc/<pid>/task
    DIR *dir = opendir(path);
    if(dir == NULL){
        return -1; // errno will be set to (ENOENT, EACCESS)
    }

    // Allocate initial array
    int capacity = INITIAL_THREAD_CAPACITY;
    int num_threads = 0;

    thread_info_t *array = malloc(capacity * sizeof(thread_info_t));
    if(array == NULL){
        closedir(dir);
        return -1; // ENOMEM
    }

    // Read directory entries
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        
        if(!is_numeric(entry->d_name)){
            continue;
        }

        pid_t tid = atoi(entry->d_name);

        // Read thread details
        read_thread_name(pid, tid, array[num_threads].name, sizeof(array[num_threads].name));
        array[num_threads].tid = tid;
        array[num_threads].state = read_thread_state(pid, tid);

        num_threads++;

        // Grow array if needed
        if(num_threads == capacity){
            capacity *= 2;
            thread_info_t *new_array = realloc(array, capacity * sizeof(thread_info_t));

            if(new_array == NULL){
                free(array);
                closedir(dir);
                return -1;
            }
            array = new_array;
        }
    }

    closedir(dir);

    // Handle edge cases: no threads found 
    if(num_threads == 0){
        free(array);
        *threads = NULL;
        *count = 0;
        return 0;
    }

    // Shrink array to exact size
    thread_info_t *final_array = realloc(array, num_threads * sizeof(thread_info_t));

    if(final_array != NULL){
        array = final_array;
    }

    *threads = array;
    *count = num_threads;
    return 0;
}

/*
 * Free memory allocated by enumerate_threads().
 */
void thread_info_free(thread_info_t *threads)
{
    free(threads);
}