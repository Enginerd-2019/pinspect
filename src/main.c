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
#include "util.h"

#define PROGRAM_NAME "pinspect"
#define VERSION "0.1.0"

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

static int parse_args(int argc, char *argv[])
{
    /* TODO: Implement argument parsing
     *
     * Use getopt_long() for both short and long options:
     * - "-v" / "--verbose"  -> options.verbose = true
     * - "-n" / "--network"  -> options.network_only = true
     * - "-h" / "--help"     -> options.help = true
     * - "-V" / "--version"  -> options.version = true
     *
     * After options, expect exactly one positional argument (PID).
     * Use parse_pid() from util.c to validate and convert.
     *
     * Return 0 on success, -1 on error (invalid options or PID).
     */

    // Argument mapping
    static struct option long_options[] = {

        {"verbose", no_argument, NULL, 'v'},
        {"network", no_argument, NULL, 'n'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {NULL,      0,           NULL,  0 }  /* Terminator */
    };  

    int opt;

    while((opt = getopt_long(argc, argv, "vnhV", long_options, NULL)) != -1){

        switch (opt)
        {
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
        
        // Unknown case
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

    // Check for pid
    if(options.pid == -1){
        fprintf(stderr, "Invalid PID: %s\n", argv[optind]);
        return -1;
    }

    return 0;
}

static void print_process_info(const proc_info_t *info)
{
    /* TODO: Implement output formatting
     *
     * Format similar to:
     *
     * Process: firefox (PID 1234)
     * State:   Running
     * UID:     1000 (real), 1000 (effective)
     * GID:     1000 (real), 1000 (effective)
     * Memory:  VmSize: 2.1 GB, VmRSS: 512 MB, VmPeak: 2.5 GB
     * Threads: 47
     *
     * Notes:
     * - Use state_to_string() for state display
     * - Consider using format_memory_size() for human-readable memory
     * - Handle zero Vm* values gracefully (zombies, kernel threads)
     */
    printf("%-10s %s (PID %d)\n", "Process:", info->name, info->pid);
    printf("%-10s %s\n", "State:", state_to_string(info->state));
    printf("%-10s %u (real), %u (effective)\n", "UID:", info->uid_real, info->uid_effective);
    printf("Memory:    VmSize: %lu KB, VmRSS: %lu KB, VmPeak: %lu KB\n", 
       info->vm_size_kb, info->vm_rss_kb, info->vm_peak_kb);
    printf("Threads:   %d\n", info->thread_count);

}


/* TODO: Week 3 - Add print_file_descriptors() for FD list output */

/* TODO: Week 4 - Add print_network_connections() for socket info */

int main(int argc, char *argv[])
{
    /* TODO: Implement main flow
     *
     * Steps:
     * 1. Parse command-line arguments
     * 2. Handle --help and --version early exits
     * 3. Validate PID exists using pid_exists()
     * 4. Read process status with read_proc_status()
     * 5. Print process information
     * 6. TODO Week 3: Enumerate file descriptors if not --network
     * 7. TODO Week 4: Find network connections if --network or --verbose
     * 8. Return appropriate exit code
     *
     * Exit codes:
     * - 0: Success
     * - 1: Invalid arguments
     * - 2: Process not found
     * - 3: Permission denied
     */

    if (parse_args(argc, argv) != 0) {
        fprintf(stderr, "Try '%s --help' for more information.\n", PROGRAM_NAME);
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

    /* TODO: Validate PID exists */

    /* TODO: Read and display process info */
    proc_info_t info;
    if (read_proc_status(options.pid, &info) != 0) {
        fprintf(stderr, "%s: cannot read process %d: %s\n",
                PROGRAM_NAME, options.pid, strerror(errno));
        return (errno == ENOENT) ? 2 : 3;
    }

    print_process_info(&info);

    /* TODO: Week 3 - Add FD enumeration here */

    /* TODO: Week 4 - Add network connection lookup here */

    return 0;
}
