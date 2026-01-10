/*
 * pinspect.h - Core types and constants for pinspect
 *
 * Main header defining data structures used across all modules.
 */

#ifndef PINSPECT_H
#define PINSPECT_H

#include <sys/types.h>
#include <stdbool.h>
#include <linux/limits.h>

/* Constants */
#define PROC_ROOT "/proc"
#define PROC_NAME_MAX 16  /* Kernel truncates to 15 chars + null */

/* Process states from /proc/<PID>/status */
typedef enum {
    PROC_STATE_RUNNING,      /* R */
    PROC_STATE_SLEEPING,     /* S */
    PROC_STATE_DISK_SLEEP,   /* D */
    PROC_STATE_ZOMBIE,       /* Z */
    PROC_STATE_STOPPED,      /* T */
    PROC_STATE_IDLE,         /* I */
    PROC_STATE_UNKNOWN
} proc_state_t;

/*
 * Basic process info from /proc/<PID>/status
 *
 * TODO: Consider adding ppid (parent PID) field
 * TODO: Consider adding voluntary/nonvoluntary context switches
 */
typedef struct {
    pid_t pid;
    char name[PROC_NAME_MAX];
    proc_state_t state;
    uid_t uid_real;
    uid_t uid_effective;
    gid_t gid_real;
    gid_t gid_effective;
    unsigned long vm_size_kb;
    unsigned long vm_rss_kb;
    unsigned long vm_peak_kb;
    int thread_count;
} proc_info_t;

/*
 * File descriptor entry - describes one open file descriptor.
 *
 * Each entry in /proc/<pid>/fd/ is a symlink. We capture the FD number
 * and what it points to.
 */
typedef struct {
    int fd;                         /* File descriptor number */
    char target[PATH_MAX];          /* Symlink target (what FD points to) */
    bool is_socket;                 /* True if target is a socket */
    unsigned long socket_inode;     /* If is_socket, the inode number */
} fd_entry_t;

/* TODO: Week 4 - Add tcp_state_t enum for TCP connection states */

/* TODO: Week 4 - Add socket_info_t for network socket information */

#endif /* PINSPECT_H */
