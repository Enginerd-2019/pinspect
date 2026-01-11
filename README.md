# pinspect - Linux Process Inspector

The pininspect program is a command-line tool for inspecting Linux process internals by directly parsing the `/proc` filesystem.

## Overview

`pinspect` provides detailed information about running Linux processes without relying on external tools or libraries. It demonstrates systems programming fundamentals by interfacing directly with the kernel through `/proc`.

### Why This Project?

Understanding how Linux exposes process information through `/proc` is foundational knowledge for systems programming. This tool:

- Parses `/proc/<PID>/status` for process state and memory usage
- Enumerates threads from `/proc/<PID>/task/` with TID, name, and state
- Reads `/proc/<PID>/fd/` to enumerate open file descriptors
- Resolves symlinks to show actual file paths
- Handles errors and permission issues gracefully
- Supports verbose mode (`-v`) for detailed thread and FD info

Unlike `ps`, `top`, or `lsof`, this tool is built from scratch using only standard C library calls, making the underlying system calls and data formats explicit.

## Features

- **Process Info:** Name, state, UID/GID, memory usage (VmSize, VmRSS)
- **Thread Details:** Enumerate threads with TID, name, and state via `/proc/<PID>/task/`
- **Open Files:** List file descriptors and their targets
- **Network Connections:** TCP/UDP sockets with addresses and states

## Building

```bash
make
```

## Usage

```bash
# Inspect a process by PID
./pinspect 1234

# Verbose output (more file descriptor details)
./pinspect -v 1234

# Network connections only
./pinspect -n 1234
```

## Example Output

```
Process: firefox (PID 1234)
State:   Running
UID:     1000 (tcrumb)
Memory:  VmSize: 2.1 GB, VmRSS: 512 MB

Threads: 47

Open Files: 23
  0 -> /dev/pts/1
  1 -> /dev/pts/1
  2 -> /dev/pts/1
  3 -> socket:[12345]
  ...

Network Connections:
  TCP  127.0.0.1:8080 -> 127.0.0.1:45678  ESTABLISHED
  TCP  0.0.0.0:443    -> 0.0.0.0:0        LISTEN
```

**Verbose mode (-v)** shows per-thread details:
```
Thread Details:
  TID     State       Name
  ------  ----------  ----------------
  1234    Sleeping    firefox
  1235    Sleeping    GMPThread
  1236    Sleeping    Gecko_IOThread
  1237    Running     Compositor
  ...
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
- **Dynamic array growth**: FD and thread arrays start at 64 slots and double when full, then shrink to exact size on return. Balances memory efficiency with allocation overhead.
- **Graceful degradation**: Individual FD resolution failures (e.g., FD closed mid-enumeration) are skipped rather than aborting the entire operation.
- **NULL safety**: All public APIs check for NULL inputs to prevent crashes from caller mistakes.

See [docs/decisions.md](docs/decisions.md) for detailed decision records.

## Implementation Notes

Challenges encountered and solutions:

- **readlink() doesn't null-terminate**: The `readlink()` syscall writes bytes without a trailing `\0`. Must explicitly add `target[len] = '\0'` after the call.
- **TOCTOU races**: File descriptors can close between `readdir()` and `readlink()`. Solution: treat ENOENT as "skip this entry" rather than fatal error.
- **Socket inode parsing**: Socket FDs appear as `socket:[12345]` symlinks. Used `sscanf()` pattern matching to extract inode numbers for Week 4 network correlation.
- **Thread vs process paths**: Thread files live at `/proc/<pid>/task/<tid>/<file>`, requiring a separate `build_task_path()` helper.

## Requirements

- Linux (tested on Ubuntu 22.04+)
- GCC or Clang
- Make

## License

MIT License - See [LICENSE](LICENSE)

---

*Part of a systems programming portfolio. See [2-Year Roadmap](../2-YearRoadmap.md) for context.*
