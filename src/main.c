/*
 * main.c - pinspect entry point
 *
 * Handles argument parsing, orchestrates data collection, and formats output.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include "pinspect.h"
#include "proc_status.h"
#include "proc_fd.h"
#include "proc_task.h"
#include "net.h"
#include "util.h"

#define PROGRAM_NAME "pinspect"
#define VERSION "1.0.0"

/* Command-line options */
static struct {
    bool verbose;
    bool network_only;
    bool help;
    bool version;
    pid_t pid;
} options = {0};

static void print_usage(void)
{
    printf("Usage: %s [OPTIONS] <PID>\n", PROGRAM_NAME);
    printf("\n");
    printf("Inspect Linux process information via /proc filesystem.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -v, --verbose    Show detailed file descriptor information\n");
    printf("  -n, --network    Show network connections only\n");
    printf("  -h, --help       Display this help message\n");
    printf("  -V, --version    Display version information\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s 1234          Inspect process 1234\n", PROGRAM_NAME);
    printf("  %s -v $$         Inspect current shell (verbose)\n", PROGRAM_NAME);
    printf("  %s -n $(pgrep firefox)  Show Firefox network connections\n",
           PROGRAM_NAME);
}

static void print_version(void)
{
    printf("%s version %s\n", PROGRAM_NAME, VERSION);
}

/*
 * Display network connections for a process.
 *
 * In normal mode: Just show count
 * In verbose mode: Show detailed list of all connections
 */
static void print_network_connections(pid_t pid, bool verbose)
{
    socket_info_t *sockets = NULL;
    int count = 0;

    if (find_process_sockets(pid, &sockets, &count) != 0) {
        printf("\nNetwork Connections: Unable to read (permission denied)\n");
        return;
    }

    printf("\nNetwork Connections: %d open\n", count);

    if (verbose && count > 0) {
        printf("\n  Proto  Local Address          Remote Address         State\n");
        printf("  -----  ---------------------  ---------------------  -----------\n");

        for (int i = 0; i < count; i++) {
            char local[32], remote[32];
            format_ip_port(sockets[i].local_addr, sockets[i].local_port,
                          local, sizeof(local));
            format_ip_port(sockets[i].remote_addr, sockets[i].remote_port,
                          remote, sizeof(remote));

            printf("  %-5s  %-21s  %-21s  %s\n",
                   sockets[i].is_tcp ? "TCP" : "UDP",
                   local,
                   remote,
                   tcp_state_to_string(sockets[i].state));
        }
    }

    socket_list_free(sockets);
}

/*
 * Parse command-line arguments using getopt_long().
 * Returns 0 on success, -1 on error.
 */
static int parse_args(int argc, char *argv[])
{
    static struct option long_options[] = {
        {"verbose", no_argument, NULL, 'v'},
        {"network", no_argument, NULL, 'n'},
        {"help",    no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {NULL,      0,           NULL,  0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "vnhV", long_options, NULL)) != -1) {
        switch (opt) {
        case 'v':
            options.verbose = true;
            break;
        case 'n':
            options.network_only = true;
            break;
        case 'h':
            options.help = true;
            break;
        case 'V':
            options.version = true;
            break;
        case '?':
            fprintf(stderr, "Unknown Argument: %s\n", argv[optind]);
            return -1;
        }
    }

    /* Help and version don't require a PID */
    if (options.help || options.version) {
        return 0;
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected a PID argument\n");
        return -1;
    }

    options.pid = parse_pid(argv[optind]);
    if (options.pid == -1) {
        fprintf(stderr, "Invalid PID: %s\n", argv[optind]);
        return -1;
    }

    return 0;
}

/*
 * Format and display process information.
 * Memory values are in KB; zero values indicate zombie or kernel thread.
 */
static void print_process_info(const proc_info_t *info)
{
    printf("%-10s %s (PID %d)\n", "Process:", info->name, info->pid);
    printf("%-10s %s\n", "State:", state_to_string(info->state));
    printf("%-10s %u (real), %u (effective)\n", "UID:",
           info->uid_real, info->uid_effective);
    printf("Memory:    VmSize: %lu KB, VmRSS: %lu KB, VmPeak: %lu KB\n",
           info->vm_size_kb, info->vm_rss_kb, info->vm_peak_kb);
    printf("Threads:   %d\n", info->thread_count);
}


/*
 * Display file descriptor information for a process.
 *
 * In normal mode: Just show count
 * In verbose mode: Show detailed list of all FDs
 */
static void print_file_descriptors(pid_t pid, bool verbose)
{
    fd_entry_t *fds = NULL;
    int count = 0;

    if (enumerate_fds(pid, &fds, &count) != 0) {
        /* Graceful degradation for permission errors or race conditions */
        printf("\nFile Descriptors: Unable to read (permission denied)\n");
        return;
    }

    printf("\nFile Descriptors: %d open\n", count);

    if (verbose && count > 0) {
        printf("\n  FD    Type      Target\n");
        printf("  ----  --------  ----------------------------------------\n");

        for (int i = 0; i < count; i++) {
            const char *type = fds[i].is_socket ? "socket" : "file";
            printf("  %-4d  %-8s  %s\n",
                   fds[i].fd,
                   type,
                   fds[i].target);
        }
    }

    fd_entries_free(fds);
}

/*
 * Display thread information for a process.
 *
 * In normal mode: Just show count (already from proc_status)
 * In verbose mode: Show detailed list of all threads
 */
static void print_threads(pid_t pid, bool verbose)
{
    if (!verbose) {
        return;  // Thread count already shown by print_process_info()
    }

    thread_info_t *threads = NULL;
    int count = 0;

    if (enumerate_threads(pid, &threads, &count) != 0) {
        printf("\nThreads: Unable to enumerate (permission denied)\n");
        return;
    }

    printf("\nThread Details:\n");
    printf("  TID     State       Name\n");
    printf("  ------  ----------  ----------------\n");

    for (int i = 0; i < count; i++) {
        printf("  %-6d  %-10s  %s\n",
               threads[i].tid,
               state_to_string(threads[i].state),
               threads[i].name);
    }

    thread_info_free(threads);
}

/*
 * Main entry point for pinspect.
 *
 * Exit codes:
 * - 0: Success
 * - 1: Invalid arguments
 * - 2: Process not found
 * - 3: Permission denied
 */
int main(int argc, char *argv[])
{
    if (parse_args(argc, argv) != 0) {
        fprintf(stderr, "Try '%s --help' for more information.\n",
                PROGRAM_NAME);
        return 1;
    }

    if (options.help) {
        print_usage();
        return 0;
    }

    if (options.version) {
        print_version();
        return 0;
    }

    /* Read and display process info */
    proc_info_t info;
    if (read_proc_status(options.pid, &info) != 0) {
        fprintf(stderr, "%s: cannot read process %d: %s\n",
                PROGRAM_NAME, options.pid, strerror(errno));
        return (errno == ENOENT) ? 2 : 3;
    }

    if (options.network_only) {
        print_network_connections(options.pid, options.verbose);
    } else {
        print_process_info(&info);
        print_file_descriptors(options.pid, options.verbose);
        print_threads(options.pid, options.verbose);
        print_network_connections(options.pid, options.verbose);
    }

    return 0;
}
