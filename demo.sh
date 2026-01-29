#!/bin/bash
#
# demo.sh - Comprehensive demonstration of pinspect capabilities
#
# This script demonstrates all features of pinspect including process
# inspection, thread enumeration, file descriptor listing, and network
# connection tracking.

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Track cleanup tasks
CLEANUP_PIDS=()

cleanup() {
    echo ""
    echo "Cleaning up demo processes..."
    for pid in "${CLEANUP_PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
            wait "$pid" 2>/dev/null || true
        fi
    done
}

trap cleanup EXIT INT TERM

section() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
}

demo_command() {
    echo -e "${YELLOW}\$ $1${NC}"
    eval "$1"
    echo ""
}

pause_demo() {
    echo -e "${GREEN}[Press Enter to continue]${NC}"
    read -r
}

section "pinspect Demonstration"
echo "This demo showcases all features of pinspect, a Linux process"
echo "inspector that parses /proc directly."
echo ""
pause_demo

# Demo 1: Help and Version
section "Demo 1: Help and Version Information"
demo_command "./pinspect --help"
pause_demo

demo_command "./pinspect --version"
pause_demo

# Demo 2: Basic process inspection on current shell
section "Demo 2: Basic Process Inspection"
echo "Inspecting the current shell process (PID $$):"
demo_command "./pinspect $$"
pause_demo

# Demo 3: Verbose mode on current shell
section "Demo 3: Verbose Mode - Detailed Output"
echo "Verbose mode shows file descriptors and thread details:"
demo_command "./pinspect -v $$"
pause_demo

# Demo 4: Network demonstration with Firefox
section "Demo 4: Network Connection Tracking"

FIREFOX_STARTED=false
if command -v firefox >/dev/null 2>&1; then
    echo "Starting Firefox to demonstrate network and thread tracking..."
    echo "(This will launch a temporary Firefox instance)"
    echo ""

    # Start Firefox in private window with new instance
    firefox --new-instance --private-window "about:blank" >/dev/null 2>&1 &
    FIREFOX_PID=$!
    CLEANUP_PIDS+=("$FIREFOX_PID")

    echo "Waiting for Firefox to initialize (PID $FIREFOX_PID)..."
    sleep 3

    # Check if Firefox is still running
    if kill -0 "$FIREFOX_PID" 2>/dev/null; then
        echo "Firefox started successfully!"
        echo ""

        # Give it more time to establish connections
        echo "Waiting for Firefox to establish network connections..."
        sleep 4

        if kill -0 "$FIREFOX_PID" 2>/dev/null; then
            FIREFOX_STARTED=true

            # Basic inspection
            echo -e "${GREEN}4a. Basic Firefox inspection:${NC}"
            demo_command "./pinspect $FIREFOX_PID"
            pause_demo

            # Verbose with thread details (truncate for readability)
            echo -e "${GREEN}4b. Thread details (first 50 lines):${NC}"
            demo_command "./pinspect -v $FIREFOX_PID | head -50"
            echo "  (output truncated for readability)"
            pause_demo

            # Network connections
            echo -e "${GREEN}4c. Network connections only:${NC}"
            demo_command "./pinspect -n $FIREFOX_PID"
            pause_demo

            # Verbose network
            echo -e "${GREEN}4d. Verbose network mode:${NC}"
            demo_command "./pinspect -vn $FIREFOX_PID | head -60"
            echo "  (output truncated for readability)"
            pause_demo
        fi
    fi
fi

# Fallback: Python HTTP server if Firefox didn't work
if [ "$FIREFOX_STARTED" = false ]; then
    echo -e "${YELLOW}Firefox unavailable, using Python HTTP server instead...${NC}"
    echo ""

    # Start Python HTTP server
    python3 -m http.server 8888 >/dev/null 2>&1 &
    SERVER_PID=$!
    CLEANUP_PIDS+=("$SERVER_PID")

    echo "Started HTTP server (PID $SERVER_PID) on port 8888"
    sleep 2

    if kill -0 "$SERVER_PID" 2>/dev/null; then
        echo -e "${GREEN}4a. HTTP server inspection:${NC}"
        demo_command "./pinspect $SERVER_PID"
        pause_demo

        echo -e "${GREEN}4b. Network connections (LISTEN socket on port 8888):${NC}"
        demo_command "./pinspect -n $SERVER_PID"
        pause_demo
    fi
