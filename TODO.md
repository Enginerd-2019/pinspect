# Process Inspector - Task Tracking

## Current Sprint: Foundation Setup
**Target:** End of Week 1 (Jan 12, 2026)

### Done
- [✅] Create project directory structure
- [✅] Set up local task tracking (this file)
- [✅] Write initial README
- [✅] Create Makefile skeleton
- [✅] Read TLPI Chapters 3-4 (file I/O basics)
- [✅] Explore `/proc/<PID>/status` manually for 3-5 processes
- [✅] Document `/proc` file format observations in docs/
- [✅] Initialize GitHub repository with project board

---

## Milestone 1: Basic Process Info (Weeks 1-2)
**Goal:** Parse `/proc/<PID>/status` for core process info

### Tasks
- [ ] Implement PID validation and `/proc/<PID>/` existence check
- [ ] Parse process name from `/proc/<PID>/status`
- [ ] Parse process state (running, sleeping, zombie, etc.)
- [ ] Parse UID/GID information
- [ ] Parse memory usage (VmSize, VmRSS, VmPeak)
- [ ] Format and display output cleanly
- [ ] Handle errors gracefully (invalid PID, permission denied)
- [ ] Write basic tests for parsing functions

### Acceptance Criteria
- `./pinspect <PID>` displays name, state, UID, and memory info
- Works for own processes and readable system processes
- Clear error messages for invalid input

---

## Milestone 2: Thread and File Descriptor Info (Week 3)
**Goal:** Extend tool to show thread count and open files

### Tasks
- [ ] Count threads from `/proc/<PID>/task/`
- [ ] List open file descriptors from `/proc/<PID>/fd/`
- [ ] Resolve symlinks to show actual file paths
- [ ] Handle permission errors gracefully
- [ ] Add `-v` verbose flag for detailed FD info
- [ ] Update README with new features

### Acceptance Criteria
- Shows thread count for any process
- Lists open files (when readable)
- Graceful degradation when lacking permissions

---

## Milestone 3: Network Connections (Week 4)
**Goal:** Show network sockets owned by the process

### Tasks
- [ ] Parse `/proc/net/tcp` for TCP connections
- [ ] Parse `/proc/net/udp` for UDP connections
- [ ] Match socket inodes to process via `/proc/<PID>/fd/`
- [ ] Display local/remote addresses and ports
- [ ] Show connection state (ESTABLISHED, LISTEN, etc.)
- [ ] Add `-n` flag for network-only output

### Acceptance Criteria
- Correctly identifies network connections for a process
- Displays addresses in human-readable format
- Handles processes with no network connections

---

## Milestone 4: Polish and Documentation (Week 5)
**Goal:** Professional-quality deliverable

### Tasks
- [ ] Code cleanup and consistent style
- [ ] Add inline comments for complex logic
- [ ] Complete README with:
  - [ ] Project overview and motivation
  - [ ] Build instructions
  - [ ] Usage examples with sample output
  - [ ] Architecture/design decisions
  - [ ] Implementation challenges faced
- [ ] Create architecture diagram (docs/architecture.md)
- [ ] Add LICENSE file (MIT)
- [ ] Final testing on various processes
- [ ] Record demo or screenshots

### Acceptance Criteria
- Code is clean and well-commented
- README is comprehensive and professional
- Can inspect any readable process successfully

---

## Backlog (Future Enhancements)
*Not in scope for initial release, but noted for potential future work*

- [ ] Watch mode (`-w`) for real-time updates
- [ ] JSON output format (`--json`)
- [ ] Compare two PIDs side-by-side
- [ ] Memory map visualization from `/proc/<PID>/maps`
- [ ] CPU usage tracking over time
- [ ] Integration with `perf` for performance data

---

## Notes

### Key `/proc` Files to Understand
| File | Contains |
|------|----------|
| `/proc/<PID>/status` | Process state, memory, UID/GID |
| `/proc/<PID>/stat` | CPU times, priority, threads |
| `/proc/<PID>/fd/` | Open file descriptors |
| `/proc/<PID>/task/` | Thread information |
| `/proc/net/tcp` | TCP connection table |
| `/proc/net/udp` | UDP connection table |

### Resources
- *The Linux Programming Interface* - Chapters 3-6, 13, 20-22
- `man proc` - Comprehensive /proc documentation
- `man 2 open`, `man 2 read`, `man 2 readdir`

---

*Last updated: January 5, 2026*
