# CLAUDE.md - Process Inspector Development Guidelines

> This is Project 1 of 4 in a 2-year systems programming portfolio targeting remote infrastructure roles at companies like Tailscale, Cloudflare, and HashiCorp.

## Project Context

**What:** CLI tool (`pinspect`) to inspect Linux processes by parsing `/proc` filesystem directly.

**Why:** Demonstrates foundational Linux systems knowledge—the kind that makes you valuable at infrastructure companies. Every line of this code shows you understand how the kernel exposes process information.

**Constraints:**
- 500-1000 lines of C
- Standard library only (no external dependencies)
- C11 standard, POSIX-compliant
- Must be interview-ready code quality

**Timeline:**
- Weeks 1-2: Basic `/proc/<PID>/status` parsing
- Week 3: Threads (`/proc/<PID>/task/`) and file descriptors (`/proc/<PID>/fd/`)
- Week 4: Network connections (`/proc/net/tcp`, `/proc/net/udp`)
- Week 5: Polish and documentation

---

## Architectural Principles

### Module Structure

```
src/
├── main.c          # Entry point, argument parsing, output orchestration
├── proc_status.c   # Parse /proc/<PID>/status (name, state, memory, UIDs)
├── proc_fd.c       # Enumerate /proc/<PID>/fd/ (file descriptors)
├── proc_task.c     # Count /proc/<PID>/task/ entries (threads)
├── net.c           # Parse /proc/net/tcp and /proc/net/udp
└── util.c          # Shared utilities (error handling, string helpers)

include/
├── pinspect.h      # Main header with common types and constants
├── proc_status.h   # Status parsing interface
├── proc_fd.h       # File descriptor enumeration interface
├── proc_task.h     # Thread counting interface
├── net.h           # Network parsing interface
└── util.h          # Utility function declarations
```

**Module Evolution Strategy:**
- Start with `main.c` + `proc_status.c` + `util.c` in Weeks 1-2
- Add `proc_task.c` and `proc_fd.c` in Week 3
- Add `net.c` in Week 4
- Keep modules loosely coupled—each can be understood in isolation

### Data Flow

```
main()
  → validate_pid()           [util.c]
  → read_proc_status()       [proc_status.c]  → returns ProcessInfo struct
  → count_threads()          [proc_task.c]    → returns int
  → enumerate_fds()          [proc_fd.c]      → returns FDList struct
  → find_process_sockets()   [net.c]          → returns SocketList struct
  → print_output()           [main.c]
```

### Core Types

Define these in `pinspect.h`:

```c
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

/* Basic process info from /proc/<PID>/status */
typedef struct {
    pid_t pid;
    char name[16];           /* Truncated to 15 chars by kernel */
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

/* File descriptor entry */
typedef struct {
    int fd;
    char target[PATH_MAX];   /* What the FD points to */
    bool is_socket;
    unsigned long socket_inode;  /* If is_socket, the inode number */
} fd_entry_t;

/* TCP connection state */
typedef enum {
    TCP_ESTABLISHED = 1,
    TCP_SYN_SENT,
    TCP_SYN_RECV,
    TCP_FIN_WAIT1,
    TCP_FIN_WAIT2,
    TCP_TIME_WAIT,
    TCP_CLOSE,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK,
    TCP_LISTEN,
    TCP_CLOSING
} tcp_state_t;

/* Network socket info */
typedef struct {
    bool is_tcp;             /* true = TCP, false = UDP */
    uint32_t local_addr;
    uint16_t local_port;
    uint32_t remote_addr;
    uint16_t remote_port;
    tcp_state_t state;       /* Only meaningful for TCP */
    unsigned long inode;
} socket_info_t;
```

### Error Handling Philosophy

**Fail Fast, Fail Descriptively:**

```c
/* Good: Specific error with context */
if (access(proc_path, R_OK) != 0) {
    fprintf(stderr, "pinspect: cannot read %s: %s\n",
            proc_path, strerror(errno));
    return -1;
}

/* Bad: Silent failure or generic message */
if (access(proc_path, R_OK) != 0)
    return -1;  /* Caller has no idea what went wrong */
```

**Error Return Convention:**
- Functions returning `int`: return `0` on success, `-1` on error
- Functions returning pointers: return `NULL` on error
- Always set/preserve `errno` when propagating system errors
- Print user-facing errors in `main.c`, not in library functions

