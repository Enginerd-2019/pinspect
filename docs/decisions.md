# Design Decision Records

This document captures architectural and implementation decisions made during development.

---

## 2026-01-05: Output Parameter Pattern for Process Info

**Decision:** Use output pointer parameter instead of returning allocated struct.

**Context:** Need to return process info from `read_proc_status()`.

**Options Considered:**
1. Return struct by value (stack)
2. Allocate and return pointer (heap)
3. Take output pointer parameter (caller controls memory)

**Choice:** Option 3 - output pointer parameter.

**Rationale:**
- Caller controls memory lifetime
- No hidden allocations
- Consistent with file descriptor and thread enumeration patterns
- Zero risk of memory leak from this function
- Matches common POSIX API conventions

---

## 2026-01-08: Dynamic Array Growth Strategy

**Decision:** Use doubling strategy (2x) for FD and thread arrays.

**Context:** We don't know how many FDs or threads a process has until enumeration.

**Options Considered:**
1. Fixed-size array (e.g., 1024 entries)
2. Linked list
3. Dynamic array with 1.5x growth
4. Dynamic array with 2x growth

**Choice:** Option 4 - dynamic array with 2x growth, initial capacity 64.

**Rationale:**
- Amortized O(1) insertion time
- Simple implementation
- 64 initial slots handles typical processes without reallocation
- Doubling is simpler than 1.5x (no floating point)
- Final realloc shrinks to exact size, reclaiming unused memory

---

## 2026-01-09: Graceful Degradation on Permission Errors

**Decision:** Continue enumeration when individual FDs fail to resolve.

**Context:** FDs can close between `readdir()` and `readlink()` (TOCTOU race).

**Options Considered:**
1. Fail entire enumeration on any error
2. Skip failed entries and continue
3. Return partial results with error flag

**Choice:** Option 2 - skip and continue.

**Rationale:**
- Process state is inherently racy; FDs can close at any time
- Returning what we can is more useful than failing completely
- Matches behavior of tools like `lsof`
- ENOENT on a single FD shouldn't abort the whole operation

---

## 2026-01-10: NULL Safety in Public APIs

**Decision:** All public functions check for NULL input parameters.

**Context:** `parse_socket_inode()` was crashing when passed NULL.

**Options Considered:**
1. Document "undefined behavior on NULL" and let caller handle it
2. Assert on NULL (crash with message in debug builds)
3. Check for NULL and return error/false gracefully

**Choice:** Option 3 - check and return gracefully.

**Rationale:**
- Defensive programming prevents crashes from caller mistakes
- Minimal performance overhead (single pointer comparison)
- Consistent with robust C library conventions
- Makes testing easier (can test NULL case explicitly)
- Discovered via unit testing with AddressSanitizer

**Implementation:**
```c
bool parse_socket_inode(const char *target, unsigned long *inode)
{
    if (target == NULL) {
        return false;
    }
    return sscanf(target, "socket:[%lu]", inode) == 1;
}
```

---

## 2026-01-10: Thread Enumeration Returns Full Details

**Decision:** `enumerate_threads()` returns TID, name, and state for each thread.

**Context:** Thread count is already available from `/proc/<pid>/status`.

**Options Considered:**
1. Just return thread count (redundant with status)
2. Return array of TIDs only
3. Return full thread details (TID, name, state)

**Choice:** Option 3 - full details.

**Rationale:**
- Thread count alone is redundant (already in proc_status)
- Per-thread details are genuinely useful for debugging
- Thread names help identify what each thread does
- Thread states help identify stuck/blocked threads
- Minimal additional cost (reading comm and status files)

---

## 2026-01-11: Separate Path Builders for Process and Task Files

**Decision:** Create `build_task_path()` in addition to `build_proc_path()`.

**Context:** Thread files live at `/proc/<pid>/task/<tid>/<file>`, different from process files.

**Options Considered:**
1. Extend `build_proc_path()` with optional TID parameter
2. Have callers construct task paths manually with snprintf
3. Create dedicated `build_task_path()` function

**Choice:** Option 3 - dedicated function.

**Rationale:**
- Clear separation of concerns
- Self-documenting API (function name indicates purpose)
- Consistent error handling and buffer safety
- Avoids complicating `build_proc_path()` signature
- Example in header documents exact output format

---

## 2026-01-12: Socket Detection via Pattern Matching

**Decision:** Use `sscanf()` with pattern `"socket:[%lu]"` to detect and extract socket inodes.

**Context:** File descriptor symlinks pointing to sockets have format `socket:[12345]` where the number is the inode.

**Options Considered:**
1. Manual string parsing (check prefix, find brackets, parse number)
2. Regular expressions (overkill, requires regex library)
3. `sscanf()` pattern matching

**Choice:** Option 3 - `sscanf()` pattern matching.

