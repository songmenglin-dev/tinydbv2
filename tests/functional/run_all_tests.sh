#!/bin/bash
#
# run_all_tests.sh - Run all functional tests for TinyDB v2
#
# This script runs each test script and produces a summary report.
# Exit code is non-zero if any tests fail.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Tests expect to run from project root where ./build/ paths work
# SCRIPT_DIR is tests/functional, so go up two levels to project root
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test scripts to run (in order)
TEST_SCRIPTS=(
    "test_ddl.sh"
    "test_dml.sh"
    "test_dql.sh"
    "test_transaction.sh"
    "test_utilities.sh"
    "test_errors.sh"
    "test_index.sh"
)

# Global counters
TOTAL_PASS=0
TOTAL_FAIL=0
TOTAL_SCRIPTS=0
FAILED_SCRIPTS=""

# ============================================================================
# Main Execution
# ============================================================================

echo ""
echo "========================================"
echo "TinyDB v2 - Functional Test Suite"
echo "========================================"
echo ""
echo "Running tests from: $SCRIPT_DIR"
echo ""

# Check if binaries exist
if [ ! -x "$PROJECT_ROOT/build/tinydb-server" ]; then
    echo -e "${RED}ERROR: Server binary not found at $PROJECT_ROOT/build/tinydb-server${NC}"
    echo "Please build the project first: make build"
    exit 1
fi

if [ ! -x "$PROJECT_ROOT/build/tinydb-cli" ]; then
    echo -e "${RED}ERROR: CLI binary not found at $PROJECT_ROOT/build/tinydb-cli${NC}"
    echo "Please build the project first: make build"
    exit 1
fi

# Cleanup stale lock files and processes before running tests
rm -rf /run/tinydb/* /tmp/tinydb_test_* 2>/dev/null || true
pkill -9 -f "tinydb-server" 2>/dev/null || true
sleep 1
rm -rf /run/tinydb/* /tmp/tinydb_test_* 2>/dev/null || true

# Run each test script
for script in "${TEST_SCRIPTS[@]}"; do
    script_path="$SCRIPT_DIR/$script"

    # Cleanup before each test
    rm -rf /run/tinydb/* /tmp/tinydb_test_* 2>/dev/null || true
    pkill -9 -f "tinydb-server" 2>/dev/null || true
    sleep 1

    if [ ! -f "$script_path" ]; then
        echo -e "${YELLOW}WARNING: Test script $script_path not found, skipping${NC}"
        continue
    fi

    if [ ! -x "$script_path" ]; then
        echo -e "${YELLOW}WARNING: Test script $script_path not executable, skipping${NC}"
        continue
    fi

    echo "----------------------------------------"
    echo "Running: $script"
    echo "----------------------------------------"

    TOTAL_SCRIPTS=$((TOTAL_SCRIPTS + 1))

    # Run the test script from project root with proper environment
    # The test scripts use ./build/ paths which are relative to project root
    set +e  # Don't exit on error within test script
    output=$(cd "$PROJECT_ROOT" && "$script_path" 2>&1)
    exit_code=$?
    set -e

    # Display the output
    echo "$output"

    # Extract pass/fail counts from the output
    # The report_results function outputs: "Test Results: X total" followed by "PASS: Y" and "FAIL: Z"
    pass_count=$(echo "$output" | grep -E "^\s*PASS:" | awk '{print $2}' | head -1)
    fail_count=$(echo "$output" | grep -E "^\s*FAIL:" | awk '{print $2}' | head -1)

    # Handle cases where output doesn't have expected format
    if [ -z "$pass_count" ]; then
        pass_count=0
    fi
    if [ -z "$fail_count" ]; then
        fail_count=0
    fi

    # If we can't parse counts but exit code was non-zero, count as fail
    if [ "$exit_code" -ne 0 ] && [ "$fail_count" -eq 0 ]; then
        fail_count=1
    fi

    TOTAL_PASS=$((TOTAL_PASS + pass_count))
    TOTAL_FAIL=$((TOTAL_FAIL + fail_count))

    if [ "$fail_count" -gt 0 ] || [ "$exit_code" -ne 0 ]; then
        FAILED_SCRIPTS="$FAILED_SCRIPTS $script"
    fi

    echo ""
done

# ============================================================================
# Summary Report
# ============================================================================

echo "========================================"
echo "OVERALL TEST RESULTS"
echo "========================================"
echo ""
echo "Test Scripts Run: $TOTAL_SCRIPTS"
echo "Total PASS: $TOTAL_PASS"
echo "Total FAIL: $TOTAL_FAIL"
echo ""

if [ -n "$FAILED_SCRIPTS" ]; then
    echo -e "${RED}FAILED SCRIPTS:$NC$FAILED_SCRIPTS"
fi

# Exit with non-zero if any tests failed
if [ "$TOTAL_FAIL" -gt 0 ]; then
    echo ""
    echo -e "${RED}OVERALL STATUS: FAILED${NC}"
    exit 1
else
    echo ""
    echo -e "${GREEN}OVERALL STATUS: ALL TESTS PASSED${NC}"
    exit 0
fi