**Graceful Degradation:**
- Missing permissions? Print what we can, note what we can't.
- Process disappeared mid-read? Report it cleanly, don't crash.
- Unexpected format in `/proc`? Log and continue, don't abort.

### Memory Management Rules

**Prefer Stack Allocation:**
```c
/* Good: Stack-allocated buffer for known-bounded data */
char path[PATH_MAX];
char line[256];
proc_info_t info;

/* Bad: Unnecessary heap allocation */
char *path = malloc(PATH_MAX);  /* Now you must track and free it */
```

**When Heap is Necessary:**
- Variable-length lists (file descriptors, sockets)
- Data that must outlive the function scope

**Explicit Ownership:**
```c
/* Document who frees what */
/* Caller must free returned list with fd_list_free() */
fd_entry_t *enumerate_fds(pid_t pid, int *count);

/* Cleanup function for each allocated type */
void fd_list_free(fd_entry_t *list, int count);
```

**No Hidden Allocations:**
- If a function allocates, its name or documentation makes it clear
- Prefer output parameters over returning allocated memory when practical

---

## Code Quality Standards

### Style Guide

**Indentation:** 4 spaces (no tabs)

**Line Length:** 80 characters maximum

**Naming:**
- Functions: `snake_case` (e.g., `read_proc_status`, `parse_tcp_line`)
- Types: `snake_case_t` for typedefs (e.g., `proc_info_t`, `tcp_state_t`)
- Constants/Macros: `UPPER_SNAKE_CASE` (e.g., `MAX_PATH_LEN`, `PROC_ROOT`)
- Local variables: `snake_case`, short but descriptive

**Braces:** K&R style
```c
if (condition) {
    /* ... */
} else {
    /* ... */
}

/* Single-line bodies still get braces */
if (error) {
    return -1;
}
```

**Function Order in Files:**
1. Static helper functions (forward-declared at top if needed)
2. Public API functions (matching header declaration order)

### Comment Philosophy

**Comment WHY, not WHAT:**

```c
/* Good: Explains the non-obvious reason */
/*
 * Kernel stores IP in host byte order, but prints it as hex.
 * On little-endian x86, bytes appear reversed.
 * inet_ntoa() handles the conversion.
 */
struct in_addr addr = { .s_addr = ip_hex };

/* Bad: Restates the code */
/* Convert hex to in_addr */
struct in_addr addr = { .s_addr = ip_hex };
```

**Document Edge Cases:**
```c
/*
 * Zombie processes have no Vm* fields in status.
 * We set memory values to 0 rather than treating as error.
 */
if (info->state == PROC_STATE_ZOMBIE) {
    info->vm_size_kb = 0;
    info->vm_rss_kb = 0;
}
```

**Function Documentation:**
```c
/*
 * Read process info from /proc/<pid>/status.
 *
 * Returns 0 on success, -1 on error.
 * On ENOENT (process exited), returns -1 with errno set.
 * On EACCES (permission denied), returns partial info where possible.
 */
int read_proc_status(pid_t pid, proc_info_t *info);
```

### Memory Safety Requirements

**Valgrind Clean:**
Every release build must pass:
```bash
valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 \
    ./pinspect $$
```

**Sanitizer Clean:**
Debug builds include ASan/UBSan. All tests must pass without sanitizer errors.

**Buffer Safety:**
```c
/* Good: Bounded reads */
if (fgets(line, sizeof(line), fp) == NULL) { ... }
sscanf(line, "%15s", name);  /* Limit matches buffer size */

/* Bad: Unbounded operations */
gets(line);                   /* Never use gets() */
sscanf(line, "%s", name);     /* No length limit */
strcpy(dest, src);            /* Use strncpy or snprintf */
```

### Portfolio-Ready Code Quality

**This code will be reviewed by interviewers.** Every file should demonstrate:

1. **Consistent Style:** No mixed conventions, no sloppy formatting
2. **Clear Structure:** Functions are focused, files are organized
3. **Proper Error Handling:** No ignored return values, no silent failures
4. **No Dead Code:** Remove commented-out code, unused variables
5. **No TODOs in Final Code:** Address them or remove them before release

---

## Build and Testing

### Makefile Targets