fi

# Demo 5: Error handling
section "Demo 5: Error Handling"

echo -e "${GREEN}5a. Invalid PID (non-existent process):${NC}"
demo_command "./pinspect 99999999 || echo '  Exit code: \$?'"
pause_demo

echo -e "${GREEN}5b. Invalid argument (non-numeric):${NC}"
demo_command "./pinspect abc 2>&1 || echo '  Exit code: \$?'"
pause_demo

echo -e "${GREEN}5c. Permission denied (inspecting init without root):${NC}"
demo_command "./pinspect 1 2>&1 || echo '  Exit code: \$?'"
pause_demo

# Demo 6: System process inspection
section "Demo 6: System Process Inspection"

echo -e "${GREEN}6a. Inspecting systemd/init (PID 1) with sudo:${NC}"
if sudo -n true 2>/dev/null; then
    demo_command "sudo ./pinspect 1"
else
    echo -e "${YELLOW}Sudo access not available, skipping...${NC}"
    echo "Command would be: sudo ./pinspect 1"
fi
pause_demo

# Demo 7: Common system processes
section "Demo 7: Inspecting Common System Processes"

# Try to find interesting processes
for proc_name in systemd-resolved sshd cron; do
    PID=$(pgrep -o "$proc_name" 2>/dev/null || echo "")
    if [ -n "$PID" ]; then
        echo -e "${GREEN}7. Inspecting $proc_name (PID $PID):${NC}"
        demo_command "./pinspect $PID"
        pause_demo
        break
    fi
done

# Demo 8: Comparison with traditional tools
section "Demo 8: Comparison with Traditional Tools"

echo "pinspect vs traditional tools (on current shell $$):"
echo ""
echo -e "${GREEN}Traditional ps output:${NC}"
demo_command "ps -p $$ -o pid,comm,state,rss,vsz"

echo -e "${GREEN}pinspect output:${NC}"
demo_command "./pinspect $$"

echo "Notice how pinspect provides more detailed memory info and"
echo "integrates FD and network data in one view."
pause_demo

# Demo 9: Multiple process inspection
section "Demo 9: Inspecting Multiple Processes"

echo "Inspecting several processes in sequence:"
echo ""

PIDS=($$)
for proc in bash zsh sh; do
    PID=$(pgrep -o "$proc" 2>/dev/null || echo "")
    if [ -n "$PID" ] && [ "$PID" != "$$" ]; then
        PIDS+=("$PID")
    fi
done

for pid in "${PIDS[@]}"; do
    echo -e "${GREEN}Process $pid:${NC}"
    demo_command "./pinspect $pid | head -7"
done
pause_demo

# Demo 10: Performance demonstration
section "Demo 10: Performance"

echo "Measuring performance on 10 consecutive runs:"
echo ""

TIME_START=$(date +%s%N)
for i in {1..10}; do
    ./pinspect $$ >/dev/null
done
TIME_END=$(date +%s%N)

ELAPSED=$((TIME_END - TIME_START))
ELAPSED_MS=$((ELAPSED / 1000000))
AVG_MS=$((ELAPSED_MS / 10))

echo "10 runs completed in ${ELAPSED_MS}ms"
echo "Average per run: ${AVG_MS}ms"
echo ""
pause_demo

# Summary
section "Demo Complete"

echo "Summary of demonstrated features:"
echo ""
echo "  ✓ Process metadata (name, state, UID, memory)"
echo "  ✓ Thread enumeration with names and states"
echo "  ✓ File descriptor listing"
echo "  ✓ Network connection tracking (TCP/UDP)"
echo "  ✓ Socket inode correlation"
echo "  ✓ Graceful error handling"
echo "  ✓ Permission checks"
echo "  ✓ Multiple output modes (basic, verbose, network-only)"
echo ""
echo "pinspect demonstrates systems programming fundamentals:"
echo "  • Direct /proc filesystem parsing"
echo "  • Memory-safe C implementation"
echo "  • POSIX compliance"
echo "  • Production-quality error handling"
echo ""
echo -e "${GREEN}Demo completed successfully!${NC}"
