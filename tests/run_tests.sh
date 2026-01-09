#!/bin/bash
#
# run_tests.sh - Test runner for pinspect
#
# Runs all test binaries and reports results

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo ""
echo "==================================="
echo "  pinspect Test Suite"
echo "==================================="
echo ""

# Build tests
echo -e "${YELLOW}Building tests...${NC}"
cd "$PROJECT_DIR"
make tests

# Track results
total_passed=0
total_failed=0
failed_tests=()

# Run each test
for test_bin in "$BUILD_DIR"/test_*; do
    if [ -f "$test_bin" ] && [ -x "$test_bin" ]; then
        test_name=$(basename "$test_bin")

        echo ""
        if "$test_bin"; then
            echo -e "${GREEN}✓ $test_name completed successfully${NC}"
        else
            echo -e "${RED}✗ $test_name failed${NC}"
            failed_tests+=("$test_name")
        fi
    fi
done

echo ""
echo "==================================="
if [ ${#failed_tests[@]} -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed:${NC}"
    for test in "${failed_tests[@]}"; do
        echo -e "  ${RED}✗${NC} $test"
    done
    exit 1
fi
