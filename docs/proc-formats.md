# /proc File Format Observations

> Reference: `man proc`, *The Linux Programming Interface* Ch. 12

---

## /proc/\<PID\>/status

### Fields

#### Name
- **Format:** `Name: <string>` 
- **Notes:** The name identifies the command run by this process. Truncated to 15 characters. 

#### State
- **Format:** `State: <char> <description>`
- **Observed values:** S, I, R

| Char | Meaning | Example Process |
|------|---------|-----------------|
| R | Running | CPU-bound loop |
| S | Sleeping (interruptible) | bash waiting for input |
| D | Disk sleep (uninterruptible) | process waiting on I/O |
| Z | Zombie | terminated but not reaped |
| T | Stopped (signal) | process hit breakpoint |

#### Uid / Gid
- **Format:** `UID: <real> <effective> <saved set> <filesystem>`
- **Notes:** Uid and GID have the same format. Uid is the user id, and Gid id the group id for with the process belongs.

#### VmSize
- **Format:** `VmSize: <integer> <unit>`
- **Units:** kilobytes
- **Notes:** Current virtual memory size

#### VmRSS
- **Format:** `VmRss: <integer> <unit>`
- **Units:** kilobytes
- **Notes:** Current resident size (memory actually used)

#### VmPeak
- **Format:** `VmPeak: <integer> <unit>`
- **Units:** kilobytes
- **Notes:** Peak vitual memory usage since process start

#### Threads
- **Format:** `Threads: <integer>`
- **Notes:** Number of threads in thread group. Same as the number on entries in /proc/<pid>/task

---

## /proc/\<PID\>/fd/

- **Structure:** Directory containing numbered symlinks (0, 1, 2, ...) for each open file descriptor
- **Symlink target format:** Points to the actual file/resource the FD references

### Common symlink targets

| Target | Meaning |
|--------|---------|
| `/dev/null` | Black hole device — writes discarded, reads return EOF |
| `/dev/pts/<N>` | Pseudo-terminal (interactive shell) |
| `/path/to/file` | Regular file open for read/write |
| `pipe:[<inode>]` | One end of a pipe (IPC between processes) |
| `socket:[<inode>]` | Network or Unix socket; match inode in `/proc/net/*` |
| `anon_inode:[pidfd]` | File descriptor referencing another process |
| `anon_inode:[eventfd]` | Event notification mechanism |
| `anon_inode:[timerfd]` | Timer file descriptor |
| `anon_inode:[signalfd]` | Signal handling via file descriptor |
| `anon_inode:[inotify]` | Filesystem event watcher |
| `anon_inode:[eventpoll]` | epoll instance for I/O multiplexing |

- **Socket notation:** `socket:[<inode>]` — the inode number can be matched against `/proc/net/tcp`, `/proc/net/udp`, or `/proc/net/unix` to find connection details

---

## /proc/\<PID\>/task/

- **Structure:** Directory containing subdirectories named by thread ID (TID), e.g., `/proc/1234/task/1234/`, `/proc/1234/task/1235/`
- **Notes:** Each TID subdirectory mirrors the structure of `/proc/<PID>/` (has `status`, `stat`, `fd/`, etc.)

### Why the structure mirrors /proc/\<PID\>/

Linux doesn't distinguish between "processes" and "threads" internally — both are tasks (`task_struct`). The difference is what they share:

| Resource | Processes | Threads |
|----------|-----------|---------|
| Memory space | Separate | Shared |
| File descriptors | Separate | Shared |
| PID | Unique | Unique TID, shares TGID (thread group ID) |
| `/proc` entry | `/proc/<PID>/` | `/proc/<PID>/task/<TID>/` |

### Thread-specific vs shared values

- **Thread-specific:** CPU time, state, stack pointer (differ per thread)
- **Shared:** Memory maps, open files (identical across threads in same process)

### Counting threads

List `/proc/<PID>/task/` — each subdirectory is a thread. Count matches `Threads:` field in `/proc/<PID>/status`.

---

## /proc/net/tcp

- **Columns:**
- **Address format:**
- **State codes:**

| Hex | State |
|-----|-------|
|     |       |

---

## /proc/net/udp

- **Columns:**
- **Differences from tcp:**

---

## Edge Cases & Quirks

### sudo with shell built-ins
- `sudo cd /proc/<PID>/fd` fails because `cd` is a shell built-in, not an executable
- Use `sudo ls -la /proc/<PID>/fd` or `sudo -s` to get a root shell first

### Zombie processes
-

### Kernel threads
-

### Short-lived processes
-

---

## Permission Considerations

| Scenario | Access Level |
|----------|--------------|
| Own processes | Full access to all files |
| Other users' processes | Limited; `/proc/<PID>/fd/` returns Permission denied |
| Root/system processes (e.g., PID 1) | Requires sudo/root privileges to read fd/, environ, etc. |

---

*Last updated:*
