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