**Rationale:**
- Simple one-liner: `sscanf(target, "socket:[%lu]", inode) == 1`
- Automatically validates format and extracts inode in one operation
- Type-safe (parses as unsigned long)
- No manual string manipulation needed
- Returns 1 on match, 0 on non-match (easy boolean conversion)

**Implementation:**
```c
bool parse_socket_inode(const char *target, unsigned long *inode)
{
    if (target == NULL) {
        return false;
    }
    return sscanf(target, "socket:[%lu]", inode) == 1;
}
```

---

## 2026-01-13: Network Connection Correlation via Inode Matching

**Decision:** Correlate process sockets with network connections by matching socket inodes.

**Context:** Need to identify which TCP/UDP connections belong to a specific process.

**Options Considered:**
1. Parse `/proc/<pid>/net/tcp` (per-process view, but doesn't exist for all kernels)
2. Match by port numbers (unreliable - ports can be reused, multiple processes)
3. Match by socket inode numbers (unique kernel identifier)

**Choice:** Option 3 - inode matching.

**Rationale:**
- Socket inodes are unique kernel identifiers
- Process FDs show `socket:[INODE]` format
- `/proc/net/tcp` includes inode column for each connection
- One-to-one correspondence: each socket has exactly one inode
- Works reliably across all connection states (ESTABLISHED, LISTEN, etc.)
- Demonstrates how different `/proc` files can be correlated

**Algorithm:**
1. Enumerate process FDs with `enumerate_fds()`
2. Extract socket inodes from entries where `is_socket == true`
3. Parse `/proc/net/tcp` and `/proc/net/udp`
4. Keep only rows where inode matches our set
5. Return array of matching connections

---

## 2026-01-14: Network Byte Order for IP Addresses

**Decision:** Store IP addresses in network byte order (big-endian) in `socket_info_t`.

**Context:** `/proc/net/tcp` stores IPs in host byte order (little-endian on x86), but network APIs expect network byte order.

**Options Considered:**
1. Store in host byte order, convert when displaying
2. Store in network byte order (ready for `inet_ntoa()`)
3. Store as string (wasteful, loses type safety)

**Choice:** Option 2 - network byte order.

**Rationale:**
- Directly compatible with `struct in_addr` and `inet_ntoa()`
- No conversion needed at display time
- Matches standard socket API conventions
- Easier to extend for future network operations
- Convert once during parsing with `htonl()`, use many times

**Implementation:**
```c
// Parse from /proc/net/tcp (hex is in host order on x86)
unsigned int ip_hex;
sscanf(hex, "%X:%X", &ip_hex, &port_hex);
*ip = htonl(ip_hex);  // Convert to network byte order

// Display (no conversion needed)
struct in_addr addr;
addr.s_addr = socket_info.local_addr;  // Already in network order
char *ip_str = inet_ntoa(addr);
```

---

## 2026-01-15: Linear Search for Inode Matching

**Decision:** Use simple linear search to check if socket inode matches target set.

**Context:** Need to determine if a `/proc/net/tcp` entry belongs to our process.

**Options Considered:**
1. Linear search - O(n) lookup
2. Hash set - O(1) lookup
3. Sorted array with binary search - O(log n) lookup

**Choice:** Option 1 - linear search.

**Rationale:**
- Most processes have few sockets (< 100)
- Linear search is simple and sufficient for small n
- No need for hash table complexity or sorting overhead
- Can optimize later if profiling shows it's a bottleneck
- Keeps code simple and readable

**Implementation:**
```c
static bool inode_in_set(unsigned long inode,
                         unsigned long *set, int set_size)
{
    for (int i = 0; i < set_size; i++) {
        if (set[i] == inode) {
            return true;
        }
    }
    return false;
}
```

---

## 2026-01-16: Skip Unwanted Fields with sscanf %*

**Decision:** Use `%*` format specifiers to skip unneeded columns in `/proc/net/tcp`.

**Context:** `/proc/net/tcp` has 11+ columns but we only need 4 (local_addr, rem_addr, state, inode).

**Options Considered:**
1. Parse all fields into variables (wasteful)
2. Use multiple `sscanf()` calls to extract specific fields
3. Use `%*` to skip unwanted fields in single `sscanf()`

**Choice:** Option 3 - `%*` format specifiers.

**Rationale:**
- Single `sscanf()` call handles entire line
- `%*s` and `%*d` read and discard values
- No unnecessary variable allocations
- Self-documenting (shows which fields we ignore)
- Efficient - parser skips directly to next field

**Implementation:**
```c
sscanf(line,
    " %u: %63s %63s %X %*s %*s %*s %u %*d %lu",
    //                    ^^^  ^^^  ^^^    ^^^
    //                    Skip tx:rx, timers, retrans, timeout
    &slot, local_addr, remote_addr, &state, &uid, &inode);
```
