# pinspect - Linux Process Inspector

A command-line tool for inspecting Linux process internals by directly parsing the `/proc` filesystem.

## Overview

`pinspect` provides detailed information about running Linux processes without relying on external tools or libraries. It demonstrates systems programming fundamentals by interfacing directly with the kernel through `/proc`.

### Why This Project?

Understanding how Linux exposes process information through `/proc` is foundational knowledge for systems programming. This tool:

- Parses `/proc/<PID>/status` for process state and memory usage
- Reads `/proc/<PID>/fd/` to enumerate open file descriptors
- Analyzes `/proc/net/tcp` and `/proc/net/udp` to identify network connections
- Handles errors and edge cases gracefully

Unlike `ps`, `top`, or `lsof`, this tool is built from scratch using only standard C library calls, making the underlying system calls and data formats explicit.

## Features

- **Process Info:** Name, state, UID/GID, memory usage (VmSize, VmRSS)
- **Thread Count:** Enumerate threads via `/proc/<PID>/task/`
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

## Project Structure

```
process-inspector/
├── src/           # Source files
│   ├── main.c     # Entry point and argument parsing
│   ├── proc.c     # /proc parsing functions
│   └── net.c      # Network connection parsing
├── include/       # Header files
│   ├── proc.h
│   └── net.h
├── docs/          # Documentation
│   └── architecture.md
├── tests/         # Test files
├── Makefile
├── README.md
├── TODO.md        # Task tracking
└── LICENSE
```

## Design Decisions

*To be documented as implementation progresses.*

## Implementation Notes

*To be documented as challenges are encountered and solved.*

## Requirements

- Linux (tested on Ubuntu 22.04+)
- GCC or Clang
- Make

## License

MIT License - See [LICENSE](LICENSE)

---

*Part of a systems programming portfolio. See [2-Year Roadmap](../2-YearRoadmap.md) for context.*
