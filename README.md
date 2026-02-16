# pinspect - Linux Process Inspector

`pinspect` is a command-line tool for inspecting Linux process internals by directly parsing the `/proc` filesystem.

## Overview

`pinspect` provides detailed information about running Linux processes without relying on external tools or libraries. It demonstrates systems programming fundamentals by interfacing directly with the kernel through `/proc`.

### Why This Project?

Understanding how Linux exposes process information through `/proc` is foundational knowledge for systems programming. This tool:

- Parses `/proc/<PID>/status` for process state and memory usage
- Enumerates threads from `/proc/<PID>/task/` with TID, name, and state
- Reads `/proc/<PID>/fd/` to enumerate open file descriptors and detect socket inodes
- Correlates socket inodes with `/proc/net/tcp` and `/proc/net/udp` to identify network connections
- Resolves symlinks to show actual file paths
- Handles errors and permission issues gracefully
- Supports verbose mode (`-v`) for detailed output
- Supports network-only mode (`-n`) to show only network connections

Unlike `ps`, `top`, or `lsof`, this tool is built from scratch using only standard C library calls and POSIX APIs, making the underlying system calls and data formats explicit.

## Features

- **Process Info:** Name, state, UID/GID, memory usage (VmSize, VmRSS, VmPeak), thread count
- **Thread Details (verbose):** Enumerate all threads with TID, name, and state
- **File Descriptors (verbose):** List all open file descriptors with their targets
- **Socket Detection:** Automatically identify socket FDs and extract inode numbers
- **Network Connections:** Correlate process sockets with TCP/UDP connection details including:
  - Local and remote addresses (IP:port)
  - Connection state (ESTABLISHED, LISTEN, etc.)
  - Protocol type (TCP/UDP)

## Building

```bash
make
```

## Usage

```bash
# Basic usage - show process summary
./pinspect <PID>

# Verbose mode - show detailed thread list and file descriptors
./pinspect -v <PID>

# Network-only mode - show only network connections
./pinspect -n <PID>

# Combined flags
./pinspect -vn <PID>

# Inspect your own shell
./pinspect $$

# Show help
./pinspect --help

# Show version
./pinspect --version
```

## Example Output

### Basic Mode

```
$ ./pinspect 1234
Process:   firefox (PID 1234)
State:     Sleeping
UID:       1000 (real), 1000 (effective)
Memory:    VmSize: 2048000 KB, VmRSS: 512000 KB, VmPeak: 2200000 KB
Threads:   47

File Descriptors: 156 open

Network Connections: 3 open
```

### Verbose Mode (-v)

Shows detailed file descriptor list and thread enumeration:

```
$ ./pinspect -v 1234
Process:   firefox (PID 1234)
State:     Sleeping
UID:       1000 (real), 1000 (effective)
Memory:    VmSize: 2048000 KB, VmRSS: 512000 KB, VmPeak: 2200000 KB
Threads:   47

File Descriptors: 156 open

  FD    Type      Target
  ----  --------  ----------------------------------------
  0     file      /dev/null
  1     file      /dev/null
  2     file      /dev/null
  3     socket    socket:[67890]
  4     socket    socket:[67891]
  5     file      /home/user/.mozilla/firefox/profile.db
  ...

Thread Details:
  TID     State       Name
  ------  ----------  ----------------
  1234    Sleeping    firefox
  1235    Sleeping    GMPThread
  1236    Sleeping    Gecko_IOThread
  1237    Running     Compositor
  1238    Sleeping    ImageIO
  ...

Network Connections: 3 open

  Proto  Local Address          Remote Address         State
  -----  ---------------------  ---------------------  -----------
  TCP    192.168.1.100:54321    142.250.80.46:443      ESTABLISHED
  TCP    192.168.1.100:54322    151.101.1.140:443      ESTABLISHED
  UDP    0.0.0.0:5353           0.0.0.0:0              CLOSE
```

### Network-Only Mode (-n)

Shows only network connections for the process:

```
$ ./pinspect -n 1234
Network Connections: 3 open

  Proto  Local Address          Remote Address         State
  -----  ---------------------  ---------------------  -----------
  TCP    192.168.1.100:54321    142.250.80.46:443      ESTABLISHED
  TCP    192.168.1.100:54322    151.101.1.140:443      ESTABLISHED
  UDP    0.0.0.0:5353           0.0.0.0:0              CLOSE
```

## Project Structure

```
process-inspector/
├── src/                # Source files
│   ├── main.c          # Entry point, argument parsing, output
│   ├── proc_status.c   # Parse /proc/<PID>/status
│   ├── proc_fd.c       # Enumerate /proc/<PID>/fd/
│   ├── proc_task.c     # Enumerate /proc/<PID>/task/ (thread details)
│   ├── net.c           # Parse /proc/net/tcp and /proc/net/udp
│   └── util.c          # Shared utilities
├── include/            # Header files
│   ├── pinspect.h      # Common types and constants
│   ├── proc_status.h   # Status parsing API
│   ├── proc_fd.h       # File descriptor API
│   ├── proc_task.h     # Thread enumeration API
│   ├── net.h           # Network parsing API
│   └── util.h          # Utility functions
├── docs/               # Documentation
│   ├── proc-formats.md # /proc file format observations
│   └── decisions.md    # Design decision records
├── tests/              # Test files
├── Makefile
├── README.md
├── TODO.md             # Task tracking
├── CLAUDE.md           # Development guidelines
└── LICENSE
```

## Design Decisions

Key architectural choices made during development:

- **Output parameter pattern**: Functions like `read_proc_status()` take output pointers rather than returning allocated memory, giving callers control over memory lifetime.
- **Dynamic array growth**: FD, thread, and socket arrays start at a fixed capacity (16-64 slots) and double when full, then shrink to exact size on return. Balances memory efficiency with allocation overhead.
- **Graceful degradation**: Individual FD resolution failures (e.g., FD closed mid-enumeration) are skipped rather than aborting the entire operation.
- **NULL safety**: All public APIs check for NULL inputs to prevent crashes from caller mistakes.
- **Inode correlation**: Network connections are identified by matching socket inodes from `/proc/<pid>/fd/` with entries in `/proc/net/tcp` and `/proc/net/udp`, demonstrating how different `/proc` files can be correlated.
- **Byte order handling**: IP addresses from `/proc/net/tcp` are converted from host byte order to network byte order using `htonl()` for compatibility with standard network APIs.

See [docs/decisions.md](docs/decisions.md) for detailed decision records.

## Implementation Notes

Challenges encountered and solutions:

- **readlink() doesn't null-terminate**: The `readlink()` syscall writes bytes without a trailing `\0`. Must explicitly add `target[len] = '\0'` after the call.
- **TOCTOU races**: File descriptors can close between `readdir()` and `readlink()`. Solution: treat ENOENT as "skip this entry" rather than fatal error.
- **Socket inode parsing**: Socket FDs appear as `socket:[12345]` symlinks. Used `sscanf()` pattern matching to extract inode numbers for network correlation.
- **Thread vs process paths**: Thread files live at `/proc/<pid>/task/<tid>/<file>`, requiring a separate `build_task_path()` helper.
- **Hexadecimal IP parsing**: `/proc/net/tcp` stores IP addresses in little-endian hexadecimal format. Must parse with `sscanf("%X")` and convert byte order with `htonl()`.
- **Network file format**: The `/proc/net/tcp` format includes many fields we don't need. Used `%*s` in `sscanf()` to skip unwanted columns efficiently.

## Requirements

- Linux (tested on Ubuntu 22.04+)
- GCC or Clang
- Make

## License

MIT License - See [LICENSE](LICENSE)