```makefile
# Development build (with sanitizers)
make           # or: make all

# Release build (optimized, no sanitizers)
make release

# Run tests
make test

# Memory leak check
make valgrind

# Format code
make format

# Check formatting without modifying
make format-check

# Clean build artifacts
make clean
```

### Compilation Flags

**Debug Build (default):**
```
-Wall -Wextra -Werror -pedantic -std=c11 -g
-fsanitize=address,undefined
```

**Release Build:**
```
-Wall -Wextra -Werror -pedantic -std=c11 -O2
```

### Testing Approach

**Manual Testing Scenarios:**
```bash
# Own process
./pinspect $$

# System process (needs root for full info)
./pinspect 1
sudo ./pinspect 1

# Process with many threads
./pinspect $(pgrep -o firefox)

# Process with network connections
./pinspect $(pgrep -o ssh)

# Invalid PID
./pinspect 99999999

# Non-numeric input
./pinspect abc

# Zombie process (create one for testing)
./tests/create_zombie.sh && ./pinspect <zombie_pid>

# Kernel thread
./pinspect 2
```

**Test File Structure:**
```
tests/
├── test_proc_status.c    # Unit tests for status parsing
├── test_net.c            # Unit tests for network parsing
├── test_util.c           # Unit tests for utilities
├── create_zombie.sh      # Helper to create zombie for testing
└── run_tests.sh          # Test runner script
```

**Testing Guidelines:**
- Test success cases AND failure cases
- Test edge cases: zombies, kernel threads, permission denied
- Test with sanitizers enabled
- Run valgrind before each milestone completion

---

## Documentation Requirements

### Function Documentation

Every public function (declared in header) gets a doc comment:

```c
/*
 * Brief one-line description.
 *
 * Longer explanation if needed, covering:
 * - What the function does
 * - Parameter meanings
 * - Return value semantics
 * - Error conditions
 * - Any side effects
 *
 * Example usage if non-obvious.
 */
```

### Design Decision Records

When you make a non-obvious choice, document it in `docs/decisions.md`:

```markdown
## 2026-01-10: Stack vs Heap for Process Info

**Decision:** Use stack-allocated struct for basic process info.

**Context:** Need to return process info from read_proc_status().

**Options Considered:**
1. Return struct by value (stack)
2. Allocate and return pointer (heap)
3. Take output pointer parameter (caller controls memory)

**Choice:** Option 3 - output pointer parameter.

**Rationale:**
- Caller controls memory lifetime
- No hidden allocations
- Consistent with file descriptor enumeration pattern
- Zero risk of memory leak from this function
```

### README Structure

The README should include:

1. **Overview** - What and why (already drafted)
2. **Building** - How to compile
3. **Usage** - Command-line interface with examples
4. **Example Output** - Show what users will see
5. **Architecture** - High-level design
6. **Implementation Notes** - Interesting challenges solved
7. **Limitations** - What it doesn't do (and why)
8. **References** - TLPI chapters, man pages consulted

### /proc Learnings

Continue documenting discoveries in `docs/proc-formats.md`. This serves two purposes:
1. Reference for yourself during development
2. Shows interviewers you did deep research

---

## Module Evolution Strategy

### Week 1-2: Foundation

**Files to Create:**
- `src/main.c` - Argument parsing, output formatting
- `src/proc_status.c` - Parse `/proc/<PID>/status`
- `src/util.c` - Error helpers, path building
- `include/pinspect.h` - Core types
- `include/proc_status.h` - Status parsing API
- `include/util.h` - Utility API

**Key Functions:**
```c
/* util.c */
int build_proc_path(pid_t pid, const char *file, char *buf, size_t buflen);
bool pid_exists(pid_t pid);
const char *state_to_string(proc_state_t state);

/* proc_status.c */
int read_proc_status(pid_t pid, proc_info_t *info);
```

**Testing Focus:**
- Various process types (normal, zombie, kernel thread)
- Permission scenarios
- Invalid PIDs

### Week 3: Threads and File Descriptors

**Files to Add:**
- `src/proc_task.c` - Thread counting
- `src/proc_fd.c` - FD enumeration
- `include/proc_task.h`
- `include/proc_fd.h`

**Key Functions:**
```c
/* proc_task.c */
int count_threads(pid_t pid);

/* proc_fd.c */
int enumerate_fds(pid_t pid, fd_entry_t **entries, int *count);
void fd_entries_free(fd_entry_t *entries);
bool parse_socket_inode(const char *target, unsigned long *inode);
```

