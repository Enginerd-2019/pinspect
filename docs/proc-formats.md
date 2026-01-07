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

### Columns

| Column | Meaning |
|--------|---------|
| `sl` | Slot number (line index) |
| `local_address` | Local IP:Port in hex |
| `rem_address` | Remote IP:Port in hex |
| `st` | State code in hex |
| `tx_queue:rx_queue` | Transmit/receive queue sizes |
| `tr:tm->when` | Timer active and jiffies until timeout |
| `retrnsmt` | Retransmit timeout |
| `uid` | User ID owning this socket |
| `timeout` | Timeout value |
| `inode` | Socket inode — matches `socket:[<inode>]` in `/proc/<PID>/fd/` |

### Address format

Format: `IIIIIIII:PPPP` where:
- **IP:** 8 hex digits, **little-endian** byte order
- **Port:** 4 hex digits, big-endian (just convert hex to decimal)

#### Why little-endian for IP addresses?

The kernel stores IP addresses as 32-bit integers in the CPU's native byte order (little-endian on x86/x64). When printed as hex, the bytes appear reversed compared to the human-readable dotted-decimal format.

**Decoding steps:**
1. Take the hex string: `0100007F`
2. Split into bytes: `01 00 00 7F`
3. Reverse the byte order: `7F 00 00 01`
4. Convert each byte to decimal: `127.0.0.1`

**Decoding example: `0100007F:0277`**
- IP bytes: `01 00 00 7F` → reversed: `7F 00 00 01` → `127.0.0.1`
- Port: `0277` hex → `631` decimal
- Result: `127.0.0.1:631`

**In C, you would use:**
```c
#include <arpa/inet.h>

uint32_t ip_hex = 0x0100007F;
struct in_addr addr = { .s_addr = ip_hex };
printf("%s\n", inet_ntoa(addr));  // prints "127.0.0.1"
```
The `inet_ntoa()` function handles the byte order conversion automatically.

**Common patterns:**
- `00000000:PPPP` → `0.0.0.0:port` (listening on all interfaces)
- `0100007F:PPPP` → `127.0.0.1:port` (localhost)

### State codes

| Hex | Decimal | State |
|-----|---------|-------|
| `01` | 1 | ESTABLISHED |
| `02` | 2 | SYN_SENT |
| `03` | 3 | SYN_RECV |
| `04` | 4 | FIN_WAIT1 |
| `05` | 5 | FIN_WAIT2 |
| `06` | 6 | TIME_WAIT |
| `07` | 7 | CLOSE |
| `08` | 8 | CLOSE_WAIT |
| `09` | 9 | LAST_ACK |
| `0A` | 10 | LISTEN |
| `0B` | 11 | CLOSING |

### Matching socket to process

The `inode` column links to process file descriptors:
```bash
# Find which process owns socket inode 3469235
sudo ls -la /proc/*/fd/* 2>/dev/null | grep 3469235
```

---

## /proc/net/udp

- **Columns:** Same as `/proc/net/tcp`
- **Differences from tcp:**
  - No connection states (UDP is connectionless) — `st` is typically `07` (CLOSE)
  - No `rem_address` for listening sockets (always `00000000:0000`)

---

## Edge Cases & Quirks

### sudo with shell built-ins
- `sudo cd /proc/<PID>/fd` fails because `cd` is a shell built-in, not an executable
- Use `sudo ls -la /proc/<PID>/fd` or `sudo -s` to get a root shell first

### Zombie processes
- `State: Z (zombie)`
- **No `Vm*` fields** — memory already released back to system
- `FDSize: 0` — all file descriptors closed
- `fd/` directory is empty
- Shows as `<defunct>` in `ps` output

**How zombies are created:**
1. Child process terminates (calls `exit()` or receives fatal signal)
2. Kernel releases all resources (memory, file descriptors, etc.)
3. Kernel keeps minimal entry in process table holding exit status
4. Child becomes a zombie, waiting for parent to retrieve exit status

**What removes zombies:**
- Parent calls `wait()`, `waitpid()`, or `waitid()` to retrieve exit status — this "reaps" the zombie
- If parent terminates without reaping, zombie is re-parented to PID 1 (`init`/`systemd`)
- PID 1 automatically reaps orphaned zombies
- A zombie that persists indicates a bug in the parent (not calling `wait()`)

### Kernel threads
- `Kthread: 1` in status file — explicitly identifies kernel threads
- **No `Vm*` fields** — kernel threads don't have user-space memory
- `readlink /proc/<PID>/exe` returns **empty string** (no executable file)
- `fd/` directory is empty (no open file descriptors)
- `VSZ` and `RSS` show `0` in `ps` output
- Command names appear in brackets: `[kthreadd]`, `[kworker/0:0]`, `[ksoftirqd/0]`

**What are kernel threads?**

Kernel threads are processes that run entirely in kernel space. Unlike user-space processes, they:
- Have no user-space memory mapping (no `mm_struct`)
- Cannot be started by users — only the kernel creates them
- Run with full kernel privileges
- Handle background kernel tasks that need process context (can sleep, be scheduled)

**Why do they exist?**

Some kernel work can't be done in interrupt context (interrupts can't sleep or block). Kernel threads provide a schedulable context for:
- Deferred work that may block (disk I/O, memory management)
- Periodic maintenance tasks
- Asynchronous operations

**Common kernel threads:**

| Name | Purpose |
|------|---------|
| `kthreadd` (PID 2) | Parent of all kernel threads; spawns new kernel threads on request |
| `kworker/*` | Generic worker threads for the workqueue subsystem; handle deferred work |
| `ksoftirqd/*` | Process software interrupts (softirqs) when load is high |
| `migration/*` | Move processes between CPUs for load balancing |
| `rcu_preempt` | Read-Copy-Update synchronization for lock-free data structures |
| `kswapd*` | Background memory page reclamation (swapping) |
| `jbd2/*` | Journaling block device — handles filesystem journal commits |
| `kblockd` | Block device I/O handling |
| `irq/*` | Threaded interrupt handlers |

**Resources for further reading:**
- *The Linux Programming Interface* — Chapter 12 (System and Process Information)
- *Linux Kernel Development* by Robert Love — Chapter 3 (Process Management)
- `man 5 proc` — `/proc` filesystem documentation
- Kernel source: `kernel/kthread.c` — kernel thread implementation
- LWN.net articles:
  - "The workqueue API" — https://lwn.net/Articles/355700/
  - "Kernel threads made easy" — https://lwn.net/Articles/65178/
- Kernel docs: https://www.kernel.org/doc/html/latest/core-api/workqueue.html

### Short-lived processes
- `/proc/<PID>/` directory can disappear between listing and reading
- Handle `ENOENT` (No such file or directory) gracefully in code
- Common with shell commands, child processes that exit quickly
- Test: `bash -c 'echo $$' &` then immediately try to read `/proc/<pid>/status`

---

## Permission Considerations

| Scenario | Access Level |
|----------|--------------|
| Own processes | Full access to all files |
| Other users' processes | Limited; `/proc/<PID>/fd/` returns Permission denied |
| Root/system processes (e.g., PID 1) | Requires sudo/root privileges to read fd/, environ, etc. |

---

*Last updated: 1/6/2026*