**Testing Focus:**
- Multi-threaded processes
- Processes with many open files
- Socket detection in FD list

### Week 4: Network Connections

**Files to Add:**
- `src/net.c` - Parse `/proc/net/tcp` and `/proc/net/udp`
- `include/net.h`

**Key Functions:**
```c
/* net.c */
int find_process_sockets(pid_t pid, socket_info_t **sockets, int *count);
void socket_list_free(socket_info_t *sockets);
const char *tcp_state_to_string(tcp_state_t state);
void format_ip_port(uint32_t addr, uint16_t port, char *buf, size_t buflen);
```

**Algorithm:**
1. Enumerate FDs for target PID
2. Extract socket inodes from FD targets
3. Parse `/proc/net/tcp` and `/proc/net/udp`
4. Match inodes to find connections belonging to this process

**Testing Focus:**
- Process with TCP connections (ssh, web browser)
- Process with UDP sockets
- Process with no network activity

### Keeping Code Cohesive

As features are added:
- Maintain consistent naming patterns
- Use the same error handling style everywhere
- Keep the same documentation format
- Refactor common patterns into `util.c`
- Each module should be independently understandable

---

## Future-Proofing Considerations

### Patterns That Might Inform Project 2 (Go Networking)

1. **Protocol Parsing:** The `/proc/net/tcp` parsing teaches binary/hex format handling. Similar skills apply to custom protocol design.

2. **Error Handling:** The graceful degradation pattern (report what we can, note what we can't) applies to distributed systems.

3. **State Machines:** TCP states form a state machine. Understanding this helps with connection handling in network services.

4. **Documentation Style:** The `/proc` format documentation style transfers to protocol specification documents.

### Potentially Reusable Utilities

While each project should stand alone, note patterns that work well:
- Error message formatting
- Hexadecimal parsing utilities
- State-to-string conversion macros
- Testing patterns for system interfaces

**Don't over-engineer:** Don't create a "shared library" between projects. But if a pattern works, remember it.

---

## Portfolio Context

### Interview-Ready Characteristics

This code should demonstrate:

1. **Linux Internals Knowledge**
   - Understanding of `/proc` filesystem
   - Process state machine awareness
   - File descriptor model comprehension
   - Network stack visibility (TCP states, socket inodes)

2. **C Proficiency**
   - Memory management without leaks
   - Proper error handling
   - Clean, readable code structure
   - Appropriate use of system calls

3. **Professional Engineering**
   - Consistent code style
   - Meaningful documentation
   - Proper build system
   - Testing approach

### Code Review Lens

Before finalizing any file, ask:
- Would I be proud to show this in an interview?
- Can someone understand this without me explaining?
- Are edge cases handled?
- Is there any clever code that should be simplified?

**Clarity > Cleverness:** A straightforward implementation that's easy to understand beats a "clever" one that requires explanation.

### GitHub Presentation

- Meaningful commit messages (not "fix stuff")
- Atomic commits (one logical change per commit)
- No committed debug code or dead code
- Clean history (squash messy development commits before merge)

---

## Quick Reference

### Compilation

```bash
make              # Debug build with sanitizers
make release      # Optimized build
make valgrind     # Memory leak check
make test         # Run test suite
```

### Code Style

- 4 spaces, 80 char lines
- `snake_case` functions, `snake_case_t` types
- K&R braces, always use braces
- Comments explain WHY, not WHAT

### Error Handling

- Return 0/-1 or valid/NULL
- Preserve errno for system errors
- User-facing messages in main.c only
- Fail fast with descriptive messages

### Memory

- Stack by default
- Document allocation ownership
- Provide cleanup functions
- Valgrind clean always

---

## Getting Started Checklist

Before writing code for any milestone:

- [ ] Read relevant man pages (`man proc`, `man 2 open`, etc.)
- [ ] Explore the `/proc` files manually for sample processes
- [ ] Document format observations in `docs/proc-formats.md`
- [ ] Design function signatures before implementing
- [ ] Write test cases for expected behavior and edge cases

Before marking a milestone complete:

- [ ] All tests pass
- [ ] Valgrind reports no leaks
- [ ] Code formatted with `make format`
- [ ] Documentation updated
- [ ] README reflects current capabilities
